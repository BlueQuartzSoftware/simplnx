# V&V Report: ErodeDilateMaskFilter

<!-- Write the report in ASD-STE100 Simplified Technical English. Use short sentences, active voice, one meaning per sentence, and consistent technical terms. -->

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `cab66cd1-f64c-42b4-8f94-18f0835a967f` |
| SIMPLNX Human Name | Erode/Dilate Mask |
| DREAM.3D v6.5.171 equivalent | `ErodeDilateMask` (Processing plugin, SIMPL UUID `4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-08-20; second engineer: Nathan Young, 2026-08-25 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `ErodeDilateMask` in DREAM.3D v6.5.171. SIMPLNX uses rolling slice buffers for the same synchronous face-neighbour operations. |
| Oracle (confirmed) | Class 1 analytical masks are encoded in 8 test cases in `test/ErodeDilateMaskTest.cpp`. All pass in CI. |
| Code paths enumerated | 19 of 27 exercised by the mapped test cases. All-directions-off, six I/O failure paths and external-store execution lack direct tests in this file. |
| Tests today | 14 test cases: 8 analytical, 2 exemplar comparisons, 2 validation, 1 SIMPL conversion and 1 hidden data generator. |
| Exemplar archive | `6_6_erode_dilate_test.tar.gz` supplies inputs and expected outputs for the two exemplar comparisons. It does not establish parity with DREAM.3D v6.5.171. |
| Legacy comparison | Not run for the current algorithm. The recorded comparison with DREAM.3D v6.5.171 predates the change to rolling slice buffers. |
| Bug flags | None. |
| V&V phase | Analytical fixtures and regressions for both fixed defects are encoded. All pass in CI. A legacy comparison with the current algorithm remains to be done. |

## Summary

`ErodeDilateMaskFilter` grows or shrinks a boolean cell mask by one face-neighbour layer per iteration on the selected axes. Class 1 fixtures check exact masks, and two archive tests compare reconstruction data with stored exemplars. The filter now rejects non-positive iteration counts and applies each direction flag.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; the DREAM.3D v6.5.171 source uses the same six-neighbour rules, and SIMPLNX uses rolling slice buffers for storage access.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Hand-derived true-index sets and four-cell literal arrays specify synchronous dilation, erosion, boundary clipping and axis restrictions.

*Encoded:* `src/Plugins/SimplnxCore/test/ErodeDilateMaskTest.cpp` — seven `Class 1` cases use three fixture layouts; `SimplnxCore::ErodeDilateMaskFilter: Literal One-Dimensional Morphology` adds two axis layouts and ten sections with exact mask and unchanged-sibling checks. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-08-25.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ErodeDilateMaskFilter-D1` | A non-positive iteration count returned success without changing the mask. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Preflight rejects values below 1 with error `-14701`. |
| `ErodeDilateMaskFilter-D2` | Disabled X, Y or Z directions still participated in morphology. | DREAM3D-NX v7.4.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. | The algorithm applies each direction flag to its two face neighbours. |

## Code path coverage

19 of 27 paths exercised. The uncovered rows identify the remaining gaps.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateMask.cpp` (255 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ErodeDilateMaskFilter.cpp` (171 lines), and `src/simplnx/Utilities/NeighborUtilities.hpp` (634 lines).

Test names below omit the common `SimplnxCore::ErodeDilateMaskFilter` prefix. The phases are setup, scan, I/O and preflight.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Setup | Allocate read/write slice buffers and seed both from the mask | `Class 1 dilate, one iteration` |
| 2 | I/O | Write the completed mask back within each iteration | `Class 1 dilate, two iterations` |
| 3 | Setup | Read the previous iteration's output for a second pass | `Class 1 dilate, two iterations`; `Literal One-Dimensional Morphology` / `Dilate two iterations` |
| 4 | Scan | A true voxel skips the neighbour scan | `Class 1 dilate, one iteration`; `Class 1 erode honours direction flags` |
| 5 | Scan | Reject an out-of-bounds face neighbour | `Class 1 dilate from a boundary seed`; `Literal One-Dimensional Morphology` |
| 6 | Scan | Disable X neighbours | `Class 1 dilate honours direction flags`; `Literal One-Dimensional Morphology` / `X` / `Disabled active axis erosion` |
| 7 | Scan | Dilate a false voxel that has a true enabled neighbour | `Class 1 dilate, one iteration`; `Literal One-Dimensional Morphology` / `Z` / `Dilate one iteration` |
| 8 | Scan | Erode a true neighbour of a false voxel, within or across slices | `Class 1 erode, one iteration`; `Class 1 erode honours direction flags` / `All directions on` |
| 9 | Scan | A false neighbour causes no write | `Class 1 dilate, one iteration` |
| 10 | Preflight | Reject `NumIterations < 1` with `-14701` | `Invalid Number of Iterations` — values 0 and -2 |
| 11 | Scan | Disable Y neighbours | `Class 1 dilate with Y direction off` |
| 12 | Scan | Disable Z neighbours | `Class 1 erode honours direction flags` / `Z direction off`; `Literal One-Dimensional Morphology` / `Z` / `Disabled active axis` |
| 13 | Scan | Disable all three directions; preserve the mask | *Not directly tested. No case sets all three flags to false.* |
| 14 | Setup | Read the second slice when `dims[2] > 1` | `Class 1 erode honours direction flags` |
| 15 | Scan | Rotate both windows; read another slice when available and skip that read at the last slice | `Literal One-Dimensional Morphology` / `Z` |
| 16 | I/O | Write the completed previous slice when `zIdx > 0` | `Class 1 erode honours direction flags` |
| 17 | I/O | Flush a single-slice image | `Class 1 dilate, one iteration` |
| 18 | I/O | Flush the last slice of a multi-slice image | `Class 1 erode honours direction flags`; `Literal One-Dimensional Morphology` / `Z` |
| 19 | Preflight | Accept a positive iteration count and mark the mask modified | `Class 1 dilate, one iteration` — preflight succeeds; execution checks mask values |
| 20 | I/O | Propagate failure while reading the first slice | *Not directly tested. Requires an injected store read failure.* |
| 21 | I/O | Propagate failure while reading the second initial slice | *Not directly tested. Requires an injected store read failure.* |
| 22 | I/O | Propagate failure while reading a slice after window rotation | *Not directly tested. Requires an injected store read failure.* |
| 23 | I/O | Propagate failure while writing a completed previous slice | *Not directly tested. Requires an injected store write failure.* |
| 24 | I/O | Propagate failure while flushing a single-slice image | *Not directly tested. Requires an injected store write failure.* |
| 25 | I/O | Propagate failure while flushing the last multi-slice image slice | *Not directly tested. Requires an injected store write failure.* |
| 26 | I/O | Read and write a mask held in an external store | *Not directly tested. No case in this file requires an external mask store.* |
| 27 | Preflight | Reject a mask tuple count that differs from the selected geometry's cell count with `-14702` | `Geometry cell tuple count` — 0, 11 and 13 tuples for 12 cells |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ErodeDilateMaskFilter: Generate Test Data` | kept | Hidden (`[.GenerateTestData]`). Writes 20×20×20 and 200×200×200 patterned masks; does not execute morphology or assert its output. |
| `SimplnxCore::ErodeDilateMaskFilter(Dilate)` | kept | Runs two iterations with all axes enabled and in-memory storage. Compares cell arrays with `Exemplar Mask Dilate` and checks tuple dimensions. |
| `SimplnxCore::ErodeDilateMaskFilter(Erode)` | kept | Runs two iterations with all axes enabled and in-memory storage. Compares cell arrays with `Exemplar Mask Erode` and checks tuple dimensions. |
| `SimplnxCore::ErodeDilateMaskFilter: Literal One-Dimensional Morphology` | kept | Checks exact four-cell masks and unchanged sibling values on X and Z lines. Each axis has `Dilate one iteration`, `Dilate two iterations`, `Erode one iteration`, `Disabled active axis` and `Disabled active axis erosion` sections. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, one iteration` | new-for-V&V | On a 5×5×1 centre seed, checks all 25 cells against `{7, 11, 12, 13, 17}`. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 erode, one iteration` | new-for-V&V | On the centre seed, checks that all 25 cells become false. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate honours direction flags` | new-for-V&V | Disables X and checks the vertical bar `{7, 12, 17}`. Covers `ErodeDilateMaskFilter-D2`. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate with Y direction off` | new-for-V&V | Disables Y and checks the horizontal bar `{11, 12, 13}`. Covers `ErodeDilateMaskFilter-D2`. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate, two iterations` | new-for-V&V | Checks the exact 13-cell diamond after two synchronous passes on the centre seed. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 dilate from a boundary seed` | new-for-V&V | Seeds the x = 0 face and checks `{5, 10, 11, 15}` across all 25 cells, including the false row-wrap cell at index 9. |
| `SimplnxCore::ErodeDilateMaskFilter: Class 1 erode honours direction flags` | new-for-V&V | Checks all 27 hollow-cube cells in `All directions on` and `Z direction off`: 20 and 22 cells remain true. Covers `ErodeDilateMaskFilter-D2`. |
| `SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations` | new-for-V&V | Checks invalid preflight and error `-14701` for 0 and -2. Covers `ErodeDilateMaskFilter-D1`. |
| `SimplnxCore::ErodeDilateMaskFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check the converted UUID and all arguments; they do not execute the filter. |
| `SimplnxCore::ErodeDilateMaskFilter: Geometry cell tuple count` | new-for-V&V | Checks preflight error `-14702` for 0, 11 and 13 mask tuples against 12 cells, with the array path and both counts in the message. Accepts 12 tuples in another Attribute Matrix with a different tuple shape. |

## Exemplar archive

- **Archive:** `6_6_erode_dilate_test.tar.gz`
- **SHA512:** `5f0773e5d296936effbb2239965f5847e7c18533b0a2c3ec6a1d6a83b03417e5b459cce29808c8e0273613b3b6fa032c675e84926eb35d8da8a6ddc0641a0ef5`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/6_6_erode_dilate_test.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ErodeDilateMaskFilter.md` for the root cause, affected users, and recommendation. This table lists only Part 1; the fixed direction-flag defect is in Part 2.

| Deviation | Observed difference |
|-----------|---------------------|
| `ErodeDilateMaskFilter-D1` | Retired 2026-08-19: DREAM3D-NX v7.0.0 through v7.4.2 accepted non-positive iteration counts, while DREAM.3D v6.5.171 rejected them. Both now reject these values; the error codes differ (`-14701` and `-5555`). |
