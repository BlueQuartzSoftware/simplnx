# V&V Report: RotateEulerRefFrameFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID | `0458edcd-3655-4465-adc8-b036d76138b5` |
| SIMPLNX Human Name | Rotate Euler Reference Frame |
| DREAM3D 6.5.171 equivalent | `RotateEulerRefFrame` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/RotateEulerRefFrame.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** of legacy `RotateEulerRefFrame::execute()`. Same per-tuple kernel `gNew = normalize_cols(eu2om(euler) * ax2om(axis, angle))` → `om2eu`; library swaps only (OrientationLib → EbsdLib, hand-rolled `MatrixMath` → Eigen) plus a double-precision degree→radian conversion and progress/cancel plumbing. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — 8 hand/script-derived fixtures (`AnalyticalFixtures::k_Fixtures`) with closed-form derivations for Z-axis (`phi1' = phi1 - w mod 2pi`), identity + X/111 axes, normalization, and zero-angle cases. **Class 4 (Invariant) companion** — output-range bounds, (n,w)/(-n,w) round-trip, 45°+45° = 90° composability. Cross-checked by an independent numpy script (Rowenhorst 2015 Eq. A.5/A.9 + first-principles frame-rotation derivation). All pass. |
| Code paths enumerated  | 7 enumerated; **5 exercised**. The 2 gaps are both cancel branches (cancel-before-start and mid-loop cancel) — only their false path ever runs; taking the true path requires cancel-signal injection (same accepted gap as prior V&V reports). |
| Tests today            | **5 TEST_CASEs / 13 ctest sections** — 8 Class 1 fixtures (DYNAMIC_SECTION), 3 Class 4 invariant sections, 1 zero-axis preflight-error test (new guard added this cycle), 1 legacy-parity 480k-tuple regression pin (ASCIIData), 1 SIMPL 6.4/6.5 backwards-compat. |
| Exemplar archive       | `ASCIIData.tar.gz` (pre-existing, shared archive) — provides the 480k-tuple legacy-parity input/comparison CSVs only. **Not an oracle** (legacy-DREAM3D provenance); the Class 1 oracle is inline in the test source. No new archive needed. |
| Legacy comparison      | **Run** against DREAM3D 6.5.171 on 6 axis/angle cases × 12 orientations (shared CSV input). Max wrap-aware diff 7.2e-7 rad (float32 ULP level). Both implementations independently match the numpy oracle (NX 2.3e-7, legacy 8.1e-7). No deviations; two non-deviations documented. |
| Bug flags              | None affecting output. Two robustness/policy items addressed: (1) zero-length rotation axis previously produced silent NaN corruption — preflight now rejects it (`-96200`) and the Algorithm class guards it as well (`-67050`); (2) the parallel kernel writes the in-place Euler array via `operator[]` from TBB workers — per the project thread-safety policy this is now gated with `requireArraysInMemory` so parallelization is only enabled for in-core stores (the codebase-sanctioned pattern). Legacy 6.5.171 retains the zero-axis NaN behavior (documented as a non-deviation — not output-correctness). |
| V&V phase              | Oracle design + reconciliation, algorithm review (fixes applied), code-path coverage, test inventory, legacy comparison, deviations, and provenance complete. **V&V complete and signed off by Michael Jackson (technical authority) 2026-07-16.** **Outstanding:** dual-build (OOC) run deferred — no OOC-specific variant of this algorithm and no OOC build configured in Workspace4. |

## Summary

