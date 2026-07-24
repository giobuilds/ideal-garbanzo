#ifndef SIMLOG_H
#define SIMLOG_H

/* =========================================================
 * simlog.h  --  The simulation's logging shim
 *
 * The sim library must build without SDL (MMO_PLAN ground rule 3), and
 * SDL_Log was the last SDL symbol left in otherwise pure sim files:
 * island.c announcing world-gen, population.c reporting a hungry house,
 * game.c narrating save/load. sim_log() replaces it one-for-one.
 *
 * Output shape deliberately matches SDL's default log handler ("INFO: "
 * on stderr) so mixed client output reads uniformly and ci/smoke-test.sh
 * keeps matching the lines it greps for.
 *
 * sim_log_set_enabled(0) silences it — the headless replay tool and the
 * future server host care about the hash, not the narration.
 * ========================================================= */

/* printf-style; a trailing newline is added. */
#if defined(__GNUC__)
void sim_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
void sim_log(const char *fmt, ...);
#endif

/* 1 = print (the default), 0 = discard. Returns the previous setting. */
int  sim_log_set_enabled(int enabled);

#endif /* SIMLOG_H */
