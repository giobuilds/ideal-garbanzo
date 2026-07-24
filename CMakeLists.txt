# 3.21 for TARGET_RUNTIME_DLLS, used by the Windows DLL copy below.
cmake_minimum_required(VERSION 3.21)
project(saltmarch C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
elseif(MSVC)
    # Without this branch MSVC builds with no warning flags at all, so
    # the project's "zero warnings is the bar" rule would silently apply
    # to three platforms out of four.
    #
    # /W4 is the closest practical equivalent to -Wall -Wextra. MSVC has
    # no direct -Wconversion analogue; C4244/C4267 (narrowing) are the
    # useful part of it and are on at /W4.
    #
    # /std:c17 is not optional: the code uses ~46 designated initialisers
    # (RESOURCE_NAMES, PROFILE_PARAMS, BUILDING_DEFS costs), which MSVC
    # only accepts in C mode from C11 onward. The default C mode rejects
    # them outright.
    add_compile_options(/W4 /std:c17)
endif()

# Opt-in, used by CI: make any warning fail the build. Off by default so
# local work is never blocked mid-experiment, on in CI so the bar is
# actually enforced rather than merely stated.
option(SALTMARCH_WERROR "Treat compiler warnings as errors" OFF)
if(SALTMARCH_WERROR)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Werror)
    elseif(MSVC)
        add_compile_options(/WX)
    endif()
endif()

find_package(SDL3       REQUIRED CONFIG)
find_package(SDL3_ttf   REQUIRED CONFIG)

# ---------------------------------------------------------------------
# The headless twin (MMO_PLAN Phase 6).
#
# The world is a pure function of (seed, ordered command log), so it can
# — and now does — build with no client attached: no SDL, no window, no
# assets. Everything below is the simulation proper plus the two things
# it legitimately owns at the edges (the save/replay format and the
# logging shim that replaced SDL_Log).
#
# The rule this target enforces is MMO_PLAN ground rule 3: the sim may
# not touch SDL. It is enforced twice over — this library links no SDL,
# so an SDL call fails to link, and ci/sim-sdl-free.sh greps the sources
# so it fails even earlier and with a better message.
# ---------------------------------------------------------------------
set(SALTMARCH_SIM_SOURCES
    src/game.c
    src/command.c
    src/map.c
    src/camera.c
    src/building.c
    src/resource.c
    src/population.c
    src/connectivity.c
    src/agent.c
    src/island.c
    src/ship.c
    src/faction.c
    src/replay.c
    src/simlog.c
)

add_library(saltmarch_sim STATIC ${SALTMARCH_SIM_SOURCES})
target_include_directories(saltmarch_sim PUBLIC ${CMAKE_SOURCE_DIR}/src)

# The client: window, input, rendering, UI overlays, and the two
# out-of-process-ish extras (the shared feed, the co-op socket layer).
# Everything here may use SDL; nothing here may be needed by the sim.
set(SALTMARCH_CLIENT_SOURCES
    src/main.c
    src/client.c
    src/input.c
    src/render.c
    src/ui.c
    src/fonts.c
    src/trade_ui.c
    src/build_confirm_ui.c
    src/demolish_confirm_ui.c
    src/tier_upgrade_ui.c
    src/world_ui.c
    src/feed.c
    src/net.c
    src/escrow_ui.c
)

add_executable(saltmarch ${SALTMARCH_CLIENT_SOURCES})

# The CLI half of the headless twin: loads a .smlog, replays it, prints
# the hash, exits 0/1. This is the determinism harness CI runs, and it
# links no SDL at all.
add_executable(saltmarch_replay src/replay_main.c)
target_link_libraries(saltmarch_replay PRIVATE saltmarch_sim)

# Copy assets next to the binary after every build. fonts.c resolves the
# font via SDL_GetBasePath() (the executable's directory), which is what
# makes one code path work on Linux, macOS and Windows -- but only if
# the assets are actually there. Doing it as a POST_BUILD step means a
# plain `cmake --build` is enough; no install step required.
add_custom_command(TARGET saltmarch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:saltmarch>/assets
    COMMENT "Staging assets/ beside the executable"
)

# Windows has no rpath and no system library directory: an .exe finds its
# DLLs next to itself or not at all. Without this, saltmarch.exe builds
# perfectly and then dies on launch with "SDL3.dll was not found" -- the
# same shape of failure as the missing font, where the build is green and
# the program is unusable.
#
# TARGET_RUNTIME_DLLS resolves the imported SDL3::SDL3 / SDL3_ttf::SDL3_ttf
# targets to their actual .dll paths, so this keeps working if the SDL
# version or install location changes. The genex is empty on non-DLL
# platforms, hence the if(WIN32) guard -- and copy_if_different tolerates
# an empty argument list on the first build.
if(WIN32)
    add_custom_command(TARGET saltmarch POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:saltmarch> $<TARGET_FILE_DIR:saltmarch>
        COMMAND_EXPAND_LISTS
        COMMENT "Staging SDL runtime DLLs beside the executable"
    )
endif()

target_link_libraries(saltmarch PRIVATE
    saltmarch_sim
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)

# The math library is a separate link target on Unix (-lm) but does not
# exist on MSVC, where the math functions live in the C runtime -- naming
# it unconditionally fails the Windows link with
# "LNK1104: cannot open file 'm.lib'".
#
# find_library rather than if(UNIX): it asks whether libm exists instead
# of predicting which platforms have one, so this stays correct on
# platforms nobody here has tried.
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    # PUBLIC on the sim library: map generation's noise is the heaviest
    # libm user, so anything linking the sim (the replay tool, the future
    # server host) needs -lm whether or not it calls one itself.
    target_link_libraries(saltmarch_sim PUBLIC ${MATH_LIBRARY})
    target_link_libraries(saltmarch     PRIVATE ${MATH_LIBRARY})
endif()

# Winsock for the co-op TCP layer (net.c). BSD sockets need no extra
# library on Linux/macOS.
if(WIN32)
    target_link_libraries(saltmarch PRIVATE ws2_32)
endif()
