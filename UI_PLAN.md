# UI/UX Reorganisation Plan

> Status: **planned, not started.** Written for a future session to pick up
> cold. Nothing in here has been implemented.

## Why

The roadmap is ~19 production chains implying **25+ goods and 30-40
buildings**. The current UI is 1585 lines across six hand-rolled files and
will not survive that.

Measured against the real constants (not estimated — computed from
`ui.h`, `trade_ui.h` and `render.c`):

| surface | capacity | used today | headroom |
|---|---|---|---|
| **Trade screen** (`TRADE_H = 110 + 102*N`) | **10 goods** | **6** | **4 goods** |
| Dock bar (right-anchored buttons start at x=1684) | 22 buildings | 11 | 11 |
| Resource panel (22px rows) | ~43 goods | 7 | 36 |

**The nearest cliff is the trade screen, not the dock bar.** It is 1028px
at 9 goods and 1130px at 10 — 50px off-screen. The failure is *silent*:
buttons render outside the window and simply stop being clickable. Four
more resources triggers it.

### Two pre-existing bugs found while measuring

Both verified, both worth folding into this work:

1. **Mouse wheel is not overlay-aware.** `game_update()` (`game.c`) applies
   `input.scroll_y` to camera zoom with no check on any overlay flag, so
   scrolling over an open modal silently zooms the world behind it.
2. **`building_can_place()`'s `reason` string is dead code.** Every call
   site passes `NULL, 0`. The "why can't I build here" explanation has
   existed since the original build and has never been shown to a player.

---

## Decisions

### No layout library — build a ~250-line in-house `ui_kit`

Nuklear and Clay were both evaluated. **Neither earns its place.**

The decisive argument is specific to this repo: **Clay's hover/hit model
requires a retained layout pass**, which would break the headless
`.o`-linking test programs that are the *only* automated verification
available in this environment (there is no `xdotool`, so anything visual
needs a human). Those tests work precisely because `*_hit_test()` is a
pure function of `(screen_w, screen_h, x, y)`.

Supporting arguments:
- The canvas is a fixed 1920x1080 (`SDL_LOGICAL_PRESENTATION_STRETCH`), so
  the single biggest reason to want a layout engine — responsive reflow —
  does not apply.
- The shapes actually needed are a horizontal strip of equal cells, a
  vertical stack of equal rows, and a centred panel. Three functions.
- Third-party headers under `-Wall -Wextra -Wpedantic -Wshadow
  -Wconversion` means institutionalising a warning exemption, against
  CLAUDE.md's "treat new warnings as bugs, not noise to suppress".
- Ethos: this repo deliberately *deleted* `src/sprite.c`. A third-party
  layout engine to position ~40 rectangles is what that deletion was about.

**Nuklear** is additionally all-or-nothing: it owns the frame, input
capture, font atlas and command buffer (~20k lines), and its official SDL
backends target SDL2, not SDL3. Adopting it for one panel still costs the
entire pipeline plus a re-skin to match the existing palette.

**Keep Clay as a documented fallback.** If variable resolution or
wrapped-text panels (a tooltip/encyclopedia system) ever arrive, revisit:
its `CLAY_RENDER_COMMAND_TYPE_RECTANGLE/TEXT/BORDER` maps almost 1:1 onto
`SDL_RenderFillRect` / `font_draw_text` / `SDL_RenderRect`, so later
adoption stays cheap.

### Category tabs on the existing dock bar (not a full-screen build menu)

### Rule-driven vitals strip + inventory overlay (not "show everything")

---

## The three-layer framing

The clutter fixes are **design** decisions; layout plumbing only helps
*implement* them. Two amendments to that:

1. **The plumbing has a correctness role.** `TRADE_H` is a compile-time
   macro nobody compares against `SCREEN_H` — that is exactly why the
   overflow is silent. Moving to *measured-then-clamped* geometry converts
   "renders off-screen" into "paginates", eliminating a defect class
   regardless of any UX choice.
2. **A data-model layer gates the other two.** Category tabs are not
   implementable until `BuildingDef` carries a category; rule-driven
   vitals are not until resources do.

Order of work: **data model → UX design → plumbing.**

> **Use designated initialisers for every new parallel table.** The
> `RES_COL` bug — 4 positional entries for 7 resources, silently
> mis-colouring Hops and blanking Malt/Beer/Gold — is exactly this failure
> mode. `RESOURCE_NAMES` / `SELL_PRICE` / `BUY_PRICE` already do it right.

---

## `ui_kit.c` / `.h` — scope

Not a framework. The smallest thing that makes geometry authored once.

- **Layout cursor**: `UiLayout { SDL_FRect bounds; float y; float pad; }`,
  with `ui_row(&lay, h)` returning a rect and advancing, and
  `ui_split_h(row, n, i, gap)` for button groups. Panel height becomes a
  **measured output** rather than a `#define`d input.
- **Widget list**: `UiItem { SDL_FRect r; int id; const char *label; ... }`
  in a fixed, stack-allocated `UiList` — no malloc.
