#include <math.h>
#include "gSP.h"
#include "Stereo.h"

namespace {
	bool   s_active = false;

	/* Disparity at infinity, as a fraction of NDC (NDC spans 2.0). 0.04 is
	 * about 2% of the image width, roughly an IPD on a cinema-sized screen.
	 * libretro.c scales it by the headset's real IPD. */
	float  s_separation = 0.04f;

	/* Clip-space w that lands on the screen plane (zero parallax). N64 games
	 * use wildly different world scales, so it follows the scene: slow EMA of
	 * the geometric mean of the w values seen. */
	float  s_convergence = 400.0f;
	double s_logSum = 0.0;
	u32    s_logCount = 0;
}

namespace Stereo
{
	bool isActive() { return s_active; }

	u32 eyes() { return s_active ? 2U : 1U; }

	void shearForRightEye(SPVertex * _pVtx, u32 _count)
	{
		if (_count == 0)
			return;

		// Perspective projections carry -1 in [2][3]. Microcodes that DMA the
		// whole matrix into the model-view leave the projection as identity,
		// so fall back to "w varies across the batch".
		bool perspective = fabsf(gSP.matrix.projection[2][3]) > 1e-6f;
		if (!perspective) {
			const float w0 = _pVtx[0].w;
			for (u32 i = 1; i < _count && !perspective; ++i)
				perspective = fabsf(_pVtx[i].w - w0) > 1e-3f;
		}
		if (!perspective)
			return;

		for (u32 i = 0; i < _count; ++i) {
			SPVertex & v = _pVtx[i];
			if ((v.modify & MODIFY_XY) != 0)
				continue;
			if (v.w > 1.0f) {
				s_logSum += logf(v.w);
				++s_logCount;
			}
			// x_ndc' = x_ndc + s * (1 - conv / w)
			v.x += s_separation * (v.w - s_convergence);
		}
	}

	void endFrame()
	{
		if (s_logCount != 0) {
			const float target = expf(static_cast<float>(s_logSum / s_logCount));
			s_convergence += (target - s_convergence) * 0.05f;
			if (s_convergence < 1.0f)
				s_convergence = 1.0f;
			else if (s_convergence > 1.0e7f)
				s_convergence = 1.0e7f;
		}
		s_logSum = 0.0;
		s_logCount = 0;
	}
}

extern "C" int gliden64SetStereo(int _active, float _separation)
{
	const bool active = _active != 0;
	const bool changed = active != s_active;
	s_active = active;
	if (_separation > 0.0f)
		s_separation = _separation;
	return changed ? 1 : 0;
}
