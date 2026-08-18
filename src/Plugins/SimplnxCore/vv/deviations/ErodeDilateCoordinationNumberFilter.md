# Deviations from DREAM3D 6.5.171: ErodeDilateCoordinationNumberFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ErodeDilateCoordinationNumberFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Headline: legacy A/B comparison performed — one deliberate deviation (`-D1`)

A genuine DREAM3D 6.5.171 `PipelineRunner` was run against hand-built input twins of the C++ test fixtures across **27 combinations** and each output was diffed element-wise against SIMPLNX `nxrunner` output on **two** arrays: `FeatureIds` and an index-encoded `CopiedValues` tracer. **54 of 54 (combination, array) pairs match exactly**, **38** of them on non-trivial output (output ≠ input); the other **16** are no-ops — `CoordinationNumber ∈ {5, 6}` × `Loop ∈ {false, true}` × the `single` and `seam` fixtures = 8 combinations × 2 arrays, at thresholds those grids' maximum coordination number of 4 can never meet.

The 27 combinations break down as:

- **22 from the first pass:** the `single` and `seam` fixtures × (`CoordinationNumber` 1–6 with `Loop=false`, plus `CoordinationNumber` 2–6 with `Loop=true`).
- **5 from the follow-up pass:** the `majority`, `loopdisc` and `reset` fixtures at the exact parameter settings their unit tests assert — `majority` at `CoordinationNumber = 4` with `Loop ∈ {false, true}`, `loopdisc` at `CoordinationNumber = 2` with `Loop ∈ {false, true}`, and `reset` at `CoordinationNumber = 3, Loop = false`. All 10 of these pairs are non-trivial.

Every 5×5×1 grid the C++ oracle tests build has now been through the legacy binary. The `CoordinationNumber = 0, Loop = false` setting is additionally covered on the `single` fixture by the `extra/` probe outputs, which both binaries produce identically.

The legacy `ErodeDilateCoordinationNumber.{h,cpp}` source was additionally diffed against the SIMPLNX algorithm line by line. Both the binary and the legacy source live in a sibling checkout on the authoring engineer's machine and are not committed to this repository.

**No behavioural difference was observed on any *runnable* tested combination**, and no algorithm change was made in consequence. One `-D<N>` entry is open, and it is not an A/B finding: `ErodeDilateCoordinationNumberFilter-D1` records a **two-tier preflight response added deliberately on 2026-08-19** (requester decision, uniform preflight validation across the erode/dilate family, refined at final gate review to "errors or warns") covering `CoordinationNumber < 2` with `Loop` enabled. Those are precisely the settings the A/B could never run, because *neither* binary terminated there on any of its boundary-bearing fixtures under a 45 s guard. SIMPLNX now **rejects** `CoordinationNumber = 0` with `Loop` — non-terminating on every possible input — and **warns** on `CoordinationNumber = 1` with `Loop`, whose termination is **data-dependent**: it always completes as a no-op on a boundary-free volume, and it *may* oscillate indefinitely on one containing a good/bad boundary, so it must stay runnable. Legacy accepts both silently. That is a user-visible difference in accepted input and is written up as a deviation rather than buried as an improvement.

The sections below record what was investigated and ruled out, so the next engineer does not re-open settled questions, and record three latent defects that both implementations share.

---

## ErodeDilateCoordinationNumberFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateCoordinationNumberFilter-D1` |
| **Filter UUID** | `e4553e82-ab82-49d7-993c-8b55b15c7724` |
| **Status** | active |

**Symptom:** A pipeline that sets `Coordination Number` below 2 with `Loop Until Gone` enabled can run forever in DREAM3D 6.5.171. SIMPLNX now responds to the two thresholds differently, because their non-termination is not of the same kind. `Coordination Number = 0` with `Loop` is rejected at preflight with error `-16801`, and the pipeline does not start. `Coordination Number = 1` with `Loop` is **accepted with warning `-16802`** and runs: its termination is data-dependent — it always completes as a no-op on a volume with no good/bad boundary, and it *may* hang on one that has a boundary.

**Root cause:** Algorithmic choice — a deliberate two-tier guard added in SIMPLNX, not a defect in either implementation's arithmetic. Both versions iterate until a trailing counter over `coordinationNumber[]` reaches zero (`Algorithms/ErodeDilateCoordinationNumber.cpp:171-186`, predicate at `:180`; legacy `ErodeDilateCoordinationNumber.cpp:300-330`). The predicate is `coordinationNumber[voxelIndex] >= CoordinationNumber`, and the threshold decides how much of the volume can satisfy it:

- **`CoordinationNumber = 0` — universal non-termination.** The predicate reads `coordination >= 0`, which *every* voxel of *every* volume satisfies, boundary or not. The counter can never reach zero on any input whatsoever, so no volume exists on which the sweep converges.
- **`CoordinationNumber = 1` — data-dependent non-termination.** The predicate reads `coordination >= 1`, which only voxels actually sitting on a good/bad boundary satisfy. On a **boundary-free** volume — one that is entirely a single feature, or entirely bad data, which is a realistic post-cleanup input — no voxel satisfies the predicate, the first sweep changes nothing, and the loop terminates immediately; this half is unconditional. On a volume that **does** contain a boundary the outcome depends on the data. The sweep reassigns *both* good voxels adjacent to bad *and* bad voxels adjacent to good, so it commonly recreates the boundary rather than removing it and the counter never reaches zero — observed on the 5×5×1 fixtures, where both binaries were still running when killed at 45 s. But it is not universal: `FeatureIds` is transferred **in place** during the sweep (`copyTuple` writes into the array the vote reads), so voxels visited later in the same sweep see the updated ids, and a configuration in which one sweep consumes the boundary outright does converge. A 2×1×1 volume holding `[feature 1, bad]` terminates in two sweeps (derived from the source, not executed). So `CoordinationNumber = 1` with `Loop` **may** oscillate indefinitely on a boundary-bearing volume; it is not guaranteed to. Both implementations behave the same way, so a boundary-free 6.5.171 run at this setting completed normally.

Confirmed by execution, not inference: both binaries were killed after 45 s at both settings on a boundary-bearing fixture (`ww_work/ErodeDilateCoordinationNumber/extra/`, exit 124). Legacy's `dataCheck()` (legacy `:118-123`) validates only the closed interval `[0,6]`, so it permits both thresholds unconditionally. SIMPLNX's `preflightImpl` now rejects only the universally non-terminating threshold (`CoordinationNumber < 1 && Loop == true`, `-16801`) and warns on the data-dependent one (`CoordinationNumber == 1 && Loop == true`, `-16802`). `Loop = false` at either threshold is untouched, terminates in one sweep, and remains fully supported in both versions.

The boundary-free half of the claim — that `CoordinationNumber = 1, Loop = true` terminates as a no-op on a volume with no good/bad boundary — is now pinned by execution in NX itself, added 2026-08-19: `ErodeDilateCoordinationNumberTest.cpp`'s `Boundary-Free Volume Terminates` case runs that exact combination to completion on an all-feature-1 5×5×1 grid and asserts `FeatureIds`, `CopiedValues`, and `IgnoredValues` are all byte-unchanged. This closes the gap the warning message's own text had been resting on source reading alone for. The 6.5.171 half of the same claim remains **source-derived only** (legacy `ErodeDilateCoordinationNumber.cpp:300-330`) rather than confirmed by running the legacy binary to completion — a boundary-free no-op grid carries no comparative A/B information, so it was not added to the legacy sweep in the Legacy A/B protocol.

**Affected users:** Anyone migrating a saved 6.5.171 pipeline that sets `Coordination Number` below 2 *with* `Loop Until Gone` enabled — and the two thresholds affect different populations. A pipeline at `Coordination Number = 0` with `Loop` can never have produced a result in 6.5.171, because it hangs on every input; no existing legacy output is invalidated, and what changes is that the pipeline now fails fast with an actionable message instead of appearing to run. A pipeline at `Coordination Number = 1` with `Loop` **may well have completed** in 6.5.171 — it does so on any boundary-free volume, as a no-op — so rejecting it would have broken a legacy pipeline that ran successfully. Such a pipeline still completes in SIMPLNX, and now carries a warning saying that termination is data-dependent and describing the input class on which it is not guaranteed. Users at `CoordinationNumber >= 2` with `Loop`, or at any `CoordinationNumber` with `Loop` disabled, see no change whatsoever.

**Recommendation:** Trust SIMPLNX. The rejected setting (`0` with `Loop`) has no correct output in either version, so refusing it loses nothing and converts a silent hang into a diagnosable error. The warned setting (`1` with `Loop`) is left runnable precisely because it *does* have a correct output on some inputs, and the warning tells the user which inputs those are; the run is also cancellable (see the shared-latent-defect note below), so a user who hits the oscillating case can interrupt it. A 6.5.171 user who wants guaranteed termination should not enable `Loop` below a `Coordination Number` of 2. No 6.5.172 patch was raised, because the non-termination is a shared design limitation rather than a coding error in the legacy source.

