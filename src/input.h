#ifndef INPUT_H
#define INPUT_H

/* =========================================================
 * input.h  --  Input state
 *
 * We track which pan keys are currently held so that
 * SDL_AppIterate() can move the camera every frame
 * proportionally to elapsed time (later) or at a fixed
 * speed (Phase 1).
 *
 * Mouse position is stored in screen pixels.
 * ========================================================= */

#include <SDL3/SDL.h>

typedef struct {
    /* Camera panning keys – 1 while held, 0 otherwise */
    int pan_left;
    int pan_right;
    int pan_up;
    int pan_down;

    /* Mouse cursor position in screen pixels */
    int mouse_x;
    int mouse_y;
} InputState;

/* Initialise all fields to 0. */
void input_init(InputState *input);

/* Process one SDL event and update input state accordingly.
 * Returns SDL_APP_SUCCESS if the user requested quit,
 * SDL_APP_CONTINUE otherwise. */
SDL_AppResult input_handle_event(InputState *input,
                                 const SDL_Event *event);

#endif /* INPUT_H */
