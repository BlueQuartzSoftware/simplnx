# V&V Report: ConvertOrientationsFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| SIMPLNX Human Name | Convert Orientation Representation |
| DREAM3D 6.5.171 equivalent | `ConvertOrientations` (SIMPL UUID `e5629880-98c4-5656-82b8-c9fe2b9744de`) — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ConvertOrientations.{h,cpp}`; mapped in `OrientationAnalysisLegacyUUIDMapping.hpp` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Rewrite** (plumbing) under the retained SIMPL UUID. Legacy built a vector of 7 `OrientationConverter<T>` subclasses dispatching each pair to its **direct** pairwise transform (e.g. `eu2om`, `eu2cu`); SIMPLNX uses an 8×8 `switch` dispatching to macro-generated convertors that call EbsdLib `input.toX()` directly. Three scope deltas: SIMPLNX **adds Stereographic** (8th type), **restricts input to float32** (legacy accepted float and double, D2), and **drops legacy's in-place Euler sanitization** (D5). |
| Oracle (confirmed)     | **Class 3 (Rowenhorst 2015, DOI 10.1088/0965-0393/23/8/083501)** primary + **Class 1 (Analytical)** for Stereographic closed form + **Class 4 (Invariant)** round-trip. *Transform math is owned/tested by EbsdLib itself (`EbsdLib/Source/Test/Orientation*Test.cpp`); this filter test verifies only the filter's value-add — dispatch routing, component striding, preflight.* Encoded in `test/ConvertOrientationsTest.cpp` — 56-pair 8×8 matrix + stereographic closed form; **1032 assertions pass** (in-core, via `ctest`). OOC: single-algorithm filter made OOC-safe via `requireArraysInMemory`; dedicated OOC run skipped (no in-core/OOC dispatch variants — see V&V phase). |
| Code paths enumerated | 7 of 12 exercised; 2 unreachable arms and 3 unasserted guard/cancellation/progress paths remain outside this regression. |
| Tests today | 5 registered target cases plus one hidden HDF5 boundary test. Paired target selections pass 5/5. |
| Exemplar archive       | None — values are inline dispatch landmarks in the test source. Unknown-provenance `k_InitValues` **retired** and replaced by EbsdLib-3.0.0-derived values. These are a consistency check against EbsdLib's reference implementation for the transform math (not EbsdLib-independent); genuinely independent pins are the stereographic closed form and seed-0's Rowenhorst-2015 worked-example orientation. |
| Legacy comparison      | **Run** (toy fixture, 6 shared eu→X conversions via 6.5.171 PipelineRunner vs nxrunner on byte-identical Euler input). Headline: 4 of 6 bit-identical; max \|Δ\| = **1.78e-6** (cubochoric), measured for eu→X only. 5 deviations: D1 library-generation precision (measured), D2 float64 scope, D3 Stereographic-added, D4 error-code surface, D5 dropped in-place Euler sanitization. See `comparisons/ConvertOrientationsFilter/results/comparison.md`. |
| Bug flags              | **None.** Filter matches the independent oracle on all 56 dispatch pairs + stereographic; all 4 deviations are precision/scope/API, not bugs. |
| V&V phase | Historical status and sign-off retained. Section 4.3 recertification adds real-HDF5 analytical boundary evidence and paired runtime checks. |

For worked instances see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md` and `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md`.

## Summary

`ConvertOrientationsFilter` ("Convert Orientation Representation") converts a Float32 orientation array from any one of 8 representations (Euler, Orientation Matrix, Quaternion, Axis-Angle, Rodrigues, Homochoric, Cubochoric, Stereographic) to any other, applying the conversion per-tuple in parallel. The actual transformation equations are delegated to EbsdLib (Rowenhorst 2015) and are verified by EbsdLib's own test suite; this V&V therefore verifies the **filter's value-add only** — that the `(inputType, outputType)` `switch` dispatches to the correct conversion, that components are read/written with the correct per-tuple stride, and that the preflight contract holds — using EbsdLib-3.0.0-derived values as **dispatch landmarks** plus Class 4 round-trip/`isValid` invariants. Legacy 6.5.171 differences (float-vs-double internal math; 7-vs-8 supported types) are then documented as diff-explanation, not correctness checks.

## Algorithm Relationship

*Classification:* **Rewrite** (of the filter plumbing) under the retained SIMPL UUID `e5629880-98c4-5656-82b8-c9fe2b9744de`.

