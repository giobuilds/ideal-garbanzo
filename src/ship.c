/*  ship.c  --  Vessels moving goods between islands  */

#include "ship.h"

void ships_update(Ship ships[], int ship_count, float dt)
{
    int i;

    for (i = 0; i < ship_count; i++) {
        Ship *s = &ships[i];

        if (!s->active) continue;
        if (s->at_island >= 0) continue;   /* docked: nothing to do */

        s->progress += dt / SHIP_VOYAGE_SECONDS;

        if (s->progress >= 1.0f) {
            s->progress  = 0.0f;
            s->at_island = s->to_island;   /* arrived */
        }
    }
}

int ships_cargo_total(const Ship ships[], int ship_count, ResourceType res)
{
    int i, total = 0;

    for (i = 0; i < ship_count; i++)
        if (ships[i].active)
            total += ships[i].cargo[res];

    return total;
}
