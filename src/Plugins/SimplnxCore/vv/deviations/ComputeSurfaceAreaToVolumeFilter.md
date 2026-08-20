# Deviations from DREAM3D 6.5.171: ComputeSurfaceAreaToVolumeFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent, `FindSurfaceAreaToVolume` (SIMPL UUID `5d586366-6b59-566e-8de1-57aa9ae8a91c`).

Entries are referenced by stable ID (`ComputeSurfaceAreaToVolumeFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

All five entries were produced by the 2026-08-20 V&V cycle. The A/B binary of record is the 6.5.171 `PipelineRunner` shipped in `~/Applications/DREAM3D.app`; the fixtures are the ten A/B fixtures described in the report's *Deviations* section.

---

## ComputeSurfaceAreaToVolumeFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSurfaceAreaToVolumeFilter-D1` |
| **Filter UUID** | `94e83e4f-797d-4594-b130-3819b7676f01` |
| **Status** | active |

**Symptom:** On an **anisotropic** Image Geometry, `SurfaceAreaVolumeRatio` (and, through it, `Sphericity`) differs between SIMPLNX and 6.5.171. On the V&V fixture F3 — a 6×8×3 grid with spacing (1, 2, 4) holding one 4-cell rod along X (feature 1) and one 4-cell rod along Y (feature 2) — 6.5.171 reports `SAVR = [_, 2.75, 2.0]` where the correct values are `[_, 2.0, 2.75]`. The two features' values are *exchanged*. On isotropic spacing there is no difference at all.

**Root cause:** Bug, shared. Both codebases assigned the wrong spacing product to two of the three face orientations. The face shared with a ±Y neighbor lies in the XZ plane and has area `dx·dz`; the face shared with a ±X neighbor lies in the YZ plane and has area `dy·dz`. Both implementations had those two swapped — 6.5.171 at `Source/Plugins/Statistics/StatisticsFilters/FindSurfaceAreaToVolume.cpp:299-306` (`l == 1 || l == 4` → `yRes * zRes`, `l == 2 || l == 3` → `zRes * xRes`) and SIMPLNX at the corresponding lines of `Algorithms/ComputeSurfaceAreaToVolume.cpp`. The misleading `// YZ face shared` / `// XZ face shared` comments in both files record the same misconception. With `dx == dy == dz` the two products are equal, which is why the defect survived: every dataset in the shipping test suite is isotropic, and the retired exemplar archive is a Small IN100 slice at spacing (0.25, 0.25, 0.25) — structurally incapable of showing it (see the report's *Oracle* section for the executed proof).

**Affected users:** Anyone who ran the filter on an Image Geometry with non-cubic voxels — which includes most serial-sectioning EBSD data, where the Z spacing is the section thickness and rarely equals the in-plane step. The error is not a uniform scale factor: it redistributes area between features according to their elongation direction, so relative comparisons between features are affected as well as absolute values. Isotropic-spacing users are unaffected and their prior results stand.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result was geometrically wrong. Fixed in SIMPLNX on this branch. To prove that this single change is what separated the two codebases, the same two lines were patched in a local build of the legacy source (`BUG: FindSurfaceAreaToVolume — correct the per-face areas and the sphericity exponents`, commit `304706bae` on the local 6.5.172 staging branch; diff preserved as `patch_6_5_172.diff` in the V&V working folder): with it, the legacy binary reproduces the fixed SIMPLNX output and the independent oracle on all nine legacy-admissible fixtures. The patched build is internal proof tooling and is not a shipping comparison target.

---

## ComputeSurfaceAreaToVolumeFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSurfaceAreaToVolumeFilter-D2` |
| **Filter UUID** | `94e83e4f-797d-4594-b130-3819b7676f01` |
| **Status** | active |

**Symptom:** `Sphericity` differs between SIMPLNX and 6.5.171 by a small relative amount on **every** feature of **every** dataset, isotropic spacing included. Measured across the six isotropic V&V fixtures (F1, F2, F2b, F4, F6a, F6b) the difference runs from 1.405e-6 to 2.009e-5 relative; on the anisotropic fixtures (F3, F7) it is compounded by D1 and much larger; on the 619 features of the retired Small IN100 exemplar archive it reaches 4.9e-5. `SurfaceAreaVolumeRatio` is unaffected.

**Root cause:** Bug (precision, but a deterministic bias rather than round-off), shared. Sphericity is `π^(1/3) · (6V)^(2/3) / A`. Both codebases wrote the exponents as truncated decimal literals — `0.333333f` and `0.66666f` — instead of the exact rationals (6.5.171 at `FindSurfaceAreaToVolume.cpp:316,322`; SIMPLNX at the corresponding lines of `Algorithms/ComputeSurfaceAreaToVolume.cpp`). The `2/3` truncation is the dominant term: `2/3 - 0.66666 = 6.67e-6`, and the resulting relative error in `(6V)^(2/3)` is that residual times `ln(6V)`, so the bias grows with feature volume and is not bounded by float32 round-off. It is a systematic bias in a dimensionless shape descriptor that users compare across datasets, and it costs nothing to remove.

**Affected users:** Every user of the sphericity output, on every dataset. The magnitude is below the level at which the aliasing warning in the user documentation matters — a digital cube's sphericity is under-estimated by ~19% by voxelization alone — so no published conclusion is likely to change. The value is that sphericity is now the quantity the documented equation says it is.

**Recommendation:** Trust SIMPLNX. Fixed in SIMPLNX on this branch. Covered by the same alignment patch cited in D1 (`304706bae`): with the exponents corrected, the legacy binary reproduces the fixed SIMPLNX sphericity to within 1e-6 relative on all nine legacy-admissible fixtures. Users migrating a stored sphericity column should expect a change in the fifth significant figure; the pre-fix values for the V&V fixtures are tabulated in the report's *Oracle* section for anyone reconciling old output.

---

## ComputeSurfaceAreaToVolumeFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSurfaceAreaToVolumeFilter-D3` |
| **Filter UUID** | `94e83e4f-797d-4594-b130-3819b7676f01` |
| **Status** | active |

**Symptom:** An input whose Feature AttributeMatrix is *over-provisioned* — more feature tuples than `max(FeatureIds) + 1` — is accepted by SIMPLNX and rejected by 6.5.171 with error `-5555` ("The number of Features in the NumCells array (4) does not match the largest Feature Id in the FeatureIds array"). Observed on V&V fixture F1b: a 3×3×3 grid with a single cell of feature 1 and a 4-tuple Feature AttributeMatrix. SIMPLNX writes feature 1's correct values and leaves the two unused tuples at `SAVR = 0`, `Sphericity = +inf`; 6.5.171 produces no output at all.

**Root cause:** Algorithmic choice. 6.5.171's `execute()` scans the FeatureIds array and errors when `largestFeature != numFeatures - 1` (`FindSurfaceAreaToVolume.cpp:216-222`), i.e. it demands an exactly-sized Feature AttributeMatrix. SIMPLNX calls `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` (`DataArrayUtilities.cpp`), which rejects only ids that would index *past* the feature arrays (`maxFeatureId >= numFeatures`, error `-5351`) and negative ids (`-5355`). Extra unused feature tuples are legal in SIMPLNX — they are simply never written. This is the same relaxation already documented for `CopyFeatureArrayToElementArrayFilter-D2`.

**Affected users:** Anyone whose pipeline produces a Feature AttributeMatrix sized independently of the FeatureIds actually present — for example after removing features without renumbering. Such pipelines failed outright in 6.5.171 and now run. The unused tuples' `Sphericity = +inf` is the ordinary zero-area degenerate case (see D5) and is not new behavior.

**Recommendation:** Trust SIMPLNX. Rejecting an input the filter can process correctly is not a safety property; the case that *is* dangerous (an id larger than the feature arrays) is still an error. Users who relied on `-5555` as a renumbering canary should add an explicit check, and should note that SIMPLNX's leniency means unused tuples carry `0` / `+inf` rather than being absent.

---

## ComputeSurfaceAreaToVolumeFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSurfaceAreaToVolumeFilter-D4` |
| **Filter UUID** | `94e83e4f-797d-4594-b130-3819b7676f01` |
| **Status** | active |

**Symptom:** The *Calculate Sphericity* switch has no effect in a 6.5.171 pipeline: a saved pipeline that sets it to false still produces a sphericity array, and that array is always named literally `Sphericity` regardless of the *Sphericity Array Name* in the pipeline file. In SIMPLNX both parameters are honored. Additionally, 6.5.171 will **crash** (null pointer dereference) if the flag is somehow false at execute time — reachable from the GUI or the C++ API, though not from a pipeline file.

**Root cause:** Bug in 6.5.171, corrected at port time. `FindSurfaceAreaToVolume::readFilterParameters()` reads only `FeatureIdsArrayPath`, `NumCellsArrayPath` and `SurfaceAreaVolumeRatioArrayName` (`FindSurfaceAreaToVolume.cpp:99-106`); `CalculateSphericity` and `SphericityArrayName` are never read, so both keep their constructor defaults of `true` and `"Sphericity"` (`:57-58`). Separately, `execute()` writes `m_Sphericity[i]` unconditionally (`:322`) while `dataCheck()` only allocates that array when `getCalculateSphericity()` is true (`:150-158`) — so honoring a false flag would dereference a null pointer. SIMPLNX guards the write with `if(m_InputValues->CalculateSphericity)` and creates the array only under the same condition, so both halves are correct.

**Affected users:** Every 6.5.171 pipeline user who turned sphericity off expecting it not to be computed (it was, and was written under a fixed name), or who renamed the sphericity output (the rename was ignored). Nobody hit the crash from a pipeline file, because the flag could never become false there.

**Recommendation:** Trust SIMPLNX. Migration consequence: this is the one code path in the filter that **cannot** be A/B'd against 6.5.171 at all — the legacy binary has no way to take it. The SIMPLNX behavior is verified by the `Sphericity Toggle Off` TEST_CASE, which asserts the array is not created, and that test is an oracle test with no legacy counterpart.

---

## ComputeSurfaceAreaToVolumeFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSurfaceAreaToVolumeFilter-D5` |
| **Filter UUID** | `94e83e4f-797d-4594-b130-3819b7676f01` |
| **Status** | active |

**Symptom:** A negative *Feature Id* is a hard error in SIMPLNX (`-5355`, no output). 6.5.171 accepts the same input silently and produces output. **Executed** on the F1 geometry with `FeatureIds[0]` set to `-1`: `PipelineRunner` exits 0 with no error or warning about the negative value and writes `SurfaceAreaVolumeRatio = [0, 12]`, `Sphericity = [0, 0.80599713]` — bit-for-bit what it produces when that cell carries id 0.

**Root cause:** Bug in 6.5.171 (latent undefined behavior), corrected at port time. 6.5.171's feature-count scan tracks only the *largest* id (`FindSurfaceAreaToVolume.cpp:195-206`), so a negative never trips a guard. Execution then reaches the unconditional accumulator `featureSurfaceArea[m_FeatureIds[...]] = featureSurfaceArea[m_FeatureIds[...]] + onsurf` (`:311`), which for id `-1` reads and writes one `float` *before* the start of a `std::vector<float>`. Scope of the risk, stated precisely: because the `feature > 0` guard at `:263` skips the face loop for negative ids, `onsurf` is always `0.0f` at that point, so the out-of-bounds write stores back the value it just read. The access is real undefined behavior — a hardened allocator or a sanitizer build will trap it — but it is not observed data corruption, and the run's own output is unaffected. SIMPLNX's `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` call rejects the input with `-5355` before any accumulation, and its per-cell loop additionally `continue`s on ids below 1.

**Affected users:** Anyone whose upstream pipeline can emit a negative feature id (a masked or sentinel-marked segmentation, for instance). Under 6.5.171 the run succeeded and those cells were effectively treated as background; under SIMPLNX the pipeline now stops.

**Recommendation:** Trust SIMPLNX — an out-of-bounds index should not be accepted, and silently reinterpreting a negative id as background hides an upstream defect. Migration note: a pipeline that used to run under 6.5.171 with negative ids will now fail with `-5355`. To reproduce the old numbers deliberately, replace negative ids with 0 upstream; the legacy output above is exactly what SIMPLNX produces for the id-0 equivalent. Covered by the `Execute error - negative FeatureIds (-5355)` section of the Error Paths TEST_CASE. Deliberately **not** included in the alignment patch: the patch's charter is output alignment for the two shared arithmetic bugs, and 6.5.171's output for this input already matches SIMPLNX's id-0 interpretation, so there is nothing to align.

---

## Confirmed non-deviations

Recorded so they are not relitigated. Each was checked against both sources and, where an observable follows, against both binaries.

- **Outer-boundary faces are skipped in both codebases.** The six boundary guards are line-for-line equivalent (6.5.171 `:269-292` sets a `good` flag; SIMPLNX `continue`s). Fixtures F4 (corner voxel, sphericity 1.612) and F5 (feature fills the volume, area 0, sphericity `+inf`) produce identical output from both binaries. The behavior is a real limitation and is now documented in the user doc rather than changed — changing it would alter every existing result.
- **Feature Id 0 counts as a differing neighbor in both codebases.** Neither implementation gates the face test on the neighbor's id being positive. Fixtures F6a (id-0 shell) and F6b (positive-id shell) give feature 1 the same area in both binaries. The old SIMPLNX user documentation claimed the opposite; the documentation was wrong, not the code.
- **Volume comes from the user-supplied *Number of Cells* array in both codebases**, never from a recount of FeatureIds. Fixture F2b (one cell of feature 1, `NumCells[1] = 8`) yields `SAVR = 1.5` from both binaries, not the `12.0` a recount would give.
- **Index 0 of both outputs is never written by either codebase** (both finalize loops start at feature 1). Fixture F1 sets `NumCells[0] = 5` so that a loop starting at 0 would write `Sphericity[0] = +inf`; both binaries leave it at 0.
- **Progress and cancel behavior.** SIMPLNX emits a per-Z-slice info message and reads `m_ShouldCancel` once per Z slice; 6.5.171 has neither. Additive, no output change on a run to completion.
