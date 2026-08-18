# V&V Report: RequireMinimumSizeFeaturesFilter

|                             |                                                                      |
|-----------------------------|----------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                          |
| SIMPLNX UUID                | `074472d3-ba8d-4a1a-99f2-2d56a0d082a0`                               |
| SIMPLNX Human Name          | Remove Minimum Size Features                                         |
| DREAM3D 6.5.171 equivalent  | `MinSize` — SIMPL UUID `53ac1638-8934-57b8-b8e5-4b91cdda23ec`        |
| Verified commit             | *<filled at SBIR deliverable assembly>*                              |
| Status | READY FOR REVIEW |
| Sign-off                    | Delegated to the PR reviewer (requester decision 2026-08-19)          |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port** — legacy `MinSize.{h,cpp}` was read against `Algorithms/RequireMinimumSizeFeatures.cpp` this pass. Threshold test, the six-neighbour vote and its tie-break, the multi-pass coarsening loop, and the feature-array compaction are structurally identical. No divergence found. |
| Oracle (confirmed)     | **Class 1 (Analytical) + Class 4 (Invariant).** Every expected FeatureId, every reassignment *source voxel*, and the compacted feature arrays were derived by hand from the algorithm source and written into the test **before** the filter was run. Encoded in `test/RequireMinimumSizeFeaturesTest.cpp`. All pass. |
| Code paths enumerated  | **18 of 22 exercised.** Gaps: the `m_ShouldCancel` early-exit group and the feature-0-votable edge (both untested), plus two dead-defensive branches that are unreachable rather than untested. Two further defensive branches in `RemoveInactiveObjects` are also unreachable through this filter — see the coverage table. |
| Tests today            | **5 TEST_CASEs, 2417 assertions, all pass**: a two-phase 6×6×6 analytical fixture swept over both phase modes (2243), an at-threshold boundary sweep over three thresholds (79), a vote-counter-reset strip (55), one execute-error test (13), and the untouched SIMPL backwards-compat test (27). |
| Exemplar archive       | **Retired.** `6_6_min_size_input.tar.gz` and `6_6_min_size_output.tar.gz` were the only inputs to the removed 6.6-derived exemplar comparison. Both `download_test_data()` blocks are deleted from `test/CMakeLists.txt` in this change; `grep -rn "6_6_min_size" src/` leaves no code, test, CMake, or pipeline consumer — the only remaining matches are this report's own self-references. |
| Legacy comparison      | **Run.** 8 parameter combinations × 4 arrays through both binaries — **32/32 array-level matches, zero differences.** For 3 of the 8 combinations (`cube6_min4_sp0`, `cube6_min4_sp1`, `strip5_min3_sp0`), each binary was additionally checked against the same hand oracle independently — **24/24 arrays** — so an agreed-upon *wrong* answer would have been caught on those three. The other 5 combinations rest on binary-to-binary agreement only. |
| Bug flags              | **None.** No deviation entries. One non-deviating robustness gap is recorded under Follow-ups (SIMPLNX and 6.5.171 share it, so it is not a deviation). |
| V&V phase              | Oracle chosen, derived first, and confirmed; three decision points mutation-verified (threshold operator, vote tie-break, vote-counter reset); legacy A/B run and clean; archives retired. Outstanding before promotion to COMPLETE: second-engineer sign-off (delegated to the PR reviewer — requester decision 2026-08-19, per the program plan's standing practice) and the follow-up recorded below. The out-of-core build test run was waived by the requester 2026-08-19. |

## Summary

`RequireMinimumSizeFeaturesFilter` deletes every feature whose cell count falls below
`MinAllowedFeaturesSize` (optionally restricted to a single phase), reassigns the
orphaned cells to a neighbouring feature by a six-face-neighbour vote, and compacts the
feature-level arrays. Verification is **Class 1 + Class 4**: the 6.6-derived exemplar
comparison was deleted and replaced by a two-phase 6×6×6 fixture, an at-threshold 5×1×1
strip, and a 9×1×1 vote-counter-reset strip, whose every expected value — including the
*source voxel* of each reassignment — was hand-derived from the algorithm source before
the filter was executed. **No defect was found**; SIMPLNX
matched the oracle on the first run, and a binary A/B against DREAM3D 6.5.171
`PipelineRunner` produced 32/32 identical arrays across 8 parameter combinations.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* `RequireMinimumSizeFeaturesFilter::FromSIMPLJson` consumes the legacy
`MinAllowedFeatureSize` / `ApplyToSinglePhase` / `PhaseNumber` / `FeatureIdsArrayPath` /
`FeaturePhasesArrayPath` / `NumCellsArrayPath` parameter set unchanged, and
`test/simpl_conversion/{6_4,6_5}/RequireMinimumSizeFeaturesFilter.json` round-trip
through it. The same filter with the same parameter model, not a reimplementation.

*Port-time deltas (all read against the legacy source this pass, none observable):*

1. **Threshold test** — legacy `MinSize.cpp:452` (all-phase) and `:463` (single-phase),
   against SIMPLNX `Algorithms/RequireMinimumSizeFeatures.cpp:297,308`. Both keep a
   feature when `NumCells >= MinAllowedFeatureSize`. Identical.
2. **Neighbour offsets and boundary validity** — legacy computes `neighpoints[]` and six
   inline boundary conditionals; SIMPLNX calls `initializeFaceNeighborOffsets(dims)` and
   `computeValidFaceNeighbors(x, y, z, dims)` from `NeighborUtilities`. Same six offsets
   in the same `[-Z, -Y, -X, +X, +Y, +Z]` order and the same six conditions.
3. **Vote tie-break** — legacy `MinSize.cpp:358-364` (`if(current > most)`) and SIMPLNX
   `RequireMinimumSizeFeatures.cpp:229-235` (`if(currentVoteCount > maxVoteCount)`).
   Both strictly greater-than. Confirmed by the A/B at the fixture's deliberate
   2-vs-2 tie cell, not only by reading.
4. **Vote-counter reset** — legacy re-walks the six neighbours and zeroes their entries
   (`MinSize.cpp:368-404`); SIMPLNX does `std::fill` over the whole counter
   (`RequireMinimumSizeFeatures.cpp:240`). Same end state. That the reset happens *at
   all* is now pinned by the `Vote Counter Reset Between Cells` fixture and its mutation
   run — see the Oracle section.
5. **Transfer ordering** — legacy copies every cell array (FeatureIds included) inside
   one loop; SIMPLNX copies the non-FeatureIds arrays first in parallel tasks and
   FeatureIds last. This cannot diverge: a cell's recorded source is only ever a voxel
   whose FeatureId was already `>= 0` when the vote was cast, so no chain of
   bad-cell-copies-from-bad-cell can form inside a single pass.
6. **Compaction** — legacy `AttributeMatrix::removeInactiveObjects` erases removed tuples;
   SIMPLNX `RemoveInactiveObjects` copies the keep-list forward and resizes. Both produce
   `keepCount + 1` tuples and the same `newNames` remap of the cell FeatureIds.

## Oracle

*Class:* **1 (Analytical)** for the cell and feature values; **4 (Invariant)** for the
four structural properties asserted after every successful run.

*Applied:* The 6×6×6 fixture holds six features with hand-chosen cell counts. Every
expected value was derived on paper from `Algorithms/RequireMinimumSizeFeatures.cpp`
before the filter was executed:

* **which features are removed** — from the `NumCells >= MinAllowedFeatureSize` test at
  `:297` (all-phase) and `:308` (single-phase);
* **which voxel each orphaned cell copies from** — by walking the six face neighbours in
  the fixed `[-Z, -Y, -X, +X, +Y, +Z]` order and applying the strict `>` vote test at
  `:231`, for each of the seven orphaned cells individually. The full vote sequence for
  every removed cell is written out as a comment block in the test file;
* **the final FeatureIds** — by applying the `newNames` compaction map built in
  `RemoveInactiveObjects` (`DataGroupUtilities.cpp:45-58`).

The reassignment *source* is made observable, not inferred: two companion cell arrays
carry `10000 + flat index` (int32) and `index + {0.25, 0.5, 0.75}` (float32×3), so an
assertion on their post-run values names the exact winning neighbour rather than merely
the winning feature id.

*Class 4 invariants* (`CheckClass4Invariants`, asserted in every non-error TEST_CASE):

Numbered in the order they are asserted in `CheckClass4Invariants`:

1. the total cell count is unchanged;
2. no cell retains a removed feature's id (removed features compact to 0 and unassigned
   cells are negative, so a minimum observed id of 1 proves it);
