# V&V Report: ComputeSurfaceAreaToVolumeFilter

|                            |                                                                                     |
|----------------------------|-------------------------------------------------------------------------------------|
| Plugin                     | SimplnxCore                                                                         |
| SIMPLNX UUID               | `94e83e4f-797d-4594-b130-3819b7676f01`                                              |
| SIMPLNX Human Name         | Compute Surface Area to Volume & Sphericity                                         |
| DREAM3D 6.5.171 equivalent | `FindSurfaceAreaToVolume` — SIMPL UUID `5d586366-6b59-566e-8de1-57aa9ae8a91c`       |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                             |
| Status                     | READY FOR REVIEW                                                                    |
| Sign-off                   | Authored by Michael Jackson <mike.jackson@bluequartz.net>, 2026-08-20. Second-engineer sign-off **delegated to the PR reviewer** (requester decision, 2026-08-19). |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port.** `SimplnxCoreLegacyUUIDMapping.hpp:186` maps the legacy SIMPL UUID straight onto this filter. Legacy `FindSurfaceAreaToVolume.cpp` was diffed line-by-line against `Algorithms/ComputeSurfaceAreaToVolume.cpp` this pass: neighbor offsets, boundary guards, the differing-id face test, the face-area assignments and both finalize loops are structurally identical, including two shared arithmetic bugs. Four SIMPLNX-side additions (validation guard, cancel check, progress messages, honoring the sphericity toggle) — see *Port-time deltas*. |
| Oracle (confirmed)     | **Class 1 (Analytical)** primary, **Class 4 (Invariant)** companion. Surface area is a hand-countable sum of face areas and sphericity is a closed form, so every expected value is derivable without reference to any implementation. Confirmed — 10 fixtures as `SECTION`s/TEST_CASEs in `test/ComputeSurfaceAreaToVolumeTest.cpp`, all pass. Invariants riding along: sphericity scale-invariance (F1 vs F2), the X-rod/Y-rod mirror pair (F3), and id-0 / positive-shell area equality (F6). |
| Code paths enumerated  | **20 of 22 exercised.** Gaps: the per-Z-slice `m_ShouldCancel` early return (needs cancel-signal injection) and the per-Z-slice progress message (emitted on every run, never asserted). |
| Tests today            | **4 TEST_CASEs / 4 ctest entries, 211 assertions, all pass** — 1 Class 1 analytical TEST_CASE carrying 9 fixtures (158 assertions) + 1 sphericity-toggle-off test (13) + 1 four-section error-path test (15) + 1 SIMPL backwards-compatibility test (25). Full `SimplnxCore::` suite green at 981/981 and `PIPELINE::OrientationAnalysis` green at 21/21 after the new preflight guard. |
| Exemplar archive       | **None consumed.** The prior test's `6_6_stats_test_v2.tar.gz` dependency is retired (circular oracle that additionally carried both bugs — proof below). The `download_test_data()` entry in `test/CMakeLists.txt` is left untouched because other tests still consume the archive. |
| Legacy comparison      | **Run.** Ten fixtures through the 6.5.171 `PipelineRunner`, plus one extra negative-id probe. **10/10 SIMPLNX matches the oracle; 10/10 legacy matches the prediction made from the legacy source before the run.** Legacy differs from fixed SIMPLNX on surface area for the two anisotropic fixtures and on sphericity for eight — both differences are the pre-identified shared bugs. Root cause proven by surgically patching a local build of the legacy source: patched-legacy == fixed-SIMPLNX == oracle on **9 of 9** legacy-admissible fixtures. |
| Bug flags              | `-D1` (±X/±Y face areas swapped) and `-D2` (truncated sphericity exponents) — both **confirmed shared bugs, fixed in SIMPLNX this pass**. `-D3`/`-D4`/`-D5` are behavior differences where SIMPLNX is already correct. One SIMPLNX-only gap closed: no preflight cross-check of FeatureIds tuple count against the geometry cell count (out-of-bounds read), now error `-12803`. |
| V&V phase              | Oracle designed and confirmed before any legacy run; two shared bugs found, fixed and alignment-validated; preflight guard added RED-first; six-mutation verification sweep passed; user documentation rewritten; deviations written. Outstanding before promotion to COMPLETE: PR-reviewer sign-off (see header), the uncovered cancel path, and **out-of-core build runs, waived by requester decision 2026-08-19**. |

