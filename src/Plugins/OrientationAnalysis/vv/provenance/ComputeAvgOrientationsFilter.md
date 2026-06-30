# Exemplar / Oracle Provenance: ComputeAvgOrientationsFilter

This filter's V&V oracle is **inline hand-built data** encoded directly in `test/ComputeAvgOrientationsTest.cpp` — there is no exemplar `.tar.gz` archive for the correctness oracle. This sidecar documents the oracle derivation and the retirement of the prior circular-oracle archive.

## Retired archive (circular oracle)

| Field | Value |
|---|---|
| **Archive** | `7_ComputeAvgOrientation_v2.tar.gz` |
| **SHA512** | `2c2a691f1da301c449c20bafec65512d5134db38384ac7cb4c910880ccd87a260a5f011e905f35b97abff3952309f109c737c63ec3c833708926827a62a92efc` |
| **Status** | **Retired YYYY-MM-DD** (replaced by inline oracle) |

This archive bundled the filter's own computed `AvgQuats` / `Watson Avg Quats` / `vMF Avg Quats` / Euler / Kappa arrays as the comparison target. It was regenerated from post-fix SIMPLNX output in PR #1577 and therefore could not serve as an independent oracle (any bug present at capture time would be confirmed forever). It is named in the V&V audit's cross-cutting circular-oracle list. The V&V replaces it with the Class 1/2/4 oracle below.

## Class 1 (Analytical) — Rodrigues average, Triclinic symmetry

Triclinic (`CrystalStructures = 4`) has only the identity proper-rotation operator, so `getFZQuat` is a no-op and `getNearestQuat(ref, q)` reduces to choosing the double-cover representative (`q` or `−q`) nearest `ref`. With all input quaternions chosen in the northern hemisphere (`w > 0`), the Rodrigues running average reduces to the closed form **`normalize(Σ qᵢ)`, positive-oriented** — a hand-computable quaternion mean.

Quaternions below are in `(x, y, z, w)` storage order (the order the filter reads/writes). `Rz(θ)` denotes a rotation by θ about z = `(0, 0, sin(θ/2), cos(θ/2))`.

| Fixture | Voxels (phase) | Expected AvgQuats `(x,y,z,w)` | Derivation |
|---|---|---|---|
| **F0** background | (phase 0, ignored) | `(0,0,0,1)` identity | `count==0` finalize → identity |
| **F1** single identity | 1× identity | `(0, 0, 0, 1)` | single voxel → itself |
| **F2** single Rz(90°) | 1× `(0,0,0.70710678,0.70710678)` | `(0, 0, 0.70710678, 0.70710678)` | single voxel → itself (Triclinic FZ no-op) |
| **F3** mean of identity & Rz(90°) | identity + Rz(90°) | `(0, 0, 0.38268343, 0.92387953)` = **Rz(45°)** | `normalize(identity + Rz90) = Rz(45°)` (see derivation note) |
| **F4** 3× Rz(90°) | 3× Rz(90°) | `(0, 0, 0.70710678, 0.70710678)` = Rz(90°) | N identical → same orientation |
| **F5** empty feature | none | `(0,0,0,1)` identity | `count==0` finalize → identity |

**F3 derivation:** `identity + Rz(90°) = (0,0,sin45°, 1+cos45°)`; for a general `Rz(α)`, `normalize(identity + Rz(α)) = (0,0,sin(α/4),cos(α/4)) = Rz(α/2)` because `1+cos(α/2)=2cos²(α/4)` and `sin(α/2)=2sin(α/4)cos(α/4)`. For α=90° → `Rz(45°) = (0,0,sin22.5°,cos22.5°) = (0,0,0.38268343,0.92387953)`.

**Euler note:** A pure z-rotation has `Φ = 0`, a gimbal-lock case where Bunge `φ1` and `φ2` are individually non-unique (only `φ1+φ2` is determined). Therefore Euler is asserted tightly only for the identity fixture (`(0,0,0)`); for z-rotation fixtures the quaternion is the tight assertion and Euler is checked as an invariant (`Φ ≈ 0`, all finite).

### Class 4 (Invariant) companions — Rodrigues
- Output quats unit-norm and northern-hemisphere (`w ≥ 0`).
- Zero-voxel feature → identity `(0,0,0,1)` + zero Euler.
- Voxel-ordering independence: F3 with voxels in order [identity, Rz90] vs [Rz90, identity] → identical result (verified: both accumulate to `(0,0,0.7071,1.7071)` pre-normalize).

## Class 2 (Reference — EbsdLib, trusted) — vMF / Watson average

The vMF/Watson EM math is owned and tested by EbsdLib (`EbsdLib/Source/Test/DirectionalStatsTest.cpp`). Its `DirectionalStatsTest:VMF` / `:Watson` cases run **at this filter's exact configuration** (`numEM=5, numIter=10, seed=43514`, Cubic_High ops) over a fixed 22-quaternion set (`detail::k_TestQuats`, each FZ-reduced via `cubicOps->getFZQuat`), and hard-assert the EM result. The filter performs the identical pipeline (`getFZQuat` → `EMforDS(43514,…)` → `positiveOrientation`).

**Oracle fixture:** one SIMPLNX feature whose 22 voxels are exactly `detail::k_TestQuats` (Phase 1, `CrystalStructures[1] = Cubic_High = 1`). The filter's per-feature result must reproduce EbsdLib's documented values (which have `w > 0`, so `positiveOrientation` is a no-op):

| Method | Expected AvgQuats `(x,y,z,w)` | Expected Kappa | Source |
|---|---|---|---|
| **vMF** | `(0.3322000547718371, −0.1964639452260062, 0.2450656693404858, 0.8893749825279105)` | `88.9943042750539774` | `DirectionalStatsTest.cpp:202–206` |
| **Watson** | `(0.2948298270586034, −0.2106011604618418, 0.2378717152588106, 0.9011878668560466)` | `30.5730272919979669` | `DirectionalStatsTest.cpp:249–253` |

This proves the filter feeds EbsdLib correctly and lands the result in the right output tuple, **without re-deriving any EM math** (per the "test the value-add, not upstream" rule). Tolerance: match EbsdLib's `Approx` margin (quaternion ±1e-6; kappa ±1e-4 relative).

### Class 4 (Invariant) companions — vMF / Watson
- Single-voxel feature → `muhat` = that voxel's FZ quat, `kappa == 0` (filter shortcut, EM skipped; algorithm lines 103–106 / 139–141).
- Zero-voxel feature → all vMF/Watson outputs `NaN` (fill at lines 339–355, never overwritten).
- Output quats unit-norm + northern-hemisphere; results at the feature-index tuple (not voxel index).

## Second-engineer oracle review

- **Reviewer:** *pending* — recommend an OA-domain engineer confirm the Triclinic closed-form derivations and the 22-quat Class-2 reuse.
- **Date:** *pending*

## Reproduction

The toy data is constructed entirely in `test/ComputeAvgOrientationsTest.cpp` from the literals above (no external files). To reproduce: read the fixture tables here, build the arrays, run the filter with the relevant method toggles, and compare against the expected columns.
