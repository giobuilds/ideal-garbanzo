/*  ui.c  --  HUD rendering and hit-testing  */

#include "ui.h"
#include <string.h>   /* strlen (unused yet, kept for later) */

/* ---- slot_rect -----------------------------------------
 * Compute the screen rectangle for HUD slot `i`.
 * The bar is pinned to the bottom of the screen.
 * -------------------------------------------------------- */
static SDL_FRect slot_rect(int screen_w, int screen_h, int i)
{
    SDL_FRect r;
    int bar_y = screen_h - HUD_HEIGHT;
    (void)screen_w;

    r.x = (float)(HUD_MARGIN_LEFT + i * (HUD_SLOT_SIZE + HUD_SLOT_PAD));
    r.y = (float)(bar_y + (HUD_HEIGHT - HUD_SLOT_SIZE) / 2);
    r.w = (float)HUD_SLOT_SIZE;
    r.h = (float)HUD_SLOT_SIZE;
    return r;
}

/* ---- ui_hit_test --------------------------------------- */
BuildingType ui_hit_test(int screen_w, int screen_h,
                         int mouse_x, int mouse_y)
{
    int i;
    for (i = 0; i < BUILDING_TYPE_COUNT; i++) {
        SDL_FRect r = slot_rect(screen_w, screen_h, i);
        if ((float)mouse_x >= r.x && (float)mouse_x < r.x + r.w &&
            (float)mouse_y >= r.y && (float)mouse_y < r.y + r.h) {
            return (BuildingType)i;
        }
    }
    return BUILDING_NONE;
}

/* ---- ui_draw ------------------------------------------- */
void ui_draw(SDL_Renderer *renderer,
             int screen_w, int screen_h,
             BuildingType selected,
             int mouse_x, int mouse_y)
{
    int i;
    float bar_y = (float)(screen_h - HUD_HEIGHT);

    /* --- Background bar --------------------------------- */
    SDL_SetRenderDrawColor(renderer, 30, 25, 20, 220);
    SDL_FRect bar = { 0, bar_y, (float)screen_w, (float)HUD_HEIGHT };
    SDL_RenderFillRect(renderer, &bar);

    /* Thin top border line */
    SDL_SetRenderDrawColor(renderer, 90, 75, 55, 255);
    SDL_RenderLine(renderer,
        0.0f,           bar_y,
        (float)screen_w,bar_y);

    /* --- Building slots --------------------------------- */
    for (i = 0; i < BUILDING_TYPE_COUNT; i++) {
        const BuildingDef *def  = &BUILDING_DEFS[i];
        SDL_FRect          r    = slot_rect(screen_w, screen_h, i);
        int                hovr = ui_hit_test(screen_w, screen_h,
                                              mouse_x, mouse_y) == i;
        int                sel  = (selected == (BuildingType)i);

        /* Slot background: slightly lighter when hovered */
        if (hovr || sel)
            SDL_SetRenderDrawColor(renderer, 60, 50, 35, 255);
        else
            SDL_SetRenderDrawColor(renderer, 40, 33, 22, 255);
        SDL_RenderFillRect(renderer, &r);

        /* Building colour swatch (inner rectangle) */
        SDL_FRect swatch = {
            r.x + 8.0f, r.y + 8.0f,
            r.w - 16.0f, r.h - 24.0f   /* leave room for label */
        };
        SDL_SetRenderDrawColor(renderer,
            def->col_r, def->col_g, def->col_b, 255);
        SDL_RenderFillRect(renderer, &swatch);

        /* Slot border: gold if selected, dim grey otherwise */
        if (sel)
            SDL_SetRenderDrawColor(renderer, 255, 210, 50, 255);
        else if (hovr)
            SDL_SetRenderDrawColor(renderer, 160, 140, 90, 255);
        else
            SDL_SetRenderDrawColor(renderer, 70, 60, 40, 255);
        SDL_RenderRect(renderer, &r);

        /* Building size annotation (small dot grid in corner) */
        /* e.g. 2x2 gets four dots, 1x1 gets one */
        {
            int dw = def->tile_w, dh = def->tile_h;
            int dr, dc;
            float dot_area_x = r.x + r.w - 4.0f - (float)(dw) * 5.0f;
            float dot_area_y = r.y + r.h - 4.0f - (float)(dh) * 5.0f;
            SDL_SetRenderDrawColor(renderer, 200, 180, 120, 200);
            for (dr = 0; dr < dh; dr++) {
                for (dc = 0; dc < dw; dc++) {
                    SDL_FRect dot = {
                        dot_area_x + (float)(dc) * 5.0f,
                        dot_area_y + (float)(dr) * 5.0f,
                        3.0f, 3.0f
                    };
                    SDL_RenderFillRect(renderer, &dot);
                }
            }
        }
    }

    /* --- Tooltip: building name above hovered slot ------ */
    {
        BuildingType hov = ui_hit_test(screen_w, screen_h,
                                       mouse_x, mouse_y);
        if (hov != BUILDING_NONE) {
            /* We can't render text without SDL_ttf (Phase 5).
             * For now draw a small label rectangle as a stub
             * so the structure is in place. */
            SDL_FRect slot = slot_rect(screen_w, screen_h, (int)hov);
            SDL_FRect tip  = {
                slot.x, slot.y - 22.0f,
                slot.w, 18.0f
            };
            SDL_SetRenderDrawColor(renderer, 50, 42, 28, 240);
            SDL_RenderFillRect(renderer, &tip);
            SDL_SetRenderDrawColor(renderer, 140, 120, 70, 255);
            SDL_RenderRect(renderer, &tip);
            /* Name will be drawn here once we add SDL_ttf */
        }
    }
}
