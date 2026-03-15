#ifndef RESOURCE_H
#define RESOURCE_H

/* =========================================================
 * resource.h  --  Resource types and stockpile  (Phase 4)
 *
 * A Stockpile holds one integer count per ResourceType.
 * All buildings read and write the single global Stockpile
 * that lives in GameState.
 *
 * GOLD is special: it is a currency, not a physical good.
 * In Phase 4 it simply accumulates (no spending yet).
 * Spending mechanics arrive in Phase 5 with population needs.
 * ========================================================= */

/* ---- Resource types ------------------------------------ */
typedef enum {
    RES_WOOD  = 0,
    RES_FISH  = 1,
    RES_GRAIN = 2,
    RES_GOLD  = 3,
    RES_COUNT          /* always last */
} ResourceType;

/* Human-readable name for each resource (for debug / future UI). */
extern const char *RESOURCE_NAMES[RES_COUNT];

/* ---- Stockpile ----------------------------------------- */
typedef struct {
    int amount[RES_COUNT];   /* current count per resource   */
} Stockpile;

/* Initialise all amounts to zero. */
void stockpile_init(Stockpile *s);

/* Add `delta` units of `res` to the stockpile.
 * delta may be negative (consumption).
 * Clamps to zero on the low end — stock never goes negative. */
void stockpile_add(Stockpile *s, ResourceType res, int delta);

#endif /* RESOURCE_H */
