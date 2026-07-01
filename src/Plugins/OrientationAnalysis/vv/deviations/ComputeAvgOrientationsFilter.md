# Deviations from DREAM3D 6.5.171: ComputeAvgOrientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent, `FindAvgOrientations` (SIMPL UUID `bf7036d8-25bd-540e-b6de-3a5ab0e42c5f`).

Entries are referenced by stable ID (`ComputeAvgOrientationsFilter-D<N>`) from the V&V report and from public migration guidance. The deviations apply **only to the Rodrigues (original) averaging method**; the von Mises-Fisher and Watson methods are new in SIMPLNX and have no 6.5.171 equivalent.

> **Status:** Empirically validated 2026-06-30 via `compare-legacy-dream3d` against the **official DREAM3D 6.5.171 release** (`~/Applications/DREAM3D.app/Contents/bin/PipelineRunner`), reading byte-identical legacy-format input shared with NX `nxrunner`.
>
> Two fixtures were run: **(A)** the realistic dataset (`ASCIIData` CSVs — 480,000 cells / 409 feature tuples / single cubic phase) and **(B)** a hand-built 3-feature fixture designed to *force* the edge-case deviations.
>
> **Headline:** on fixture A, SIMPLNX and 6.5.171 agree to float32 epsilon on all 408 **real** features (`AvgQuats` identical ≤1e-6, zero sign flips; `AvgEulerAngles` max diff 4.77e-7). The divergences are confined to the **feature-0 / unindexed-voxel handling**: **D3** (empty feature 0) on fixture A, and **D2** (FeatureId-0 voxels with phase>0) demonstrated on fixture B. **D4** is a confirmed sub-epsilon precision difference. **D1** was downgraded — it could not be made to diverge and has no observable effect (see below). Per-deviation verdicts follow.

---

## ComputeAvgOrientationsFilter-D1 — NOT A DEVIATION (defensive normalization)

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D1` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | **downgraded 2026-06-30 — not a deviation.** No demonstrable divergence from 6.5.171. |

**What it is:** SIMPLNX appends `.getPositiveOrientation()` after normalize (`ComputeAvgOrientations.cpp:448`), canonicalizing the average quaternion into the northern hemisphere (`w ≥ 0`); legacy `FindAvgOrientations` calls only `QuaternionMathF::UnitQuaternion` (no sign canonicalization). On paper this looked like it could produce a `q` vs `−q` difference for any feature whose average lands with `w < 0`.

**Why it is not a deviation (empirical + structural, 2026-06-30):** It could not be made to diverge. The Rodrigues algorithm builds each average by symmetry-reducing every voxel toward the running average (`getNearestQuat`), so the result lands in the fundamental zone — rotation angle ≤ 180° ⇒ `w ≥ 0`. A deliberate fixture (feature with `Rz(170°)` + `Rz(200°)`, intended to push the sum to `w < 0`) was reduced identically by **both** 6.5.171 and SIMPLNX to `(0, 0, 0.0436, 0.999)` (`w > 0`) — no sign flip. Even in the only theoretical corner where `w < 0` could survive (pure-triclinic, the incremental average overshooting 180°), the two quaternions represent the **same physical orientation** (`q ≡ −q`), so there is no correctness or downstream effect. `getPositiveOrientation()` is therefore a harmless defensive canonicalization, not a behavioral difference from legacy.

**Affected users:** None.

**Recommendation:** No action. Documented here so the source difference (the extra `getPositiveOrientation()` call) is not mistaken for an unverified deviation in a future audit.

---

## ComputeAvgOrientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D2` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | **active — empirically demonstrated (2026-06-30)** on a forcing fixture |

**Symptom:** Voxels labeled `FeatureId == 0` (with `Phase > 0`) contribute to averaging in SIMPLNX and produce a computed average for feature 0; in 6.5.171 they are skipped and feature 0 is left at `(0,0,0,0)`.

**Empirical (2026-06-30):** Forcing fixture B — feature 0 given two phase-1 cells (identity + `Rz(90°)`). SIMPLNX computed `AvgQuats[0] = (0, 0, 0.382683, 0.92388)` (the `Rz(45°)` average); the official 6.5.171 wrote `AvgQuats[0] = (0, 0, 0, 0)` (skipped). A clear, large divergence — not sub-epsilon. (The realistic fixture A had no `FeatureId == 0` cells, so this gate is dormant on conventional EBSD data.)

