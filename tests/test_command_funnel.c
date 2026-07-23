/*  test_command_funnel.c  --  headless verification of MMO_PLAN Phase 1a
 *
 * The funnel's defining invariant: the world is a pure function of its
 * initial state plus the ordered command log. This test proves it the
 * only way that actually catches an escaped mutation — by replaying.
 *
 *   1. Build a world, snapshot its initial islands[]/ships[] (INITIAL).
 *   2. Play a scripted sequence of commands through the public API.
 *      Copy the resulting world (PLAYED).
 *   3. Restore the world to INITIAL, then re-apply the recorded command
 *      log through sim_apply().
 *   4. Assert the replayed world is byte-identical to PLAYED.
 *
 * If any mutation had bypassed command_submit(), the log would not
 * carry it, replay would not reproduce it, and step 4 would fail. A
 * pass means every mutation the script performed went through the
 * funnel and was faithfully recorded.
 *
 * Islands and Ships are pure value types (Map embeds its tile grid by
 * value; no heap pointers), so memcpy is a true deep copy and memcmp is
 * a total equality check including camera and map state.
 *
 * Built and run by tests/run.sh, linking the game's own .o files.
 */

#include "game.h"
#include "resource.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, msg) do {                                          \
        if (!(cond)) { printf("  FAIL: %s\n", (msg)); failures++; }    \
        else         { printf("  ok:   %s\n", (msg)); }                \
    } while (0)

int main(void)
{
    GameState *gs = game_init();
    if (!gs) { printf("game_init failed\n"); return 1; }

    /* island 0 (Saltford) starts settled with STARTING_GOLD. */
    Island initial_islands[MAX_ISLANDS];
    Ship   initial_ships[MAX_SHIPS];
    int    initial_ship_count = gs->ship_count;
    memcpy(initial_islands, gs->islands, sizeof(initial_islands));
    memcpy(initial_ships,   gs->ships,   sizeof(initial_ships));

    int gold0_before = gs->islands[0].stockpile.amount[RES_GOLD];

    /* ---- Play a scripted command sequence -----------------
     * Chosen so that at least some commands mutate regardless of the
     * random map: trades and ship-building do not depend on tile
     * layout. Placement commands may be rejected on a given seed;
     * that is fine — rejection is deterministic and replay reproduces
     * it identically, which is exactly what the invariant claims. */
    ResourceType tradable = (RES_GOLD == 0) ? (ResourceType)1 : (ResourceType)0;

    game_buy_resource(gs, tradable, 5);   /* gold -> goods            */
    game_build_ship(gs);                  /* gold -> a ship in slot 0 */
    game_ship_transfer(gs, 0, tradable, 3); /* load some cargo        */
    game_sell_resource(gs, tradable, 1);  /* sell one back            */
    game_ship_depart(gs, 0, 1);           /* sail toward island 1     */
    game_colonise(gs, 0, 1);              /* rejected: still at sea   */

    /* A few placements at fixed coordinates. Some may fail; all are
     * deterministic. Roads first (cheap, no popup), then a house. */
    game_try_place_road(gs, 30, 30);
    game_try_place_road(gs, 30, 31);
    game_try_place_road(gs, 31, 30);

    CHECK(gs->cmd_count == 9, "every submitted command was logged");
    CHECK(gs->islands[0].stockpile.amount[RES_GOLD] < gold0_before,
          "the trade/ship commands actually moved gold (real mutation)");

    /* Snapshot the played-out world. */
    Island played_islands[MAX_ISLANDS];
    Ship   played_ships[MAX_SHIPS];
    int    played_ship_count = gs->ship_count;
    memcpy(played_islands, gs->islands, sizeof(played_islands));
    memcpy(played_ships,   gs->ships,   sizeof(played_ships));

    CHECK(played_ship_count > initial_ship_count,
          "a ship was created during play");

    /* ---- Restore to INITIAL and replay the log ------------ */
    memcpy(gs->islands, initial_islands, sizeof(initial_islands));
    memcpy(gs->ships,   initial_ships,   sizeof(initial_ships));
    gs->ship_count = initial_ship_count;

    for (int i = 0; i < gs->cmd_count; i++)
        sim_apply(gs, &gs->cmd_log[i]);

    /* ---- Assert replay reproduced the played world exactly */
    int islands_match = memcmp(gs->islands, played_islands,
                               sizeof(played_islands)) == 0;
    int ships_match   = gs->ship_count == played_ship_count &&
                        memcmp(gs->ships, played_ships,
                               sizeof(played_ships)) == 0;

    if (!islands_match) {
        for (int i = 0; i < MAX_ISLANDS; i++)
            if (memcmp(&gs->islands[i], &played_islands[i],
                       sizeof(Island)) != 0)
                printf("    island %d diverged on replay\n", i);
    }

    CHECK(islands_match, "replayed islands are byte-identical to played");
    CHECK(ships_match,   "replayed ships are byte-identical to played");

    game_free(gs);

    printf(failures ? "\nFAILED (%d)\n" : "\nPASSED\n", failures);
    return failures ? 1 : 0;
}