## Summary

`ComputeSurfaceAreaToVolumeFilter` computes, for each Feature in an Image Geometry, the ratio of its voxelized surface area to its volume, and optionally its sphericity. Verification is **Class 1 (Analytical)**: surface area is a hand-countable sum of shared-face areas and sphericity is a closed form, so every expected value is derivable without reference to any implementation. Fixtures F1–F7 (eight TEST_CASE sections, counting F6's two shell variants) were hand-derived, frozen in `derive.py`, and RED-run before any binary was invoked; F1b and F2b were added afterward — F1b for deviation D3's over-provisioned Feature AM case, F2b to close the volume-recount blind spot — with their expected values likewise hand-derived from the same rules (F2b's checked post-hoc against `derive.py` as well) rather than read off any observed run. Two bugs shared with DREAM3D 6.5.171 were found and fixed — the ±X and ±Y face areas were swapped (invisible on isotropic spacing, wrong on every anisotropic dataset) and the sphericity exponents were truncated decimals — a missing FeatureIds/geometry preflight cross-check was closed, the user documentation's two false claims were corrected, and a surgical patch to a local legacy build proves the two fixes bring SIMPLNX and SIMPL back into output alignment on all nine legacy-admissible fixtures.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp:186` maps legacy SIMPL UUID `5d586366-6b59-566e-8de1-57aa9ae8a91c` directly to `FilterTraits<ComputeSurfaceAreaToVolumeFilter>`, and `test/simpl_conversion/{6_4,6_5}/ComputeSurfaceAreaToVolumeFilter.json` carry the legacy `FeatureIdsArrayPath` / `NumCellsArrayPath` / `SurfaceAreaVolumeRatioArrayName` / `CalculateSphericity` / `SphericityArrayName` parameter set unchanged. The legacy source (`Source/Plugins/Statistics/StatisticsFilters/FindSurfaceAreaToVolume.cpp`, from a sibling `DREAM3D` checkout on the authoring engineer's machine, not committed to this repository) was diffed line-by-line against `Algorithms/ComputeSurfaceAreaToVolume.cpp` this pass rather than inferred from documentation. The correspondence is close enough that both files carried the same two arithmetic defects and the same three mislabeled comments.

*Port-time deltas:*

1. **Feature-count validation.** Legacy scans FeatureIds itself and errors `-5555` unless `max(FeatureIds) == numFeatures - 1` exactly. SIMPLNX calls `ValidateFeatureIdsToFeatureAttributeMatrixIndexing`, which errors `-5355` on negative ids and `-5351` when an id would index past the feature arrays, and accepts an over-provisioned Feature AttributeMatrix. Changes which inputs are *accepted*, not the values computed for accepted inputs — deviations D3 and D5.
2. **Sphericity toggle honored.** Legacy never reads `CalculateSphericity` from a pipeline file and writes `m_Sphericity[i]` unconditionally; SIMPLNX creates the array and runs the loop only when the flag is set. Fixes a latent legacy null dereference — deviation D4.
3. **Cancel check.** SIMPLNX reads `m_ShouldCancel` once per Z slice and returns early (added by PR #1582); legacy has none. Additive; no output change on a run to completion.
4. **Progress reporting.** SIMPLNX emits a per-Z-slice `Computing Z Slice: 'n'` info message and a `Computing Sphericity` message (PR #1267); legacy is silent. No output change.
5. **Preflight output creation.** SIMPLNX creates both outputs through `CreateArrayAction` sized from the Feature AttributeMatrix shape; legacy used `createNonPrereqArrayFromPath` with an initial value of 0. Both zero-initialize, which is what makes the untouched index-0 tuple deterministically `0.0f` in each.
6. **This branch.** Added preflight error `-12803` (FeatureIds tuple count vs geometry cell count), fixed D1 and D2, and rebuilt the test suite.

*Material PRs since baseline:* six PRs have touched these two files; none is behavioral except as noted. #1278 (`BUG: Ensure FeatureId arrays are range checked against the Feature Attribute Matrix`) and #1308 (validation API update) introduced delta 1; #1267 and #1582 introduced deltas 3–4; #1439 and #1238 are store-API and pipeline-path churn.

## Bug Fixes (this pass)

### D1: ±X and ±Y face areas were swapped — fixed

The face shared with a ±Y neighbor lies in the XZ plane (`dx·dz`); the face shared with a ±X neighbor lies in the YZ plane (`dy·dz`). Both were assigned the other's product. Fixed in `Algorithms/ComputeSurfaceAreaToVolume.cpp`, along with the three comments that had recorded the same misconception. RED evidence: fixture F3 reported `SAVR = [_, 2.75, 2.0]` against the derived `[_, 2.0, 2.75]` — the two rods' values exchanged — and fixture F7's 2-cell rod reported `2.5` against `2.0`. Shared with 6.5.171; see deviations doc and the alignment patch.

### D2: sphericity exponents were truncated decimals — fixed

`0.333333f` / `0.66666f` replaced with `1.0f / 3.0f` / `2.0f / 3.0f`. RED evidence: every one of the nine sphericity-bearing fixtures whose value is exponent-sensitive failed pre-fix at a 1e-6 relative tolerance — F5's degenerate `+inf` is the tenth fixture and is immune to either exponent — by 1.405e-6 to 2.009e-5 relative on the isotropic fixtures, larger still where compounded with D1 on the anisotropic ones. Shared with 6.5.171.

### SIMPLNX-only gap closed: FeatureIds/geometry cross-check (`-12803`)

The algorithm walks the ImageGeom's cell extents and indexes FeatureIds with the resulting flat index, but nothing forced the FeatureIds selection to come from the geometry's own cell AttributeMatrix — a smaller selection was read out of bounds with no diagnostic. `preflightImpl` now errors `-12803` when the tuple counts disagree, naming both actual values. Written RED-first: the new Error Paths section failed (`preflight` returned valid) before the guard existed. Legacy has no equivalent check either, but this is a new guard rather than a behavior difference on any valid input, so it is recorded here and not as a deviation.

## Oracle

*Class:* **1 (Analytical)**, with **4 (Invariant)** companions.

*Applied:* The two outputs have closed forms that need no implementation to evaluate:

- `A(f)` = the sum, over every cell of feature `f` and every one of its six face neighbors, of the shared face area — counted iff the neighbor is **inside** the grid and carries a **different** id (id 0 included as "different"). The ±Z face is `dx·dy`, the ±Y face is `dx·dz`, the ±X face is `dy·dz`.
- `V(f) = NumCells[f] · dx·dy·dz`, from the user-supplied array, never recounted.
- `SAVR(f) = A/V`; `Sphericity(f) = π^(1/3)(6V)^(2/3)/A` (Wadell 1935).

Expected values for F1–F7 (eight TEST_CASE sections) were produced by an independent brute-force face enumeration (`derive.py` in the V&V working folder) written from those rules and RED-run before either binary was invoked on any fixture. F1b and F2b were added afterward — F1b for deviation D3's over-provisioned Feature AM case, F2b to close the volume-recount blind spot the blind-suite proof exposed — and their expected values were likewise hand-derived from the same rules (F2b's also checked post-hoc against `derive.py`) rather than taken from an observed run. Spacings are dyadic (0.5) or small integers (1, 2, 4), so every expected area, volume and ratio is exactly representable in float32 and the ratio assertions are **exact equalities**, not tolerances. Sphericity involves cube roots; the tolerance is 1e-6 relative, chosen after confirming with a float32 simulation that the fixed expression reproduces the double-precision oracle to within 1.3e-7 on every fixture — so 1e-6 is a real constraint, and indeed the pre-fix truncated exponents miss it on all nine sphericity-bearing fixtures whose value is exponent-sensitive (F5's degenerate `+inf` is immune to either exponent).

*Ordering evidence (the one ordering rule):* the derivation predicted, before any run, both the correct values **and** what the pre-fix code must produce under the swapped-area and truncated-exponent hypotheses. The observed pre-fix output matched those predictions to nine decimal places (F1 `0.805997133` predicted vs `0.8059971333` observed; F2 `0.805986106` vs `0.8059861064`; F4 `1.611994267` vs `1.6119942665`; F3 `SAVR[1] = 2.75` vs `2.75`). That agreement is what establishes the oracle and the bug model together rather than one being fitted to the other.

*Fixtures (all in `test/ComputeSurfaceAreaToVolumeTest.cpp`):*

| # | Fixture | Geometry / spacing | What it pins | Pre-fix value |
|---|---|---|---|---|
| F1 | single interior voxel | 3³, spacing 0.5 | `A = 6a²`, `SAVR = 6/a = 12`, `Ψ = (π/6)^⅓ = 0.805996`; `NumCells[0] = 5` sentinel proves index 0 of both outputs is never written | `Ψ = 0.805997133` |
| F1b | over-provisioned Feature AM | 3³, 4 feature tuples, `max(id) = 1` | SIMPLNX accepts; unused tuples get `SAVR = 0`, `Ψ = +inf`. Legacy rejects (`-5555`) — deviation D3 | (same) |
| F2 | 2×2×2 interior cube | 4³, spacing 0.5 | `A = 24a²`, `SAVR = 3/a = 6`; `Ψ` identical to F1 → **sphericity scale invariance** (Class 4) | `Ψ = 0.805986106` |
| F2b | NumCells ≠ cell count | 3³, one cell of id 1, `NumCells[1] = 8` | volume comes from `NumCells`: `SAVR = 1.5`, not the `12` a recount gives | `Ψ = 3.223944` |
| F3 | anisotropic X-rod + Y-rod | 6×8×3, spacing (1, 2, 4) | **the D1 discriminator.** `A₁ = 2·8 + 8·4 + 8·2 = 64` (`SAVR = 2.0`); the mirror `A₂ = 2·4 + 8·8 + 8·2 = 88` (`SAVR = 2.75`). The swap exchanges them exactly — `SAVR[1]` and `SAVR[2]` individually are what catch D1. The fixture also asserts `SAVR[1] + SAVR[2] == 4.75`, a **conservation check** (Class 4), not a discriminator: the sum is exchange-invariant and passes identically whether or not the swap is present, so it only documents that the pre-fix failure was an exchange rather than a scale error | `SAVR = [2.75, 2.0]` |
| F4 | corner voxel | 3³, spacing 0.5, cell at (0,0,0) | **outer-boundary faces are not counted**: `A = 3a²` not `6a²`, and `Ψ = 2(π/6)^⅓ = 1.612 > 1` — the signature of the boundary rule, commented at length in the test so it is not "corrected" | `Ψ = 1.611994` |
| F5 | feature fills the volume | 3³, all cells id 1 | degenerate divide: `A = 0`, `SAVR = 0`, `Ψ = +inf` | (same) |
| F6 | id-0 vs positive shell | 3³, `GENERATE(0, 2)` shell id | **falsifies the old doc claim** that only ids > 0 count: feature 1's `A = 6a²` for both shells. Had id 0 been skipped, the shell-0 run would give `A = 0` | `Ψ = 0.805997` |
| F7 | 2D slab | 7×3×1, spacing (1, 2, 4) | `zPoints == 1` → no `dx·dy` face ever counted; feature 1 has exactly 4 counted faces. Feature 2's 2-cell rod also discriminates D1 in the 2D path (`SAVR = 2.0` vs the swap's `2.5`) | `SAVR[2] = 2.5` |
| F8 | sphericity off | 3³ | `Sphericity` array is **not created**; `SAVR` still computed. **No legacy counterpart** (deviation D4) | n/a |

Error paths are a separate TEST_CASE with four sections: `-5355` (negative ids), `-5351` (id exceeds the Feature AM), `-12802` (NumCells not in an AttributeMatrix) and `-12803` (FeatureIds/geometry tuple mismatch — new).

*Retired oracle, and proof it was circular AND bug-carrying:* the prior test compared freshly computed `SurfaceAreaVolumeRatioNX` / `SphericityNX` against sibling arrays of the same base name inside `6_6_stats_test_v2.dream3d` — output of the code under test, i.e. a consistency-with-self oracle (`docs/vv_templates/oracle_classes.md`, "What is NOT an oracle"). Two **executed** facts settle its value (script `blind_suite.py`, output `results_blind_suite.txt`, in the V&V working folder):

1. That archive's spacing is `(0.25, 0.25, 0.25)`. D1 swaps `dy·dz` with `dx·dz`, which are **equal** at that spacing, so the test was *structurally* incapable of detecting D1 — no amount of extra assertions against that file would have found it.
2. Reconstructing `A` from the archive's own `SurfaceAreaVolumeRatio` and `NumElements` and recomputing sphericity both ways over its 619 features: the archived `Sphericity` agrees with the **truncated**-exponent formula to 1.6e-7 relative and disagrees with the exact-exponent formula by up to 4.9e-5. The archive carries D2. After the D2 fix the old test would have started failing against its own reference data.

*Mutation verification:* six mutations were applied to the fixed source, each rebuilt and run against the gate, then reverted; the MD5 of both touched files is identical before and after the sweep. Full transcript in `mutation_transcript.md` (V&V working folder). All six were killed. Kill sets matched the pre-run prediction exactly for M1, M2, M4 and M6. M3 killed a strict superset of its prediction — F1, F2, F4, F6 (shell 0) and F7 were predicted, but F1b, F2b, F3 and the sphericity-toggle test were also killed and were not called out beforehand. M5's pre-run prediction misnamed F1 as a killer (F1 in fact survived); the actual killers were F1b and F2b, with F1b unpredicted. Both misses are benign for discriminating power — a mutation killing more than predicted, never fewer, and M5 was still killed twice over — but they are prediction misses, recorded as such rather than retrofitted:

| Mutation | Killed by | Surviving fixtures, and why that is correct |
|---|---|---|
| M1 — D1 revert (face areas swapped back) | F3, F7 | every isotropic fixture — `dy·dz == dx·dz` there, so the swap is genuinely a no-op |
| M2 — outer-boundary faces counted as surface | F4, F5, F6 (shell 2), F7 | F1, F1b, F2, F2b, F3, F6 (shell 0) — their asserted features are fully interior, so no boundary face is involved |
| M3 — skip id-0 neighbors (the false doc claim) | F1, F1b, F2, F2b, F3, F4, F6 (shell 0), F7, and the sphericity-toggle test | F5 (no id-0 cell exists) and F6 (shell 2) (the shell is a positive id) |
| M4 — D2 revert (exponents re-truncated) | all nine sphericity-bearing fixtures | F5 only, whose sphericity is `+inf` under either exponent |
| M5 — volume recounted from FeatureIds instead of NumCells | F1b, F2b | every fixture whose `NumCells` happens to equal the true cell count — which is all the others |
| M6 — drop the new `-12803` preflight guard | the `-12803` error-path section | everything else; the guard rejects only invalid input |

M5 is the load-bearing case for the blind-suite question. Every fixture except F1b and F2b sets `NumCells` to the true cell count, so the suite as it stood after F1–F7 could not tell "volume from `NumCells`" from "volume from a recount". **F2b was added specifically to close that hole** — the mutation was predicted by inspection to survive the F1–F7 set (every one of those fixtures sets `NumCells` to the true cell count), and F2b was written before the sweep was ever run; F1b, added later for deviation D3, turns out to catch it too, because a recount gives its unused features a zero volume and hence `NaN` rather than `0`. The retired archive test could not have caught M5 either: **executed** check on `6_6_stats_test_v2.dream3d` — its `NumElements` array equals `bincount(FeatureIds)` exactly, for all 620 features, so a recount and the array are indistinguishable there.

*Second-engineer review:* **delegated to the PR reviewer** (requester decision, 2026-08-19). The two items most worth a reviewer's attention are the F3 area derivation (`A₁ = 64`, `A₂ = 88` — the numbers the whole D1 argument rests on) and the deliberate `Ψ > 1` expectation in F4, which looks like a bug and is not.

## Code path coverage

20 of 22 paths exercised. The filter has four logical phases: (a) preflight validation and output creation, (b) execute-time FeatureId validation, (c) the per-cell face accumulation sweep, (d) the per-feature finalize loops.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeSurfaceAreaToVolume.cpp` (177 lines) and `Filters/ComputeSurfaceAreaToVolumeFilter.cpp` (201 lines).

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) Preflight | NumCells path is not an `Int32Array` → `-12801` | *Not directly tested. The `ArraySelectionParameter` restricts the selection to `DataType::int32`, so parameter validation rejects a wrong-typed selection before `preflightImpl` runs; the branch is a defensive guard only reachable by calling `preflightImpl` directly.* |
| 2  | (a) Preflight | NumCells parent is not an `AttributeMatrix` → `-12802` | `Error Paths` — *NumCells not in an AttributeMatrix* |
| 3  | (a) Preflight | FeatureIds tuple count ≠ geometry cell count → `-12803` | `Error Paths` — *FeatureIds tuple count does not match the geometry*. Added this pass, RED-first |
| 4  | (a) Preflight | create `SurfaceAreaVolumeRatio` sized from the Feature AM shape | all tests |
| 5  | (a) Preflight | `CalculateSphericity` true → also create `Sphericity` | all except `Sphericity Toggle Off` |
| 6  | (a) Preflight | `CalculateSphericity` false → `Sphericity` not created | `Sphericity Toggle Off` (asserts the array is absent) |
| 7  | (b) Validate | negative FeatureId → `-5355`, no output written | `Error Paths` — *negative FeatureIds* |
| 8  | (b) Validate | `max(FeatureId) >= numFeatures` → `-5351` | `Error Paths` — *FeatureId exceeds the Feature AM* |
| 9  | (b) Validate | valid, including an over-provisioned Feature AM → proceed | F1b (over-provisioned), all others (exactly sized) |
| 10 | (c) Per-cell | `currentFeatureId < 1` → skip the cell entirely | F1, F6 (shell 0), F7 — most cells in these fixtures are id 0 |
| 11 | (c) Per-cell | `zIdx == 0` → skip the −Z face | F4 (corner cell at z=0), F5, F7 |
| 12 | (c) Per-cell | `zIdx == zPoints-1` → skip the +Z face | F5 (top layer), F7 (`zPoints-1 == 0`) |
| 13 | (c) Per-cell | `yIdx == 0` → skip the −Y face | F4, F5 |
| 14 | (c) Per-cell | `yIdx == yPoints-1` → skip the +Y face | F5 |
| 15 | (c) Per-cell | `xIdx == 0` → skip the −X face | F4, F5, F6 (shell 2) |
| 16 | (c) Per-cell | `xIdx == xPoints-1` → skip the +X face | F5, F6 (shell 2) |
| 17 | (c) Per-cell | in-bounds neighbor with the **same** id → no contribution | F2 (cube interior), F3 (rod interior), F5 (every neighbor) |
| 18 | (c) Per-cell | ±Z neighbor differs → add `dx·dy` | F1, F3 (8 such faces per rod) |
| 19 | (c) Per-cell | ±Y neighbor differs → add `dx·dz` | F1, F3, F7 |
| 20 | (c) Per-cell | ±X neighbor differs → add `dy·dz` | F1, F3, F7 |
| 21 | (d) Finalize | `SAVR` loop from feature 1; `Sphericity` loop from feature 1 when enabled | every fixture; F1's `NumCells[0] = 5` sentinel is what makes the "starts at 1" claim falsifiable |
| 22 | (c) Cancel / progress | `m_ShouldCancel` read once per Z slice → early `return {}`; per-Z-slice info message | *Not directly tested. The cancel path requires cancel-signal injection, which no test performs; the progress messages are emitted on every run but no test asserts them. Legacy has neither, so SIMPLNX is ahead here — not a deviation.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeSurfaceAreaToVolume` | **retired** | The prior sole correctness test. It compared freshly computed `SurfaceAreaVolumeRatioNX` / `SphericityNX` against sibling arrays inside `6_6_stats_test_v2.dream3d` produced by the code under test — a circular oracle. Executed proof that it was also incapable of doing its job: the archive's spacing is isotropic, so D1 is invisible to it, and its `Sphericity` array itself carries D2 (see *Oracle*). It also called `getDataRefAs<IDataArray>()` four times with no `REQUIRE_NOTHROW`, so a missing array surfaced as an uncaught exception rather than a test failure. Replaced wholesale. |
| `SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Class 1 - Analytical Surface Area & Sphericity` | new-for-V&V | Nine `SECTION` fixtures (F1, F1b, F2, F2b, F3, F4, F5, F6 × 2 shell ids, F7); 158 assertions. Ratio assertions are exact float32 equalities; sphericity to 1e-6 relative via `Approx().epsilon()`. Each section carries its hand derivation as a comment, and F4's `Ψ > 1` expectation carries an explicit "do not correct this" note. |
| `SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Sphericity Toggle Off` | new-for-V&V | F8. Asserts `SAVR` is still computed and `dataStructure.getDataAs<Float32Array>(sphericityPath) == nullptr`; 13 assertions. The one code path with no legacy counterpart (deviation D4). |
| `SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Error Paths` | new-for-V&V | Four sections: `-5355`, `-5351`, `-12802`, `-12803`; 15 assertions. Each asserts `invalid()` **and** the exact error code. The `-12803` section was written before the guard existed and observed to fail. |
| `SimplnxCore::ComputeSurfaceAreaToVolumeFilter: SIMPL Backwards Compatibility` | kept | Untouched. `DYNAMIC_SECTION` over `simpl_conversion/{6_5,6_4}/ComputeSurfaceAreaToVolumeFilter.json`, checking the converted geometry / FeatureIds / NumCells paths and the three name/flag arguments; 25 assertions. Not an oracle test. |

All four tests pass at 211 assertions total. **Out-of-core build runs are waived** for this cycle by requester decision 2026-08-19; the assertions that depend on storage behavior are the two index-0 zero-initialization checks in F1, which rely on `CoreDataIOManager`'s data-store factory zero-filling `float32` stores — verified by source inspection (`CoreDataIOManager.cpp`, `Float32DataStore(..., 0.0f)`) and executed only in the in-core build.

Regression scope checked after the new preflight guard: full `SimplnxCore::` suite 981/981 pass, and `PIPELINE::OrientationAnalysis` 21/21 pass (the `(03) Small IN100 Morphological Statistics` and GBCD/GBPD pipelines use this filter).

## Exemplar archive

**None.** As of this V&V cycle the filter's correctness tests are inline Class 1 / Class 4 fixtures in `test/ComputeSurfaceAreaToVolumeTest.cpp` (`namespace SavToy`); there is no `.dream3d` gold master to hash. Provenance sidecar: `src/Plugins/SimplnxCore/vv/provenance/ComputeSurfaceAreaToVolumeFilter.md`.

`6_6_stats_test_v2.tar.gz` is no longer consumed by this filter's tests. Its `download_test_data()` entry in `src/Plugins/SimplnxCore/test/CMakeLists.txt` is **deliberately left untouched** — `ComputeEuclideanDistMapTest` and `ComputeFeatureNeighborsTest` still consume it in this plugin, as do `ComputeShapesFilterTest`, `ComputeSchmidsTest` and `AlignSectionsMutualInformationTest` in OrientationAnalysis.

## Deviations from DREAM3D 6.5.171

Comparison run on the ten A/B fixtures listed in *Oracle* (the unit-test fixtures themselves, written as legacy-format `.dream3d` with matching SIMPL and NX pipelines), plus a one-off negative-id probe. Results: **10/10 SIMPLNX matches the oracle**, and **10/10 legacy matches the prediction derived from the legacy source before the run** — no unpredicted differences, so nothing entered the bug-adjudication protocol beyond the two pre-identified findings.

Where legacy and fixed SIMPLNX differ, and why:

| Fixture | `SurfaceAreaVolumeRatio` | `Sphericity` |
|---|---|---|
| F1, F2, F2b, F4, F6a, F6b | identical (D1 is a no-op at isotropic spacing) | differs, 1.405e-6 – 2.009e-5 relative (D2) |
| F5 | identical (`A = 0`) | identical (`+inf` both) |
| F3 | **differs**: legacy `[_, 2.75, 2.0]` vs `[_, 2.0, 2.75]` (D1) | differs (D1 + D2) |
| F7 | **differs** at feature 2: legacy `2.5` vs `2.0` (D1) | differs (D1 + D2) |
| F1b | legacy produces no output (`-5555`) — D3 | — |

*Alignment validation.* To prove the two fixes are exactly what separated the codebases, the same two changes were applied to a local build of the legacy source (commit `304706bae`, `BUG: FindSurfaceAreaToVolume — correct the per-face areas and the sphericity exponents`; diff preserved as `patch_6_5_172.diff` in the V&V working folder — a two-hunk change touching seven lines). Result: **9 of 9 legacy-admissible fixtures give patched-legacy == fixed-SIMPLNX == oracle**, and the patch changed the legacy output on 8 of those 9 (F5's `A = 0` / `Ψ = +inf` is invariant under both bugs). F1b is excluded because the patch deliberately leaves the `-5555` feature-count guard alone, which is deviation D3 in its own right. The patched build is internal proof tooling, not a shipping comparison target.

Deviation entries — full text in [`deviations/ComputeSurfaceAreaToVolumeFilter.md`](deviations/ComputeSurfaceAreaToVolumeFilter.md):

- `ComputeSurfaceAreaToVolumeFilter-D1` — ±X/±Y face areas swapped; anisotropic surface areas exchanged between differently oriented features. **Shared bug, fixed in SIMPLNX, alignment-patch validated.**
- `ComputeSurfaceAreaToVolumeFilter-D2` — sphericity exponents truncated to `0.333333` / `0.66666`; systematic bias up to 4.9e-5 relative on real data. **Shared bug, fixed in SIMPLNX, alignment-patch validated.**
- `ComputeSurfaceAreaToVolumeFilter-D3` — an over-provisioned Feature AttributeMatrix is accepted by SIMPLNX and rejected by legacy (`-5555`). Algorithmic choice; trust SIMPLNX.
- `ComputeSurfaceAreaToVolumeFilter-D4` — legacy never reads `CalculateSphericity` or `SphericityArrayName` from a pipeline file (and would null-dereference if the flag were honored false); SIMPLNX honors both. Legacy bug, fixed at port time; trust SIMPLNX. This is why the sphericity-off path has no A/B counterpart.
- `ComputeSurfaceAreaToVolumeFilter-D5` — negative FeatureIds: SIMPLNX errors `-5355`, legacy silently succeeds via an out-of-bounds (but, as measured, output-neutral) write. Latent legacy UB; trust SIMPLNX.

Five confirmed **non**-deviations are recorded in the same file so they are not relitigated: the outer-boundary face skip, id 0 counting as surface, volume coming from `NumCells`, index 0 never being written, and the additive progress/cancel behavior.