**Root cause:** Algorithmic choice. Legacy gates accumulation on `m_FeatureIds[i] > 0 && m_CellPhases[i] > 0` (`FindAvgOrientations.cpp:246`). SIMPLNX gates on `currentPhase > 0` only (`ComputeAvgOrientations.cpp:412`), deliberately allowing the documented use-case of averaging an unlabeled bag of orientations all tagged `FeatureId 0` (algorithm comment lines 401–411).

**Affected users:** Only datasets where `FeatureId 0` legitimately carries `Phase > 0` data (atypical — `FeatureId 0` is conventionally background/unindexed with `Phase 0`). For conventional data there is no observable difference.

**Recommendation:** Either acceptable within tolerance. For conventional data the outputs match; the SIMPLNX behavior is a strict superset supporting an additional use-case (computing the average of an unlabeled orientation set).

---

## ComputeAvgOrientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D3` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | **active — empirically confirmed (2026-06-30) at feature 0** |

**Symptom:** For a feature with zero contributing voxels, SIMPLNX writes a clean identity quaternion `(0,0,0,1)`; 6.5.171 writes garbage. **Confirmed:** feature 0 is `(0,0,0,1)` in NX and `(0,0,0,0)` in 6.5.171 — the only divergent tuple in the entire 409-tuple comparison.

**Root cause:** Bug in 6.5.171, via two mechanisms. (a) The legacy init and finalize loops both run `for(i = 1; i < totalFeatures)` (`FindAvgOrientations.cpp:239,263`), so feature 0 is **never finalized** and keeps its allocation default `(0,0,0,0)` — this is what the comparison observed. (b) For a zero-count feature at index ≥ 1, the legacy finalize sets `Identity` then still executes `QuaternionMathF::ScalarDivide(avgQuats[i], counts[i])` — a divide by zero — then `UnitQuaternion`. SIMPLNX (`ComputeAvgOrientations.cpp:440–444`) iterates `for(featureId = 0)` and, on `counts == 0`, writes identity and `continue`s, handling both cases cleanly. (No zero-count feature at index ≥ 1 existed in the dataset, so mechanism (b) is source-confirmed only.)

**Affected users:** Anyone whose Feature Attribute Matrix contains a feature index with no contributing voxels (feature 0 always; index ≥ 1 after feature removal/renumbering gaps), and any consumer of feature 0's `AvgQuats` in legacy.

**Recommendation:** Trust SIMPLNX. The legacy zero-count result was uninitialized/undefined (`(0,0,0,0)` or division by zero); SIMPLNX's identity quaternion is well-defined.

---

## ComputeAvgOrientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D4` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | **active — empirically confirmed (2026-06-30), sub-epsilon** |

**Symptom:** `AvgEulerAngles` (and possibly `AvgQuats` at the last ULPs) may differ from 6.5.171 at the sub-epsilon level.

**Root cause:** Library + precision. Legacy uses `QuaternionMathF` arithmetic and `OrientationTransforms::qu2eu` for the quaternion→Euler conversion; SIMPLNX uses EbsdLib `ebsdlib::QuatF` and `QuaternionFType::toEuler()`. Both operate in `float32`, but the differing intermediate-math implementations and quaternion→Euler routines can produce last-bit differences.

**Empirical (2026-06-30):** Across all 408 real features, `AvgEulerAngles` differed by at most **4.77e-7** (122 of 1227 components in the 1e-7–1e-6 band, the rest below 1e-7; mean 3.1e-8). `AvgQuats` were identical within 1e-6. Confirms the divergence is purely float32 round-off in the two independent library code paths.

**Affected users:** Anyone doing bit-exact comparison of `AvgEulerAngles` between versions. Differences are at the floating-point-noise level and not materially significant for any downstream calculation.

**Recommendation:** Either acceptable within tolerance (≈1e-6). Neither implementation is more correct; the difference is float round-off in independent library code paths.
