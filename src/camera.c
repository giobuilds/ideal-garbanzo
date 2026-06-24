/*  camera.c  --  Camera implementation  */

#include "camera.h"
#include "map.h"

void camera_init(Camera *cam, int screen_w, int screen_h,
                 int map_cols, int map_rows)
{
    (void)screen_h;
    (void)map_cols;
    (void)map_rows;

    cam->offset_x = (float)(screen_w / 2);
    cam->offset_y = (float)(TILE_H * 3);
    cam->zoom     = ZOOM_DEFAULT;
}