3. FeatureIds are contiguous starting at 1 with no gaps after compaction;
4. every feature-level array has `max(FeatureId) + 1` tuples.

*Encoded:* `test/RequireMinimumSizeFeaturesTest.cpp` —
`Two-Phase 6x6x6 Analytical Oracle` (2 generated sections),
`At-Threshold Boundary` (3 generated sections),
`Vote Counter Reset Between Cells`,
`Execute Error - unavailable phase (-5555)`. All pass.

*Oracle discrimination proven by mutation testing.* An oracle that is merely green
proves nothing, so each of the three decision points below was deliberately mutated in
`Algorithms/RequireMinimumSizeFeatures.cpp`, rebuilt, run, and reverted. Nothing outside
this table has been mutation-verified:

| Mutation | Simulates | Result |
|---|---|---|
| `NumCells >= min` → `NumCells > min` (`:297`, `:308`) | a `<=` removal rule | **Killed** by both `Two-Phase 6x6x6 Analytical Oracle` and `At-Threshold Boundary` |
| `currentVoteCount > maxVoteCount` → `>=` (`:231`) | a non-strict vote tie-break | **Killed** by the tie cell (2,5,5): observed source index 213 (feature 2) vs oracle 211 (feature 1) |
| delete `std::fill(voteCounter.begin(), voteCounter.end(), 0)` (`:240`) | vote tallies leaking from one orphan cell into the next | **Killed** *only* by `Vote Counter Reset Between Cells` — strip index 6 came back FeatureId 1 / CopiedScalar 10007 against an oracle of 2 / 10005. Every pre-existing TEST_CASE stayed green under this mutant, which is why the fixture was added |

