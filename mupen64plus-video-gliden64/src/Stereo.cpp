#include <math.h>
#include "gSP.h"
#include "Stereo.h"

namespace {
	bool   s_active = false;

	/* Disparity of a point at the convergence depth, as a fraction of the
	 * game picture's half-width (NDC). libretro.c derives it from the real IPD,
	 * the virtual screen distance and the picture's angular size. */
	float  s_separation = 0.03f;

	float  s_rect[2][4] = { { 0.f, 0.f, 1.f, 1.f }, { 0.f, 0.f, 1.f, 1.f } };

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
			// A constant clip-space offset is a camera translation: the
			// disparity is s * conv / w. Zero at infinity, crossed for
			// everything nearer. (The old s * (w - conv) was +s at infinity:
			// divergent, which doubles the image on a head-locked display.)
			v.x -= s_separation * s_convergence;
		}
	}

	void eyeRect(u32 _eye, float _out[4])
	{
		const float * r = s_rect[_eye < 2 ? _eye : 0];
		_out[0] = r[0]; _out[1] = r[1]; _out[2] = r[2]; _out[3] = r[3];
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

extern "C" void gliden64SetStereoLayout(const float * _rects)
{
	for (int e = 0; e < 2; ++e)
		for (int i = 0; i < 4; ++i)
			s_rect[e][i] = _rects[e * 4 + i];
}
