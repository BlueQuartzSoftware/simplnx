# V&V Report: ComputeSchmidsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| DREAM3D 6.5.171 equivalent | `FindSchmids` (SIMPL UUID `e67ca06a-176f-58fc-a676-d6ee5553511a`) - `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindSchmids.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Pending second-engineer PR review. |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | Port with with bug fixes. See 'Dependency' section |
| Oracle (confirmed) | Class 1 (Analytical) - **12 value-asserting fixtures**: 5 auto-path loading directions, 4 exactly-representable quaternions, and 3 override-path cases. Class 4 assertions cover the physical bound `0 <= m <= 0.5`, angle-component relationships, fixed-point `Poles`, scale invariance, and cubic-symmetry invariance. The fixtures are inline and independent of archived filter output. |
| Code paths enumerated | 20 of 21 paths exercised - only the cancel path is untested. |
| Tests today | 6 inline test cases (**598 assertions**, measured): 3 Class 1 / Class 4 oracle cases, 1 options-and-guards case, 1 preflight-validation case, and 1 SIMPL backwards-compatibility case. |
| Exemplar archive | **None for this filter.** The circular `6_6_stats_test_v2.tar.gz` comparison was retired. |
| Legacy comparison | **Run — SIMPLNX vs DREAM3D 6.5.171; independently repeated during review.** One shared six-Feature input was exercised through 10 original pipeline pairs plus 3 zero-vector validation pairs. The independent rerun confirmed **291/291 expected relationships**: auto-path floats matched the exact D1 bias factors, auto-path `SlipSystems` and `Poles` were bit-identical, override-path floats agreed within `1.2e-7` relative, and D5 was traced to equivalent symmetry tables with different ordering and tied maxima. 6 deviations: **D1** truncated cubic normalizers; **D2** legacy `-301` skipped-row sentinel; **D3** undefined library outputs; **D4** mode-dependent angle units, names, and error reporting; **D5** table-relative override index; **D6** malformed-input validation. See the deviations document for root-cause detail. |
| Bug flags | **Four bugs resolved in SIMPLNX or EbsdLib:** `ComputeSchmidsFilter-D1` (truncated cubic normalizers), `ComputeSchmidsFilter-D2` (legacy `-301` skipped-row initialization), `ComputeSchmidsFilter-D3` (undefined orientation-library outputs), and `ComputeSchmidsFilter-D6` (missing malformed-input validation). |
| V&V phase | **COMPLETE** |

## Summary

`ComputeSchmidsFilter` computes, for each Feature, the largest Schmid factor over the slip systems of that Feature's crystal structure, given a sample-frame loading direction and the Feature's average orientation. Verification uses inline Class 1 analytical fixtures plus Class 4 invariants; the circular production-scale exemplar was retired. The cycle found the original four defects plus two input-validation gaps during adversarial review and documents six deviations from DREAM3D 6.5.171.

## Dependency state

- EbsdLib version 3.1.1

## Algorithm Relationship

**Port with minor algorithm improvements and updates for the SIMPLNX API.**

*Evidence:* `ComputeSchmids::operator()` preserves the legacy `FindSchmids::execute()` feature iteration, loading-direction transformation, crystal-structure dispatch, and automatic/override slip-system calculations.

Port-time updates:

1. **Higher-precision calculations.** Legacy holds the orientation matrix, crystal-frame loading direction, Schmid factor, and angle components in `float`; SIMPLNX computes them in `double` and narrows only when storing the `float32` outputs. This improves numerical accuracy without changing the selected slip system.
2. **SIMPLNX API updates and input validation.** Selection parameters validate the required arrays, output actions declare every created array, and preflight/execute guards reject mismatched tuple counts, empty arrays, invalid phase indices, and zero-length input vectors before unsafe access or normalization.
3. **Improved error messages.** SIMPLNX identifies the offending array paths, tuple counts, vector components, phase values, and valid ranges so users can correct invalid inputs directly.

**Material PRs since baseline:** none identified for this filter.

## Oracle

