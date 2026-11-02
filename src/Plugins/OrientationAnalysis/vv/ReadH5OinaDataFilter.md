# V&V Report: ReadH5OinaDataFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| SIMPLNX Human Name | Read Oxford Aztec Data (.h5oina) |
| DREAM.3D v6.5.171 equivalent | None (new filter) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Michael Jackson, 2026-08-24; second engineer: pending |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | New filter. DREAM.3D v6.5.171 has no H5OINA importer. |
| Oracle (confirmed) | Classes 1 and 4 use analytical fixtures and invariants in `test/ReadH5OinaDataTest.cpp`; Class 2 uses independent vendor-file readback. All pass in CI. |
| Code paths enumerated | 29 of 46 exercised. Gaps include defensive guards, cancellation, store failures, and a hidden boundary fixture. |
| Tests today | 21 test cases: 20 ordinary cases and 1 hidden boundary case. They cover analytical values, conversion options, scan order, malformed inputs, and vendor-file readback. |
| Exemplar archive | `H5Oina_Test_Data.tar.gz` supplies the vendor input. Its generated `.dream3d` output is no longer an oracle. |
| Legacy comparison | Not run — DREAM.3D v6.5.171 has no H5OINA importer. No legacy comparison date applies. |
| Bug flags | None. |
| V&V phase | Complete. Analytical fixtures and regression checks are encoded. |

## Summary

`ReadH5OinaDataFilter` imports Oxford Instruments AZtec H5OINA scans into one Image Geometry. Analytical fixtures, invariants, and independent vendor-file readback verify the imported values. All pass in CI; the fixed defects are listed below.

## Algorithm Relationship

*Classification:* New filter

*Evidence:* DREAM.3D v6.5.171 has no H5OINA importer; `ReadH5OinaDataFilter.cpp` has no SIMPL conversion function, and the legacy UUID mapping has no entry for this filter.

## Oracle

*Class:* 1 (Analytical) and 4 (Invariant), with 2 (Independent readback) for the vendor input.

*Applied:* Explicit scan specifications define geometry, cell and ensemble values, conversions, and errors; raw HDF5 readback supplies the vendor-file values independently of H5OINAReader.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ReadH5OinaDataTest.cpp::Class 1 Analytical Oracle`, `Multi-Scan Slab Placement`, and `Multi-Scan Hexagonal Alignment` use three base fixtures; `Conversion Option Combinations` adds Hexagonal-Low values, and `Real AZtec File Readback` checks one vendor input. All pass in CI.

*Second-engineer review:* Pending second-engineer review.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `ReadH5OinaDataFilter-D1` through `D13` and `D15` through `D18`. The fix for `ReadH5OinaDataFilter-D14` will be in the DREAM3D-NX release after v7.4.2.

See `vv/deviations/ReadH5OinaDataFilter.md`, Part 2, for the causes and migration guidance.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ReadH5OinaDataFilter-D1` | Hexagonal alignment added 30 radians instead of 30 degrees to phi2. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Adds pi/6 radians with a double-precision intermediate. |
| `ReadH5OinaDataFilter-D2` | Later scans overwrote earlier Euler values. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Multiplies the destination tuple offset by three Euler components. |
| `ReadH5OinaDataFilter-D3` | Alignment repeatedly modified the first scan and skipped later scans. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Applies alignment once to each point in the current scan slab. |
| `ReadH5OinaDataFilter-D4` | Pattern import reported missing data when the reader did not support pattern import. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Rejects pattern import explicitly with error `-9583` and identifies an alternative. |
| `ReadH5OinaDataFilter-D5` | The gamma lattice-angle slot contained beta. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Copies the third lattice angle into the gamma slot. |
| `ReadH5OinaDataFilter-D6` | H5OINA lattice angles used radians while the shared phase model used degrees. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Converts lattice angles to degrees with a double-precision intermediate. |
| `ReadH5OinaDataFilter-D7` | Missing lattice datasets caused invalid access during preflight. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Validates required phase data before indexing it. |
| `ReadH5OinaDataFilter-D8` | Phase groups could exceed ensemble bounds or give different meanings to a shared phase index. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Validates phase indices, counts, and definitions in every selected scan. |
| `ReadH5OinaDataFilter-D9` | The Stacking Order setting had no effect. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Reverses the selected scan list for High To Low stacking. |
| `ReadH5OinaDataFilter-D10` | Reader failures could have empty explanations or lose the specific inner error. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Stores the composed error text and includes the inner failure in wrapper messages. |
| `ReadH5OinaDataFilter-D11` | Invalid cell counts could become unsigned dimensions; zero columns returned inconsistent error codes. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Validates both signed cell counts before conversion and returns the stored reader error code. |
| `ReadH5OinaDataFilter-D12` | A non-numeric phase-group name caused an exception after opening the HDF5 group. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Validates the complete positive integer name before opening the group. |
| `ReadH5OinaDataFilter-D13` | Non-positive or non-finite spacing could create invalid geometry. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Requires finite, positive X, Y, and Z spacing. |
| `ReadH5OinaDataFilter-D14` | EDAX alignment skipped Hexagonal-Low phases. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Applies pi/6 to both Hexagonal-High and Hexagonal-Low phases. |
| `ReadH5OinaDataFilter-D15` | Different scan grids could cause incomplete copies or reads beyond a scan buffer. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Rejects incompatible scan dimensions or step sizes with `-9585`. |
| `ReadH5OinaDataFilter-D16` | A missing later scan passed preflight and failed after earlier scans had been copied. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Checks every selected scan name before execution and reports `-9586`. |
| `ReadH5OinaDataFilter-D17` | Dataset lengths inconsistent with the header could cause out-of-bounds reads or discarded values. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Rejects dataset extents that differ from the required point and component counts. |
| `ReadH5OinaDataFilter-D18` | An out-of-range cell Phase value could index beyond CrystalStructures. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Validates every cell Phase value before storing it or using it for alignment. |

