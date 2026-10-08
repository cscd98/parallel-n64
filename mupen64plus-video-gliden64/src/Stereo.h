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

	/* Call once per presented frame: adapts the convergence plane. */
	void endFrame();
}

/* Called from libretro.c. Returns 1 if the active state changed, in which case
 * the caller must recreate the GL objects (gliden64DestroyGfxContext +
 * gliden64ReinitGfxContext) so frame buffers are rebuilt at the new width. */
extern "C" int gliden64SetStereo(int _active, float _separation);

#endif // STEREO_H
