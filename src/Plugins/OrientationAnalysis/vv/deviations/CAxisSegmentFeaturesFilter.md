# Deviations from DREAM3D 6.5.171: CAxisSegmentFeaturesFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`CAxisSegmentFeaturesFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## CAxisSegmentFeaturesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D1` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active (SIMPLNX bug **fixed during this V&V cycle**; documented for users of prior SIMPLNX releases) |

**Symptom:** In SIMPLNX releases containing PR #1466 (2025-11-14) and prior to this V&V cycle, the filter could report one extra (empty) feature, shift every FeatureId up by one, or grow a feature from a masked-out / unindexed voxel — whenever voxel 0 of the image was not a legitimate seed. 6.5.171 never exhibits this.

**Root cause:** Bug (SIMPLNX). The shared driver `src/simplnx/Utilities/SegmentFeatures.cpp::execute()` started the flood fill from the raw index `seed = 0` without calling `getSeed()`, so the first seed was neither validated against the mask/phase requirements nor stamped with its FeatureId. Legacy `SegmentFeatures::execute()` obtains every seed — including the first — from `getSeed()`. Restored in this V&V cycle (`seed = getSeed(gnum, nextSeed)` before the loop); the fix also applies to `EBSDSegmentFeatures` and `ScalarSegmentFeatures`, which share the driver. Pinned by the `Class 1 Analytical (Mask Excludes Voxel 0)` and `Execute Error - No Features Found (-87000)` test cases.

**Affected users:** SIMPLNX users (post-#1466, pre-fix builds) whose datasets have a masked-out, unindexed, or already-owned cell at linear index 0 — common in EBSD scans with a mask. Legacy 6.5.171 users are unaffected.

**Recommendation:** Trust SIMPLNX at or after this V&V commit (which agrees with 6.5.171). Results from affected intermediate SIMPLNX builds on masked data should be regenerated.

---

## CAxisSegmentFeaturesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D2` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |

**Symptom:** FeatureIds from 6.5.171 are a different (random) labeling on every run, while SIMPLNX produces the same FeatureIds on every run; the two versions never produce bit-identical FeatureIds arrays.

**Root cause:** Algorithmic choice. 6.5.171 hard-codes `m_RandomizeFeatureIds = true` and seeds its RNG from the wall clock (`CAxisSegmentFeatures.cpp::initializeVoxelSeedGenerator`), and the option is not exposed as a pipeline parameter — legacy output labeling is irreproducible by construction. SIMPLNX exposes `Randomize Feature Ids` as a parameter (default `false`) and, when enabled, uses a fixed-seed `std::mt19937_64` (`ClusterUtilities::RandomizeFeatureIds`), so output is deterministic either way. The segmentation *partition* (which cells share a feature) is unaffected; the 2026-07-22 A/B run (`vv/comparisons/CAxisSegmentFeaturesFilter/`) matched partitions exactly on all four fixtures.

**Affected users:** Anyone diffing raw FeatureIds arrays between versions or between two 6.5.171 runs; downstream statistics keyed by feature (sizes, misorientations) are invariant to the labeling.

**Recommendation:** Trust SIMPLNX. Deterministic labeling is strictly more reproducible; compare segmentations at the partition level when validating against legacy runs.

---

## CAxisSegmentFeaturesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D3` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |

**Symptom:** On data containing non-hexagonal phases that participate in segmentation, 6.5.171 silently produces a segmentation; SIMPLNX fails with error `-8363` (and `-8364` for phase values with no CrystalStructures entry).

**Root cause:** Algorithmic choice (deliberate SIMPLNX guard). The c-axis is only a physically meaningful unique axis for hexagonal (6/m, 6/mmm) Laue classes, but the c-axis math itself never consults the crystal structure — legacy computes the [001] misalignment for cubic/other phases and returns scientifically meaningless groupings. SIMPLNX validates that every cell that can participate in segmentation (phase > 0, not masked out) has a Hexagonal_High or Hexagonal_Low crystal structure (`CAxisSegmentFeatures.cpp::operator()`). Unindexed (phase 0) cells and masked-out cells are exempt, since they can never seed or join a feature.

**Affected users:** Anyone who ran the legacy filter on multi-phase data with non-hexagonal phases — their legacy results for those phases were never meaningful. Pure-hexagonal workflows are unaffected.

**Recommendation:** Trust SIMPLNX. The error is a correctness guard; mask out non-hexagonal phases (supported) to segment only the hexagonal cells.

---

## CAxisSegmentFeaturesFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D4` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active (SIMPLNX bug **fixed during this V&V cycle**; documented for users of prior SIMPLNX releases) |

**Symptom:** Prior to this V&V cycle, SIMPLNX rejected (error `-8363`) any dataset containing unindexed (phase 0) cells — whose `CrystalStructures[0]` entry is the conventional `999` sentinel — or masked-out non-hexagonal cells; 6.5.171 processes such datasets normally.

**Root cause:** Bug (SIMPLNX). The D3 validation loop checked every cell's crystal structure, including phase-0 cells and cells excluded by the mask, neither of which can ever participate in segmentation (`getSeed` requires phase > 0 and a set mask bit; `determineGrouping` requires equal phases and a set mask bit). It also indexed `CrystalStructures[phase]` without a bounds check, so an out-of-range phase value read out of bounds instead of producing an error. Fixed by exempting phase ≤ 0 and masked-out cells and adding the `-8364` bounds error. Pinned by the `Phase 0 (Unindexed) Cells Tolerated`, `Masked Non-Hexagonal Cells Tolerated`, and `Execute Error - Phase Out of Ensemble Bounds (-8364)` test cases; the 2026-07-22 A/B run confirms post-fix parity with 6.5.171 on phase-0 data (TC4).

**Affected users:** SIMPLNX users (pre-fix builds) with EBSD scans containing unindexed points — a very common case — or deliberately masked non-hexagonal phases.

**Recommendation:** Trust SIMPLNX at or after this V&V commit; upgrade if the filter spuriously rejects data with unindexed points.

---

## CAxisSegmentFeaturesFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D5` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active (SIMPLNX bug **fixed during this V&V cycle**; no legacy equivalent behavior) |

**Symptom:** SIMPLNX accepts a RectGrid geometry as input (6.5.171 accepts only Image geometry); prior to this V&V cycle, selecting a RectGrid geometry passed preflight and then crashed at execute.

**Root cause:** Bug (SIMPLNX). The algorithm fetched the geometry with a stale `getDataAs<ImageGeom>()` cast, returning a null pointer for RectGrid input, which the shared segmentation driver dereferenced. Fixed to `getDataAs<IGridGeometry>()`, matching the parameter's allowed types and the sibling EBSD/Scalar segmentation algorithms. Pinned by the `Class 1 Analytical (RectGrid Geometry)` test case.

**Affected users:** SIMPLNX users (pre-fix builds) segmenting RectGrid data — e.g., regularized serial-sectioning data. Legacy users are unaffected (the capability does not exist in 6.5.171).

**Recommendation:** Trust SIMPLNX at or after this V&V commit.
