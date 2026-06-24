/*  game.c  --  Game state management  (Phase 5)  */

#include "game.h"
#include "render.h"
#include "building.h"
#include "resource.h"
#include "population.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

/* ---- game_init ----------------------------------------- */
GameState *game_init(void)
{
    GameState *gs = (GameState *)malloc(sizeof(GameState));
    if (!gs) return NULL;

    uint32_t seed = (uint32_t)SDL_GetTicksNS();
    SDL_Log("Map seed: %u", seed);
    map_init(&gs->map, seed);

    camera_init(&gs->camera, SCREEN_W, SCREEN_H, MAP_COLS, MAP_ROWS);
    input_init(&gs->input);

    gs->hovered_row       = -1;
    gs->hovered_col       = -1;
    gs->last_tick         = SDL_GetTicksNS();
    gs->delta_time        = 0.0f;

    memset(gs->buildings, 0, sizeof(gs->buildings));
    memset(gs->pop_data,  0, sizeof(gs->pop_data));  /* Phase 5 */
    gs->building_count    = 0;
    gs->selected_building = BUILDING_NONE;
    gs->placement_valid   = 0;
    gs->menu_open         = 0;

    stockpile_init(&gs->stockpile);

    return gs;
}

/* ---- game_free ----------------------------------------- */
void game_free(GameState *gs)
{
    free(gs);
}

/* ---- game_tick_buildings (unchanged from Phase 4) ------ */
static void game_tick_buildings(GameState *gs, float dt)
{
    int i;
    for (i = 0; i < gs->building_count; i++) {
        Building          *b   = &gs->buildings[i];
        const BuildingDef *def = &BUILDING_DEFS[b->type];

        if (!b->active || def->tick_seconds <= 0.0f) continue;

        b->timer += dt;
        if (b->timer < def->tick_seconds) continue;
        b->timer = 0.0f;

        if (def->consumes != RES_COUNT) {
            if (gs->stockpile.amount[def->consumes] < def->consume_amt) {
                SDL_Log("%s idle: needs %d %s", def->name,
                    def->consume_amt, RESOURCE_NAMES[def->consumes]);
                continue;
            }
            stockpile_add(&gs->stockpile, def->consumes, -def->consume_amt);
        }

        if (def->produces != RES_COUNT) {
            stockpile_add(&gs->stockpile, def->produces, def->produce_amt);
            SDL_Log("%s produced %d %s  (total: %d)",
                def->name, def->produce_amt,
                RESOURCE_NAMES[def->produces],
                gs->stockpile.amount[def->produces]);
        }
    }
}

/* ---- game_update --------------------------------------- */
void game_update(GameState *gs, SDL_Renderer *renderer)
{
    float lx, ly;

    Uint64 now = SDL_GetTicksNS();
    float  dt  = (float)(now - gs->last_tick) / 1000000000.0f;
    if (dt > 0.1f) dt = 0.1f;
    gs->last_tick  = now;
    gs->delta_time = dt;

    if (gs->input.pan_left)  gs->camera.offset_x += CAMERA_PAN_SPEED * dt;
    if (gs->input.pan_right) gs->camera.offset_x -= CAMERA_PAN_SPEED * dt;
    if (gs->input.pan_up)    gs->camera.offset_y += CAMERA_PAN_SPEED * dt;
    if (gs->input.pan_down)  gs->camera.offset_y -= CAMERA_PAN_SPEED * dt;

    /* CHANGED: zoom toward cursor on mouse wheel scroll.
     * Keeps the tile under the cursor stationary while zooming —
     * the same behaviour as Google Maps or Anno 1800.
     * Steps:
     *   1. Compute cursor offset from camera origin.
     *   2. Apply zoom delta.
     *   3. Rescale that offset by the zoom ratio.
     *   4. Rewrite camera origin so cursor stays in place. */
    if (gs->input.scroll_y != 0.0f) {
        float old_zoom = gs->camera.zoom;
        float new_zoom = old_zoom + gs->input.scroll_y * ZOOM_STEP;
        if (new_zoom < ZOOM_MIN) new_zoom = ZOOM_MIN;
        if (new_zoom > ZOOM_MAX) new_zoom = ZOOM_MAX;
        if (new_zoom != old_zoom) {
            float cx    = (float)gs->input.logical_x;
            float cy    = (float)gs->input.logical_y;
            float dx    = cx - gs->camera.offset_x;
            float dy    = cy - gs->camera.offset_y;
            float ratio = new_zoom / old_zoom;
            gs->camera.offset_x = cx - dx * ratio;
            gs->camera.offset_y = cy - dy * ratio;
            gs->camera.zoom     = new_zoom;
        }
    }

    SDL_RenderCoordinatesFromWindow(renderer,
        (float)gs->input.mouse_x, (float)gs->input.mouse_y, &lx, &ly);
    gs->input.logical_x = (int)lx;
    gs->input.logical_y = (int)ly;

    if (gs->input.logical_y < SCREEN_H - HUD_HEIGHT) {
        screen_to_iso(gs->input.logical_x, gs->input.logical_y,
                      &gs->camera, &gs->hovered_row, &gs->hovered_col);
        if (gs->hovered_row < 0 || gs->hovered_row >= MAP_ROWS ||
            gs->hovered_col < 0 || gs->hovered_col >= MAP_COLS) {
            gs->hovered_row = -1;
            gs->hovered_col = -1;
        }
    } else {
        gs->hovered_row = -1;
        gs->hovered_col = -1;
    }

    gs->placement_valid = 0;
    if (gs->selected_building != BUILDING_NONE && gs->hovered_row >= 0)
        gs->placement_valid = building_can_place(&gs->map,
            gs->selected_building, gs->hovered_row, gs->hovered_col,
            NULL, 0);

    game_tick_buildings(gs, dt);

    /* Phase 5: update population needs */
    pop_update(gs->pop_data, gs->building_count, &gs->stockpile, dt);
}

/* ---- game_place_building -------------------------------
 * Phase 5: when a House is placed, initialise its PopData.
 * -------------------------------------------------------- */
void game_place_building(GameState *gs)
{
    int idx;

    if (gs->selected_building == BUILDING_NONE) return;
    if (gs->hovered_row < 0) return;

    idx = building_place(gs->buildings, &gs->building_count,
                         &gs->map, gs->selected_building,
                         gs->hovered_row, gs->hovered_col);

    /* Phase 5: if a house was just placed, activate its PopData */
    if (idx >= 0 && gs->selected_building == BUILDING_HOUSE)
        pop_init(&gs->pop_data[idx]);
}
