#ifndef MAP_H
#define MAP_H

/* =========================================================
 * map.h  --  Tile map data structures
 *
 * The world is a 2D grid of tiles.  Each tile has a type
 * (grass, water, forest …) and a screen position is derived
 * from its grid coordinates using isometric projection.
 *
 * Isometric projection (the "2:1 diamond" used by Anno):
 *
 *   screen_x = (col - row) * (TILE_W / 2)
 *   screen_y = (col + row) * (TILE_H / 2)
 *
 * TILE_W and TILE_H refer to the full bounding box of one
 * tile diamond, not the visible edge length.
 * ========================================================= */

/* Tile pixel dimensions  (64 wide, 32 tall) */
#define TILE_W 64
#define TILE_H 32

/* Map size in tiles */
#define MAP_COLS 40
#define MAP_ROWS 40

/* ---- Tile types ----------------------------------------- */
typedef enum {
    TILE_GRASS  = 0,
    TILE_WATER  = 1,
    TILE_FOREST = 2,
    TILE_SAND   = 3,
    TILE_TYPE_COUNT          /* always last – gives us the count */
} TileType;

/* ---- One tile in the grid ------------------------------- */
typedef struct {
    TileType type;
    int      elevation;   /* reserved for later (hills etc.)  */
} Tile;

/* ---- The whole map -------------------------------------- */
typedef struct {
    Tile tiles[MAP_ROWS][MAP_COLS];
    int  rows;
    int  cols;
} Map;

/* ---- Function declarations ----------------------------- */

/* Initialise every tile to TILE_GRASS (or a simple pattern). */
void map_init(Map *map);

/* Return a pointer to the tile at (row, col), or NULL if out of
 * bounds.  Callers should always check for NULL. */
Tile *map_get_tile(Map *map, int row, int col);

#endif /* MAP_H */
