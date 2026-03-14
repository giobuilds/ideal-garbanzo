/*  render.c  --  SDL rendering implementation
 *
 *  Phase 1: coloured flat diamonds only – no sprite assets.
 *  Each TileType gets a distinct SDL colour so we can verify
 *  projection maths before we worry about artwork.
 *
 *  A "diamond" is drawn as a filled polygon using four points:
 *
 *        top  (sx + TILE_W/2,  sy)
 *       left  (sx,             sy + TILE_H/2)
 *     bottom  (sx + TILE_W/2,  sy + TILE_H)
 *      right  (sx + TILE_W,    sy + TILE_H/2)
 *
 *  where (sx, sy) is the top-left corner of the tile's
 *  bounding box, NOT the top point of the diamond.
 *  iso_to_screen() returns the top-left corner.
 */

#include "render.h"
#include "game.h"   /* SCREEN_W, SCREEN_H */
#include <SDL3/SDL.h>

/* ---- Colour table (R, G, B, A) for each TileType ------- */
static const SDL_Color TILE_COLOURS[TILE_TYPE_COUNT] = {
    /* TILE_GRASS  */ { 106, 168,  79, 255 },
    /* TILE_WATER  */ {  30, 120, 200, 255 },
    /* TILE_FOREST */ {  38,  94,  46, 255 },
    /* TILE_SAND   */ { 220, 200, 130, 255 },
};

/* Slightly darker shade for the left face of the diamond
 * (gives a very cheap depth illusion even without sprites). */
static const SDL_Color TILE_DARK[TILE_TYPE_COUNT] = {
    /* TILE_GRASS  */ {  80, 130,  55, 255 },
    /* TILE_WATER  */ {  20,  90, 160, 255 },
    /* TILE_FOREST */ {  25,  65,  30, 255 },
    /* TILE_SAND   */ { 185, 165, 100, 255 },
};

/* ---- iso_to_screen -------------------------------------
 * Converts a grid cell (row, col) into the screen pixel
 * coordinate of the TOP-LEFT corner of its bounding box.
 * -------------------------------------------------------- */
void iso_to_screen(int row, int col, const Camera *cam,
                   int *out_x, int *out_y)
{
    *out_x = (int)cam->offset_x + (col - row) * (TILE_W / 2);
    *out_y = (int)cam->offset_y + (col + row) * (TILE_H / 2);
}

/* ---- screen_to_iso -------------------------------------
 * Inverse of iso_to_screen.  We first remove the camera
 * offset, then solve the two linear equations:
 *
 *   px = (col - row) * (TILE_W/2)
 *   py = (col + row) * (TILE_H/2)
 *
 * Adding the equations:  col = (px/TILE_W_HALF + py/TILE_H_HALF) / 2 ... etc.
 *
 * Integer arithmetic rounds to the nearest cell.
 * -------------------------------------------------------- */
void screen_to_iso(int sx, int sy, const Camera *cam,
                   int *out_row, int *out_col)
{
    /* Remove camera offset to get into "world iso space" */
    float px = (float)sx - cam->offset_x;
    float py = (float)sy - cam->offset_y;

    /* Half-tile dimensions as floats */
    float hw = (float)(TILE_W / 2);
    float hh = (float)(TILE_H / 2);

    /* Solve for col and row */
    *out_col = (int)( (px / hw + py / hh) / 2.0f );
    *out_row = (int)( (py / hh - px / hw) / 2.0f );
}

/* ---- draw_diamond --------------------------------------
 * Draw a filled isometric diamond for one tile.
 * (bx, by) is the top-left of the bounding box.
 * We split the diamond into two triangles and use
 * SDL_RenderGeometry() which accepts coloured vertices.
 * -------------------------------------------------------- */
