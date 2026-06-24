#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>

typedef struct {
    int pan_left, pan_right, pan_up, pan_down;

    int mouse_x, mouse_y;
    int logical_x, logical_y;

    /* CHANGED: mouse wheel scroll accumulated this frame.
     * Positive = scroll up (zoom in), negative = scroll down (zoom out).
     * Reset to 0 by input_clear_clicks() each frame. */
    float scroll_y;

    int left_click;
    int right_click;
} InputState;

void           input_init(InputState *input);
SDL_AppResult  input_handle_event(InputState *input,
                                  const SDL_Event *event);
void           input_clear_clicks(InputState *input);

#endif /* INPUT_H */
