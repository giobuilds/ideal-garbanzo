/*  game.c  --  Game state management  */

#include "game.h"
#include "render.h"    /* screen_to_iso() */
#include <SDL3/SDL.h>  /* SDL_GetTicksNS() */
#include <stdlib.h>    /* malloc, free    */

/* ---- game_init ----------------------------------------- */
GameState *game_init(void)
{
    GameState *gs = (GameState *)malloc(sizeof(GameState));
    if (!gs) return NULL;

    uint32_t seed = (uint32_t)SDL_GetTicksNS();
    SDL_Log("Map seed: %u", seed);
    map_init(&gs->map, seed);
    camera_init(&gs->camera, SCREEN_W, SCREEN_H,
                MAP_COLS, MAP_ROWS);
    input_init(&gs->input);

    gs->hovered_row = -1;
    gs->hovered_col = -1;
    gs->last_tick   = SDL_GetTicksNS();   /* nanosecond timer – always available */
    gs->delta_time  = 0.0f;

    return gs;
}

/* ---- game_free ----------------------------------------- */
void game_free(GameState *gs)
{
    free(gs);
}

/* ---- game_update ---------------------------------------
 * Per-frame logic:
 *   1. Pan camera based on held keys.
 *   2. Convert mouse position to hovered tile.
 * -------------------------------------------------------- */
void game_update(GameState *gs)
{
    Uint64 now, elapsed_ns;
    float  dt;
 
    /* --- Delta time ------------------------------------ */
    now        = SDL_GetTicksNS();
    elapsed_ns = now - gs->last_tick;
    gs->last_tick = now;
 
    /* Convert nanoseconds → seconds.  Cap at 0.1s (100ms) so a
     * stall or breakpoint doesn't teleport the camera. */
    dt = (float)elapsed_ns / 1000000000.0f;
    if (dt > 0.1f) dt = 0.1f;
    gs->delta_time = dt;

    /* --- Camera panning -------------------------------- */
    if (gs->input.pan_left)  gs->camera.offset_x += CAMERA_PAN_SPEED * dt;
    if (gs->input.pan_right) gs->camera.offset_x -= CAMERA_PAN_SPEED * dt;
    if (gs->input.pan_up)    gs->camera.offset_y += CAMERA_PAN_SPEED * dt;
    if (gs->input.pan_down)  gs->camera.offset_y -= CAMERA_PAN_SPEED * dt;

    /* --- 2. Hovered tile ------------------------------- */
    screen_to_iso(gs->input.mouse_x, gs->input.mouse_y,
                  &gs->camera,
                  &gs->hovered_row, &gs->hovered_col);

    /* Clamp to valid range (-1 signals "off map") */
    if (gs->hovered_row < 0 || gs->hovered_row >= MAP_ROWS ||
        gs->hovered_col < 0 || gs->hovered_col >= MAP_COLS) {
        gs->hovered_row = -1;
        gs->hovered_col = -1;
    }
}
