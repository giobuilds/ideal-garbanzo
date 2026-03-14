#ifndef GAME_H
#define GAME_H

/* =========================================================
 * game.h  --  Top-level game state
 *
 * GameState owns every sub-system: the map, the camera,
 * and the input tracker.  A single pointer to GameState
 * is stored in SDL's appstate so all three callbacks
 * (AppInit, AppEvent, AppIterate) can reach it.
 * ========================================================= */

#include "map.h"
#include "camera.h"
#include "input.h"

#define SCREEN_W 1920
#define SCREEN_H 1080

typedef struct {
    Map        map;
    Camera     camera;
    InputState input;

    /* Tile currently under the mouse cursor (-1 if none) */
    int hovered_row;
    int hovered_col;

    /* Time tracking for frame-rate-independent movement.
    * last_tick  – SDL timestamp (ms) at the end of the previous frame.
    * delta_time – seconds elapsed since that frame (e.g. 0.016 at 60fps).
    * All per-frame movement is multiplied by delta_time so the game
    * behaves identically at 30, 60, or 144 fps. */
    Uint64 last_tick;
    float delta_time;
} GameState;

/* Allocate and initialise a new GameState.
 * Returns NULL on allocation failure. */
GameState *game_init(void);

/* Free a GameState allocated by game_init(). */
void game_free(GameState *gs);

/* Called once per frame.  Moves the camera based on held
 * keys and updates the hovered tile from mouse position. */
void game_update(GameState *gs);

#endif /* GAME_H */
