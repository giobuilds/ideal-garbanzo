# Saltmarch server

`saltmarch_host` is the persistent world: a C99 process that owns the
canonical command log, ticks in real time whether or not anyone is
connected, and checkpoints the world to a file. It links
`libsaltmarch_sim` (the game's own simulation) and `libsaltmarch_net`
(the game's own protocol). There is no server-side game logic — the
server runs `sim_run_one_tick()`, the same function the client runs,
because a second implementation of the world is exactly the thing this
architecture exists to avoid.

## Running one

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/saltmarch_host --world world.smlog --seed 12345
```

First run creates the world; every run after that resumes `world.smlog`
where it left off. Ctrl-C writes a final checkpoint and exits 0.

| Flag | Meaning |
|---|---|
| `--port N` | listen port (default 7777) |
| `--world FILE` | checkpoint to load/create (default `world.smlog`) |
| `--seed N` | seed for a **new** world; ignored when resuming |
| `--checkpoint-seconds N` | checkpoint interval, `0` = only at shutdown (default 60) |
| `--ticks N` | run N ticks, then checkpoint and exit — how the tests drive it |
| `--quiet` | silence the simulation's narration |

Connecting:

```bash
./build/saltmarch --join your.host:7777          # new player
./build/saltmarch --join your.host:7777 --as 3   # come back as player 3
```

The client cannot tell a dedicated server from a friend running
`--host`; it is the same protocol either way.

## What the server guarantees

- **Continuous time.** The server ticks on a monotonic clock, so a world
  progresses while its players are asleep. Its accumulator is
  deliberately *not* clamped the way the client's is: time the server
  owes is time it pays back, by running the same ticks faster. There is
  no separate "offline production" formula, and there must never be one
  — divergent offline and online rates would break the promise that
  logging off is safe and fair.
- **Restart is not a rollback.** A checkpoint is `(seed, tick, command
  log)` — an ordinary `.smlog`. `saltmarch_replay --replay world.smlog`
  reconstructs the same world and prints the same hash, so the server's
  state format is verifiable with the same tool CI uses.
- **Identity survives disconnects.** Each connection is assigned a
  player id at join; a client may ask for one back with `--as N`, which
  is honoured if that player owns an island and nobody is currently
  connected as them.
- **Privacy is validation.** `sim_apply` rejects commands against
  islands the sender does not own, so being on the same server does not
  give anyone reach into your island. This is enforced by the sim, not
  by the server.

## What it does not do yet

- **No authentication.** `--as N` is an honour system: anyone who knows
  an id can claim it while its owner is away. Fine for friends, not fine
  for a public server. This is the first thing to fix before opening one
  up.
- **No log truncation.** Join transfers the entire command log and the
  client replays it, so join cost grows with world age. The plan's
  answer is a checkpoint containing state rather than history, which
  needs a full-state serialisation format the save file deliberately
  does not have today. At 10 ticks/sec on a 64x64 grid there is a lot of
  headroom before this bites, and `NET_MAX_PEERS` connections is a small
  world anyway.
- **No sharding.** One process, one archipelago, one thread.

## Why it is C99, and what happened to Carbon

MMO_PLAN.md proposed prototyping the host twice — plain threads versus
Carbon's `scheduler` (github.com/carbonengine, MIT), one greenlet per
island — and adopting Carbon only if the greenlet model demonstrably
simplified the code. The host was written in C99 instead, and Carbon was
not prototyped. The honest reasoning:

1. **The host has no concurrency to simplify.** The thing Carbon's
   scheduler is good at is many independent, blocking-ish actors. This
   server is a single deterministic tick loop over four islands with a
   non-blocking socket drain either side of it; it idles at ~0% CPU. A
   greenlet per island would be four greenlets that must run in a fixed
   order to preserve determinism (`island_update`'s ordering constraint
   is not incidental — connectivity.c keeps BFS scratch in file
   statics). Concurrency here buys nothing and costs the one property
   the whole design is built on.
2. **Its real payoff is sharding, which is not close.** Carbon's value
   in the EVE lineage is moving actors across processes. That matters at
   thousands of islands, not four. Adopting it now would be paying the
   integration cost years before the benefit.
3. **C++ would have been the only C++ in the repo.** The client and sim
   are C99 and stay that way (a project non-goal is porting the client
   to C++). One language means the server shares the game's warning
   flags, its build, its tests, and its `.o` files rather than an
   `extern "C"` boundary and a second toolchain on three CI platforms.
4. **It could not have been evaluated honestly here anyway.** The
   development environment for this phase had no access to fetch or
   build Carbon, so prototype (b) would have been a paper exercise
   presented as a comparison.

**What would change this.** Carbon becomes worth revisiting when the
world stops fitting one loop: island counts in the hundreds, or an
appetite to shard islands across processes. At that point the boundary
to cross is the host process only — `libsaltmarch_sim` is C99 with no
opinion about who calls it, which is what keeps this decision cheap to
reverse. That is the actual insurance the Phase 6 split bought, and it
is why the architecture, not the library, was always the point.
