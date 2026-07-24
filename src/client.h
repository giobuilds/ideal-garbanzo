#ifndef CLIENT_H
#define CLIENT_H

/* =========================================================
 * client.h  --  The SDL-facing half of the game loop
 *
 * MMO_PLAN Phase 6 split the codebase in two: libsaltmarch_sim (the
 * world — SDL-free, linked by the headless replay tool and, later, the
 * server host) and the client (window, input, rendering). GameState
 * itself belongs to the sim, so anything that touches it AND SDL had to
 * come out of game.c. That is exactly two things, and they live here:
 *
 *   - client_update()      the old game_update(): camera, hover, road
 *                          drag, and the real-time -> fixed-tick pump
 *   - input_handle_event() implemented in input.c, declared here
 *                          because its signature is SDL's
 *
 * The division of labour inside client_update is unchanged: everything
 * above the accumulator is cosmetic and scaled by delta_time; the world
 * advances only in whole SIM_TICK_MS ticks, so frame rate cannot change
 * the simulation. Wall clock exists here, at the edge, and nowhere
 * inside the sim.
 * ========================================================= */

#include <SDL3/SDL.h>
#include "game.h"
#include "input.h"

/* Called once per frame, after net_pump and before rendering. Moves the
 * camera from held keys, updates the hovered tile from the mouse, runs
 * road drag-placement, then spends the frame's elapsed real time on
 * zero or more sim ticks (a co-op guest stops at the host's authorised
 * tick). `renderer` is needed only to map window pixels to the 1920x1080
 * logical space. */
void client_update(GameState *gs, SDL_Renderer *renderer);

/* Fold one SDL event into `input` (input.c). Returns SDL_APP_SUCCESS to
 * quit (window close, Escape), otherwise SDL_APP_CONTINUE. */
SDL_AppResult input_handle_event(InputState *input, const SDL_Event *event);

#endif /* CLIENT_H */
