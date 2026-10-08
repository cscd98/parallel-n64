#ifndef STEREO_H
#define STEREO_H

#include "Types.h"

struct SPVertex;

/* Side-by-side stereo for the VR path.
 *
 * Main (VI-visible) frame buffers get a colour texture twice as wide as
 * normal: the left eye is the game's own image in the left half, the right
 * eye is drawn into the right half. Everything is expressed in N64 pixels
 * per eye; only GL viewport/scissor X is shifted for the right eye. */
namespace Stereo
{
	bool isActive();

	/* 2 when active, otherwise 1. */
	u32 eyes();

	/* Offsets already-transformed (clip space) vertices for the right eye.
	 * Vertices with MODIFY_XY (screen-space) and orthographic batches are
	 * left alone. */
	void shearForRightEye(SPVertex * _pVtx, u32 _count);

	/* Where the game picture sits inside each eye's frame, as fractions of the
	 * eye frame, measured from the top-left: x0, y0, x1, y1. Identity until
	 * libretro.c sets it from the headset's per-eye field of view. */
	void eyeRect(u32 _eye, float _out[4]);

	/* Call once per presented frame: adapts the convergence plane. */
	void endFrame();
}

/* Called from libretro.c. Returns 1 if the active state changed, in which case
 * the caller must recreate the GL objects (gliden64DestroyGfxContext +
 * gliden64ReinitGfxContext) so frame buffers are rebuilt at the new width. */
extern "C" int gliden64SetStereo(int _active, float _separation);

/* _rects: 8 floats, eye 0 then eye 1, each x0,y0,x1,y1 (see Stereo::eyeRect). */
extern "C" void gliden64SetStereoLayout(const float * _rects);

#endif // STEREO_H
