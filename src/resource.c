/*  resource.c  --  Resource stockpile implementation  (Phase 4)  */

#include "resource.h"

const char *RESOURCE_NAMES[RES_COUNT] = {
    "Wood",
    "Fish",
    "Grain",
    "Gold"
};

void stockpile_init(Stockpile *s)
{
    int i;
    for (i = 0; i < RES_COUNT; i++)
        s->amount[i] = 0;
}

/* stockpile_add -------------------------------------------
 * We clamp to zero rather than allowing negative stock.
 * If a building tries to consume more than is available it
 * simply does nothing — in Phase 5 this will trigger a
 * "needs not met" penalty on population happiness.
 * -------------------------------------------------------- */
void stockpile_add(Stockpile *s, ResourceType res, int delta)
{
    s->amount[res] += delta;
    if (s->amount[res] < 0)
        s->amount[res] = 0;
}