## Code path coverage

29 of 46 paths exercised. Each uncovered row states the missing fixture or the earlier guard that prevents direct coverage.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadH5OinaData.cpp` (360 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ReadH5OinaDataFilter.cpp` (390 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/IEbsdOemReader.hpp` (193 lines), and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EbsdReaderUtilities.hpp` (332 lines).

Test names omit the common `OrientationAnalysis::ReadH5OinaDataFilter: ` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Reject non-positive or non-finite Z spacing (`-9580`) | `Parameter Rejections` |
| 2 | Preflight | Reject an empty scan selection (`-9581`) | `Parameter Rejections` |
| 3 | Preflight | Reject pattern import (`-9583`) | `Parameter Rejections` |
| 4 | Preflight | Fail while listing scans (`-9582`) | *Not directly tested. Requires a present file that cannot be read.* |
| 5 | Preflight | Reject a selected scan that is not in the file (`-9586`) | `Missing Scan Name rejected (-9586)` |
| 6 | Preflight | Fail while reading the first scan header (`-9582`) | `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)`; `Invalid Phase Group Name rejected (-9582)` |
| 7 | Preflight | Reject cell counts below one (`-9584`) | `Invalid Cell Counts rejected (-9584)` |
| 8 | Preflight | Reject non-positive or non-finite X or Y spacing (`-9591`) | `Invalid Scan Spacing rejected (-9591)` |
| 9 | Preflight | Reject a phase index outside the ensemble bounds (`-9587`) | `Phase Index Out Of Range rejected (-9587)` |
| 10 | Preflight | Fail while reading a later scan header (`-9582`) | `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` |
| 11 | Preflight | Reject a later scan with a different grid (`-9585`) | `Scan Header Mismatch rejected (-9585)` |
| 12 | Preflight | Reject a later scan with a different phase count (`-9589`) | `Scan Phase Count Mismatch rejected (-9589)` |
| 13 | Preflight | Reject a later scan with different phase definitions (`-9590`) | `Scan Phase Definition Mismatch rejected (-9590)` |
| 14 | Preflight | Create geometry dimensions, spacing, origin, and cell matrix | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 15 | Preflight | Create the ensemble matrix and three ensemble arrays | `Class 1 Analytical Oracle` |
| 16 | Preflight | Create nine cell arrays and select the Phase type | `Conversion Option Combinations` |
| 17 | Preflight | Generate display-only scan and phase information | *Not directly tested. The values are shown only in preflight.* |
| 18 | Execute | Propagate an EbsdLib data-read failure (`-8970`) | `EbsdLib Error Passthrough - Missing Data Column (-8970)` |
| 19 | Execute | Reject an empty phase vector (`-8971`) | *Not directly tested. H5OINAReader rejects this file during preflight first.* |
| 20 | Execute | Initialize ensemble tuple 0 and fill phase tuples | `Class 1 Analytical Oracle`; `Real AZtec File Readback` |
| 21 | Execute | Apply Low-to-High or High-to-Low stacking | `Stacking Order` |
| 22 | Copy | Fail when the file cannot be reopened (`-34971`) | *Not directly tested. Requires file replacement during execution.* |
| 23 | Copy | Skip an extent probe that cannot inspect a dataset | *Not directly tested. The reader rejects a missing required dataset first.* |
| 24 | Copy | Reject a dataset extent that differs from the header (`-34971`) | *Not directly tested. The current reader rejects the short-data fixture before this guard.* |
| 25 | Copy | Copy four uint8 arrays into the current scan slab | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 26 | Copy | Copy Euler values with a three-component offset | `Multi-Scan Slab Placement` |
| 27 | Copy | Reject a Phase value outside the ensemble bounds (`-34972`) | `Out-of-Range Phase Value rejected (-34972)` |
| 28 | Copy | Widen Phase to `int32` or retain `uint8` | `Conversion Option Combinations` |
| 29 | Copy | Copy MAD, X, and Y into the current scan slab | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 30 | Copy | Align Hexagonal-High and Hexagonal-Low points in each slab for both Phase types | `Conversion Option Combinations`; `Multi-Scan Hexagonal Alignment` |
| 31 | Execute/Copy | Cancel before or after scan operations, during copy/alignment batches, or before alignment | *Not directly tested. Requires cancellation injection.* |
| 32 | EbsdLib | Reject a noncanonical phase-group name (`-90034`, surfaced as `-9582`) | `Invalid Phase Group Name rejected (-9582)` |
| 33 | EbsdLib | Reject an unreadable phase group, lattice dimensions, or Laue group | *Not directly tested. The fixture writer cannot create the unreadable-group case; the other required-dataset cases use the same checked path as lattice angles.* |
| 34 | Bounded I/O | Copy full 65,536-value batches, partial tails, and a second scan | *Not directly tested. The boundary fixture is hidden from default test selection.* |
| 35 | EbsdLib | Read version 5.0, version 2.0, or no Format Version dataset | `Format Version Variants` |
| 36 | Copy | Disable EDAX alignment and preserve Euler values | `Conversion Option Combinations` |
| 37 | Copy | Leave cubic and unindexed Euler values unchanged with alignment enabled | `Conversion Option Combinations` |
| 38 | Store I/O | Propagate a raw-array batch write error | *Not directly tested. Requires an injected destination-store failure.* |
| 39 | Store I/O | Propagate a CrystalStructures read error during alignment | *Not directly tested. Requires an injected ensemble-store failure.* |
| 40 | Store I/O | Propagate a Phase batch read error during alignment | *Not directly tested. Requires an injected Phase-store failure.* |
| 41 | Store I/O | Propagate an Euler batch read error during alignment | *Not directly tested. Requires an injected Euler-store read failure.* |
| 42 | Store I/O | Propagate an Euler batch write error after alignment | *Not directly tested. Requires an injected Euler-store write failure.* |
| 43 | Store I/O | Propagate an int32 Phase batch write error | *Not directly tested. Requires an injected Phase-store write failure.* |
| 44 | Preflight | Reject invalid spacing in a later scan (`-9591`) | *Not directly tested. The spacing fixture modifies only the first scan.* |
| 45 | Preflight | Reject a later phase index absent from the first phase list (`-9587`) | *Not directly tested. Canonical unique names, equal counts, and range validation prevent this case.* |
| 46 | EbsdLib | Reject short Data datasets before the filter extent guard (`-8970`) | `Dataset Extent Mismatch rejected (-8970)` |

## Test inventory

Names below omit the common `OrientationAnalysis::ReadH5OinaDataFilter: ` prefix. The 20 ordinary cases are the CI test set. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `Class 1 Analytical Oracle` | new-for-V&V | Checks geometry, all nine cell arrays, three ensemble arrays, exact alignment values, and tuple invariants. |
| `Conversion Option Combinations` | new-for-V&V | Checks all four alignment/Phase-type combinations, both hexagonal classes, and unchanged cubic and unindexed points. |
| `Multi-Scan Slab Placement` | new-for-V&V | Checks all nine arrays across two cubic scans with distinct values. |
| `Multi-Scan Hexagonal Alignment` | new-for-V&V | Checks both hexagonal classes in both scan slabs with exact Euler expectations. |
| `Format Version Variants` | new-for-V&V | Checks equivalent Euler output for version 5.0, version 2.0, and an absent version dataset. |
| `Parameter Rejections` | new-for-V&V | Checks zero and NaN Z spacing, empty selection, and unsupported pattern import. |
| `Invalid Cell Counts rejected (-9584)` | new-for-V&V | Checks zero X, zero Y, and negative X cell counts. |
| `Scan Header Mismatch rejected (-9585)` | new-for-V&V | Checks different grid dimensions and X step values in the second scan. |
| `Missing Scan Name rejected (-9586)` | new-for-V&V | Checks missing first and second scan names. |
| `Phase Index Out Of Range rejected (-9587)` | new-for-V&V | Checks phase-group index 7 with one declared phase in first and later scans. |
| `Invalid Phase Group Name rejected (-9582)` | new-for-V&V | Checks the non-numeric group name phase-one and file/group error context. |
| `Scan Phase Count Mismatch rejected (-9589)` | new-for-V&V | Checks later scans with more or fewer phase groups. |
| `Scan Phase Definition Mismatch rejected (-9590)` | new-for-V&V | Checks material name, Laue group, space group, lattice dimensions, and lattice angles. |
| `Invalid Scan Spacing rejected (-9591)` | new-for-V&V | Checks zero X step, negative Y step, and NaN X step. |
| `Dataset Extent Mismatch rejected (-8970)` | new-for-V&V | Checks the reader rejection `-101100`, wrapped as `-8970`, for 8 data tuples under a 16-point header. |
| `Out-of-Range Phase Value rejected (-34972)` | new-for-V&V | Checks invalid Phase values and error context before ensemble indexing. |
| `EbsdLib Error Passthrough - Missing Data Column (-8970)` | new-for-V&V | Checks execute error propagation for a missing required Bands dataset. |
| `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` | new-for-V&V | Checks missing lattice angles in first and later scan headers without a crash. |
| `Stacking Order` | new-for-V&V | Checks Euler, Band Contrast, and Phase values for both scan orders. |
| `genuine HDF5 batch-tail and slab oracle` | new-for-V&V | Hidden boundary fixture: checks all nine cell arrays at batch ends, tails, and scan transitions in two 65,537-point scans. |
| `Real AZtec File Readback` | kept | Uses the vendor input and independent H5Lite readback of all nine cell arrays; the generated exemplar is no longer used. |

## Exemplar archive

- **Archive:** `H5Oina_Test_Data.tar.gz`
- **SHA512:** `346573ac6b96983680078e8b0a401aa25bd9302dff382ca86ae4e503ded6db3947c4c5611ee603db519d8a8dc6ed35b044a7bfea9880fade5ab54479d140ea03`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ReadH5OinaDataFilter.md`

## Deviations from DREAM.3D v6.5.171

DREAM.3D v6.5.171 has no H5OINA importer. There are no Part 1 entries; all defect entries are in Part 2 of `vv/deviations/ReadH5OinaDataFilter.md`.

| Deviation | Observed difference |
|-----------|---------------------|
| None | No equivalent legacy filter exists. |
