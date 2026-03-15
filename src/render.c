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
 * 
 * New in Phase 3:
 *    render_buildings() – draws placed building footprints
 *    render_ghost()     – draws a transparent placement preview
 *                         green = valid, red = invalid
 */

#include "render.h"
#include "game.h"   /* SCREEN_W, SCREEN_H */
#include "building.h"
#include <SDL3/SDL.h>
#include <math.h>   /* floorf() */

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
    /* Half-tile dimensions as floats */
    float hw = (float)(TILE_W / 2);
    float hh = (float)(TILE_H / 2);

    /* CHANGED fix 1: shift input to diamond centroid.
     * iso_to_screen() maps (row,col) to the TOP-LEFT of the bounding
     * box.  The visible centre of the diamond is at (+hw, +hh) from
     * that corner.  Subtracting that offset before inverting means the
     * hit-test is evaluated relative to each tile's centre rather than
     * its corner — without this the picked tile is consistently off by
     * one diagonal neighbour near tile edges. */
    float px = (float)sx - cam->offset_x - hw;
    float py = (float)sy - cam->offset_y - hh;

    /* CHANGED fix 2: floorf() instead of (int) cast.
     * (int) truncates toward zero: -0.4f becomes 0 instead of -1.
     * floorf() rounds toward -infinity, which is correct for a grid. */
    *out_col = (int)floorf( (px / hw + py / hh) / 2.0f );
    *out_row = (int)floorf( (py / hh - px / hw) / 2.0f );
}

/* ---- draw_diamond --------------------------------------
 * Draw a filled isometric diamond for one tile.
 * (bx, by) is the top-left of the bounding box.
 * We split the diamond into two triangles and use
 * SDL_RenderGeometry() which accepts coloured vertices.
 * -------------------------------------------------------- */
static void draw_diamond(SDL_Renderer *renderer,
                         int bx, int by,
                         SDL_Color top_col, SDL_Color bot_col)
{
    int half_w = TILE_W / 2;
    int half_h = TILE_H / 2;
    SDL_Vertex verts[4];
    int indices[6] = { 0,1,2, 0,3,2 };
 
    verts[0].position.x = (float)(bx + half_w);
    verts[0].position.y = (float)(by);
    verts[1].position.x = (float)(bx);
    verts[1].position.y = (float)(by + half_h);
    verts[2].position.x = (float)(bx + half_w);
    verts[2].position.y = (float)(by + TILE_H);
    verts[3].position.x = (float)(bx + TILE_W);
    verts[3].position.y = (float)(by + half_h);
 
    verts[0].tex_coord.x = 0.0f; verts[0].tex_coord.y = 0.0f;
    verts[1].tex_coord.x = 0.0f; verts[1].tex_coord.y = 0.0f;
    verts[2].tex_coord.x = 0.0f; verts[2].tex_coord.y = 0.0f;
    verts[3].tex_coord.x = 0.0f; verts[3].tex_coord.y = 0.0f;
 
    /* top, left, right faces — bright colour */
    verts[0].color.r = top_col.r/255.0f; verts[0].color.g = top_col.g/255.0f;
    verts[0].color.b = top_col.b/255.0f; verts[0].color.a = top_col.a/255.0f;
    verts[1].color = verts[0].color;
    verts[3].color = verts[0].color;
 
    /* bottom face — dark colour */
    verts[2].color.r = bot_col.r/255.0f; verts[2].color.g = bot_col.g/255.0f;
    verts[2].color.b = bot_col.b/255.0f; verts[2].color.a = bot_col.a/255.0f;
 
    SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6);
}