*Second-engineer review:* delegated to the PR reviewer (requester decision 2026-08-19, per the program plan's standing practice).

### The `<` vs `<=` determination

**The comparison is `<`: a feature with exactly `MinAllowedFeaturesSize` cells survives.**

`Algorithms/RequireMinimumSizeFeatures.cpp:297`

```cpp
if(featureNumCellsStoreRef.getValue(i) >= minAllowedFeatureSize)
{
  good = true;
}
else
{
  activeObjects[i] = false;
}
```

and the single-phase form at `:308`. A feature is deactivated only in the `else`, i.e.
only when `NumCells < MinAllowedFeatureSize`. DREAM3D 6.5.171 `MinSize.cpp:452`
(and `:463` for the single-phase form) carries the identical test, so this is not a
behavioural change between versions.

The fixture is designed to discriminate rather than assume: feature 2 has **exactly** 4
cells at threshold 4 and must survive, feature 3 has 3 cells and must be removed, and the
`At-Threshold Boundary` TEST_CASE additionally sweeps a 5×1×1 strip across thresholds
2/3/4 so the at-threshold feature is on *both* sides of the boundary across the sweep.
The mutation test above confirms the assertion actually fails when the operator is
flipped.

### The reassignment vote rule as implemented

`assignBadVoxels` (`:161-270`) repeats a sweep until no cell has a negative FeatureId.
Within one sweep, for each cell whose FeatureId is `< 0`:

1. The six face neighbours are visited in the fixed order `[-Z, -Y, -X, +X, +Y, +Z]`
   (`initializeFaceNeighborInternalIdx<Image3D>()`, `NeighborUtilities.hpp:204-208`).
   Neighbours off the volume are skipped by `computeValidFaceNeighbors`, which is why
   boundary cells vote with three or four neighbours instead of six.
2. A neighbour whose FeatureId is `< 0` (another removed cell) casts no vote.
3. Each remaining neighbour increments `voteCounter[neighborFeatureId]`. The **voxel
   index** — not the feature id — is recorded as the copy source whenever that increment
   pushes the feature's running count **strictly above** the current maximum (`:231`).
4. **Tie behaviour:** because the test is strictly `>`, a tie is kept by whichever
   feature reached that count first, i.e. the earlier neighbour in traversal order.
   Within a *single* feature the opposite holds — each later neighbour of the same
   feature increments the count again and therefore takes over as the source, so among
   equal-feature neighbours the **last** one visited supplies the tuple.
5. The votes are only *recorded* during the sweep; nothing is written until the sweep
   ends, so every vote in a sweep sees the same pass-start FeatureIds. A cell with no
   valid non-negative neighbour records nothing and is retried on the next sweep.
6. After the sweep, every cell array in the FeatureIds' Attribute Matrix copies the
   recorded source tuple into the target cell; FeatureIds itself is copied last.

7. `voteCounter` is allocated **once**, outside the sweep (`:193`), while `maxVoteCount`
   is re-declared per cell (`:215`). The `std::fill` at `:240` is therefore the only
   thing that keeps one orphan cell's tallies from being carried into the next cell
   swept. `Vote Counter Reset Between Cells` is the fixture that can see this.

The 6×6×6 fixture exercises all four outcomes of the vote itself: a strict feature-1
majority (indices 1, 6, 36, 210), a strict majority for a *non-bulk* survivor (index 215,
feature 6 wins 3-0), a genuine 2-vs-2 tie resolved by traversal order (index 212), and a
cell with no votable neighbour at all that resolves on the second sweep (index 0,
all-phase run only). It cannot see rule 7, because every one of its orphan cells is
decided by a margin wider than any counter an earlier cell could leak — which is why the
9×1×1 `Vote Counter Reset Between Cells` strip exists: its cell at index 1 banks two
feature-1 votes, and its cell at index 6 is a 1-vs-1 cross-feature tie that those leaked
votes would flip from feature 2 (source index 5) to feature 1 (source index 7).

## Code path coverage

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RequireMinimumSizeFeatures.cpp` (332 lines).

Logical phases: **(a)** phase-availability guard, **(b)** `removeSmallFeatures` marking,
**(c)** `assignBadVoxels` multi-pass reassignment, **(d)** the per-array transfer in
`RequireMinimumSizeFeaturesTransferDataImpl`, **(e)** `RemoveInactiveObjects` compaction.
**18 of the 22 paths below are exercised.** The four gaps are rows 4, 16, 19 and 22;
each carries its reason in the table. Rows 4 and 19 are unreachable rather than untested.

The `ApplySinglePhase=false` / `ApplySinglePhase=true` labels below are the literal
generated-section names ctest prints for `Two-Phase 6x6x6 Analytical Oracle`.

| #  | Phase | Path | Status / test case |
|----|-------|------|--------------------|
| 1  | (a) | `ApplySinglePhase == false` → phases never read, `featurePhases` stays `nullptr` (`:103`) | **Covered** — `Two-Phase 6x6x6 Analytical Oracle` (ApplySinglePhase=false), `At-Threshold Boundary`, `Vote Counter Reset Between Cells` |
| 2  | (a) | `ApplySinglePhase == true`, phase present → proceed | **Covered** — `Two-Phase 6x6x6 Analytical Oracle` (ApplySinglePhase=true) |
| 3  | (a) | `ApplySinglePhase == true`, phase absent → error `-5555` | **Covered** — `Execute Error - unavailable phase (-5555)` |
| 4  | (a) | `:104` defensive `featurePhases != nullptr` false while `ApplySinglePhase == true` | **Not covered — unreachable.** `:103` already dereferences the same pointer (`getDataAs<Int32Array>(...)->getDataStore()`), so a missing Phases array is undefined behaviour one line earlier and the guard can never fire. Preflight rejects the missing/mistyped array first. Noted as dead-defensive code, not as a test gap |
| 5  | (b) | all-phase `:297`, `NumCells >= min` → keep | **Covered** — `Two-Phase…` (ApplySinglePhase=false), features 1, 2 (exactly at threshold), 6 |
| 6  | (b) | all-phase `:297`, `NumCells < min` → deactivate | **Covered** — `Two-Phase…` (ApplySinglePhase=false), features 3, 4, 5 |
| 7  | (b) | single-phase `:308`, phase matches and `NumCells >= min` → keep by size | **Covered** — `Two-Phase…` (ApplySinglePhase=true), features 1 (200 cells), 2 (4 cells, exactly at threshold) and 6 (5 cells), all phase 1 |
| 8  | (b) | single-phase `:308`, phase mismatch → keep regardless of size | **Covered** — `Two-Phase…` (ApplySinglePhase=true), feature 5, size 1, phase 2 |
| 9  | (b) | single-phase `:308`, phase matches and `NumCells < min` → deactivate | **Covered** — `Two-Phase…` (ApplySinglePhase=true), features 3 and 4 (3 cells each, phase 1) |
| 10 | (b) | nothing survives → error `-1` "All Features would be removed" | **Covered** — `At-Threshold Boundary`, MinAllowedFeaturesSize=4 |
| 11 | (c) | interior cell, six valid neighbours, strict majority | **Covered** — `Two-Phase…` index 36 in the corner cluster; index 212's `-Z`/`-X` votes |
| 12 | (c) | boundary cell, fewer than six valid neighbours | **Covered** — `Two-Phase…` indices 0, 1, 6, 210, 212, 215 (feature 4 lives entirely on the +Y/+Z faces) |
| 13 | (c) | cross-feature vote tie → resolved by traversal order (strict `>` at `:231`) | **Covered** — `Two-Phase…` index 212 (2-vs-2); `Vote Counter Reset Between Cells` strip index 6 (1-vs-1) |
| 14 | (c) | `voteCounter` reset between orphan cells (`:240`) → an earlier cell's tallies do not carry forward | **Covered** — `Vote Counter Reset Between Cells`; deleting `:240` flips strip index 6 and fails only that test |
| 15 | (c) | second sweep required (no votable neighbour on sweep one) | **Covered** — `Two-Phase…` (ApplySinglePhase=false) index 0; `At-Threshold Boundary` min=3, strip index 4 |
| 16 | (c) | FeatureId 0 is votable: `:227` admits `neighborFeatureId >= 0`, and feature 0 is never deactivated because `:289` loops from `i = 1`, so a cell labelled 0 can win an orphan's vote and relabel it 0 | **Not covered.** No fixture contains FeatureId-0 cells; every fixture labels all cells 1..N. Legacy-compatible: `MinSize.cpp:356` uses the identical `if(feature >= 0)` and `MinSize.cpp:448` the identical `i = 1` start, so both versions behave the same and there is nothing to deviate |
| 17 | (d) | `:57` `currentNeighborFeatureId >= 0` — true (a source was recorded) and false (no source recorded) | **Covered** — both outcomes occur on every sweep of every reassigning run; the false branch is the majority of cells, whose `neighborsVoxelIndex` entry stays `-1` |
| 18 | (d) | `:59` first conjunct — true (cell still orphaned, copy happens) and false (cell already filled on an earlier sweep, copy suppressed) | **Covered** — the `while` at `:195` always runs a final sweep with `counter == 0`, on which every previously filled cell takes the false branch |
| 19 | (d) | `:59` second conjunct `m_FeatureIds.getValue(source) >= 0` false | **Not covered — unreachable.** A source index is only ever recorded at `:234` for a neighbour whose FeatureId was already `>= 0`, and FeatureIds only ever move from `-1` to `>= 0`, so the recorded source can never be negative when it is read |
| 20 | (e) | `removeList` non-empty → compact arrays and remap FeatureIds | **Covered** — `Two-Phase…` both modes; `At-Threshold Boundary` min=3; `Vote Counter Reset Between Cells` |
| 21 | (e) | `removeList` empty → no compaction, resize is a no-op | **Covered** — `At-Threshold Boundary`, MinAllowedFeaturesSize=2 |
| 22 | (a)–(e) | `m_ShouldCancel` early-exits (`:131`, `:138`, `:200`, `:291`) | **Not covered.** No test injects a cancel signal; these are uniform early-return guards with no output of their own |

`RemoveInactiveObjects` also contains a `featureLevelBaseGroup == nullptr` early-return
and an `activeObjects.size() != totalTuples` early-return. Neither is reachable through
this filter: preflight rejects a NumCells path without a `BaseGroup` parent
(`RequireMinimumSizeFeaturesFilter.cpp:133-138`, error `-5557`) and `activeObjects` is
constructed with exactly `featureNumCellsStoreRef.getNumberOfTuples()` entries.

## Test inventory

Assertion counts are from the `ctest -R "SimplnxCore::RequireMin" --verbose` run of this
change: **5 TEST_CASEs, 2417 assertions, 0 failures.**

| Test case | Status | Notes |
|-----------|--------|-------|
| `Small IN100 Pipeline` | **retired** | Compared SIMPLNX against a 6.6-generated exemplar. Green by construction — it proved only that SIMPLNX reproduces 6.6, never that either is correct. Replaced by the three oracle tests below. Its two archives are retired with it. |
| `Two-Phase 6x6x6 Analytical Oracle` | new-for-V&V | Class 1 + Class 4 on the six-feature two-phase fixture, generated over `ApplySinglePhase ∈ {false, true}` (sections render as `ApplySinglePhase=false` / `ApplySinglePhase=true`). 2243 assertions. |
| `At-Threshold Boundary` | new-for-V&V | Class 1 + Class 4 on a 5×1×1 strip, generated over `MinAllowedFeaturesSize ∈ {2, 3, 4}`, pinning the `<` semantics from both sides and covering the all-removed error. 79 assertions. |
| `Vote Counter Reset Between Cells` | new-for-V&V | Class 1 + Class 4 on a 9×1×1 strip. The only test that can see the per-cell `voteCounter` reset at `:240`: an earlier orphan cell banks two feature-1 votes, and a later orphan cell is a genuine 1-vs-1 cross-feature tie that the leaked count would flip. 55 assertions. |
| `Execute Error - unavailable phase (-5555)` | new-for-V&V | Single-phase guard with a phase number absent from the fixture. 13 assertions. |
| `SIMPL Backwards Compatibility` | kept, untouched | 6.4 and 6.5 conversion fixtures. Byte-identical to its pre-V&V form. 27 assertions. |

## Exemplar archive

- **Archive:** *none — retired this pass.*
- **Retired:** `6_6_min_size_input.tar.gz` (SHA512 `f59d0a5c…bdb11a14`) and
  `6_6_min_size_output.tar.gz` (SHA512 `2ad1bc5e…0a5faf91`). Both `download_test_data()`
  calls are removed from `src/Plugins/SimplnxCore/test/CMakeLists.txt`.
- **Retirement guard:** after this change `grep -rn "6_6_min_size" src/` leaves no code,
  test, CMake, or pipeline consumer — the only remaining matches are this report's own
  self-references. A repo-wide grep across `*.cpp`, `*.hpp`, `*.py`, `*.d3dpipeline` and
  `*.md` confirms no other consumer existed. In particular the `SIMPL Backwards Compatibility`
  test does not read them — it loads only the JSON conversion fixtures under
  `test/simpl_conversion/`.
- **Provenance:** *n/a — the tests no longer depend on a downloaded archive; every fixture
  is constructed in-process.*

## Legacy comparison

**Run 2026-08-18.** Artifacts (never committed; uploaded to OneDrive per V&V policy):
`/Users/mjackson/Workspace9/ww_work/RequireMinimumSizeFeatures/` with a `ReadMe.md`
describing how to re-run.

| Role | Binary |
|---|---|
| Legacy | `/Users/mjackson/Applications/DREAM3D.app/Contents/Bin/PipelineRunner` (the 6.5.171 line), filter `MinSize` `{53ac1638-8934-57b8-b8e5-4b91cdda23ec}` |
| SIMPLNX | `/Users/mjackson/Workspace9/DREAM3D-Build/NX-Com-Qt69-Vtk96-Rel/Bin/nxrunner` |

The 6×6×6 and 5×1×1 unit-test fixtures were written out as legacy-format (`FileVersion 7.0`)
`.dream3d` files carrying a Cell Attribute Matrix (`FeatureIds`, `CopiedScalar`) and a
CellFeature Attribute Matrix (`NumElements`, `Phases`) — the arrays legacy
`MinSize::dataCheck()` requires. **The same grids the unit tests build were used; no
separate A/B data was invented.** The third fixture, the 9×1×1 `Vote Counter Reset
Between Cells` strip, was added *after* the A/B run to kill a mutation the other two were
blind to; it was not run through the legacy binary and is not claimed as legacy evidence.
Each combination ran `DataContainerReader → MinSize →
DataContainerWriter` and `ReadDREAM3D → RequireMinimumSizeFeatures → WriteDREAM3D`, and
the outputs were diffed array by array with h5py.

| Combination | MinAllowedFeatureSize | ApplyToSinglePhase | FeatureIds | CopiedScalar | NumElements | Phases |
|---|---|---|---|---|---|---|
| `cube6_min3_sp0` | 3 | no | match | match | match (7→6 tuples) | match |
| `cube6_min3_sp1` | 3 | yes (1) | match *(no-op)* | match *(no-op)* | match *(no-op)* | match *(no-op)* |
| `cube6_min4_sp0` | 4 | no | match (12 cells changed) | match (7 cells changed) | match (7→4 tuples) | match |
| `cube6_min4_sp1` | 4 | yes (1) | match (11 cells changed) | match (6 cells changed) | match (7→5 tuples) | match |
| `cube6_min5_sp0` | 5 | no | match (16 cells changed) | match (11 cells changed) | match (7→3 tuples) | match |
| `cube6_min5_sp1` | 5 | yes (1) | match (16 cells changed) | match (10 cells changed) | match (7→4 tuples) | match |
| `strip5_min2_sp0` | 2 | no | match *(no-op)* | match *(no-op)* | match *(no-op)* | match *(no-op)* |
| `strip5_min3_sp0` | 3 | no | match (2 cells changed) | match (2 cells changed) | match (3→2 tuples) | match |

**32/32 (combination, array) pairs match. Zero array-level differences.** The two
combinations marked *(no-op)* are labelled by the comparison script so a match that only
proves both binaries did nothing is visible rather than silently counted as evidence.

Because "both agree" is not the same as "both correct", a second script checked each
binary independently against the same hand oracle for `cube6_min4_sp0`,
`cube6_min4_sp1` and `strip5_min3_sp0`: **24/24 ORACLE CHECK PASSED**, legacy and
SIMPLNX alike. At the deliberate 2-vs-2 tie cell (2,5,5) both binaries wrote
`CopiedScalar = 10211`, confirming the strictly-greater-than vote rule in both code
lines — a `>=` rule would have produced `10213`.

No legacy bug was confirmed, so **no surgical patch to the legacy line is required.**

## Deviations from DREAM3D 6.5.171

- **No deviations observed.** Comparison run 2026-08-18 on the two-phase 6×6×6 fixture
  and the 5×1×1 at-threshold strip across 8 parameter combinations. See
  `vv/deviations/RequireMinimumSizeFeaturesFilter.md`.

## Follow-ups (not deviations)

**Input-robustness parity gap with `RequireMinNumNeighborsFilter`.** The sibling filter
was hardened during its own V&V (#1694) and now returns `-55567` for an out-of-range
FeatureId, `-55569` when every feature would be removed, and `-55572` when a coarsening
sweep makes no progress — see `vv/deviations/RequireMinNumNeighborsFilter.md` D1–D3.
`RequireMinimumSizeFeatures` still carries the unguarded forms of the first and third:

* `:325-329` indexes `activeObjects[gnum]` with the raw FeatureId, so a negative or
  out-of-range value is an out-of-bounds `std::vector` read. `voteCounter[neighborFeatureId]`
  at `:229` has the same exposure.
* `assignBadVoxels`' `while(counter != 0)` loop at `:195` has no no-progress guard, so a
  negative region with no reachable retained feature would spin indefinitely.

This is **not** a deviation: DREAM3D 6.5.171 `MinSize.cpp` contains the identical
unguarded code in both places, so the two versions behave the same and there is nothing
for a migrating user to be warned about. It is recorded here as a hardening follow-up
rather than fixed in this change, because adding new error codes and their tests is a
behavioural change outside the scope of this V&V pass and should be reviewed on its own
merits alongside the sibling filter's precedent.
