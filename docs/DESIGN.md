# Runik - Design Reference

## Simulation model

Deterministic, tick-based. Each tick:

1. **Units** are computed: a unit is a lone entity or a bonded compound
   (connected component of bonds). A unit lives on exactly one plane. A unit's
   rank is the maximum rank of its members (gems are rank 0).
2. **Movement effects** are gathered from every rune (runes act even while
   bonded, but never on their own unit, and only on their own plane):
   - *Push*: each adjacent unit gets "move 1 hex away".
   - *Pull*: for each of the 6 axes, the first entity within range 3 gets
     "move 1 hex closer" (or "hold" if already adjacent). Anything immune on
     the axis blocks the rest of that ray.
   - *Orbit*: each adjacent unit gets "rotate 60° around me" - CW on
     Material, CCW on Ethereal.
   A rune affects only units of strictly lower rank. Statics are immune to
   everything (but still act, if they are runes).
3. **Conflict resolution** per unit: the highest-rank effect wins; if several
   highest-rank effects disagree, the unit does nothing this tick.
4. **Collision resolution**: proposed moves are cancelled iteratively - board
   edge, overlap with resting units, overlapping destinations, and mutual
   pass-through all cancel. "Trains" (moving into a hex being vacated) are
   allowed. Everything remaining moves simultaneously.
5. **Post-move rune phases**, in order: **Fuse** (bond touching pairs beside a
   fuse rune) → **Detach** (sever bonds beside a detach rune) → **Shift**
   (flip adjacent units to the other plane if the space is free).
6. **Outputs**: a zone consumes when its cells are exactly covered by one
   compound whose gems match the required types (a 1-cell zone requires an
   unbonded gem). Consumption increments the delivery counter.
7. **Spawners** refill whenever every one of their cells is vacant;
   multi-cell spawners emit pre-bonded compounds.

Determinism: entities iterate in id order everywhere; ids are assigned in
spawn order (fixtures, then player runes, then gems).

## Scoring

- **Cost** - sum of rune prices: Push 10, Pull 15, Orbit 20, Detach 20,
  Fuse 25, Shift 40.
- **Ticks** - ticks until the final required delivery.
- **Runes** - number of runes inscribed.
- **Area** - distinct hexes ever occupied (tracked, shown in metrics).

The histogram comparer draws a deterministic pseudo-community distribution
peaked near par; your score is marked against it per metric.

## Campaign (all puzzles machine-verified)

| # | Id | Name | Teaches | Par (cost/ticks/runes) |
| - | -- | ---- | ------- | --- |
| 1 | c1p1 | The First Stone | Pull rays and holding | 15/8/1 |
| 2 | c1p2 | The Ward | Push deflection, rank beats rank | 25/12/2 |
| 3 | c1p3 | Turning the Wheel | Orbit, sunwise rotation | 35/12/2 |
| 4 | c2p1 | Thin Places | Shift, per-plane walls | 70/16/3 |
| 5 | c2p2 | Widdershins | Ethereal orbit reverses | 35/16/2 |
| 6 | c2p3 | The Loom | Cross-plane weaving, 2 outputs | 140/24/6 |
| 7 | c3p1 | The Greater Voice | Outranking a fixed machine | 15/14/1 |
| 8 | c3p2 | Mend the Wheel | Runes moving runes | 15/16/1 |
| 9 | c3p3 | Gear Train | Orbit handoffs by rank | 60/20/3 |
| 10 | c4p1 | The Gift | Fuse pairs | 40/10/2 |
| 11 | c4p2 | Sundered Pair | Detach, compound spawners | 35/16/2 |
| 12 | c4p3 | The Third Shard | Three-way fusion arcs | 65/10/3 |
| 13 | c5p1 | Gold and Azure | Fuse + shift pipeline | 80/12/3 |
| 14 | c5p2 | The Quickened Wheel | Gears + shift exit | 80/20/3 |
| 15 | c5p3 | The Celestial Aetherstone | Finale: triple fuse + quench | 105/14/4 |

Reference solutions live in `Source/Runik/Core/RunikPuzzles.cpp` and are
exercised by `Tests/CoreTests.cpp` on every test run.

## Art & audio

- Gems: sliced from the supplied 4x4 sheet; neon-pink chroma background
  removed with feathered despill (`Tools/process_gems.py`).
- Runes: drawn procedurally from Elder Futhark stroke data with a soft glow,
  white core tinted gold/azure at runtime (`Tools/gen_runes.py`).
- SFX: pure-python additive/subtractive synthesis - bell partials for fuse
  and win, filtered noise sweeps for push/pull, detuned sine beds for the
  ambient drones (`Tools/gen_sfx.py`).