static void draw_diamond(SDL_Renderer *renderer,
                         int bx, int by,
                         SDL_Color top_col, SDL_Color left_col)
{
    /* The four corners of the diamond */
    int half_w = TILE_W / 2;
    int half_h = TILE_H / 2;

    SDL_Vertex verts[4];
    int i;

    /* top point  */
    verts[0].position.x = (float)(bx + half_w);
    verts[0].position.y = (float)(by);

    /* left point */
    verts[1].position.x = (float)(bx);
    verts[1].position.y = (float)(by + half_h);

    /* bottom point */
    verts[2].position.x = (float)(bx + half_w);
    verts[2].position.y = (float)(by + TILE_H);

    /* right point */
    verts[3].position.x = (float)(bx + TILE_W);
    verts[3].position.y = (float)(by + half_h);

    /* Assign colours: top-right triangle gets the bright colour,
     * left-bottom triangle gets the darker shade for depth. */
    for (i = 0; i < 4; i++) {
        verts[i].tex_coord.x = 0.0f;
        verts[i].tex_coord.y = 0.0f;
    }

    /* Top half (top, left, right) — bright face */
    verts[0].color.r = top_col.r / 255.0f;
    verts[0].color.g = top_col.g / 255.0f;
    verts[0].color.b = top_col.b / 255.0f;
    verts[0].color.a = 1.0f;

    verts[1].color = verts[0].color;   /* left = same as top  */
    verts[3].color = verts[0].color;   /* right = same as top */

    /* Bottom half vertex gets the dark shade */
    verts[2].color.r = left_col.r / 255.0f;
    verts[2].color.g = left_col.g / 255.0f;
    verts[2].color.b = left_col.b / 255.0f;
    verts[2].color.a = 1.0f;

    /* Triangle 1: top, left, bottom */
    /* Triangle 2: top, right, bottom */
    int indices[6] = { 0, 1, 2,   0, 3, 2 };

    SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6);
}

/* ---- render_clear -------------------------------------- */
void render_clear(SDL_Renderer *renderer)
{
    /* Deep ocean blue for the background */
    SDL_SetRenderDrawColor(renderer, 15, 60, 110, 255);
    SDL_RenderClear(renderer);
}

/* ---- render_map ----------------------------------------
 * Draw all tiles back-to-front.  Iterating row then column
 * naturally gives painter's order for a standard isometric
 * projection (top of screen = first rows + first cols).
 * -------------------------------------------------------- */
void render_map(SDL_Renderer *renderer,
                const Map    *map,
                const Camera *cam)
{
    int r, c, sx, sy;

    for (r = 0; r < map->rows; r++) {
        for (c = 0; c < map->cols; c++) {
            const Tile *t = &map->tiles[r][c];

            iso_to_screen(r, c, cam, &sx, &sy);

            /* Simple frustum cull: skip tiles completely off screen */
            if (sx + TILE_W < 0   || sx > SCREEN_W ||
                sy + TILE_H < 0   || sy > SCREEN_H) {
                continue;
            }

            draw_diamond(renderer, sx, sy,
                         TILE_COLOURS[t->type],
                         TILE_DARK[t->type]);
        }
    }
}

/* ---- render_hovered_tile --------------------------------
 * Draw a bright outline diamond over the hovered tile.
 * -------------------------------------------------------- */
void render_hovered_tile(SDL_Renderer *renderer,
                         const Camera *cam,
                         int hovered_row, int hovered_col)
{
    int sx, sy;
    int hw = TILE_W / 2;
    int hh = TILE_H / 2;

    if (hovered_row < 0 || hovered_col < 0) return;

    iso_to_screen(hovered_row, hovered_col, cam, &sx, &sy);

    SDL_SetRenderDrawColor(renderer, 255, 230, 50, 255);

    /* Draw outline by connecting the four diamond points */
    SDL_RenderLine(renderer,
        (float)(sx + hw), (float)(sy),           /* top   */
        (float)(sx),      (float)(sy + hh));      /* left  */
    SDL_RenderLine(renderer,
        (float)(sx),      (float)(sy + hh),       /* left  */
        (float)(sx + hw), (float)(sy + TILE_H));  /* bottom*/
    SDL_RenderLine(renderer,
        (float)(sx + hw), (float)(sy + TILE_H),   /* bottom*/
        (float)(sx + TILE_W), (float)(sy + hh));  /* right */
    SDL_RenderLine(renderer,
        (float)(sx + TILE_W), (float)(sy + hh),   /* right */
        (float)(sx + hw), (float)(sy));            /* top   */
}
