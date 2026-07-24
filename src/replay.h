#ifndef REPLAY_H
#define REPLAY_H

/* =========================================================
 * replay.h  --  The headless record/replay harness
 *               (MMO_PLAN Phase 1d, formalised in Phase 6)
 *
 * A world is (seed, ordered command log). Replaying one and hashing the
 * result is therefore the project's determinism test — and, run on
 * three operating systems, its cross-platform float divergence fuzzer.
 *
 * This lives in the SDL-free sim library so the same code serves two
 * front ends: `saltmarch --replay ...` (the game binary short-circuits
 * before opening a window) and `saltmarch_replay`, the standalone CLI
 * that links no SDL at all and is what CI actually runs.
 * ========================================================= */

#include <stdint.h>
#include "game.h"

/* A scripted session that exercises the float-sensitive paths on
 * purpose — a house (population and agents run), a purchase, a ship and
 * a voyage (progress accumulates) — then 500 ticks. Leaves gs holding
 * the finished world; --record saves it as a .smlog fixture. */
void replay_record_demo_session(GameState *gs, uint32_t seed);

/* 1 if argv contains a mode flag this module handles (--record or
 * --replay), so the game binary knows to stay headless. */
int replay_cli_requested(int argc, char *argv[]);

/* Run the CLI. Returns a process exit code: 0 on success, 1 on failure
 * (unreadable file, nondeterministic replay, or a hash that does not
 * match --expect-hash).
 *
 *   --record FILE [--seed N]        write a fixture
 *   --replay FILE [--expect-hash H] load, self-check, optionally pin
 */
int replay_cli_run(int argc, char *argv[]);

/* One line of usage text for --help output, without a trailing newline. */
const char *replay_cli_usage(void);

#endif /* REPLAY_H */
