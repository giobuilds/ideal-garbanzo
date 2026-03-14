/*  input.c  --  Input handling implementation  */

#include "input.h"
#include <string.h>   /* memset */

/* ---- input_init ---------------------------------------- */
void input_init(InputState *input)
{
    memset(input, 0, sizeof(InputState));
}

/* ---- input_handle_event --------------------------------
 * We respond to:
 *   SDL_EVENT_QUIT           → signal quit
 *   SDL_EVENT_KEY_DOWN/UP    → track pan keys
 *   SDL_EVENT_MOUSE_MOTION   → update cursor position
 *
 * Arrow keys AND WASD both pan the camera so both keyboard
 * layouts feel natural.
 * -------------------------------------------------------- */
SDL_AppResult input_handle_event(InputState *input,
                                 const SDL_Event *event)
{
    int down;   /* 1 = key pressed, 0 = key released */

    switch (event->type) {

    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        down = (event->type == SDL_EVENT_KEY_DOWN) ? 1 : 0;

        switch (event->key.scancode) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            input->pan_left  = down; break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            input->pan_right = down; break;
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            input->pan_up    = down; break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            input->pan_down  = down; break;

        /* Escape also quits */
        case SDL_SCANCODE_ESCAPE:
            return SDL_APP_SUCCESS;

        default:
            break;
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        input->mouse_x = (int)event->motion.x;
        input->mouse_y = (int)event->motion.y;
        break;

    default:
        break;
    }

    return SDL_APP_CONTINUE;
}