**Class 1 (Analytical)** with **Class 4 (Invariant)** support, and Class 3 as corroboration only.

The oracle was derived by transcribing three sources by hand — `ComputeSchmids.cpp`, EbsdLib's `Quaternion<T>::toOrientationMatrix()`, and both `CubicOps::getSchmidFactorAndSS` overloads — into closed-form expressions, evaluating them exactly, and writing the results into the test as inline expected values with the derivation in a comment above each. The derivation is reproduced independently at 80 significant digits by `ww_work/ComputeSchmids/oracle.py`, which parses EbsdLib's own 24-element symmetry table out of the source rather than re-typing it.

Identity quaternion, `Cubic_High`, auto slip-system path:

| Loading | m | cos phi (Phis) | cos lambda (Lambdas) | SlipSystems | Poles |
|---|---|---|---|---|---|
| `[0,0,1]` | `1/sqrt(6)` = 0.4082482904638630 | `1/sqrt(3)` = 0.5773502691896258 | `1/sqrt(2)` = 0.7071067811865476 | 0 | (0, 0, 100) |
| `[1,1,1]` | `2/(3 sqrt(6))` = 0.2721655269759087 | `1/3` = 0.3333333333333333 | `2/sqrt(6)` = 0.8164965809277260 | 4 | (57, 57, 57) |
| `[0,1,1]` | `1/sqrt(6)` = 0.4082482904638630 | `2/sqrt(6)` = 0.8164965809277260 | `1/2` = 0.5 | 1 | (0, 70, 70) |
| `[1,2,3]` | `8/sqrt(294)` = 0.4665694748158435 | `4/sqrt(42)` = 0.6172133998483701 | `4/sqrt(28)` = 0.7559289460184544 | 10 | (26, 53, 80) |

`[0,0,1]` and `[1,1,1]` and `[0,1,1]` all carry ties at the maximum and pin the strict-`>` tie-break to the lowest index. `[1,2,3]` has a unique maximum and pins the enumeration order. `[0,1,1]` is the row that separates `Phis` from `Lambdas` (0.8165 versus 0.5) — on the other three rows a swapped pair would go unnoticed. `[3,6,9]` repeats `[1,2,3]` to check that the loading direction is normalized.

Rotated quaternions, loading `[1,2,3]`. Each is exactly representable in `float32` and produces an orientation matrix whose entries are all `0` or `+/-1`, so there is no floating-point slack in the expected `Poles`. All four are cubic symmetry operations, so `m` and both angle components must be identical across them while `SlipSystems` and `Poles` move — a Class 4 symmetry invariance riding on the Class 1 values:

| Quaternion (x,y,z,w) | Orientation matrix | crystalLoading | SlipSystems | Poles |
|---|---|---|---|---|
| `(0,0,0,1)` identity | `I` | `(1,2,3)/sqrt(14)` | 10 | (26, 53, 80) |
| `(0,0,1,0)` 180 deg about Z | `diag(-1,-1,1)` | `(-1,-2,3)/sqrt(14)` | 8 | (-26, -53, 80) |
| `(0.5,0.5,0.5,0.5)` 120 deg about [111] | `[[0,0,1],[1,0,0],[0,1,0]]` | `(3,1,2)/sqrt(14)` | 6 | (80, 26, 53) |
| `(1,0,0,0)` 180 deg about X | `diag(1,-1,-1)` | `(1,-2,-3)/sqrt(14)` | 1 | (26, -53, -80) |

Override slip-system path (`(001)[100]`, identity quaternion) — a different EbsdLib overload with different semantics for both the index and the angle components:

| Loading | m | symmetry-operator index | Phis (radians) | Lambdas (radians) |
|---|---|---|---|---|
| `[1,2,3]` | `3/7` = 0.4285714285714286 | 3 | 0.6405223126794245 | 1.0068536854342678 |
| `[1,1,1]` | `1/3` = 0.3333333333333333 | 0 | 0.9553166181245093 | 0.9553166181245093 |
| `[0,0,1]` | 0 | 0 | 0 | 0 |