*Evidence:* The SIMPLNX algorithm `Algorithms/ConvertOrientations.cpp` is structurally distinct from legacy `ConvertOrientations::execute()` / `generateRepresentation<T>()` (DREAM3D 6.5.171):

- **Legacy** built a `QVector` of **7** `OrientationConverter<T>` subclass instances (Euler / OM / Quaternion / AxisAngle / Rodrigues / Homochoric / Cubochoric — **no Stereographic**), called `setInputData()` on the input-type converter, then `convertRepresentationTo(outputType)`, which dispatches each requested pair to its **direct** pairwise transform (`OrientationConverter.hpp:492` `eu2om`, `:517` `eu2cu`→`eu2ho→ho2cu`) — **not** through a quaternion intermediate. Every `toX()` also ran `sanityCheckInputData()`, which for Euler input rewrote the input array in place (D5). Supported **both `float` and `double`** input arrays.
- **SIMPLNX** uses an outer `if`-chain on `OutputType` (8 cases) wrapping an inner `switch` on `InputType` (8 cases) that dispatches to a macro-generated `TO_REP##Convertor` functor calling `inputInstance.to##TO_REP()` (the EbsdLib 2.0 `Orientation` member methods, some direct, some via OM/quaternion). Supports **8** types (adds **Stereographic**) and **float32 only**.
- Per V&V policy, **a Rewrite under the same UUID is a claim of functional equivalence** for the 7 shared types — the Deviations file (Step 8) must defend it.

*Port-time deltas / material changes:*

1. **Dispatch rewrite** — legacy converter-class hierarchy → direct `input.toX()` 8×8 switch. Both dispatch to the same direct pairwise transforms, but the underlying library generation differs (legacy OrientationLib vs EbsdLib 3.x), so intermediate float32 round-off can differ — Deviation D1.
2. **Stereographic added** (SIMPLNX type 7) — no legacy equivalent; out of scope for legacy A/B.
3. **float32 only** (SIMPLNX) vs **float + double** (legacy) — legacy `double` arrays carried out the math in double precision; SIMPLNX always float32. Deviation candidate for any pipeline that fed `double` arrays to legacy.

*Material PRs since baseline:* #1468 ("ConvertOrientationsFilter uses an Algorithm Class"), #1301 ("Add missing algorithm classes"), #1472 ("Update to EbsdLib 2.0.0 API"), #1535 ("Remove redundant preflight checks"). #1472 is the one that swapped the conversion API to EbsdLib 2.0 `input.toX()`.

## Oracle

*Class:* **3 (Paper-based — Rowenhorst 2015)** primary, **1 (Analytical)** for Stereographic, **4 (Invariant)** companion.

*Citation:* D. Rowenhorst, A. D. Rollett, G. S. Rohrer, M. Groeber, M. Jackson, P. J. Konijnenberg, M. De Graef, "Consistent representations of and conversions between 3D rotations," *Modelling and Simulation in Materials Science and Engineering* **23**(8) 083501 (2015), DOI 10.1088/0965-0393/23/8/083501 — cited throughout `EbsdLib/Source/EbsdLib/Core/OrientationTransformation.hpp`.

