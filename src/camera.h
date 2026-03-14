#ifndef CAMERA_H
#define CAMERA_H

/* =========================================================
 * camera.h  --  Camera / viewport
 *
 * The camera tracks how far we have scrolled the world.
 * offset_x / offset_y are pixel offsets applied before
 * drawing every tile.  When both are 0 the map origin
 * (tile 0,0) sits at the top-centre of the screen.
 *
 * Pan speed is measured in pixels per frame.
 * ========================================================= */

#define CAMERA_PAN_SPEED 8   /* pixels moved per frame when key held */

typedef struct {
    float offset_x;   /* horizontal scroll in pixels */
    float offset_y;   /* vertical  scroll in pixels  */
} Camera;

/* Reset camera to a sensible starting position that centres the
 * map on a FullHD screen. */
void camera_init(Camera *cam, int screen_w, int screen_h,
                 int map_cols, int map_rows);

#endif /* CAMERA_H */