static void draw_diamond_outline(SDL_Renderer *renderer,
                                 int bx, int by,
                                 unsigned char r, unsigned char g,
                                 unsigned char b, unsigned char a)
{
    int hw = TILE_W / 2;
    int hh = TILE_H / 2;
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderLine(renderer,
        (float)(bx+hw),   (float)(by),
        (float)(bx),      (float)(by+hh));
    SDL_RenderLine(renderer,
        (float)(bx),      (float)(by+hh),
        (float)(bx+hw),   (float)(by+TILE_H));
    SDL_RenderLine(renderer,
        (float)(bx+hw),   (float)(by+TILE_H),
        (float)(bx+TILE_W),(float)(by+hh));
    SDL_RenderLine(renderer,
        (float)(bx+TILE_W),(float)(by+hh),
        (float)(bx+hw),   (float)(by));
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
/* ---- render_buildings ----------------------------------
 * For each placed building, draw a coloured diamond over
 * every tile in its footprint, plus a bright border.
 * -------------------------------------------------------- */
void render_buildings(SDL_Renderer *renderer,
                      const Building buildings[], int count,
                      const Camera *cam)
{
    int i, r, c, sx, sy;
 
    for (i = 0; i < count; i++) {
        const Building    *b   = &buildings[i];
        const BuildingDef *def = &BUILDING_DEFS[b->type];
        SDL_Color top, bot;
 
        if (!b->active) continue;
 
        top.r = def->col_r;
        top.g = def->col_g;
        top.b = def->col_b;
        top.a = 255;
        bot.r = (unsigned char)(def->col_r * 0.7f);
        bot.g = (unsigned char)(def->col_g * 0.7f);
        bot.b = (unsigned char)(def->col_b * 0.7f);
        bot.a = 255;
 
        for (r = b->row; r < b->row + def->tile_h; r++) {
            for (c = b->col; c < b->col + def->tile_w; c++) {
                iso_to_screen(r, c, cam, &sx, &sy);
                draw_diamond(renderer, sx, sy, top, bot);
                draw_diamond_outline(renderer, sx, sy,
                    255, 255, 255, 60);
            }
        }
    }
}
 
/* ---- render_ghost --------------------------------------
 * Semi-transparent preview of where a building will land.
 * Green tint = valid, red tint = invalid.
 * -------------------------------------------------------- */
void render_ghost(SDL_Renderer *renderer,
                  const Camera *cam,
                  BuildingType type,
                  int row, int col,
                  int valid)
{
    const BuildingDef *def = &BUILDING_DEFS[type];
    int r, c, sx, sy;
    SDL_Color top, bot;
 
    if (row < 0 || col < 0) return;
 
    if (valid) {
        /* Green tint overlay */
        top.r = 80;  top.g = 200; top.b = 80;  top.a = 180;
        bot.r = 50;  bot.g = 140; bot.b = 50;  bot.a = 180;
    } else {
        /* Red tint overlay */
        top.r = 200; top.g = 60;  top.b = 60;  top.a = 180;
        bot.r = 140; bot.g = 40;  bot.b = 40;  bot.a = 180;
    }
 
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (r = row; r < row + def->tile_h; r++) {
        for (c = col; c < col + def->tile_w; c++) {
            iso_to_screen(r, c, cam, &sx, &sy);
            draw_diamond(renderer, sx, sy, top, bot);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* ---- render_resources ----------------------------------
 * CHANGED Phase 4: new function.
 *
 * Draws a small panel in the top-left corner showing the
 * current stockpile count for each resource.
 *
 * Layout (one row per resource):
 *   [colour swatch]  [amount bar]
 *
 * Real numbers need SDL_ttf (Phase 5).  For now the amount
 * is encoded as a segmented bar — each filled segment = 10
 * units, so the bar fills up visually as resources grow.
 * Max bar width represents 100 units (10 segments).
 * -------------------------------------------------------- */
void render_resources(SDL_Renderer *renderer,
                      const Stockpile *s)
{
    /* Colour per resource: Wood, Fish, Grain, Gold */
    static const SDL_Color RES_COL[RES_COUNT] = {
        { 139,  90,  43, 255 },   /* WOOD  — brown  */
        {  50, 180, 230, 255 },   /* FISH  — cyan   */
        { 240, 210,  50, 255 },   /* GRAIN — yellow */
        { 255, 195,   0, 255 },   /* GOLD  — gold   */
    };
 
    int        i;
    int        panel_x  = 16;
    int        panel_y  = 16;
    int        row_h    = 22;
    int        swatch_w = 14;
    int        seg_w    = 8;    /* width of one bar segment  */
    int        seg_gap  = 2;    /* gap between segments      */
    int        max_segs = 10;   /* bar represents 0–100 units */
    int        panel_w  = swatch_w + 6 + max_segs * (seg_w + seg_gap) + 8;
    int        panel_h  = RES_COUNT * row_h + 10;
 
    /* Panel background */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 16, 10, 200);
    SDL_FRect bg = {
        (float)(panel_x - 6),  (float)(panel_y - 6),
        (float)(panel_w),       (float)(panel_h)
    };
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 90, 75, 45, 180);
    SDL_RenderRect(renderer, &bg);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
 
    /* One row per resource */
    for (i = 0; i < RES_COUNT; i++) {
        int   amount = s->amount[i];
        int   segs_filled;
        int   j;
        float ry = (float)(panel_y + i * row_h);
 
        /* Colour swatch */
        SDL_FRect swatch = {
            (float)panel_x, ry + 4.0f,
            (float)swatch_w, (float)(row_h - 8)
        };
        SDL_SetRenderDrawColor(renderer,
            RES_COL[i].r, RES_COL[i].g, RES_COL[i].b, 255);
        SDL_RenderFillRect(renderer, &swatch);
 
        /* Segmented amount bar.
         * Each segment represents 10 units.
         * Clamp: once amount >= 100 all segments are filled.
         * This gives a visual "tank level" without needing text. */
        segs_filled = amount / 10;
        if (segs_filled > max_segs) segs_filled = max_segs;
 
        for (j = 0; j < max_segs; j++) {
            float bx = (float)(panel_x + swatch_w + 6
                        + j * (seg_w + seg_gap));
            SDL_FRect seg = {
                bx, ry + 5.0f,
                (float)seg_w, (float)(row_h - 10)
            };
            if (j < segs_filled) {
                /* Filled segment: resource colour, slightly dimmed */
                SDL_SetRenderDrawColor(renderer,
                    (unsigned char)(RES_COL[i].r * 0.85f),
                    (unsigned char)(RES_COL[i].g * 0.85f),
                    (unsigned char)(RES_COL[i].b * 0.85f), 255);
                SDL_RenderFillRect(renderer, &seg);
            } else {
                /* Empty segment: dark slot */
                SDL_SetRenderDrawColor(renderer, 40, 35, 25, 255);
                SDL_RenderFillRect(renderer, &seg);
            }
            /* Segment border */
            SDL_SetRenderDrawColor(renderer, 70, 60, 40, 255);
            SDL_RenderRect(renderer, &seg);
        }
    }
}
