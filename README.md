# Project Architecture (High Level)
anno1800-clone/
├── src/
│   ├── main.c           ← your skeleton
│   ├── game.c/h         ← game state, main loop logic
│   ├── map.c/h          ← isometric tile map
│   ├── render.c/h       ← all SDL draw calls
│   ├── input.c/h        ← mouse/keyboard handling
│   ├── economy.c/h      ← production chains, resources
│   ├── population.c/h   ← residents, needs, tiers
│   └── ui.c/h           ← HUD, panels, menus
├── assets/
│   ├── tiles/
│   └── sprites/
├── CMakeLists.txt
└── Makefile

# Deliverables
File        Responsibility
main.c      SDL callbacks only — no logic, no globals
map.h/c     40×40 tile grid; TILE_GRASS, WATER, FOREST, SAND
camera.h/c  offset_x/y scroll state; centred on FullHD at init
input.h/c   Tracks held WASD/arrow keys + mouse position
render.h/c  Isometric projection math + coloured diamond drawing
game.h/c    Owns all sub-systems; game_update() drives camera + hover