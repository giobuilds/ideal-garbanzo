#!/usr/bin/env bash
# tests/run.sh -- build and run the headless behaviour tests.
#
# Follows the project's verification convention: link the game's own
# compiled .o files (everything except main.c, which owns SDL_AppMain)
# into a small C program that asserts real behaviour, headless. Requires
# the game to have been built first (cmake --build build).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
builddir="$root/build"
linkfile="$builddir/CMakeFiles/saltmarch.dir/link.txt"
simlib="$builddir/libsaltmarch_sim.a"

if [ ! -f "$linkfile" ] || [ ! -f "$simlib" ]; then
    echo "build objects not found; run: cmake -B build && cmake --build build" >&2
    exit 1
fi

# Client objects (minus main.c, which owns SDL_AppMain) plus the sim
# library, which since MMO_PLAN Phase 6 is a separate static archive.
#
# The object list is read out of CMake's own link line rather than
# globbed off disk: after the Phase 6 split, an incremental build leaves
# the pre-split game.c.o etc. sitting in saltmarch.dir/, and a glob would
# happily link those stale duplicates. link.txt is by definition what the
# current configuration actually builds.
objs=$(tr ' ' '\n' < "$linkfile" | grep '\.c\.o$' | grep -v '/main\.c\.o$' \
       | sed "s|^|$builddir/|")
sdlflags=$(pkg-config --cflags --libs sdl3 sdl3-ttf 2>/dev/null || echo "-lSDL3 -lSDL3_ttf")

status=0
for src in "$root"/tests/test_*.c; do
    name=$(basename "$src" .c)
    echo "=== $name ==="

    # test_headless is the exception that proves the Phase 6 split: it
    # links the sim archive ALONE, no client objects and no SDL. Give it
    # those and the test would be meaningless -- it would still link if
    # the sim grew a client dependency tomorrow.
    if [ "$name" = "test_headless" ]; then
        link_objs=""
        link_sdl=""
    else
        link_objs="$objs"
        link_sdl="$sdlflags"
    fi

    # shellcheck disable=SC2086
    cc -std=c99 -Wall -Wextra -I"$root/src" "$src" $link_objs "$simlib" \
       $link_sdl -lm -o "$root/build/$name"
    # Headless: no window/audio device needed for these.
    if ! SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$root/build/$name"; then
        status=1
    fi
    echo
done

exit $status
