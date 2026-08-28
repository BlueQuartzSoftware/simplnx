# V&V Report: ErodeDilateMaskFilter

|                             |                                                                          |
|-----------------------------|--------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                              |
| SIMPLNX UUID                | `cab66cd1-f64c-42b4-8f94-18f0835a967f`                                   |
| SIMPLNX Human Name          | Erode/Dilate Mask                                                        |
| DREAM3D 6.5.171 equivalent  | `ErodeDilateMask` — SIMPL UUID `4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba`    |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                  |
| Status | READY FOR REVIEW |
| Sign-off                    | *delegated to the PR reviewer (requester decision 2026-08-19)*           |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port** — legacy `ErodeDilateMask.cpp` was read against `Algorithms/ErodeDilateMask.cpp` this pass. Neighbour offsets, boundary validity, the double-buffered `maskCopy` scan, and both operation branches are structurally identical. Two divergences were found and **both are now fixed**: the direction flags were not honoured, and the legacy `NumIterations <= 0` preflight guard was absent (`ErodeDilateMaskFilter-D1`, resolved 2026-08-19 by restoring the guard per the requester's product decision). |
| Oracle (confirmed)     | **Class 1 (Analytical).** Expected mask contents were derived by hand from the face-neighbour contract on a 5×5×1 single-seed grid (centre seed and boundary seed) and a 3×3×3 hollow cube, written into the test before the filter was run. 7 `TEST_CASE`s (the erode direction-flag case carries two `SECTION`s), all pass. The five single-iteration hand-derived index sets were independently reproduced by the DREAM3D 6.5.171 binary in the A/B, so the oracle is confirmed against legacy as well. |
| Code paths enumerated  | **10 of 10 exercised.** The multi-iteration loop (`NumIterations > 1`) is covered by a 2-iteration dilate oracle, and the `NumIterations <= 0` preflight rejection added on 2026-08-19 is covered by a negative test; no enumerated path is untested. |
| Tests today            | **7 oracle `TEST_CASE`s + 1 validation `TEST_CASE` + 1 SIMPL backwards-compatibility `TEST_CASE`, 315 assertions, all pass.** Coverage shape: dilate and erode at all-directions-on, one direction-gated case per axis that the fixtures can discriminate (X off and Y off on the 5×5×1 dilate, Z off on the 3×3×3 erode), a 2-iteration dilate, a boundary-seeded dilate, and a `NumIterations` of `0` / `-2` rejected at preflight. |
| Exemplar archive       | **None consumed any more.** The two 6.6-derived exemplar tests that read `6_6_erode_dilate_test.tar.gz` were retired and replaced by the inline Class 1 oracles. The archive itself stays in `test/CMakeLists.txt` because `ErodeDilateBadDataTest` and `ErodeDilateCoordinationNumberTest` still consume it. |
| Legacy comparison      | **Run.** 14 combinations ({dilate, erode} × 7 non-empty direction combinations, 1 iteration) executed on 2 fixtures — 28 pipeline pairs, 56 binary runs — through DREAM3D 6.5.171 `PipelineRunner` and `nxrunner` against a shared legacy-format input. **28/28 exact element-wise matches** on the `Mask` array. |
| Bug flags              | Two port omissions found and fixed this pass — direction flags ignored, and the missing `NumIterations <= 0` preflight guard. Both restore behaviour 6.5.171 already had, so **neither leaves an active deviation**; see [Bug Fixes](#bug-fixes-this-pass). `ErodeDilateMaskFilter-D1` is retained as a **retired** entry documenting the historical difference. **No active deviations.** |
| V&V phase              | Oracle chosen, derived by hand, and confirmed; direction-flag regression reproduced by failing test then fixed; binary A/B run and green; multi-iteration, per-axis and boundary gaps closed; `ErodeDilateMaskFilter-D1` resolved 2026-08-19 by restoring the legacy preflight guard, reproduced by a failing test first. Outstanding before promotion to COMPLETE: PR-reviewer sign-off (see below). |

## Outstanding before COMPLETE

- **Second-engineer sign-off on the oracle design** — *delegated to the PR reviewer (requester decision
  2026-08-19, per the program plan's standing practice).*
- **Out-of-core build runs** — *waived by the requester 2026-08-19.* Verification was run in the in-core
  build (`NX-Com-Qt69-Vtk96-Rel`) only.
- `ErodeDilateMaskFilter-D1` — **closed 2026-08-19** (requester product decision: match legacy; guard
  restored and covered by a negative test). No longer outstanding.

## Summary

`ErodeDilateMaskFilter` grows (dilate) or shrinks (erode) the `true` region of a boolean cell mask on
an Image Geometry by one voxel per iteration across the six face neighbours, optionally restricted to
any non-empty combination of the X, Y, and Z axes. Verification is **Class 1 analytical**: expected
mask contents were derived by hand from the algorithm's face-neighbour contract on 5×5×1 single-seed
grids and a 3×3×3 hollow cube, written into the test first, and only then run. One SIMPLNX-side
defect was found and fixed — the three direction parameters were never read by the algorithm, a **port
regression** carrying **no deviation entry** (see [Bug Fixes](#bug-fixes-this-pass)) — after which the
binary A/B shows parity on all 28 pipeline pairs. A second port omission, the missing
`NumIterations <= 0` preflight guard, was documented as `ErodeDilateMaskFilter-D1` and left open for a
product decision; the requester decided on **2026-08-19 to match legacy**, so the guard was restored
under TDD and D1 is now **retired**. All 9 test cases pass.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp:234` maps legacy SIMPL UUID
`4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba` directly to `FilterTraits<ErodeDilateMaskFilter>`, and
`test/simpl_conversion/{6_4,6_5}/ErodeDilateMaskFilter.json` carry the legacy
`Direction`/`NumIterations`/`XDirOn`/`YDirOn`/`ZDirOn`/`MaskArrayPath` parameter set unchanged — the
same filter with the same parameter model. The legacy source
(`Source/Plugins/Processing/ProcessingFilters/ErodeDilateMask.cpp`, from a sibling `DREAM3D` checkout
on the authoring engineer's machine, not committed to this repository) was read against
`Algorithms/ErodeDilateMask.cpp` this pass rather than inferred from documentation.

*Port-time deltas:*

1. **Face-neighbour offsets** — legacy computes `neighpoints[6]` inline; SIMPLNX calls
   `initializeFaceNeighborOffsets(dims)` from `NeighborUtilities`. Same six offsets in the same
   `[-Z,-Y,-X,+X,+Y,+Z]` order (legacy's `l == 0..5` maps one-for-one onto
   `VoxelNeighbors<Image3D>::k_NegativeZNeighbor .. k_PositiveZNeighbor`). No output change.
2. **Boundary-validity checks** — legacy folds the boundary test into the same `good = 0` chain that
   carries the direction flags; SIMPLNX calls `computeValidFaceNeighbors(x, y, z, dims)` and then
   masks the result. Reproduces the same six conditions. No output change.
3. **Direction gating** — legacy ORs the direction flag into each per-face check
   (`if(l == 0 && (k == 0 || !m_ZDirOn))`); SIMPLNX masks the per-voxel `isValidFaceNeighbor` array in
   `adjustValidNeighbors`. Equivalent once wired in — but it was *not* wired in at all, which is the
   port regression fixed by this pass.
4. **`NumIterations` validation** — legacy's `dataCheck()` errors with `-5555` when
   `NumIterations <= 0`; SIMPLNX had no equivalent preflight guard and silently no-op'd. Verified by
   running both binaries. Recorded as `ErodeDilateMaskFilter-D1` and initially left unchanged, because
   adding a preflight error is a behavioural change to a shipping filter and belongs to a product
   decision rather than a V&V pass. **The requester decided on 2026-08-19 to match legacy**, and the
   guard is now in `preflightImpl` under error code `-14701`; see
   [Bug Fixes](#bug-fixes-this-pass). No output change; parity restored on validation behaviour.
5. **`maskCopy` storage** — legacy allocates a `BoolArrayType` named `_INTERNAL_USE_ONLY_MaskCopy`
   and initializes it to `false`; SIMPLNX uses a `std::vector<bool>`. Both are overwritten wholesale
   from `mask` at the top of every iteration before being read, so the initial value is immaterial.
   No output change.
6. **Progress reporting** — SIMPLNX emits a per-iteration `Info` message through the message handler;
   legacy used `notifyStatusMessage`. No output change.

*Material PRs since baseline:* `Algorithms/ErodeDilateMask.cpp` has been touched by the
`NeighborUtilities` factoring (deltas 1–2, which introduced the `isValidFaceNeighbor` array that
delta 3 masks) and by store-API/rename churn with no behavioural content. The direction-flag
regression is a consequence of delta 3 never being completed for this filter — the same omission that
`ErodeDilateBadData` carried until PR #1687.

*SIMPLNX implementation:* `Algorithms/ErodeDilateMask.cpp` (123 lines). Single-threaded; no parallel
algorithm, no cancel check.

## Bug Fixes (this pass)

### Direction parameters had no effect — fixed, port regression, no deviation entry

The `X Direction` / `Y Direction` / `Z Direction` parameters were declared, presented in the UI, and
converted from SIMPL pipelines, and they reached the algorithm's input-values struct intact —
`ErodeDilateMaskFilter.cpp:59-61` declares the three parameters, `:107-109` copies them in, and
`ErodeDilateMask.hpp:29-31` declares the fields. `Algorithms/ErodeDilateMask.cpp` then read
`m_InputValues->Operation`, `NumIterations`, `MaskArrayPath`, and `InputImageGeometry`, and never read
`XDirOn`, `YDirOn`, or `ZDirOn`. Every voxel visited all six in-bounds face neighbours regardless of
the flags, so all three parameters silently did nothing.

Reproduced first, by two tests written before the fix:

- `Class 1 dilate honours direction flags` — 5×5×1, single true seed, `XDirOn = false`. Expected the
  vertical 3-cell bar `{7, 12, 17}`; pre-fix the filter produced the full plus shape, failing at
  index 11.
- `Class 1 erode honours direction flags` — 3×3×3 hollow cube, `ZDirOn = false`. Expected the ±Z
  neighbours of the centre (indices 4 and 22) to survive; pre-fix they were cleared, failing at
  index 4.

A third case, `Class 1 dilate with Y direction off`, was added afterwards to pin the remaining axis:
with only X and Z gating asserted, a partial reintroduction of the regression that drops or
mis-targets the Y flag would go unnoticed. Confirmed discriminating — with the `&& yDir` term removed
from `adjustValidNeighbors`, that case is the only one in the suite that fails.

Fixed by adding a file-local `adjustValidNeighbors(isValidFaceNeighbor, xDir, yDir, zDir)` that ANDs
each of the six entries against its axis flag using the named `VoxelNeighbors<Image3D>` constants, and
calling it at `Algorithms/ErodeDilateMask.cpp:93` immediately after `computeValidFaceNeighbors(...)`,
alongside — not instead of — the boundary check. The axis mapping was **copied verbatim** from
`Algorithms/ErodeDilateBadData.cpp:64-80`, per the branch-history note on
`ErodeDilateBadDataFilter-D1`: an earlier attempt at that fix bitwise-ANDed face indices against
booleans and swapped axes, and re-deriving the mapping is exactly how that was reintroduced.

The helper was kept file-local rather than hoisted into `NeighborUtilities.hpp` to share with
`ErodeDilateBadData`. Rationale: hoisting touches a core header consumed by filters outside the scope
of this V&V, and the duplication is 8 lines. If a reviewer prefers de-duplication it is a mechanical
follow-up.

DREAM3D 6.5.171 honours the flags (`ErodeDilateMask.cpp:227-247` gates each of the six neighbour
indices on `m_XDirOn` / `m_YDirOn` / `m_ZDirOn`), so this is a **port regression, not a deviation from
legacy behaviour**. Per the bug-adjudication protocol that makes it Verdict B, second branch: it gets
**no deviation entry**, because once fixed no difference from legacy remains. It is described here and
summarised in the Summary instead. With the fix in place all test cases pass and the post-fix binary
A/B shows parity on all 28 pipeline pairs.

### `NumIterations <= 0` accepted silently — fixed, port omission, legacy-shared behaviour restored

`Number of Iterations` is an unconstrained `Int32Parameter`, and `ErodeDilateMaskFilter::preflightImpl`
validated nothing — it only called `MarkDataPathModified`. The algorithm's
`for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)` loop therefore never
executed for a value below 1, and the filter reported success while writing the mask out bit-identical
to the input. DREAM3D 6.5.171's `ErodeDilateMask::dataCheck()` rejected exactly that configuration with
error `-5555`, so this is a **port omission** of a guard legacy already had, not a SIMPLNX design
choice.

The V&V pass as first written recorded it as `ErodeDilateMaskFilter-D1` and deliberately did not change
the filter: adding a preflight error is a behavioural change to a shipping filter and is a product
decision. **The requester made that decision on 2026-08-19 — match legacy, add the preflight error** —
on the grounds that it restores parity, has zero migration impact (a `NumIterations <= 0` pipeline
already failed in legacy, so no working legacy pipeline can depend on the SIMPLNX no-op), and requires
no parameter-version bump because no parameter key changes. `parametersVersion()` accordingly stays
at `1`.

Reproduced first, under TDD: `SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations` was
written and run **before** the guard existed and failed on both `GENERATE`d values —
`REQUIRE((preflightResult.outputActions).invalid())` with expansion `false` at `numIterations := 0` and
again at `numIterations := -2` — confirming the pre-fix silent-accept. The guard was then added to
`ErodeDilateMaskFilter.cpp::preflightImpl`:

- Condition `NumIterations < 1`, returning `MakeErrorResult<OutputActions>(k_InvalidNumIterationsError, ...)`.
- Error code **`-14701`**, declared as a file-local `constexpr int32` in an anonymous namespace, per the
  house convention used by the sibling `ErodeDilateBadDataFilter` (`k_NoDirectionsError = -14601`,
  `k_NoGeometryDimensionsError = -14602`). `-147xx` is the first free block adjacent to that family;
  legacy's `-5555` was deliberately **not** reused: it belongs to the legacy code space, and `-5555` is
  already taken in SIMPLNX for unrelated conditions (`RequireMinNumNeighborsFilter`'s
  unavailable-phase execute error, `ApplyTransformationToGeometry`'s unused-cell-matrix warning).
- The message carries the offending value and the affected array, per the error-messages house rule:
  `Number of Iterations (-2) must be at least 1. Erode/Dilate Mask performs one erode or dilate pass per iteration, so a value less than 1 would leave the mask array '<path>' unmodified.`

With the guard in place the test passes (13 assertions across the two generated values) and the other
8 test cases are unaffected — no existing test configures a non-positive iteration count.

## Oracle

*Class:* **1 (Analytical)** — closed-form expected output on hand-built input.

*Applied:* `ErodeDilateMask::operator()` double-buffers. It copies `mask` into `maskCopy`, walks every
voxel, and for voxels where `!mask[voxelIndex]` inspects the six face neighbours. Dilate sets
`maskCopy[voxelIndex] = true` when a face neighbour is true (the false voxel joins the region); erode
sets `maskCopy[neighpoint] = false` (the true *neighbour* leaves the region). The asymmetry is
deliberate and both formulations were checked against the legacy source before being used as the
oracle basis. Because only `mask` is read during a pass and only `maskCopy` is written, one iteration
is a pure function of the input grid and can be evaluated by hand.

Three fixtures:

- **5×5×1, single true voxel at (2, 2) — flat index 12.** Dilate/all-on: the four false voxels
  touching the seed flip, giving the plus shape `{7, 11, 12, 13, 17}`; the grid is one plane thick so
  `computeValidFaceNeighbors` reports ±Z invalid everywhere and no Z growth is possible. Erode/all-on:
  each of those four false voxels sees the seed and clears it, giving `{}`. Dilate with
  `XDirOn = false`: (1,2) and (3,2) can no longer reach the seed, leaving the vertical bar
  `{7, 12, 17}`. Dilate with `YDirOn = false`: the mirror case, leaving the horizontal bar
  `{11, 12, 13}`. Dilate with `NumIterations = 2`: pass 1 gives the plus, and pass 2 — derived cell by
  cell over all 20 remaining false voxels against pass 1's written-back mask — gives the 13-cell
  diamond `{2, 6, 7, 8, 10, 11, 12, 13, 14, 16, 17, 18, 22}`, every cell within Manhattan distance 2
  of the seed that fits on the grid.
- **5×5×1, single true voxel on the x = 0 face at (0, 2) — flat index 10.** Dilate/all-on gives
  `{5, 10, 11, 15}`. This fixture exists because a *centred* seed cannot detect a swap of the −X and
  +X entries of `neighborVoxelIndexOffsets`: `computeValidFaceNeighbors` gates by coordinate, not by
  offset, so for an interior seed both X gates are open on both neighbours and a swap merely re-labels
  which entry reaches the seed — the reached set `{11, 13}` is unchanged. An off-centre but still
  interior seed does not help either, since a plus shape is mirror-symmetric about its own centre
  wherever that centre sits. Seating the seed against the face breaks the symmetry through the *gate*:
  under a swapped ±X pair, voxel 9 = (4, 1) — whose −X gate is open because 4 > 0 — would read offset
  +1, land on index 10, and grow a cell that wrapped around the row end. Index 9 is therefore the
  discriminator, and it is also the only case in the suite where the seed's own −X neighbour is
  rejected by the boundary check.
- **3×3×3, true everywhere except the centre (1,1,1) — flat index 13.** The centre is the only false
  voxel and therefore the only voxel that enters the neighbour loop; its six face neighbours are
  `-Z = 4`, `-Y = 10`, `-X = 12`, `+X = 14`, `+Y = 16`, `+Z = 22`. Erode/all-on clears all six
  (20 voxels stay true). Erode with `ZDirOn = false` clears only four, so 4 and 22 survive
  (22 voxels stay true). This fixture exists because the 5×5×1 grid cannot discriminate the Z flag.

Every expected index set is spelled out in a derivation comment in the test immediately above the
assertion, and each assertion checks the **exact** expected true-index set element-wise over the whole
grid, not a count or a spot check.

*Independent confirmation:* the five single-iteration index sets were extracted from the DREAM3D
6.5.171 binary's A/B output by `ww_work/ErodeDilateMask/oracle_check.py` and match exactly. The oracle
is therefore confirmed by two sources that are independent of SIMPLNX's own behaviour: hand
derivation, and the legacy implementation. The 2-iteration and boundary-seed sets rest on hand
derivation alone (the A/B sweep runs `NumIterations = 1` on the centred fixtures).

*Encoded:*

- `src/Plugins/SimplnxCore/test/ErodeDilateMaskTest.cpp::SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, one iteration` — 34 assertions.
- `...::Class 1 erode, one iteration` — 34 assertions.
- `...::Class 1 dilate honours direction flags` — 34 assertions.
- `...::Class 1 dilate with Y direction off` — 34 assertions.
- `...::Class 1 dilate, two iterations` — 34 assertions.
- `...::Class 1 dilate from a boundary seed` — 34 assertions.
- `...::Class 1 erode honours direction flags` — two `SECTION`s (`All directions on`, `Z direction off`), 71 assertions.

7 `TEST_CASE`s, 275 assertions, all pass.

*Discrimination evidence:* the three tests added in this pass were each checked against the specific
single-line algorithm mutation they exist to catch, by temporarily applying the mutation, rebuilding
`SimplnxCoreUnitTest`, and re-running the suite. Dropping `&& yDir` from `adjustValidNeighbors` fails
`Class 1 dilate with Y direction off` **and nothing else**. Hoisting the per-iteration write-back
below the iteration loop fails `Class 1 dilate, two iterations` **and nothing else**. Swapping the −X
and +X entries of `neighborVoxelIndexOffsets` fails `Class 1 dilate from a boundary seed` at index 9 —
exactly the predicted discriminator — plus the 2-iteration case, and none of the four pre-existing
oracle tests. The mutations were reverted and the suite re-confirmed green before commit.

One mutation from the review list proved to be an **equivalent mutant** rather than a coverage gap:
hoisting the per-iteration `maskCopy[j] = mask[j]` seeding *above* the loop changes no output and
cannot be killed by any test. Because the write-back at the end of each iteration already leaves
`mask == maskCopy`, re-seeding at the top of the next iteration is the identity, and both operations
write `maskCopy` monotonically (dilate only sets `true`, erode only sets `false`). The in-loop seeding
is therefore redundant given the in-loop write-back; only the write-back carries the inter-iteration
dataflow, and that is what `Class 1 dilate, two iterations` pins.

*Second-engineer review:* **delegated to the PR reviewer** (requester decision 2026-08-19, per the
program plan's standing practice). Sign-off on the oracle design happens in PR review rather than as a
separate named review.

## Code path coverage

10 of 10 paths exercised. The algorithm has two logical phases: (a) per-iteration setup and write-back,
and (b) the per-voxel face-neighbour scan; phase (c) is the filter's preflight validation.

Sources: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateMask.cpp` (123 lines)
for phases (a) and (b), and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ErodeDilateMaskFilter.cpp`
`preflightImpl` for phase (c).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Setup | `maskCopy` allocation, dims conversion, face-offset and internal-index initialization, per-iteration `maskCopy[j] = mask[j]` seeding | All 7 oracle tests |
| 2 | (a) Write-back | `mask[j] = maskCopy[j]` at the end of each iteration | All 7 oracle tests (every assertion reads the written-back array). `Class 1 dilate, two iterations` additionally pins the write-back's *position inside* the loop: hoisting it below the loop fails that test and no other. |
| 3 | (a) Iteration loop | `NumIterations > 1` — a second pass over the already-updated mask | `Class 1 dilate, two iterations` — 5×5×1 centre seed, all flags on, `NumIterations = 2`, asserting the 13-cell diamond derived cell by cell from pass 1's written-back mask. This is the only test that reads the inter-iteration dataflow; every other oracle case, and the whole binary A/B, runs a single iteration. |
| 4 | (b) Per-voxel | `mask[voxelIndex] == true` → voxel skipped, neighbours never inspected | All 7 oracle tests (24 of 25 cells in the 5×5×1 dilate case, 26 of 27 in the hollow cube) |
| 5 | (b) Neighbour gate | `!isValidFaceNeighbor[faceIndex]` from the **boundary** check → `continue` | All 7 oracle tests (±Z always invalid on the 5×5×1 grid; every hollow-cube face voxel loses at least one neighbour). `Class 1 dilate from a boundary seed` is the only case where the boundary check fires on the **seed's own** missing neighbour, and the only one whose result changes if the ±X offsets are swapped (it fails at index 9). |
| 6 | (b) Neighbour gate | `!isValidFaceNeighbor[faceIndex]` from the **direction** mask in `adjustValidNeighbors` → `continue` | Per axis: `Class 1 dilate honours direction flags` (X masked), `Class 1 dilate with Y direction off` (Y masked), `Class 1 erode honours direction flags` / `Z direction off` (Z masked). The X and Z cases failed before the fix and pass after it; the Y case was added afterwards and is the sole failure when the `yDir` term is removed. |
| 7 | (b) Dilate | `Operation == k_DilateIndex && mask[neighpoint]` → `maskCopy[voxelIndex] = true` | `Class 1 dilate, one iteration`, `Class 1 dilate honours direction flags`, `Class 1 dilate with Y direction off`, `Class 1 dilate, two iterations`, `Class 1 dilate from a boundary seed` |
| 8 | (b) Erode | `Operation == k_ErodeIndex && mask[neighpoint]` → `maskCopy[neighpoint] = false` | `Class 1 erode, one iteration`, `Class 1 erode honours direction flags` (both sections) |
| 9 | (b) Neighbour false | valid neighbour with `mask[neighpoint] == false` → neither branch fires, nothing written | All 7 oracle tests (in the 5×5×1 dilate case most valid neighbour pairs are false-false) |
| 10 | (c) Preflight validation | `NumIterations < 1` → `MakeErrorResult<OutputActions>(-14701, ...)`, added 2026-08-19 | `Class 1 dilate, one iteration` and the other 6 oracle tests take the accepting branch (`NumIterations` of 1 or 2). `Invalid Number of Iterations` takes the rejecting branch, `GENERATE`ing `0` (the boundary) and `-2` (negative) and asserting both `outputActions.invalid()` and `errors()[0].code == -14701`. Written before the guard and confirmed failing on both values. |

Observations that are **not** deviations:

- **No cancel check.** `m_ShouldCancel` is stored and exposed via `getCancel()` but never polled in
  the voxel loop. Legacy has no cancel check either, so SIMPLNX is at parity; it is a
  responsiveness gap on very large volumes, not a correctness one, and is out of scope for this pass.
- **Preflight error paths.** `preflightImpl` calls `MarkDataPathModified` and, as of 2026-08-19,
  validates `NumIterations >= 1` (`-14701`). The geometry and mask are selection parameters and are
  validated by the parameter system, so they need no explicit check. With the `NumIterations` guard
  restored, SIMPLNX's preflight validation now matches 6.5.171's `dataCheck()`.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ErodeDilateMaskFilter(Dilate)` | retired | 6.6-derived exemplar comparison against `Exemplar Mask Dilate` in `6_6_erode_dilate_mask.dream3d`. Retired: the exemplar was generated with all three directions on, so it could not discriminate direction gating — a build with the regression still present passed it. Replaced by the Class 1 oracles. |
| `SimplnxCore::ErodeDilateMaskFilter(Erode)` | retired | Same, for `Exemplar Mask Erode`. Same reason. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, one iteration` | new-for-V&V | Class 1 oracle. 5×5×1 single seed, all directions on, 1 iteration. Asserts the exact expected true-index set element-wise over all 25 cells; 34 assertions. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 erode, one iteration` | new-for-V&V | Class 1 oracle. Same fixture, erode. Asserts the mask goes entirely false; 34 assertions. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate honours direction flags` | new-for-V&V | Class 1 oracle, direction-gated. 5×5×1, `XDirOn = false`, dilate. Asserts the vertical bar `{7, 12, 17}`; 34 assertions. Written before the fix and confirmed failing at index 11 with the regression present. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate with Y direction off` | new-for-V&V | Class 1 oracle, direction-gated. 5×5×1 centre seed, `YDirOn = false`, dilate. Asserts the horizontal bar `{11, 12, 13}`; 34 assertions. Pins the Y flag specifically: removing the `&& yDir` term from `adjustValidNeighbors` fails this test and no other. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, two iterations` | new-for-V&V | Class 1 oracle, multi-iteration. 5×5×1 centre seed, all flags on, `NumIterations = 2`. Asserts the 13-cell diamond `{2, 6, 7, 8, 10, 11, 12, 13, 14, 16, 17, 18, 22}`, derived cell by cell over all 20 false voxels of pass 1's output; 34 assertions. Closes coverage path 3 and pins the write-back's position inside the iteration loop. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate from a boundary seed` | new-for-V&V | Class 1 oracle, boundary. 5×5×1 with the seed on the x = 0 face at (0, 2), dilate, all flags on. Asserts `{5, 10, 11, 15}`; 34 assertions. The only case that discriminates a −X/+X offset swap (fails at index 9, the row-wrap cell) and the only one exercising the boundary rejection on the seed's own neighbour. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 erode honours direction flags` | new-for-V&V | Class 1 oracle, direction-gated. 3×3×3 hollow cube, erode, `SECTION`s for `ZDirOn = true` (20 cells true) and `ZDirOn = false` (22 cells true, ±Z neighbours survive); 71 assertions. Written before the fix and confirmed failing at index 4 with the regression present. |
| `SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations` | new-for-V&V | Validation test (not an oracle test). `GENERATE`s `NumIterations` of `0` and `-2` on the 5×5×1 centre-seed fixture, preflights, and asserts `SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)` plus `errors()[0].code == -14701`; 13 assertions. Closes coverage path 10. Written before the guard existed and confirmed failing on both generated values. Pins the resolution of `ErodeDilateMaskFilter-D1`. |
| `SimplnxCore::ErodeDilateMaskFilter: SIMPL Backwards Compatibility` | kept | Byte-identical to the pre-V&V version. `DYNAMIC_SECTION` over `simpl_conversion/6_5/ErodeDilateMaskFilter.json` (matched by `Filter_Uuid`) and `simpl_conversion/6_4/ErodeDilateMaskFilter.json` (matched by `Filter_Name`). Confirms a single `PipelineFilter` with the right UUID and checks the converted arguments (`Operation == 0`, `NumIterations == 5`, all three direction flags `true`, geometry `DataPath({"DataContainer"})`, mask `DataPath({"DataContainer","CellData","TestArray"})`); 27 assertions. Not an oracle test. |

All 9 remaining test cases pass in the in-core build (`NX-Com-Qt69-Vtk96-Rel`), 315 assertions total
(275 across the 7 oracle cases, 13 in the validation case, 27 in the backwards-compatibility case).
Out-of-core build runs were waived by the requester on 2026-08-19.

## Exemplar archive

- **Archive:** none consumed by this filter's tests any more.
- **SHA512:** n/a
- **Provenance:** n/a

The retired tests read `6_6_erode_dilate_mask.dream3d` out of `6_6_erode_dilate_test.tar.gz`. That
archive is **not** removed from `src/Plugins/SimplnxCore/test/CMakeLists.txt:225` by this PR:
`ErodeDilateBadDataTest.cpp:357` and `ErodeDilateCoordinationNumberTest.cpp:38` still consume it.
Retirement of the archive completes in whichever PR lands last among the erode/dilate conversions,
per the plan's grep guard.

The A/B artifacts (legacy-format inputs, 56 pipelines, per-combination outputs, diff scripts, ReadMe)
live in `/Users/mjackson/Workspace9/ww_work/ErodeDilateMask/` and are uploaded to OneDrive; they are
never committed.

## Deviations from DREAM3D 6.5.171

The comparison was run as a full binary A/B: 14 combinations ({dilate, erode} × the 7 non-empty
direction combinations, `NumIterations = 1`) on 2 fixtures — the 5×5×1 single-seed grid and the
3×3×3 hollow cube the unit tests build — giving 28 pipeline pairs and 56 binary runs through
`PipelineRunner` (6.5.171) and `nxrunner` against a single shared legacy-format input per fixture.

**28/28 combinations match element-wise on the `Mask` array. No A/B difference was observed.**
Per-combination results and the re-run recipe are in
`/Users/mjackson/Workspace9/ww_work/ErodeDilateMask/ReadMe.md` (`results_compare.txt`).

Two of the 28 (`5x5x1_dilate_Z`, `5x5x1_erode_Z`) are no-ops on both sides — a one-plane-thick grid
has no Z face neighbours — and are reported as such by the diff script rather than being quietly
counted as evidence. The 3×3×3 fixture carries the Z-direction evidence.

The direction-flag regression fixed by this pass produces **no deviation entry**: 6.5.171 was correct
(`ErodeDilateMask.cpp:227-247`), SIMPLNX was wrong, and once fixed there is no remaining difference.
Comparing the legacy outputs against what pre-fix SIMPLNX would have produced (the all-directions
result for every flag setting) shows the A/B would have failed on 12 of the 28 combinations.

**No deviations remain open.** One was recorded during this pass, found by reading the legacy
`dataCheck()` and then confirmed by running both binaries, and has since been resolved:

- `ErodeDilateMaskFilter-D1` — `NumIterations <= 0` was a preflight error in 6.5.171 and a silent
  no-op in SIMPLNX. **Retired 2026-08-19:** on the requester's product decision to match legacy, the
  preflight guard was restored (error `-14701`, message carrying the offending value) under TDD, so no
  difference remains. The entry is retained in
  [`deviations/ErodeDilateMaskFilter.md`](deviations/ErodeDilateMaskFilter.md) with `Status: retired`
  because it documents a real historical difference between the shipped versions.
