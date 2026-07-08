# Deviations from DREAM3D 6.5.171: FillBadDataFilter

Entries use stable IDs (`FillBadDataFilter-D<N>` for legacy deviations, `FillBadDataFilter-B<N>` for SIMPLNX-side bugs).

---

## Headline: No deviations observed

Two independent comparisons confirm output equivalence between SIMPLNX and DREAM3D 6.5.x:

1. **SmallIN100 (in-unit-test):** `FillBadData_SmallIN100` compares SIMPLNX output against `6_5_exemplar.dream3d` with `MinAllowedDefectSize=1000`, `StoreAsNewPhase=false`. Passes — no differences.
2. **Test 08 A/B run (2026-07-03):** A 7×7×3 synthetic fixture with disconnected bad-data regions (one small → fills, one large → preserved) was executed through DREAM3D 6.5.171, 6.5.172, and DREAM3D-NX. All three produced identical FeatureIds output (147 voxels, element-wise match). See `fill_bad_data_vv/comparison_report.md`.

> **⚠ Evidence archival (open action):** `test_08_input.dream3d` and `fill_bad_data_vv/comparison_report.md` are **not** present in the committed `6_5_fill_bad_data.tar.gz` archive (which contains fixtures for Tests 01–07, 11, 13 only) and are not in the source tree, so this Test 08 A/B is **not reproducible or reviewable from the repository**. The fixture, both legacy runs, and the comparison report must be uploaded to the OneDrive verification archive (per the archive-filter-verification workflow) — or added to the test-data archive — and this note replaced with the archive link before the filter is promoted to COMPLETE. The SmallIN100 in-test comparison (Class 2) remains reproducible in the meantime.

## Comparison method

### SmallIN100 (automated, in-unit-test)

| | |
|---|---|
| **Comparison type** | SIMPLNX output vs `6_5_exemplar.dream3d` via `UnitTest::CompareExemplarToGeneratedData` |
| **Input** | `6_5_fill_bad_data/6_5_input.dream3d` (Small IN100 dataset) |
| **Parameters** | `MinAllowedDefectSize=1000`, `StoreAsNewPhase=false` |
| **Tolerance** | Element-wise equality, all cell-data arrays |
| **Configurations** | In-core only. `StoreAsNewPhase=false`. |
| **Result** | **Pass** |

### Test 08 binary A/B (manual, 2026-07-03)

| | |
|---|---|
| **Comparison type** | Three independent runners against the same input file |
| **Runners** | DREAM3D 6.5.171 (`PipelineRunner`), DREAM3D 6.5.172 (`PipelineRunner`), DREAM3D-NX (`nxrunner`) |
| **Input** | `test_08_input.dream3d` — 7×7×3 ImageGeometry, FeatureIds array only |
| **Parameters** | `MinAllowedDefectSize=10`, `StoreAsNewPhase=false` |
| **Tolerance** | Element-wise integer equality, 147-voxel FeatureIds array |
| **Configurations** | Disconnected small region (1 voxel < 10 → filled), disconnected large region (14 voxels ≥ 10 → preserved), 4:1 neighbor vote majority |
| **Result** | **Pass** — all three runners match each other and the analytically derived expected values |

---

## Non-deviations (documented for awareness)

### Architectural difference: 4-phase CCL vs. legacy in-memory

SIMPLNX uses chunk-sequential CCL with Union-Find (Phases 1–3) to identify connected bad-data regions before applying the size threshold. Legacy DREAM3D 6.5.171 used a simpler in-memory approach. For in-core datasets the functional output is identical; the CCL architecture is a strategy for OOC correctness, not a behavioral change.

**Evidence:** `FillBadData_SmallIN100` passes; Test 08 A/B confirms agreement across both legacy versions and NX.

### Tie-breaking in Phase 4 iterative fill

When two features have equal neighbor vote counts, the first-encountered neighbor wins by linear scan order. Both SIMPLNX and legacy DREAM3D use the same linear order, so tie-break behavior is scan-order-equivalent. This is an algorithm characteristic, not a defect.

**Evidence:** `Test11_NeighborTieBreaking` passes against its analytically derived expected file.