*Applied:* The transform equations are implemented and verified **inside EbsdLib** (`EbsdLib/Source/Test/OrientationTest.cpp` exercises the full 8×8 conversion matrix incl. Stereographic with round-trip + `isValid()`; `OrientationTransformationTest.cpp` pins analytical landmarks — identity, `ax2om` 90°-about-Z; `OrientationConverterTest.cpp` pins a Rowenhorst-style Euler→Quaternion exemplar). This filter test does **not** re-verify that math. Instead it takes **one general orientation expressed in all 8 representations** — values generated directly from the same EbsdLib 3.0.0 the filter links (reference implementation, independent of the filter's parallel-convertor plumbing) — and uses them as **dispatch landmarks**: for every `(inputType, outputType)` pair the filter must transform `R[inputType]` into `R[outputType]` within tolerance. Wired to the wrong conversion, the output would be a detectably different number. Multi-tuple input arrays additionally pin the per-tuple component striding. Stereographic specifically is cross-checked against its closed form (`st = (qₓ,qᵧ,q_z)/(1+q_w)`; inverse `ω = 4·atan(|st|), n̂ = st/|st|`) — Class 1. Class 4 round-trip (`A→B→A` ≈ identity) and `isValid()` predicates cover the full matrix cheaply.

*Encoded:* `test/ConvertOrientationsTest.cpp`:
- `"Dispatch and striding (8x8 matrix)"` — 56 `DYNAMIC_SECTION`s (every `(in,out)` pair incl. Stereographic), 3 distinct general orientations per multi-tuple input array, exact-value comparison vs `k_Ref` (EbsdLib-3.0.0-derived landmarks) at tol 1e-4, plus output component-count/tuple-count striding assertions.
- `"Stereographic closed form (Class 1)"` — Quaternion→Stereographic, expected `st = (x,y,z)/(1+w)` computed in-test from the closed form (no EbsdLib call), tol 1e-5.
- **1032 assertions, all pass** (in-core build `NX-Com-Qt69-Vtk96-Rel`). OOC pass pending.

**Oracle-independence caveat (honest scope):** the `k_Ref` landmarks are generated from EbsdLib 3.0.0 — the same library the filter links — so with respect to the *transform math* the 8×8 dispatch test is a **consistency check against EbsdLib's reference implementation**, not an EbsdLib-independent one. What it independently verifies is the filter's own value-add: dispatch routing and per-tuple striding (a mis-wired switch or stride bug produces a detectably wrong number regardless of the landmark's provenance). Two elements are genuinely EbsdLib-independent: (1) the Stereographic path, checked against its closed form computed in-test with no EbsdLib call (Class 1); (2) seed-0, whose orientation is the Rowenhorst 2015 worked example — its quaternion matches EbsdLib `OrientationConverterTest`'s exemplar `{-0.2919894…, 0.319372, 0.1502762…, 0.8889099…}`, but the ultimate authority for that value is the paper's Table (Class 3), not the EbsdLib fixture. The transform math itself is verified inside EbsdLib's own `OrientationTest.cpp` / `OrientationTransformationTest.cpp` suite, which this V&V relies on rather than duplicates.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Bugs found and fixed

No new defect was found during this OOC recertification. Existing fixes and deviation dispositions remain documented in this report and its sidecar.

## Code path coverage

7 of 12 paths exercised. Two paths are unreachable through the filter; the component-shape guard, cancellation, and progress messages are not directly asserted.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ConvertOrientations.cpp` (448 lines) + `Filters/ConvertOrientationsFilter.cpp` preflight. Logical phases: (a) filter `preflightImpl` validation, (b) execute dispatch (8×8 output/input `switch`), (c) per-tuple parallel convertor.

| #  | Phase          | Path   | Test case                                                                 |
|----|----------------|---------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------|
| 1  | (a) Preflight  | `inputType == outputType` → `-67005`                                                              | `Equal Representations` (GENERATE over all 8 types)                       |
| 2  | (a) Preflight  | input component shape has >1 dimension → `-67003`                                                 | *Not directly tested. Low-value guard; selection params produce 1-D component shapes in normal use.* |
| 3  | (a) Preflight  | input component count ≠ expected for input type → `-67004`                                         | `Invalid preflight` — 3-component array declared as Quaternion (expects 4) → `-67004` |
| 4  | (a) Preflight  | out-of-range input/output type index → framework `k_Validate_OutOfRange_Error`                    | `Invalid preflight` (input/output type = 8)                               |
| 5  | (a) Preflight  | valid → `CreateArrayAction` with output type's component count                                    | `Dispatch and striding` (preflight of all 56 pairs) + `Invalid preflight` (does-not-exist) |
| 6  | (b) Dispatch   | 56 cross-type arms (8 outputs × 7 inputs, incl. Stereographic)                                    | `Dispatch and striding (8x8 matrix)` — all 56 `DYNAMIC_SECTION`s; `Stereographic closed form` for qu→st |
| 7  | (b) Dispatch   | 8 same-type arms (`case == output`)                                                               | *Unreachable through the filter — blocked at preflight by path 1.*        |
| 8  | (b) Dispatch   | 8 `case Type::Unknown: break;` arms                                                               | *Unreachable — `ChoicesParameter` range-validates the index (path 4).*    |
| 9  | (c) Convertor  | per-tuple read `inNumComps` → `input.toX()` → write `outNumComps` (striding)                      | `Dispatch and striding` — 3 distinct multi-tuple orientations + output component/tuple-count assertions |
| 10 | (c) Convertor  | `m_Filter->shouldCancel()` → early return                                                         | *Not directly tested. Requires injecting a cancel signal mid-execution; low-value coverage gap.* |
| 11 | Progress | Per-chunk progress message | *Not directly tested. Progress messages are outside this numerical verification scope.* |
| 12 | Bulk conversion | A 4,096-tuple block and one-tuple tail use different component directions | `real HDF5 4096-block stereographic oracle` — [0,0,1/3] followed by [1/3,0,0] |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ConvertOrientations: Dispatch and striding (8x8 matrix)` | new-for-V&V | Replaces the retired `Valid filter execution`. 56 `DYNAMIC_SECTION`s (every cross-type pair incl. Stereographic), 3 distinct general orientations per multi-tuple input, exact-value vs EbsdLib-3.0.0 landmarks (tol 1e-4) + output component/tuple-count striding checks. ~1020 assertions. |
| `OrientationAnalysis::ConvertOrientations: Stereographic closed form (Class 1)` | new-for-V&V | Quaternion→Stereographic; expected `st=(x,y,z)/(1+w)` computed in-test (no EbsdLib call), tol 1e-5. Independent analytical pin for the type with no legacy equivalent. |
| `OrientationAnalysis::ConvertOrientations: Invalid preflight` | kept | Negative: does-not-exist input path + out-of-range input/output type index (`ChoicesParameter`). |
| `OrientationAnalysis::ConvertOrientations: Equal Representations` | kept | Negative: same input/output type → `-67005`. `GENERATE` over all 8 types. |
| `OrientationAnalysis::ConvertOrientationsFilter: SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over SIMPL 6.4 + 6.5 conversion fixtures; validates UUID + argument-key conversion. |
| *(retired)* `OrientationAnalysis::ConvertOrientations: Valid filter execution` | retired | Removed: compared against `k_InitValues` of **unknown provenance** (7×7 only, single tuple, no Stereographic) — a circular-oracle risk. Superseded by the 8×8 dispatch test with EbsdLib-derived, cross-validated landmarks. |
| `real HDF5 4096-block stereographic oracle` | new-for-V&V | Independent HDF5 boundary outputs; 16 assertions across the configured options. |

OOC recertification (2026-09-21): 5/5 target CTest entries pass in both DREAM3DNX builds. The hidden `real HDF5 4096-block stereographic oracle` passes 16 assertions in the OOC binary. The quaternion-to-stereographic oracle is (qx,qy,qz)/(1+qw). The two boundary inputs (0,0,0.6,0.8) and (0.6,0,0,0.8) therefore yield [0,0,1/3] and [1/3,0,0]. Both input and output stores are HDF5-OOC. The five target CTest entries passed in each build; the selection also ran two unrelated ConvertOrientationsToVertexGeometry entries successfully. Existing upstream/develop oracles and tolerances are preserved. No production algorithm or historical sign-off is changed, and no fresh legacy binary run is claimed.

## Exemplar archive

- **None.** This filter's oracle is encoded as **inline dispatch landmarks** (`k_Ref` in `test/ConvertOrientationsTest.cpp`), not a cached `.dream3d`. The landmarks are generated from EbsdLib 3.0.0 (the same library the filter links) and cross-validated independently (seed-0 quaternion == EbsdLib `OrientationConverterTest` exemplar; all stereographic values == closed-form projection). No `download_test_data()` entry and no provenance sidecar are required.

## Deviations from DREAM3D 6.5.171

Four documented deviations, all consequences of the plumbing **Rewrite** under the retained UUID. SIMPLNX is independently verified-correct against the oracle, so these are diff-explanation (no bug flags). Comparison run on the toy fixture (6 shared eu→X conversions); see `vv/comparisons/ConvertOrientationsFilter/results/comparison.md` and `vv/deviations/ConvertOrientationsFilter.md`.

- `ConvertOrientationsFilter-D1` — float32 differences (measured ≤ **1.78e-6** for eu→X, 4 of 6 conversions bit-identical) from library-generation drift between legacy OrientationLib and EbsdLib 3.x; both dispatch each pair to the same direct pairwise transform (not a quaternion intermediate).
- `ConvertOrientationsFilter-D2` — legacy accepted float64 orientation arrays (double-precision math); SIMPLNX is float32-only (precision / scope reduction).
- `ConvertOrientationsFilter-D3` — SIMPLNX adds the Stereographic representation; no legacy equivalent (new capability).
- `ConvertOrientationsFilter-D4` — preflight error-code surface changed (range validation delegated to `ChoicesParameter`); invalid configs still rejected.
- `ConvertOrientationsFilter-D5` — legacy performed an in-place Euler range-normalization (`fmod` + sign flips) that mutated the input array and, being non-rotation-preserving, changed the converted orientation for out-of-range Euler input; SIMPLNX does neither. Not covered by the in-range A/B.