- `ui_kit_draw(renderer, list, mx, my)` / `ui_kit_hit(list, x, y)`, plus
  the one canonical `ui_point_in()` replacing the four verbatim copies in
  `trade_ui.c`, `build_confirm_ui.c`, `demolish_confirm_ui.c` and
  `tier_upgrade_ui.c`.

**Each overlay gains one `*_build(UiList*, ...)`**; then `*_draw()` =
build + `ui_kit_draw`, and `*_hit_test()` = build + `ui_kit_hit`.

Critical property: **the list is built per call, never retained**, so
`*_hit_test(screen_w, screen_h, x, y)` stays a pure function and every
existing headless test keeps working unchanged. That is the whole reason
for building this instead of adopting Clay.

Encode ids as e.g. `res * 8 + button`, decoded on the way out — the same
trick `ui.c` already uses with `return (MenuHit)(i + 1)`.

Float-throughout internally; `int` only at the `font_draw_text` boundary,
to keep `-Wconversion` quiet.

---

## Phases

Each is independently shippable and verifiable.

### Phase 0 — `ui_kit`
Layout cursor, widget list, canonical `ui_point_in`. No caller changes;
add to `CMakeLists.txt`.
**Verify:** headless program asserting `ui_row()` stacking and
`ui_split_h()` gap arithmetic. Not user-visible.

### Phase 1 — Trade screen rewrite (retires the cliff)
Today: a 92px block per good with two 3-button rows — 150 buttons and
2300px at 25 goods.

- **One 34px row per good**: swatch | name | stock | prices | `[-10][-1]`
  | `[+1][+10]` | `[All/Max]`. Five buttons instead of six, 34px instead
  of 92 — a 2.7x density win before pagination.
- Widen `TRADE_W` 480 → ~760; there is room on a 1920 canvas.
- **Height computed then clamped** to `SCREEN_H - 2*MARGIN`; overflow
  paginates with `[Prev] Page 1/2 [Next]` beside Close. ~26 rows fit, so
  pagination will not engage until ~26 goods — but the clamp means it
  *cannot* silently fail at 27.
- **Pagination, not scrolling** — scrolling would require fixing the
  mouse-wheel bug first (Phase 4).
- Group rows by resource category; put the island name in the title.

**Verify headless:** for N in {6, 10, 25, 40}, assert every emitted rect
lies fully inside 1920x1080, and that `trade_ui_hit_test` at each button's
centre round-trips to the right `(resource, qty)`.

### Phase 2 — Data model
`BuildingCategory` on `BuildingDef` (CAT_INFRA / RESIDENTIAL / RAW /
PRODUCTION / PUBLIC) and a resource-category table in `resource.c`. Both
with designated initialisers. No UI change.
**Verify:** headless assert that *every* enum value has a non-default
category — catching "added a row, forgot the table" directly.

### Phase 3 — HUD category tabs
Depends on Phase 2. Raise `HUD_HEIGHT` 80 → ~112, add a 28px tab strip,
add `gs->hud_category`. `hud_slot_count()` / `hud_slot_type()` gain a
category argument — the existing "filter to a contiguous list" logic
generalises directly. Capacity becomes 22 x 5 = 110.

- Sticky selected tab. Slot click = select/deselect toggle, unchanged.
- Right-click still only deselects the building; it must **not** also
  reset the tab, or right-click starts meaning two different things.
- **Unavailable buildings render greyed but present, never hidden**, and
  **wire `building_can_place()`'s dead `reason` string into the existing
  hover tooltip.** Hiding what the player is working toward is the
  commonest city-builder UX mistake, and the explanation already exists
  unused.

**Verify headless:** with a synthetic 40-entry def table, assert every
tab's slots fit left of `world_rect()`'s x, and `ui_hit_test` returns the
right type per tab. Check nothing else assumes `HUD_HEIGHT == 80`.

### Phase 4 — Resource vitals + inventory overlay
Depends on Phase 2. Replaces `render_resources`.

**Vitals strip** (always-on, top-left), contents derived **by rule, not by
value** — "show only non-zero" is wrong, because it reflows every time a
chain stalls and hides the very resource you are waiting for:
- Gold, always, first.
- Every good consumed by a population tier present on this island, derived
  from `population.c`'s needs table so it self-maintains as tiers are added.
- Anything in an **alert state**: zero-while-consumed, or at capacity and
  therefore wasting production.
- Capped at 8 rows; overflow shows `+k more`, which is the click target.

**Inventory overlay**: full `RES_COUNT`, 2-3 columns by category, reusing
the existing segmented amount/capacity bar. Add `gs->inventory_open`.

Also in this phase, since it touches the same machinery:
- **Fix the mouse-wheel bug** — gate camera zoom on no-overlay-open.
- **Add `OverlayKind game_topmost_overlay(const GameState *gs)`** in
  `game.c`. Overlay priority is currently encoded in *three* separate
  places in `SDL_AppIterate` — the click chain, the right-click chain and
  the render order — and adding `inventory_open` to two of the three fails
  silently. State the priority once.

### Phase 5 — Island context
Per-island stockpiles are the highest-consequence hidden state in the
game, and nothing outside the archipelago map says which island you are
on. The failure mode is trading away the wrong island's goods.