---

## Non-deviations (confirmed correct — do not "fix")

### Tie-break: first-feature-to-reach-the-maximum wins, and it is deterministic

When a bad voxel's face neighbours split evenly between two features, the leader is replaced only when a later vote count is *strictly* greater than the running maximum (`Algorithms/ErodeDilateCoordinationNumber.cpp:131`). Face neighbours are visited in the fixed order `-Z, -Y, -X, +X, +Y, +Z` supplied by `initializeFaceNeighborInternalIdx()`. The winner is therefore the first feature to reach the maximum vote count in that order — fully deterministic, with no RNG, no parallelism, and no container-iteration-order dependence anywhere in the vote.

Legacy is identical: `n[feature]++`, `current = n[feature]`, `if(current > most)`, over `l = 0..5` with the same offset ordering (`ErodeDilateCoordinationNumber.cpp:234-277` in the legacy tree).

**Settled by binary evidence, not source reading.** Per the `ErodeDilateBadData` precedent (PR #1687), where a plausible tie-break bug hypothesis survived source comparison and was falsified only by running both binaries, this was carried to the A/B rather than acted on from the source. The `seam` fixture puts a genuine 2-vs-2 tie on the bad voxel (feature 1 at -Y and -X, feature 2 at +X and +Y). Both binaries produced `FeatureIds[12] == 1` and `CopiedValues[12] == 111`, i.e. both copied from source neighbour index 11 — the -X neighbour, the second of the two feature-1 votes. Identical, deterministic, and matching the hand-derived oracle in both. Not a deviation, and not a bug.

Among equal-*feature* votes the rule inverts and the **last** such neighbour supplies the tuple, because each additional vote for the already-leading feature is strictly greater than the previous count. Confirmed on the `single` fixture, where both binaries copied from index 17 (the +Y neighbour, last of four feature-1 votes).

### `featureCount` reset asymmetry: index 0 leaks, but the leak cannot change output

This was flagged going in as the most likely defect in the filter, so it was audited exhaustively. **The asymmetry is real; the corruption it would imply is not.**

The accumulator `featureCount` is allocated once outside the sweep (`:80`). It is incremented at `:129` for the neighbour's feature id whenever the boundary predicate `(featureName > 0 && feature == 0) || (featureName == 0 && feature > 0)` holds, and zeroed at `:163-166` for every valid neighbour whose feature id is `> 0`.

The two increment paths are mutually exclusive per voxel, because `featureName` is fixed for the voxel:

- **Bad voxel (`featureName == 0`).** Every qualifying neighbour has `feature > 0`, so every increment targets an index `> 0`. The reset loop zeroes `featureCount[feature]` for *all* valid neighbours with `feature > 0` — a superset of the incremented set. Fully matched.
- **Good voxel (`featureName > 0`).** Every qualifying neighbour has `feature == 0`, so every increment targets index 0 — which the `if(feature > 0)` guard on the reset loop never clears. **Unmatched: `featureCount[0]` grows monotonically for the lifetime of the call.**

The leak cannot affect the chosen neighbour. In the good-voxel branch every vote lands on the single key 0, so `most` is only ever compared against successive values of `featureCount[0]`; there is no cross-feature comparison to skew. With a leaked base `B ≥ 0`, the k qualifying neighbours produce counts `B+1, B+2, …, B+k`, each strictly greater than the previous, so the **last** qualifying neighbour wins. With a correct reset (`B = 0`) the counts are `1, 2, …, k` and the last qualifying neighbour wins again. Same selection, always. `most` is re-initialised to 0 per voxel (`:113`), so the first qualifying neighbour always sets `neighbors[voxelIndex]` and a stale entry from an earlier sweep can never be consumed.

Legacy has the identical guard (`if(feature > 0) { n[feature] = 0; }`) and therefore the identical leak. The A/B confirms behavioural equivalence directly: `CoordinationNumber` 1–3 reassign *good* voxels — the branch that feeds `featureCount[0]` — and at `CoordinationNumber = 1` fifteen of twenty-five voxels change on both fixtures, matching element-for-element between the two binaries on both `FeatureIds` and `CopiedValues`.

The follow-up A/B pass added a fixture built specifically to make the leak *matter* if it were unmatched. The `reset` fixture (`CoordinationNumber = 3, Loop = false`) places two bad voxels far enough apart that neither can see the other: the first banks three feature-1 votes, and the second carries a genuine near-tie (two feature-2 votes then one feature-1 vote). If the per-voxel reset did not fire, the leaked three would push feature 1 past feature 2 and the second voxel would take a different feature *and* a different source neighbour. Both binaries produced `FeatureIds[22] == 2` and `CopiedValues[22] == 121`, which is the reset-fires answer; the leaked answer would read 1 and 123. So the reset for `feature > 0` demonstrably fires identically in both implementations, and the `featureCount[0]` leak is the only unmatched half.

The `Class 1 Oracle - Good Voxel Branch` test case pins this in the repository rather than only in the A/B artifacts: it runs `CoordinationNumber = 1, Loop = false` on the single-bad-voxel fixture and asserts all 25 `FeatureIds` and all 25 `CopiedValues` against a voxel-by-voxel hand derivation, with `featureCount[0]` reaching 20 over the sweep. The derived grid reproduces the same 15 changed indices the A/B reported, independently of the A/B. The companion `Class 1 Oracle - Accumulator Reset` test case pins the `reset` fixture described above, and `Class 1 Oracle - Zero Coordination Number Guard` pins the `CoordinationNumber = 0` variant of the same 15-voxel footprint.

Not a deviation. See "Shared latent defects" below for the one residual concern (int32 overflow at extreme scale), which is a robustness issue in both implementations rather than a difference between them.

### `neighbors` is never re-initialised between sweeps

`std::vector<int64> neighbors(totalPoints, -1)` is allocated once (`:50`) and never reset, so a `Loop = true` run carries stale entries into later sweeps. Harmless: `neighbors[voxelIndex]` is read only under `coordinationNumber[voxelIndex] >= CoordinationNumber && coordinationNumber[voxelIndex] > 0`, and `coordination > 0` guarantees at least one qualifying neighbour, which always overwrites the entry (`most` starts at 0, and every vote count is `≥ 1`). Legacy behaves the same way (`neighborsPtr` is created once before the `while` loop and initialised to -1 once). Not a deviation.

### `IgnoredDataArrayPaths` is honoured in SIMPLNX and unreachable from legacy JSON

Both filters expose an "Attribute Arrays to Ignore" parameter, and both exclude those arrays from the `copyTuple` transfer. Legacy's `readFilterParameters` (`ErodeDilateCoordinationNumber.cpp:90-97` in the legacy tree) reads only `FeatureIdsArrayPath`, `CoordinationNumber`, and `Loop` — it never reads `IgnoredDataArrayPaths` — so a legacy pipeline loaded from JSON always runs with an empty ignore list regardless of what the file contains. The A/B therefore ran with an empty ignore list on both sides, and the parameter's effect is covered by the SIMPLNX unit tests instead (the `IgnoredValues` array is asserted unchanged in all eight oracle test cases). A SIMPLNX-side capability legacy cannot reach from a saved pipeline, not a behavioural difference in the algorithm.

### No progress messaging in SIMPLNX (and the cancel check, now added)

`operator()()` emits no progress messages. Legacy emits none from this filter's inner loop either, so this is not a difference between the two, and on any grid where the sweep terminates the run is fast enough that the absence is not user-visible.

The **cancel check** was a different matter and **was fixed in this V&V pass**. `operator()()` took `m_ShouldCancel` in its constructor, stored it, and exposed it through `getCancel()`, but never read it — so the unbounded `Loop = true` case below could not be interrupted from the DREAM3D-NX GUI at all. A house-standard early return now runs at the top of the sweep's outer (z) loop, matching `Algorithms/ErodeDilateBadData.cpp:144-148`. Legacy has no cancel check either, so this is a SIMPLNX-side improvement over a shared weakness rather than a deviation, and it is output-neutral on every terminating run.

---

## Shared latent defects (present in BOTH implementations)

The latter two are not deviations — both implementations behave identically and both remain exposed. The first was a shared defect until 2026-08-19; its universal half is **now unreachable in SIMPLNX** and its data-dependent half is now **warned about but still reachable**, so it is also carried as deviation `ErodeDilateCoordinationNumberFilter-D1` above. All three are recorded here because the A/B and the source-level comparison are the only places the evidence exists.

### `Loop = true` may never terminate for low `CoordinationNumber` (universal half closed in SIMPLNX 2026-08-19; data-dependent half warned; both still open in 6.5.171)

**Confirmed by execution, in both binaries.** The stopping condition is a trailing counter loop (`:171-186`) whose predicate at `:180` is `coordinationNumber[voxelIndex] >= CoordinationNumber`; the loop exits when no voxel satisfies it. How much of the volume can satisfy it depends on the threshold, and the two low values differ in kind rather than in degree:

- **`CoordinationNumber = 0`: non-termination is universal.** The predicate reads `coordination >= 0`, which every voxel of every volume satisfies regardless of its contents, so `counter` can never reach 0 on *any* input. There is no volume on which this setting converges.
- **`CoordinationNumber = 1`: non-termination is data dependent.** The predicate reads `coordination >= 1`, satisfied only by voxels on a good/bad boundary. Given a **boundary-free** volume — entirely one feature, or entirely bad data — no voxel satisfies the predicate at all, the first sweep is a no-op, and the loop exits immediately; that half holds unconditionally, in both implementations, so a boundary-free 6.5.171 run at this setting completed normally. Given a boundary, the sweep flips *both* good voxels adjacent to bad and bad voxels adjacent to good, so a surviving boundary is the common outcome, `counter` never reaches 0, and the run **may** oscillate indefinitely — which is what the 5×5×1 probes show. It is not a guarantee: `FeatureIds` is transferred in place during the sweep, so voxels visited later read the ids already rewritten by earlier ones, and a configuration whose single sweep consumes the boundary converges instead. A 2×1×1 volume holding `[feature 1, bad]` terminates in two sweeps (derived from the source, not executed). Boundary presence therefore makes non-termination *possible*, not certain.

Probe results (`ww_work/ErodeDilateCoordinationNumber/extra/`): on a **boundary-bearing** fixture, `CoordinationNumber = 0, Loop = true` and `CoordinationNumber = 1, Loop = true` were each killed after 45 s in both `PipelineRunner` and `nxrunner` (exit 124), while `CoordinationNumber = 0, Loop = false` completes normally in both. The probe therefore demonstrates a hang at both thresholds *on that fixture*, and nothing more; it is the source argument above that separates the two — universal at `0`, data-dependent at `1`, where the probe's fixture happens to be one of the boundary-bearing volumes that does not converge. That is exactly the distinction the guards encode.

**Status changed on 2026-08-19.** Two SIMPLNX-side changes landed against this hazard in this V&V cycle, in order:

1. *Cancellability (earlier in this cycle).* `operator()()` stored `m_ShouldCancel` and never read it, so the GUI's Cancel button did nothing and the only escape from the spin was killing the application. A check at the top of the sweep's outer (z) loop now returns `{}` as soon as the flag is set.
2. *Two-tier preflight validation (2026-08-19, requester decision — uniform preflight validation across the erode/dilate family, refined at final gate review to "errors or warns" once the data-dependence of the `1` case was established).* `preflightImpl` **rejects** `CoordinationNumber = 0` with `Loop` enabled (`-16801`), because that combination is non-terminating on every possible input and so has no correct output to lose. It **warns** on `CoordinationNumber = 1` with `Loop` enabled (`-16802`) rather than rejecting it, because that combination terminates on any boundary-free volume and a blanket rejection would break a legacy pipeline that ran to completion. The warning names the value, says termination is data-dependent — boundary-free volumes complete after a single no-op sweep, boundary-bearing ones may oscillate indefinitely — and points out that the run is cancellable. The cancel check from step 1 is therefore not merely defence in depth for the `1` case — it is the user's escape hatch on the reachable half of the hazard.

**6.5.171 remains fully exposed on both halves.** Its `dataCheck()` validates only the closed interval `[0,6]` (legacy `ErodeDilateCoordinationNumber.cpp:118-123`), so both settings stay reachable from the legacy UI with no diagnostic of any kind, and its `execute()` never consults `getCancel()`, so the resulting spin is also uninterruptible. A 6.5.171 user who enters the oscillating case still has to kill `PipelineRunner` or the GUI.

Because SIMPLNX now rejects one input that 6.5.171 accepts and warns on another, this is no longer a purely shared weakness: it is a user-visible difference in accepted parameters and is written up as deviation `ErodeDilateCoordinationNumberFilter-D1`. Note that both guards are **preflight-only and value-based**, so they change no output: every parameter combination that terminated before still terminates and still produces byte-identical results, which the eight unchanged oracle test cases and the 54/54 A/B pairs continue to demonstrate.

**Residual recommendation:** for SIMPLNX, none beyond the warning — the oscillating case is diagnosable and cancellable, and the terminating boundary-free case is preserved. For the 6.5-line, the same `CoordinationNumber = 0` rejection could be added to legacy `dataCheck()` if a 6.5.172 patch is ever cut for other reasons; it was not raised on its own account, since the non-termination is a design limitation of the stopping condition rather than a coding error, and no legacy output is wrong as a result.

### `featureCount[0]` can overflow int32 on very large volumes

Following from the reset asymmetry above: `featureCount[0]` accumulates one increment per (good voxel, bad face-neighbour) adjacency for the entire call, across all sweeps, and is `int32`. On a volume with more than ~2.1 billion such adjacencies — reachable on a 1000³ volume with pervasive bad data, especially with `Loop = true` — the counter overflows. Signed overflow is undefined behaviour, and if the value wrapped negative then `current > most` would be false on the first qualifying neighbour, leaving `neighbors[voxelIndex]` at a stale value or at the initial `-1`, which would then be passed to `copyTuple` as a source index.

Legacy has the identical `QVector<int32_t> n` accumulator and the identical guard, so the exposure is the same in both. No test can demonstrate it at a tractable size, and no observed output depends on it, so no code change was made in this cycle.

**Recommendation:** drop the `if(feature > 0)` guard on the reset loop so the reset set is an unconditional superset of the increment set. Provably output-neutral by the argument in the reset-asymmetry section above.

*Claim verified:* `ErodeDilateBadData` does use the unguarded form post-#1687 — `Algorithms/ErodeDilateBadData.cpp:199-200` reads `const int32 feature = featureIds[neighborPoint]; featureCount[feature] = 0;` with no guard, checked on this branch. Note the reason the unguarded form is sufficient there and would be a genuine fix here: `ErodeDilateBadData`'s entire vote block is inside `if(featureName == 0)` (`:158`), so it only ever has the bad-voxel branch and never accumulates into `featureCount[0]` in the first place. `ErodeDilateCoordinationNumber` processes good voxels too, which is where the leak comes from.

### A negative `FeatureId` wraps the `numFeatures` scan and yields a zero-length accumulator

The `featureCount` accumulator is sized from a maximum scan over `FeatureIds` (`:62-71`, `:80`):

```cpp
size_t numFeatures = 0;
for(size_t i = 0; i < totalPoints; i++)
{
  const int32 featureName = featureIds[i];
  if(featureName > numFeatures)   // int32 compared against size_t
  {
    numFeatures = featureName;
  }
}
// ...
std::vector<int32> featureCount(numFeatures + 1, 0);
```

`featureName` is `int32` and `numFeatures` is `size_t`. The usual arithmetic conversions promote the signed operand to the unsigned type, so a **negative** `FeatureId` is compared — and then assigned — as a huge unsigned value. The most damaging case is exactly `-1`: `numFeatures` becomes `SIZE_MAX`, `numFeatures + 1` wraps to **0**, and `featureCount` is allocated with **zero** elements. Every subsequent `featureCount[feature]++` and `featureCount[feature] = 0` then indexes past the end of an empty vector — undefined behaviour, with no bounds check anywhere on the path. Other negative ids (say `-2`) instead request a multi-exabyte allocation and throw `std::length_error` / `std::bad_alloc` out of `operator()()`, which surfaces as a crash rather than a filter error.

Nothing upstream prevents this. `FeatureIds` is taken through an `ArraySelectionParameter` restricted to `int32` with component shape `{1}` (`ErodeDilateCoordinationNumberFilter.cpp:63-64`); neither the parameter nor `preflightImpl` constrains the *values*, and negative ids are producible by hand-authored data, by a Python-written array, or by an upstream filter that uses a negative sentinel.

**Shared with 6.5.171.** Legacy sizes its accumulator the same way, with the same signed/unsigned comparison: `size_t numfeatures = 0;` with `int32_t featurename` at `ErodeDilateCoordinationNumber.cpp:182-191` in the legacy tree (predicate at `:187`), feeding `QVector<int32_t> n(numfeatures + 1, 0);` at `:208`. So the exposure is identical in both implementations and this is **not** a deviation.

No test demonstrates it: writing one would mean asserting on undefined behaviour, and the useful fix is a guard, not an expectation. No code change was made in this cycle because the fix is a behavioural change to input validation that deserves its own change and its own test.

**Recommendation:** either reject negative `FeatureIds` in `preflightImpl` with a clear error naming the offending value, or make the scan signed-safe — e.g. accumulate into an `int32` maximum, clamp at 0, and size from that — so that a negative id can never reach the accumulator index.
