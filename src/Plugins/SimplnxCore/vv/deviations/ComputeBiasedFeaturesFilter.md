# Deviations from DREAM3D 6.5.171: ComputeBiasedFeaturesFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindBoundingBoxFeatures`, SIMPL UUID `450c2f00-9ddf-56e1-b4c1-0e74e7ad2349`).

Entries are referenced by stable ID (`ComputeBiasedFeaturesFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ComputeBiasedFeaturesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeBiasedFeaturesFilter-D1` |
| **Filter UUID** | `d46f2fd7-dc68-4b57-bca3-693016512b2f` |
| **Status** | active (SIMPLNX bug **fixed during this V&V cycle** in this PR; documented for users of prior SIMPLNX releases) |

**Symptom:** On a 2D geometry whose flat axis is **not** Z (an X-normal or Y-normal slab), DREAM3D 6.5.171 classifies the wrong features as biased. On the V&V X-normal fixture (F) it flags a strictly interior feature as biased; on the Y-normal fixture (G) it does the opposite and reports one genuine surface feature (index 2) as *unbiased*, thereby violating invariant I2. SIMPLNX reports the geometrically correct answer.

Note that the *pre-fix SIMPLNX* outcome on the Y-normal fixture was worse than the legacy one, because a second and SIMPLNX-only defect compounded this one on the same code path: pre-fix SIMPLNX returned no biased features at all on fixture G, leaving all **three** genuine surface features unbiased. That combined outcome belongs to this deviation *plus* the SIMPLNX-only spacing regression described at the end of this entry — not to this deviation alone.

**Root cause:** Bug, present in both implementations and now fixed in SIMPLNX. The 2D code path drops the flat axis and builds the bounding box over the two remaining in-plane axes, selecting the two matching centroid components through `centroidShift0` / `centroidShift1` (X flat → Y and Z; Y flat → X and Z; Z flat → X and Y). The shrink loop honours that selection, but the classification loop compared the raw X and Y centroid components against the axis-shifted box:

- 6.5.171 — `FindBoundingBoxFeatures.cpp:433-448`, `m_Centroids[3 * j]` and `m_Centroids[3 * j + 1]`.
- SIMPLNX before this pass — `Algorithms/ComputeBiasedFeatures.cpp:320-335`, the same two raw components.

For a Z-normal slab the raw components happen to *be* the in-plane components, which is why the defect stayed hidden: the shipped example pipeline and the retired exemplar archive both used Z-normal 2D data. For an X-normal slab the box lives in (Y, Z) but the code tested X and Y, and the flat axis's centroid component is a single constant (the mid-plane of the one-cell-thick layer) for every feature — so the comparison degenerates into "all features biased" or "no features biased" depending on where that constant falls relative to the box.

SIMPLNX now classifies the same two components the shrink loop used (`Algorithms/ComputeBiasedFeatures.cpp`, 2D classify loop).

A second, SIMPLNX-only defect compounded this on the same code path and is *not* a deviation because 6.5.171 was correct: SIMPLNX built the 2D box from `spacing[0]` and `spacing[1]` regardless of which axis was flat, where 6.5.171 remapped the spacing per axis via `std::tie`. It is recorded in the V&V report's Bug Fixes section rather than here.

**Affected users:** Anyone who ran the filter on a 2D geometry sliced normal to X or normal to Y — for example a single-column or single-row ROI cropped out of a 3D volume, or an EBSD montage indexed along a non-Z axis. Z-normal 2D data (the overwhelmingly common case, and what every shipped example pipeline uses) is unaffected, and 3D data is unaffected. Anisotropic spacing widens the error but is not required for it. Three populations have to be distinguished:

- **DREAM3D 6.5.x users:** the bug is present. It is patched on the 6.5.172 alignment branch (see Recommendation), so a 6.5.x build carrying that patch is unaffected; a stock 6.5.171 build is not.
- **Users of SIMPLNX releases predating this PR:** the bug is present, and on X-normal and Y-normal slabs the result is *worse* than the legacy one, because the SIMPLNX-only 2D spacing regression described below compounds it on the same code path. Results from those builds on non-Z-normal 2D data should be regenerated.
- **Current SIMPLNX (this PR onward):** fixed. Both this defect and the compounding spacing regression are corrected, and the output matches the independently hand-derived oracle on all twelve V&V fixtures.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result compared coordinates on one pair of axes against a box defined on a different pair and is not meaningful for non-Z-normal slabs. Confirmed by surgically patching a local build of the legacy source with the equivalent one-expression change: the patched legacy build then reproduces the SIMPLNX output, and the independently hand-derived oracle, on all eleven V&V fixtures.

---

## ComputeBiasedFeaturesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeBiasedFeaturesFilter-D2` |
| **Filter UUID** | `d46f2fd7-dc68-4b57-bca3-693016512b2f` |
| **Status** | active |

**Symptom:** On a 2D geometry with a non-zero origin, DREAM3D 6.5.171 anchors the bounding box at (0, 0) instead of at the geometry origin, so features are classified against a box that does not overlap the data. On the V&V fixture (origin (4, 8, 0)) 6.5.171 flags an interior feature as biased; with a large enough origin offset every feature is reported biased. SIMPLNX uses the real origin.

**Root cause:** Bug in 6.5.171 only. `FindBoundingBoxFeatures::find_boundingboxfeatures2D()` declares `float xOrigin = 0.0f; float yOrigin = 0.0f;` at `FindBoundingBoxFeatures.cpp:331-332` and never assigns them — the adjacent `//float zOrigin = 0.0f;` comment shows the axis remapping was left unfinished — then builds the box as `boundbox[0] = xOrigin; boundbox[1] = xOrigin + xPoints * xRes; ...`. The 3D path in the same file has no such problem because it calls `imageGeom->getBoundingBox(boundbox)`, which reads the origin. SIMPLNX reads the origin on both paths.

**Affected users:** Anyone who ran the filter on a 2D geometry whose origin is not (0, 0) on the two in-plane axes — most commonly a 2D slab cropped out of a larger volume while preserving world coordinates, or data imported with a physical stage offset. 2D geometries at the origin and all 3D geometries are unaffected.

**Recommendation:** Trust SIMPLNX. The 6.5.171 box was anchored at a location unrelated to the data. Confirmed by surgically patching a local build of the legacy source to remap the in-plane origins alongside the spacings; the patched build then reproduces the SIMPLNX output and the hand-derived oracle.

---

## ComputeBiasedFeaturesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeBiasedFeaturesFilter-D3` |
| **Filter UUID** | `d46f2fd7-dc68-4b57-bca3-693016512b2f` |
| **Status** | active |

**Symptom:** With *Apply Phase by Phase* enabled on a 3D geometry, SIMPLNX derives the phase count from the whole *Phases* array including index 0, where 6.5.171 begins its scan at index 1. Index 0 is the "unassigned" bucket and its contents are not meaningful, so a large or garbage value there makes SIMPLNX iterate the phase loop many more times than there are real phases.

**Root cause:** Algorithmic choice, differing between the two implementations.

- SIMPLNX — `Algorithms/ComputeBiasedFeatures.cpp`, `numPhases = *std::max_element(phasesStorePtr->begin(), phasesStorePtr->end())`, which spans the whole array.
- 6.5.171 — `FindBoundingBoxFeatures.cpp:218-224`, `for(size_t i = 1; i < size; i++) { if(m_Phases[i] > numPhases) ... }`, which skips index 0.

**Output is unaffected.** Each extra iteration resets the box to the full geometry bounds, finds no surface feature matching that phase, and then finds no feature matching that phase to classify, so it writes nothing. This was verified rather than assumed: V&V fixture D sets `phases[0] = 5` while the real phases are 1 and 2, and SIMPLNX (5 iterations) and 6.5.171 (2 iterations) produce byte-identical output.

The cost is runtime, not correctness. Because the phase loop bound is taken from data, a garbage index-0 value near `INT32_MAX` would make SIMPLNX iterate effectively forever, where 6.5.171 would not. There is a per-iteration cancel check, so the run is interruptible rather than wedged.

**Affected users:** Nobody, for output purposes — on well-formed data the two implementations agree exactly. Users whose *Phases* array carries a large garbage value at index 0 (rather than the conventional 0) and who enable *Apply Phase by Phase* on 3D data would see SIMPLNX appear to hang while 6.5.171 completed.

**Recommendation:** Either acceptable for output. Left unchanged this pass because it is not an output difference and the change is value-neutral for conforming data; the recommended future change is to start the `max_element` scan at index 1 so the bound matches 6.5.171 and cannot be driven by the unassigned slot. Tracked in the V&V report's outstanding list.