- `‹ Island Name ›` header top-centre — the gap between the vitals strip
  at x=16 and the pop counter at `screen_w-110` is already free. Chevrons
  cycle **settled** islands via `game_set_current_island()`. Re-fetch
  `isl` after any switch — the constraint `main.c` already documents.
- Island name in the trade and inventory titles.
- Per-island hue stored in `Island`, tinting the header and overlay titles.

### Phase 6 — Consolidation
`demolish_confirm_ui.c` and `tier_upgrade_ui.c` are already near-identical
(same width, same height formula, same two-button footer), and `main.c`
*already* reuses `tier_upgrade_ui_draw()` for the ship-build popup with a
comment noting a second near-identical file would be duplication — three
popups sharing two implementations. Collapse into one `confirm_ui.c`
(~239 lines → ~130). Port `build_confirm_ui.c` (it has a genuine extra
concept: the payment-mode radio).
**Verify:** assert hit-test results are identical before and after.

---

## Explicitly out of scope

- **`world_ui.c` stays as-is.** Island nodes are projected map geometry via
  `render_draw_diamond`, not a layout; forcing it through a row/column kit
  gains nothing. Only lift its `point_in` to the shared one.
- **Font caching — deferred until measured.** See risks.

## Throwaway work to avoid

- A standalone "clamp `TRADE_H`" hotfix — Phase 1 deletes it. Only do it if
  something must ship before Phase 0+1 can land.
- Porting any confirm popup to `ui_kit` before Phase 1 validates the kit's
  shape. Let the hardest consumer define the API.
- Hand-tuning trade row pixel constants before pagination exists.

---

## Risks

### Text throughput — the headline risk
`font_draw_text` (`fonts.c`) does `TTF_RenderText_Blended` →
`SDL_CreateTextureFromSurface` → `SDL_RenderTexture` → `SDL_DestroyTexture`
**every call, every frame**; its own docstring flags this. Today's HUD is
~30-40 strings/frame. A 25-row trade table (~100), a 25-row inventory
(~50), plus tabs and headers reaches 150-250 strings/frame — 9-15k
rasterise+upload cycles per second. It will present as "the trade screen is
laggy", not as a font problem.

Mitigation, in ascending cost:
1. Free: the heavy screens are modal and mutually exclusive. Keeping
   always-on text small is already the Phase 4 design.
2. **`TTF_Text` objects** — `TTF_CreateRendererTextEngine` +
   `TTF_CreateText` / `TTF_DrawRendererText`. SDL_ttf caches the rendered
   result and re-rasterises only when the string changes (a few times a
   second for stockpile numbers, not 60). **Verified available: SDL3_ttf
   3.2.2 is installed and exposes this API.** Confined to `fonts.c` behind
   the existing `font_draw_text` signature.
3. A full glyph atlas — don't; option 2 gets most of it for far less code.

**Act reactively:** add a frame-time readout before Phase 1 to establish a
baseline, then act only if Phase 1 or 4 measurably regresses it.

### Save-file fragility
`game_save` writes `Stockpile` and `Building` byte-for-byte, and every
added resource changes `sizeof(Stockpile)` via `RES_COUNT` (and
`BuildingDef.cost[RES_COUNT]`). `SAVE_VERSION` exists, so the mechanism is
there — but going 7 → 25 goods will invalidate saves repeatedly. Decide
bump-and-reject vs. writing a resource count into the header and
migrating. Outside this UI work, but on the same roadmap and it will bite
during it.

### Visual verification
No `xdotool` in this environment. Headless tests can assert geometry and
hit-testing only; colour, overlap, legibility and "does the tab strip look
right" need manual confirmation. Phase boundaries are drawn so each is one
coherent thing to eyeball.

---

## Verification convention (unchanged)

Clean rebuild under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` (zero
warnings is the bar), run the binary, and write throwaway headless C
programs linking the built `.o` files that assert **real behaviour**. For
UI that means computing a widget's real on-screen rect from the public
layout constants and asserting the hit test returns the right enum — the
technique already used for the trade screen and the world map.

## Critical files

| file | phase | what |
|---|---|---|
| `src/ui_kit.c` / `.h` | 0 | new — layout cursor, widget list, `ui_point_in` |
| `src/trade_ui.c` / `.h` | 1 | `TRADE_H`, the cliff |
| `src/building.h` | 2 | `BuildingCategory` on `BuildingDef` |
| `src/resource.c` / `.h` | 2 | resource category table |
| `src/ui.c` / `.h` | 3 | `HUD_HEIGHT`, `hud_slot_count`/`hud_slot_type` |
| `src/render.c` | 4 | `render_resources`, `render_population` |
| `src/game.c` / `.h` | 4 | mouse-wheel guard, `game_topmost_overlay()` |
| `src/main.c` | 3-5 | click cascade, right-click chain, render order |
| `src/confirm_ui.c` / `.h` | 6 | new — replaces demolish_confirm + tier_upgrade |
| `src/fonts.c` | 7? | `TTF_Text` migration, only if measured |
