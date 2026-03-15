#ifndef UI_H
#define UI_H

/* =========================================================
 * ui.h  --  HUD / user interface
 *
 * Phase 3 UI is a single horizontal bar pinned to the bottom
 * of the screen.  It contains one slot per building type.
 *
 * Layout (1920 × 1080 screen):
 *
 *   ┌──────────────────────────────────────────────────────┐
 *   │                  game world                          │
 *   ├──────────────────────────────────────────────────────┤
 *   │  [Fishers][Warehouse][Farm][Lumberjack]  ← HUD bar   │
 *   └──────────────────────────────────────────────────────┘
 *
 * Each slot is a rectangle.  The selected slot gets a bright
 * border.  Hovering shows the building name above the bar.
 * ========================================================= */

#include <SDL3/SDL.h>
#include "building.h"

/* HUD dimensions */
#define HUD_HEIGHT      80    /* pixels tall                  */
#define HUD_SLOT_SIZE   64    /* width and height of one slot */
#define HUD_SLOT_PAD    12    /* gap between slots            */
#define HUD_MARGIN_LEFT 20    /* left edge inset              */

/* Draw the entire HUD bar.
 * selected  – currently selected BuildingType (or BUILDING_NONE)
 * mouse_x/y – current cursor position in screen pixels
 *             (used to highlight the hovered slot) */
void ui_draw(SDL_Renderer *renderer,
             int screen_w, int screen_h,
             BuildingType selected,
             int mouse_x, int mouse_y);

/* Hit-test: given a screen coordinate, return the BuildingType
 * whose HUD slot contains that point, or BUILDING_NONE. */
BuildingType ui_hit_test(int screen_w, int screen_h,
                         int mouse_x, int mouse_y);

#endif /* UI_H */
