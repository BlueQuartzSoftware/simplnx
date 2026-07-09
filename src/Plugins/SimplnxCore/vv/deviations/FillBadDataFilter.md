# Deviations from DREAM3D 6.5.171: FillBadDataFilter

Entries use stable IDs (`FillBadDataFilter-D<N>` for legacy deviations, `FillBadDataFilter-B<N>` for SIMPLNX-side bugs).

---

## Headline: No deviations observed

The legacy comparison is the **SmallIN100 in-unit-test check**: `FillBadData_SmallIN100` compares SIMPLNX output against `6_5_exemplar.dream3d` (generated from a DREAM3D 6.5.x pipeline) with `MinAllowedDefectSize=1000`, `StoreAsNewPhase=false`. It passes — no differences. Correctness of the individual code paths is pinned independently by the Class 1 analytical fixtures (Tests 01–07, 11, 13).

> **Note — retired Test 08 A/B.** An earlier revision of this file cited a separate 7×7×3 three-way binary A/B run (DREAM3D 6.5.171 / 6.5.172 / DREAM3D-NX, `test_08_input.dream3d` + `fill_bad_data_vv/comparison_report.md`). Those working files were **not preserved and are unrecoverable**, so that comparison is **withdrawn** rather than left as an unverifiable citation. The SmallIN100 comparison above plus the analytical oracle fixtures are the standing evidence; if a second independent binary A/B is wanted for the record, it must be regenerated from scratch.

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

---

## Non-deviations (documented for awareness)

### Architectural difference: 4-phase CCL vs. legacy in-memory

SIMPLNX uses chunk-sequential CCL with Union-Find (Phases 1–3) to identify connected bad-data regions before applying the size threshold. Legacy DREAM3D 6.5.171 used a simpler in-memory approach. For in-core datasets the functional output is identical; the CCL architecture is a strategy for OOC correctness, not a behavioral change.

**Evidence:** `FillBadData_SmallIN100` passes (SIMPLNX output matches the 6.5.x exemplar element-wise).

### Tie-breaking in Phase 4 iterative fill

When two features have equal neighbor vote counts, the first-encountered neighbor wins by linear scan order. Both SIMPLNX and legacy DREAM3D use the same linear order, so tie-break behavior is scan-order-equivalent. This is an algorithm characteristic, not a defect.

**Evidence:** `Test11_NeighborTieBreaking` passes against its analytically derived expected file.