`RotateEulerRefFrameFilter` performs a passive rotation of the sample reference frame by a user-supplied axis-angle pair (degrees), rewriting a float32 3-component Bunge ZXZ Euler-angle array in place via `gNew = normalize_cols(eu2om(euler) · ax2om(axis, w))` → `om2eu`. Verification used a **Class 1 (Analytical) oracle** — 8 fixtures with closed-form expected outputs derived from first principles (g' = g·R_active(n,w)) and Rowenhorst 2015 conversion equations, validated by an independent numpy script — plus **Class 4 invariants** (canonical output ranges, inverse-rotation round-trip, angle composability). SIMPLNX matched the oracle on every fixture with zero discrepancies; the 6.5.171 A/B comparison across 6 axis/angle cases shows ULP-level agreement and no deviations.

## Algorithm Relationship

*Classification:* **Port**

*Evidence:* Near line-by-line translation of legacy `RotateEulerRefFrame::execute()` + `RotateEulerRefFrameImpl::convert()` (SIMPL UUID `{ef9420b2-8c46-55f3-8ae4-f53790639de4}` retained in the legacy-UUID map; SIMPL 6.4/6.5 conversion fixtures at `test/simpl_conversion/6_*/RotateEulerRefFrameFilter.json`). Identical control flow: normalize axis → per-tuple eu2om, multiply by ax2om rotation matrix, column-normalize, om2eu, write back in place, parallel over tuples (now gated with `requireArraysInMemory` per the project thread-safety policy — a non-functional change from legacy).

*Port-time deltas:*

1. **Orientation library**: OrientationLib `DOrientTransformsType::{ax2om, eu2om, om2eu}` → EbsdLib `ebsdlib::{AxisAngleDType, EulerDType, OrientationMatrixDType}`. EbsdLib is the direct descendant of OrientationLib; same Rowenhorst-convention equations (`epsijk = +1`). No output change (confirmed by A/B).
2. **Matrix math**: hand-rolled `MatrixMath::Multiply3x3with3x3D` + `Normalize3x3D` → Eigen row-major multiply + `colwise().normalized()`. Semantically identical column-wise normalization; legacy's per-entry `>1` clamp is unreachable except through rounding. No output change.
3. **Degree→radian conversion precision**: legacy computes `float rotAngle = angle * pi / 180.0` in **float** before the double-precision transforms; SIMPLNX converts in **double**. ULP-level output difference only (see non-deviation N1).
4. **Progress reporting + cancel checking**: legacy has neither; SIMPLNX adds throttled progress feedback and per-element `m_ShouldCancel` checks. UX-only.
5. **Zero-axis preflight guard (SIMPLNX addition)**: preflight error `-96200` for a zero-length rotation axis, which previously NaN-corrupted the array silently in both codebases. Behavior change only for invalid input.

*Material PRs since baseline:* none identified for this filter beyond routine EbsdLib version bumps and the progress-messaging framework migration.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. Class 3 (Paper-based) partially inherent — the eu2om/om2eu/ax2om conversions follow Rowenhorst et al 2015 (doi:10.1088/0965-0393/23/8/083501) Eq. A.5/A.9, but those conversions are EbsdLib's own V&V responsibility; this filter's verifiable claim is the composition `g' = g · R_active(n, w)`.

*Applied:* Expected outputs derived independently of all three codebases (SIMPLNX, EbsdLib, legacy). First-principles derivation: rotating the sample reference frame by +w (right-hand rule) about unit axis n gives new-frame coordinates `u_new = P(w)·u_old` (P = passive matrix), hence `g' = g·P(w)ᵀ = g·R_active(n,w)` with R_active the Rodrigues rotation matrix. Closed forms follow: Z-axis rotation ⇒ `phi1' = phi1 − w (mod 2π)`, Φ/phi2 unchanged; identity orientation ⇒ `g' = R_active(n,w)`, om2eu by hand (fixtures F1–F7 each carry the derivation as a source comment). A pure-numpy script implementing the derivation + Rowenhorst equations (numpy 2.0.2; no random seed — all fixtures deterministic) validates every closed form and generates the one non-closed-form fixture (F8). Script archived in the verification working folder and reproduced in the provenance sidecar.

*Encoded:* `test/RotateEulerRefFrameTest.cpp::"OrientationAnalysis::RotateEulerRefFrameFilter: Class 1 Analytical Fixtures"` — 8 fixtures (`AnalyticalFixtures::k_Fixtures`), tolerance 1e-5 rad, all pass. Invariants: `...::"OrientationAnalysis::RotateEulerRefFrameFilter: Class 4 Invariants"` — 3 sections (range bounds, (n,w)/(−n,w) round-trip, 45°+45°=90° composability) over a 6-orientation batch, all pass.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Code path coverage

*5 of 7 paths exercised. The 2 gaps (paths 1 and 6) are both cancel branches whose true path is never taken without cancel-signal injection.*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.cpp` (~130 lines).

The algorithm is flat: (a) entry/setup in `operator()` (cancel check, axis normalization, parallel dispatch), (b) per-tuple kernel in `RotateEulerRefFrameImpl::convert`.

| # | Stage | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Entry | Cancel-before-start → early return | *Not directly tested. Requires cancel-signal injection; two-line guard, same accepted gap as prior V&V reports.* |
| 2 | (a) Entry | Axis normalization (`axis.normalize()`) | `Class 1 Analytical Fixtures` — F5 (axis (2,0,0) ≡ (1,0,0)) |
| 3 | (a) Entry | Zero-length axis → preflight error `-96200` (SIMPLNX addition) | `Zero-Length Axis Fails Preflight` |
| 4 | (b) Kernel | Per-tuple rotate: eu2om → multiply → column-normalize → om2eu → write-back | All Class 1 fixtures + Class 4 sections + legacy-parity ASCIIData test |
| 5 | (b) Kernel | om2eu gimbal-degenerate branch (`|g22| ≈ 1`) | `Class 1 Analytical Fixtures` — F1, F6 (Φ = 0 outputs) |
| 6 | (b) Kernel | Mid-loop cancel check → early return | *Not directly tested — only the false branch runs; the true (cancel) path needs cancel-signal injection (see path 1).* |
| 7 | (b) Kernel | Progress messaging (`counter >= counterIncrement`, incl. `counterIncrement == 0` for ranges < 100) | Exercised implicitly by every test (1-tuple fixtures take the `counterIncrement == 0` branch; the 480k-tuple ASCIIData test takes the batched branch). |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::RotateEulerRefFrame` | kept — **reclassified as legacy-parity regression pin** | 480k-tuple ASCIIData run, axis (1,1,1) w=30°, compared to `EulersRotated.csv` (legacy-DREAM3D-generated — *not an oracle*, see provenance). Retained for large-array/parallel-path coverage and historical parity; 1.44M element-wise assertions at 1e-4 tolerance. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: SIMPL Backwards Compatibility` | kept | DYNAMIC_SECTION over SIMPL 6.4 + 6.5 conversion fixtures; validates UUID + `FloatVec3p1FilterParameterConverter` axis+angle merge + array-path decoding. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: Class 1 Analytical Fixtures` | new-for-V&V | 8 DYNAMIC_SECTIONs (F1–F8), each with derivation comment and 3 component assertions at 1e-5 rad against the independent oracle. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: Zero-Length Axis Fails Preflight` | new-for-V&V | Pins the `-96200` preflight guard added during the algorithm-review pass (previously silent NaN corruption). |
| `OrientationAnalysis::RotateEulerRefFrameFilter: Class 4 Invariants` | new-for-V&V | 3 SECTIONs over a 6-orientation batch: canonical range bounds; (n,w)/(−n,w) round-trip at 1e-4; 45°+45° = 90° composability at 1e-4. |

All 5 TEST_CASEs (13 ctest sections) pass at the working commit (in-core build). OOC/dual-build run deferred: the algorithm has no OOC-specific variant (direct `Float32Array` access) and no OOC build is configured in this workspace — same disposition as `BadDataNeighborOrientationCheckFilter`.

## Exemplar archive

- **Archive:** `ASCIIData.tar.gz` (pre-existing shared archive; declared in `src/Plugins/SimplnxCore/CMakeLists.txt`)
- **SHA512:** *(shared archive; see `src/Plugins/SimplnxCore/CMakeLists.txt` — not duplicated here because this filter neither created nor versioned it)*
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/RotateEulerRefFrameFilter.md`

No new exemplar archive was created for this V&V cycle: the Class 1 oracle is encoded entirely as inline expected values in the test source, and the Class 4 invariants need no expected output. `EulersRotated.csv` inside `ASCIIData.tar.gz` is legacy-DREAM3D output (circular-oracle provenance) and is retained **only** as a legacy-parity regression pin, explicitly not as a correctness oracle.

## Deviations from DREAM3D 6.5.171

- **No deviations observed.** Comparison run on 6 axis/angle cases × 12 orientations (`Code_Review/RotateEulerRefFrame/euler_input.csv`); max wrap-aware difference 7.2e-7 rad. Two non-deviations documented for future-engineer awareness in `vv/deviations/RotateEulerRefFrameFilter.md`:
  - **N1 (precision)** — float vs double degree→radian conversion; ULP-level only.
  - **N2 (precision, representation)** — 0 vs 2π canonical representation at the exact wrap boundary (observed for input (π/2, π/4, ¾π) rotated z-90°). Same physical angle.
**SIMPLNX-side fix ships in DREAM3D-NX 7.4.2** — the deviation from legacy remains, since 6.5.171 is unchanged: `Non-deviation`.
