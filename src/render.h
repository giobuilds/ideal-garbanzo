#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include "map.h"
#include "camera.h"
#include "building.h"
#include "resource.h"
#include "population.h"   /* Phase 5 */

void render_clear(SDL_Renderer *renderer);

void render_map(SDL_Renderer *renderer,
                const Map *map, const Camera *cam);

void render_hovered_tile(SDL_Renderer *renderer,
                         const Camera *cam,
                         int row, int col);

void render_buildings(SDL_Renderer *renderer,
                      const Building buildings[], int count,
                      const Camera *cam);

void render_ghost(SDL_Renderer *renderer,
                  const Camera *cam,
                  BuildingType type,
                  int row, int col,
                  int valid);

void render_resources(SDL_Renderer *renderer,
                      const Stockpile *s);

/* Phase 5: population counter top-right */
void render_population(SDL_Renderer *renderer,
                       int total_pop,
                       int screen_w);

/* CHANGED: returns float positions so zoomed tiles sit flush with no gaps */
void iso_to_screen(int row, int col, const Camera *cam,
                   float *out_x, float *out_y);

void screen_to_iso(int sx, int sy, const Camera *cam,
                   int *out_row, int *out_col);

#endif /* RENDER_H */