The three angle columns on this path are `acos` values, i.e. transcendental rather than surds, so they are not covered by `oracle.py`'s exact-arithmetic table. They were recomputed independently to 60 significant digits — `acos(3/sqrt(14)) = atan(sqrt(5)/3) = 0.640522312679424574143…`, `acos(2/sqrt(14)) = atan(sqrt(10)/2) = 1.006853685434267776537…`, `acos(1/sqrt(3)) = atan(sqrt(2)) = 0.955316618124509278163…` — and rounded to the nearest `double`. The asserted tolerance is `1e-6`, far looser than that rounding.

Class 4 invariants asserted alongside the Class 1 values:

- `0 <= m <= 0.5` — the physical bound for a cubic crystal. **This became assertable only after the EbsdLib fix**: EbsdLib <= 3.1.0 reached 0.500090176 at the maximizing direction. Verified over a 400 x 400 direction sweep in `oracle.py`.
- `m == Phis * Lambdas` on the auto path (the stored values are cosines) and `m == cos(Phis) * cos(Lambdas)` on the override path (the stored values are angles). The pair of assertions is what proves the SC-3 unit flip is real behaviour rather than a documentation misreading.
- `Poles[k] == trunc(100 * crystalLoading[k])`, including negative components, which is what distinguishes truncation from rounding.
- Scale invariance: `[1,2,3]` and `[3,6,9]` agree on all five outputs.
- Cubic-symmetry invariance: all four quaternions give the same `m`, `Phis` and `Lambdas`.

**Class 3 corroboration (not the oracle):** the three canonical FCC anchors fall out of the table — `m = 0.408` for `<001>` and `<011>` loading and `m = 0.272` for `<111>` loading, with `phi = 54.74 deg` / `lambda = 45 deg` and `phi = 70.53 deg` / `lambda = 35.26 deg` respectively (Dieter, *Mechanical Metallurgy* 3rd ed. section 4-3; Schmid & Boas, *Kristallplastizität*). These are quoted as a sanity check on the Class 1 derivation, not as its source.

**Encoded test references** (all greppable in `src/Plugins/OrientationAnalysis/test/ComputeSchmidsTest.cpp`):

- `"OrientationAnalysis::ComputeSchmidsFilter: Class 1 analytical oracle, auto slip system"`
- `"OrientationAnalysis::ComputeSchmidsFilter: orientation-matrix convention and equivariance"`
- `"OrientationAnalysis::ComputeSchmidsFilter: override slip system path"`
- `"OrientationAnalysis::ComputeSchmidsFilter: options, skip path and phase guards"`

## Bugs found and fixed

This branch fixes all defects in this table. The fixes will be in the DREAM3D-NX release after v7.4.1.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|---|---|---|---|
| `ComputeSchmidsFilter-D1` | The cubic calculation used truncated normalizers. The calculated Schmid factor could exceed the physical maximum of 0.5. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.1. | EbsdLib 3.1.1 uses full-precision constants. |
| `ComputeSchmidsFilter-D2` | DREAM.3D initialized uncomputed angle values to `-301`. The value was an error code that was used as an initialization value. | DREAM.3D 6.5.171 only. DREAM3D-NX was not affected. | SIMPLNX initializes all output arrays to zero. The algorithm also initializes Feature 0. |
| `ComputeSchmidsFilter-D3` | The orientation library did not initialize all output values. The filter could reuse values from the previous Feature. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.1. | EbsdLib 3.1.1 initializes all output values. The algorithm also initializes its output variables for each Feature. |
| `ComputeSchmidsFilter-D6` | The filter accepted invalid phase values, empty arrays, arrays with different tuple counts, and zero direction vectors. These inputs could cause invalid output or invalid memory access. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.1. | The algorithm rejects invalid phase values. Preflight rejects empty arrays, arrays with different tuple counts, and zero direction vectors. |

## Code path coverage

`Source: src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeSchmids.cpp (139 lines).`

