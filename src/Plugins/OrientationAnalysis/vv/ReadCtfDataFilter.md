# V&V Report: ReadCtfDataFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `7751923c-afb9-4032-8372-8078325c69a4` |
| SIMPLNX Human Name | Read Oxford Instr. EBSD Data (.ctf) |
| DREAM.3D v6.5.171 equivalent | `ReadCtfData` (SIMPL UUID `d1df969c-0428-53c3-b61d-99ea2bb6da28`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-24; second engineer: Jared Duffey, 2026-07-28 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `ReadCtfData` in DREAM.3D v6.5.171. SIMPLNX preserves phase zero, validates malformed input, and aligns both hexagonal Laue classes. |
| Oracle (confirmed) | Classes 1 and 4 use hand-authored CTF fixtures and exact expected values in `test/ReadCtfDataTest.cpp`. All pass in CI. |
| Code paths enumerated | 18 of 33 exercised by the default test cases. Remaining paths include defensive guards, cancellation, display text, large-block processing, and I/O failures. |
| Tests today | 13 test cases: 3 analytical cases, 8 error cases, 1 SIMPL conversion case, and 1 hidden block-boundary case. The 12 default cases cover geometry, arrays, conversions, and input errors. |
| Exemplar archive | None. The retired `6_6_read_ctf_data_2.tar.gz` was replaced by inline analytical fixtures. |
| Legacy comparison | Not run on the current algorithm. The DREAM.3D v6.5.171 comparison dated 2026-07-24 predates the current hexagonal-alignment correction. |
| Bug flags | None |
| V&V phase | COMPLETE. All default tests pass in CI. |

## Summary

`ReadCtfDataFilter` imports an Oxford/HKL CTF file into an Image Geometry with cell and phase data. Classes 1 and 4 check geometry, copied arrays, phase-zero preservation, and all four angle-conversion modes. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps `ReadCtfData` to this filter; both use phase loading, Euler interleaving, and column copying, with the differences listed in the deviations sidecar.

## Oracle

*Class:* 1 (Analytical) and 4 (Invariant).

*Applied:* Hand-authored CTF text defines geometry and copied values; double-precision angle formulas define float32 expectations, and invariants check phase preservation and ensemble sizing.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ReadCtfDataTest.cpp` — `Class 1 Analytical Oracle`, `Euler Conversion Combinations`, and `3D Multi-Slice CTF` use three fixture layouts, including four conversion modes and two slice-spacing variants. All pass in CI.

*Second-engineer review:* Jared Duffey, 2026-07-28; separate oracle re-derivation skipped because the expectations follow direct copies and elementwise formulas.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `ReadCtfDataFilter-D3`, `ReadCtfDataFilter-D4`, `ReadCtfDataFilter-D6`, and `ReadCtfDataFilter-D7`. The fix for `ReadCtfDataFilter-D5` will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ReadCtfDataFilter-D3` | Missing columns or invalid phase data could cause invalid memory access or incorrect output. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Reject missing columns, empty phase definitions, invalid phase values, invalid cell counts, and changed input sizes. |
| `ReadCtfDataFilter-D4` | Multi-slice CTF imports silently kept only the first slice. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Use the header's Z extent and positive Z spacing; default missing spacing to 1. |
| `ReadCtfDataFilter-D5` | EDAX alignment omitted the 6/m hexagonal class. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | Add 30 degrees to phi2 for both 6/m and 6/mmm before optional radian conversion. |
| `ReadCtfDataFilter-D6` | Float32 intermediates changed the rounded Euler-angle results. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Use double-precision intermediates before storing each float32 result. |
| `ReadCtfDataFilter-D7` | Some reader failures produced error code zero. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Use the negative reader return code when its error-code member is not negative. |

## Code path coverage

18 of 33 paths exercised. The count uses default test cases with asserted results.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadCtfData.cpp` (287 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ReadCtfDataFilter.cpp` (277 lines), and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EbsdReaderUtilities.hpp` (332 lines).

Test names below omit the common `OrientationAnalysis::ReadCtfDataFilter: ` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Header read fails; propagate its error with a return-code fallback | *Not directly tested. No fixture fails the header read after file-path validation.* |
| 2 | Preflight | X cell count below 1 → `-19604` | `Invalid Cells Preflight Error (-19604)` / `Zero XCells` |
| 3 | Preflight | Y cell count below 1 → `-19604` | *Not directly tested. The invalid-dimension fixture sets X to zero.* |
| 4 | Preflight | Negative Z cell count → `-19604` | `Invalid Cells Preflight Error (-19604)` / `Negative ZCells` |
| 5 | Preflight | Create a 2D geometry with origin zero and micrometer units | `Class 1 Analytical Oracle` |
| 6 | Preflight | Create a multi-slice geometry with positive Z spacing | `3D Multi-Slice CTF` / `ZStep 0.75` |
| 7 | Preflight | Missing Z spacing → default to 1 | `3D Multi-Slice CTF` / `ZStep absent` |
| 8 | Preflight | Negative Z spacing → default to 1 | *Not directly tested. No fixture supplies negative ZStep.* |
| 9 | Preflight | Create int32 and float32 cell arrays | `Class 1 Analytical Oracle` — all nine cell arrays |
| 10 | Ensemble | Allocate phase count plus one; fill invalid slot and phase metadata | `Class 1 Analytical Oracle` — all three ensemble arrays |
| 11 | Preflight | Generate scan and phase display text | *Not directly tested. Tests do not assert display strings.* |
| 12 | Read | Propagate reader failures, including the fallback code | `EbsdLib Error Passthrough - Zero Step (-102)`, `EbsdLib Error Passthrough - Truncated Data (-105)`, `EbsdLib Error Passthrough - Unknown Column (-107)`, `Reader/Geometry Cell Count Mismatch rejected (-101105)` |
| 13 | Ensemble | No phase definitions → `-19600` | `Empty Phases rejected (-19600)` |
| 14 | Ensemble | Phase index exceeds preflight allocation → `-19605` | *Not directly tested. Requires a file change between preflight and the algorithm read.* |
| 15 | Copy | Reader supplies fewer cells than the geometry → `-19603` | *Not directly tested. The zero-Z fixture now fails earlier with reader error `-101105`.* |
| 16 | Copy | Missing Phase or Euler column → `-19601` | *Not directly tested. The missing-column fixture removes BS only.* |
| 17 | Copy | Missing pass-through column → `-19601` | `Missing Data Column rejected (-19601)` — checks BS in the message |
| 18 | Copy | Phase equals the ensemble tuple count → `-19602` | `Out-of-Range Phase Value rejected (-19602)` — phase 3 with two defined phases |
| 19 | Copy | Negative phase → `-19602` | *Not directly tested. The invalid-phase fixture uses a positive value.* |
| 20 | Copy | Preserve phase zero and interleave Euler components | `Class 1 Analytical Oracle`; `3D Multi-Slice CTF` |
| 21 | Convert | Degrees, no hexagonal alignment | `Euler Conversion Combinations` / `Degrees, No Hex Alignment` |
| 22 | Convert | Degrees with alignment for 6/m and 6/mmm; preserve cubic and unindexed values | `Euler Conversion Combinations` / `Degrees, Hex Alignment` |
| 23 | Convert | Radians, no hexagonal alignment | `Euler Conversion Combinations` / `Radians, No Hex Alignment` |
| 24 | Convert | Radians with alignment for 6/m and 6/mmm; preserve phase values | `Euler Conversion Combinations` / `Radians, Hex Alignment` |
| 25 | Copy | Copy four int32 and three float32 pass-through columns | `Class 1 Analytical Oracle` — checks every value |
| 26 | Copy | Process a full 65,536-point block and a tail | *Not directly tested. Default cases have fewer than 65,536 points; the hidden boundary case defines this fixture.* |
| 27 | Cancellation | Return at any of four cancellation checks | *Not directly tested. Requires cancellation injection.* |
| 28 | I/O | Phase bulk write fails | *Not directly tested. Requires an injected store write failure.* |
| 29 | I/O | Crystal-structure cache read fails | *Not directly tested. Requires an injected store read failure.* |
| 30 | I/O | Euler block write fails | *Not directly tested. Requires an injected store write failure.* |
| 31 | I/O | Int32 pass-through write fails | *Not directly tested. Requires an injected store write failure.* |
| 32 | I/O | Float32 pass-through write fails | *Not directly tested. Requires an injected store write failure.* |
| 33 | Conversion | Convert SIMPL UUID/name and arguments | `SIMPL Backwards Compatibility` |

## Test inventory

All cases are in `src/Plugins/OrientationAnalysis/test/ReadCtfDataTest.cpp`. The 12 default cases run on Linux, macOS, and Windows. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ReadCtfDataFilter: Class 1 Analytical Oracle` | new-for-V&V | Checks 3×2×1 geometry, spacing, origin, units, all nine cell arrays, all three ensemble arrays, and tuple dimensions. |
| `OrientationAnalysis::ReadCtfDataFilter: Euler Conversion Combinations` | new-for-V&V | Checks 18 Euler values, crystal structures, and unchanged phases in `Degrees, No Hex Alignment`, `Degrees, Hex Alignment`, `Radians, No Hex Alignment`, and `Radians, Hex Alignment`. Includes 6/m and 6/mmm witnesses for D5 and rounding witnesses for D6. |
| `OrientationAnalysis::ReadCtfDataFilter: 3D Multi-Slice CTF` | new-for-V&V | Checks 2×2×2 dimensions, phases, Bands, and EulerAngles in `ZStep 0.75` and `ZStep absent`; covers D4. |
| `OrientationAnalysis::ReadCtfDataFilter: Empty Phases rejected (-19600)` | new-for-V&V | Checks the explicit error for a file with no phase definitions; covers D3. |
| `OrientationAnalysis::ReadCtfDataFilter: Missing Data Column rejected (-19601)` | new-for-V&V | Removes BS and checks the error code and column name; covers D3. |
| `OrientationAnalysis::ReadCtfDataFilter: Out-of-Range Phase Value rejected (-19602)` | new-for-V&V | Uses the first invalid phase value, 3; checks the error code and valid range `[0, 2]`; covers D3. |
| `OrientationAnalysis::ReadCtfDataFilter: Reader/Geometry Cell Count Mismatch rejected (-101105)` | new-for-V&V | Sets ZCells to zero and checks the reader's raster-grid error `-101105`. Does not reach `-19603`. |
| `OrientationAnalysis::ReadCtfDataFilter: Invalid Cells Preflight Error (-19604)` | new-for-V&V | Checks `Zero XCells` and `Negative ZCells` preflight rejections; covers D3. |
| `OrientationAnalysis::ReadCtfDataFilter: EbsdLib Error Passthrough - Zero Step (-102)` | new-for-V&V | Sets XStep to zero and checks the returned reader error; covers D7. |
| `OrientationAnalysis::ReadCtfDataFilter: EbsdLib Error Passthrough - Truncated Data (-105)` | new-for-V&V | Supplies four of six data rows and checks the premature-end error. |
| `OrientationAnalysis::ReadCtfDataFilter: EbsdLib Error Passthrough - Unknown Column (-107)` | new-for-V&V | Replaces BS with Bogus and checks the column-allocation error. |
| `OrientationAnalysis::ReadCtfDataFilter: real HDF5 65536-block conversion oracle` | new-for-V&V | Hidden case. Requires HDF5 stores; checks Euler values across a 65,536-point boundary, phase-zero preservation, Bands, and array storage formats. |
| `OrientationAnalysis::ReadCtfDataFilter: SIMPL Backwards Compatibility` | new-for-V&V | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check filter identity, comments, and all six converted arguments. |

## Exemplar archive

- **Archive:** None. `6_6_read_ctf_data_2.tar.gz` is retired; current fixtures are inline.
- **SHA512:** Not applicable. `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` has no ReadCtfData archive registration.
- **Provenance:** [src/Plugins/OrientationAnalysis/vv/provenance/ReadCtfDataFilter.md](provenance/ReadCtfDataFilter.md)

## Deviations from DREAM.3D v6.5.171

See [vv/deviations/ReadCtfDataFilter.md](deviations/ReadCtfDataFilter.md) for the root cause, affected users, and recommendation. This table lists only Part 1 entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `ReadCtfDataFilter-D1` | SIMPLNX preserves unindexed phase zero; DREAM.3D remaps it to phase 1. |
| `ReadCtfDataFilter-D2` | SIMPLNX does not apply phase 1's hexagonal correction to unindexed points. |
| `ReadCtfDataFilter-D3` | SIMPLNX rejects malformed input that can crash DREAM.3D or cause invalid memory access. |
| `ReadCtfDataFilter-D5` | SIMPLNX applies EDAX alignment to both 6/m and 6/mmm; DREAM.3D applies it only to 6/mmm. |
