/*  main.c  --  A city-builder/economy sim like Anno 1800
 *              built in C and SDL3
 *
 *  by Giovanni Dick
 *  14 Mar 2026
 *
 *  SDL3 "main callbacks" model:
 *    SDL_AppInit     – called once at startup
 *    SDL_AppEvent    – called for every OS/input event
 *    SDL_AppIterate  – called once per frame
 *    SDL_AppQuit     – called once at shutdown
 *
 *  We store all game state in a heap-allocated GameState
 *  and hand a pointer to SDL via the appstate parameter.
 *  This avoids global variables and keeps each callback
 *  self-contained.
 */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.h"
#include "render.h"

/* ---- SDL_AppInit ---------------------------------------
 * One-time setup: create the window and renderer, then
 * initialise game state and store it in *appstate so the
 * other callbacks can retrieve it.
 * -------------------------------------------------------- */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Window   *window   = NULL;
    SDL_Renderer *renderer = NULL;
    GameState    *gs       = NULL;

    /* Suppress unused-parameter warning for argv/argc
     * (we don't parse command-line args in Phase 1). */
    (void)argc;
    (void)argv;

    /* --- App metadata ---------------------------------- */
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING,
                               "Anno-Clone");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING,
                               "0.1.0");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING,
                               "com.giovannidick.annoclone");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING,
                               "Giovanni Dick");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
                               "Copyright (c) 2026 Giovanni Dick");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING,
                               "game");

    /* --- SDL init -------------------------------------- */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialise SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* --- Window & renderer ----------------------------- */
    if (!SDL_CreateWindowAndRenderer("Anno Clone  –  Phase 1",
                                     SCREEN_W, SCREEN_H,
                                     0,           /* no special flags */
                                     &window,
                                     &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Logical presentation keeps the coordinate system fixed at
     * 1920×1080 even if the OS scales the window. */
    SDL_SetRenderLogicalPresentation(renderer,
                                     SCREEN_W, SCREEN_H,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    /* --- Game state ------------------------------------ */
    gs = game_init();
    if (!gs) {
        SDL_Log("Couldn't allocate game state");
        return SDL_APP_FAILURE;
    }

    /* Pack both pointers into appstate.
     * We use a small heap struct so AppQuit can free them. */
    typedef struct { SDL_Window *w; SDL_Renderer *r; GameState *g; } App;
    App *app = (App *)SDL_malloc(sizeof(App));
    if (!app) {
        game_free(gs);
        SDL_Log("Couldn't allocate app struct");
        return SDL_APP_FAILURE;
    }
    app->w = window;
    app->r = renderer;
    app->g = gs;

    *appstate = app;

    SDL_Log("Anno Clone Phase 1 started. WASD / arrows to pan.");
    return SDL_APP_CONTINUE;
}

/* ---- SDL_AppEvent --------------------------------------
 * Forward every event to the input handler.
 * -------------------------------------------------------- */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    typedef struct { SDL_Window *w; SDL_Renderer *r; GameState *g; } App;
    App *app = (App *)appstate;

    return input_handle_event(&app->g->input, event);
}

/* ---- SDL_AppIterate ------------------------------------
 * Called every frame.  Order: update → clear → draw → present.
 * -------------------------------------------------------- */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    typedef struct { SDL_Window *w; SDL_Renderer *r; GameState *g; } App;
    App *app = (App *)appstate;

    /* 1. Update game logic (camera pan, hovered tile) */
    game_update(app->g);

    /* 2. Clear the screen */
    render_clear(app->r);

    /* 3. Draw the tile map */
    render_map(app->r, &app->g->map, &app->g->camera);

    /* 4. Draw hover highlight on top of tiles */
    render_hovered_tile(app->r, &app->g->camera,
                        app->g->hovered_row,
                        app->g->hovered_col);

    /* 5. Push the finished frame to the screen */
    SDL_RenderPresent(app->r);

    return SDL_APP_CONTINUE;
}

/* ---- SDL_AppQuit ---------------------------------------
 * Free everything we allocated.  SDL cleans up the window
 * and renderer automatically after this returns.
 * -------------------------------------------------------- */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    typedef struct { SDL_Window *w; SDL_Renderer *r; GameState *g; } App;
    App *app = (App *)appstate;

    (void)result;   /* not checking exit code in Phase 1 */

    if (app) {
        game_free(app->g);
        SDL_free(app);
    }
}
