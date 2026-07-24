/*  replay_main.c  --  saltmarch_replay: the headless twin's front door
 *                     (MMO_PLAN Phase 6)
 *
 *  Twenty lines of main() over libsaltmarch_sim. No SDL, no window, no
 *  assets — which is the point: if this builds and runs, the simulation
 *  really is separable from the client, and the proof is re-checked on
 *  every CI run on all three platforms.
 *
 *  Usage:
 *    saltmarch_replay --record FILE [--seed N]
 *    saltmarch_replay --replay FILE [--expect-hash HEX]
 *
 *  Exit status is the test result: 0 means the world replayed to the
 *  same hash, non-zero means the sim went nondeterministic.
 */

#include "replay.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (!replay_cli_requested(argc, argv)) {
        fprintf(stderr, "usage: %s %s\n",
                argc > 0 ? argv[0] : "saltmarch_replay", replay_cli_usage());
        return 2;
    }
    return replay_cli_run(argc, argv);
}