The algorithm is flat: a parameter setup block, a sentinel-row write, then one pass over Features.

20 of 21 paths exercised.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | *Preflight* | Reject unequal Feature-array tuple counts (`-13503`) | "preflight input validation" / "Feature Phases and Average Quaternions must have equal tuple counts" |
| 2 | *Preflight* | Reject empty Feature arrays (`-13504`) | "preflight input validation" / "Feature arrays must contain the sentinel tuple" |
| 3 | *Preflight* | Reject an empty Crystal Structures array (`-13505`) | "preflight input validation" / "Crystal Structures must contain at least one ensemble" |
| 4 | *Preflight* | Reject a zero Loading Direction (`-13506`) | "preflight input validation" / "Loading Direction must be non-zero" |
| 5 | *Preflight* | Create the three unconditional output arrays with a zero fill | all executing V&V cases |
| 6 | *Preflight* | Create `Phis`/`Lambdas` when `StoreAngleComponents` is true | "Class 1 analytical oracle, auto slip system" |
| 7 | *Preflight* | Skip `Phis`/`Lambdas` when `StoreAngleComponents` is false | "options, skip path and phase guards" / "StoreAngleComponents == false does not create Phis/Lambdas" |
| 8 | *Preflight* | Reject a zero override Slip Plane (`-13507`) | "preflight input validation" / "Override Slip Plane must be non-zero" |
| 9 | *Preflight* | Reject a zero override Slip Direction (`-13508`) | "preflight input validation" / "Override Slip Direction must be non-zero" |
| 10 | *Preflight* | Reject a slip direction not lying in the slip plane (`-13500`) | "override slip system path" / "preflight rejects a slip direction that is not in the slip plane" |
| 11 | *Execute* | Zero the Feature-0 sentinel row | "Class 1 analytical oracle, auto slip system" |
| 12 | *Execute* | Normalize the loading direction | "Class 1 analytical oracle, auto slip system" / loading `[3,6,9]` |
| 13 | *Execute* | Normalize the user slip plane and direction (`OverrideSystem` true) | "override slip system path" |
| 14 | *Execute - per-Feature* | Negative phase id guard (`-13502`) | "options, skip path and phase guards" / "a negative phase id is an error" |
| 15 | *Execute - per-Feature* | Out-of-range phase id guard (`-13501`) | "options, skip path and phase guards" / "a phase id beyond the ensemble count is an error" |
| 16 | *Execute - per-Feature* | Skip a Feature whose Laue class is at or beyond `LaueGroupEnd` | "options, skip path and phase guards" / "Features whose Laue class is beyond LaueGroupEnd get defined zeros" |
| 17 | *Execute - per-Feature* | Valid Laue class with no enumerated slip systems | "options, skip path and phase guards" / "Valid Laue classes without slip systems keep zero Schmid outputs and computed Poles" |
| 18 | *Execute - per-Feature* | Auto slip-system path (`getSchmidFactorAndSS(load, ...)`) | "Class 1 analytical oracle, auto slip system"; "orientation-matrix convention and equivariance" |
| 19 | *Execute - per-Feature* | Override slip-system path (`getSchmidFactorAndSS(load, plane, direction, ...)`) | "override slip system path" |
| 20 | *Execute - per-Feature* | Store or omit angle components | auto, override, and StoreAngleComponents=false sections |
| 21 | *Execute - per-Feature* | `m_ShouldCancel` early return | *Not directly tested. No cancel-signal injection infrastructure exists for algorithm classes; same gap as the rest of this plugin.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `"OrientationAnalysis::ComputeSchmidsFilter"` | retired | Removed during adversarial review because `6_6_stats_test_v2.tar.gz` stores prior ComputeSchmids output and is a circular oracle. Its useful algorithmic behavior is covered by the inline Class 1 and Class 4 fixtures. |
| `"OrientationAnalysis::ComputeSchmidsFilter: Class 1 analytical oracle, auto slip system"` | new-for-V&V | 5 `DYNAMIC_SECTION` loadings x (5 outputs + Class 4 invariants + the Feature-0 sentinel row). 236 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: orientation-matrix convention and equivariance"` | new-for-V&V | 4 `DYNAMIC_SECTION` quaternions pinning the `qu2om` convention, the no-transpose property, negative `Poles` truncation and cubic-symmetry invariance. 109 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: override slip system path"` | new-for-V&V | 3 `DYNAMIC_SECTION` loadings on the second EbsdLib overload, including the degenerate zero-Schmid case, plus the radians-versus-cosines assertion and the `-13500` preflight rejection. 93 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: options, skip path and phase guards"` | new-for-V&V | `StoreAngleComponents == false`; a 20 000-Feature `LaueGroupEnd` skip fixture; a valid no-slip-system Laue class; and the `-13501`/`-13502` phase-id guards. 108 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: preflight input validation"` | new-for-V&V | Mismatched/empty Feature arrays, empty Crystal Structures, and zero loading/override vectors (`-13503` through `-13508`). 19 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: SIMPL Backwards Compatibility"` | kept, untouched | 2 `DYNAMIC_SECTION` conversion fixtures (SIMPL 6.5 UUID, SIMPL 6.4 Filter_Name). 33 assertions. |

**598 assertions total** (236 + 109 + 93 + 108 + 19 + 33), taken from a `ctest --verbose` run rather than tallied by hand. All 6 pass on the reviewed branch.

Full `OrientationAnalysis::` regression after the internal-review fixes: **268 of 268 unit tests passed** in the EbsdLib 3.1.1 build.

## Test sensitivity verification

Test sensitivity verification introduces one temporary defect at a time and confirms that the applicable test fails.

Seven temporary defects were evaluated. Each defect caused the expected V&V test to fail.

| Temporary defect | Test that detected the defect | Result |
|---|---|---|
| Transpose the orientation matrix before rotating the loading direction. | Orientation-matrix convention and equivariance | Detected |
| Swap quaternion components X and Y when the filter reads the quaternion. | Orientation-matrix convention and equivariance | Detected |
| Round the `Poles` values instead of truncating them. | Class 1 analytical oracle, orientation-matrix convention, override path, and options and guards | Detected |
| Change the EbsdLib cubic slip-system tie-break from `>` to `>=`. | Class 1 analytical oracle | Detected |
| Remove both phase-index guards. | Options, skip path, and phase guards | Detected |
| Disable the Feature-array tuple-count validation. | Preflight input validation | Detected |
| Disable all three zero-vector guards. | Preflight input validation | Detected |

## Exemplar archive

The existing test archive contained output from an earlier run of this filter. It was a circular oracle and was retired for this filter. The replacement tests use inline Class 1 analytical data and Class 4 invariants.

## Deviations from DREAM.3D 6.5.171

See `vv/deviations/ComputeSchmidsFilter.md` for the root cause, affected users, and recommendation for each deviation.

| Deviation | Observed difference |
|---|---|
| `ComputeSchmidsFilter-D1` | DREAM.3D 6.5.171 and DREAM3D-NX through v7.4.1 used truncated cubic normalizers. The calculated Schmid factor and angle components were too large. |
| `ComputeSchmidsFilter-D2` | DREAM.3D 6.5.171 initialized uncomputed angle values to `-301`. DREAM3D-NX initializes these values to zero. |
| `ComputeSchmidsFilter-D3` | The orientation libraries did not initialize all Schmid output values. The filter could return undefined or previous Feature values. |
| `ComputeSchmidsFilter-D4` | The angle outputs use cosines on the automatic path and radians on the override path. The default output names and error messages also differ. |
| `ComputeSchmidsFilter-D5` | The override path returns an index from the library symmetry table. Different table orders can return different indices for the same physical result. |
| `ComputeSchmidsFilter-D6` | SIMPLNX rejects invalid phase values, malformed arrays, and zero direction vectors. DREAM.3D 6.5.171 accepted these inputs and could produce invalid output or invalid memory access. |
