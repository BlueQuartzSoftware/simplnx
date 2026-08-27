# V&V Report: ReadH5OinaDataFilter

| | |
|---|---|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| DREAM.3D 6.5.171 equivalent | **None.** |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Pending second-engineer PR review. |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **New filter, no legacy equivalent.** `ReadH5OinaDataFilter` was added in SIMPLNX and first shipped in DREAM3D-NX 7.0.0. |
| Oracle (confirmed) | **Class 1 analytical + Class 4 invariant, with Class 2 independent h5py readback.** The tests create small H5OINA inputs and compare all imported values with independently derived values. |
| Code paths enumerated | **26 of 33 paths exercised.** The uncovered paths require file-permission changes, file replacement during execution, cancel injection, or HDF5 objects that the fixture writer cannot create. |
| Tests today | **20 test cases and 9,943 assertions through ctest.** The tests cover data conversion, multi-scan stacking, phase definitions, malformed inputs, error propagation, and a real AZtec file. |
| Exemplar archive | **`H5Oina_Test_Data.tar.gz` retained as input only.** The vendor `.h5oina` file is used. The generated `.dream3d` output was retired as a circular oracle. |
| Legacy comparison | **Not applicable.** DREAM.3D 6.5.171 has no H5OINA importer. The independent h5py readback replaces the legacy comparison. |
| Bug flags | **Thirteen bugs resolved:** `ReadH5OinaDataFilter-D1` through `ReadH5OinaDataFilter-D13`. The affected released versions are DREAM3D-NX 7.0.0 through 7.4.1. |
| V&V phase | **COMPLETE** |

## Summary

`ReadH5OinaDataFilter` imports one or more Oxford Instruments AZtec H5OINA scans into one Image Geometry. Verification uses inline Class 1 analytical data, Class 4 invariants, and an independent Class 2 h5py readback of a vendor file. The V&V found and fixed thirteen defects in the filter and EbsdLib H5OINA reader.

## Dependency state

- EbsdLib version 3.1.2
- Intended DREAM3D-NX release: 7.5.0

## Algorithm Relationship

**New filter, no legacy equivalent.**

*Evidence:* DREAM.3D 6.5.171 contains no H5OINA importer. The filter was added to SIMPLNX by PR #700 in commit `a51dd5f3d` and has no SIMPL conversion function or legacy UUID mapping.

## Oracle

*Class:* **1 (Analytical) + 4 (Invariant)**, with **2 (Independent readback)** for the vendor file.

*Applied:* The tests write small H5OINA files from explicit fixture specifications. Expected geometry, cell arrays, ensemble arrays, unit conversions, stacking order, and rejection results are derived from those specifications. A separate h5py script reads the vendor file and derives its expected imported values without EbsdLib or SIMPLNX.

*Encoded:* `test/ReadH5OinaDataTest.cpp` contains 20 test cases. The archived `h5oina_oracle.py` script reproduces its Class 1 values and the vendor-file readback.

*Second-engineer review:* Pending PR review.

## Bugs found and fixed

This branch fixes all defects in this table. The fixes are intended for DREAM3D-NX 7.5.0. EbsdLib defects require EbsdLib 3.1.2.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|---|---|---|---|
| `ReadH5OinaDataFilter-D1` | The hexagonal alignment added 30 radians instead of 30 degrees to `phi2`. | DREAM3D-NX 7.0.0 through 7.4.1. | The filter adds 30 degrees expressed in radians with a double-precision intermediate. |
| `ReadH5OinaDataFilter-D2` | The Euler destination offset used tuples instead of elements. Later scans overwrote part of an earlier scan. | DREAM3D-NX 7.0.0 through 7.4.1. | The Euler copy multiplies the tuple offset by three components. |
| `ReadH5OinaDataFilter-D3` | Hexagonal alignment repeatedly modified the first scan and did not modify later scans. | DREAM3D-NX 7.0.0 through 7.4.1. | The alignment loop uses the current scan slab. |
| `ReadH5OinaDataFilter-D4` | Pattern import was exposed but could not succeed. | DREAM3D-NX 7.0.0 through 7.4.1. | Preflight reports that H5OINA pattern import is not supported and gives an alternative. |
| `ReadH5OinaDataFilter-D5` | The gamma lattice angle copied the beta angle. | DREAM3D-NX 7.0.0 through 7.4.1. | EbsdLib copies the third lattice angle into the gamma slot. |
| `ReadH5OinaDataFilter-D6` | H5OINA lattice angles were imported in radians while other EBSD readers use degrees. | DREAM3D-NX 7.0.0 through 7.4.1. | EbsdLib converts the lattice angles to degrees. |
| `ReadH5OinaDataFilter-D7` | A phase without a required lattice dataset caused invalid access and a process crash. | DREAM3D-NX 7.0.0 through 7.4.1. | EbsdLib validates the required phase datasets before it reads their values. |
| `ReadH5OinaDataFilter-D8` | Multi-scan phase groups could exceed the ensemble bounds or describe different phases at the same index. | DREAM3D-NX 7.0.0 through 7.4.1. | Preflight validates every phase index, phase count, and phase definition in every selected scan. |
| `ReadH5OinaDataFilter-D9` | The Stacking Order setting was accepted but ignored. | DREAM3D-NX 7.0.0 through 7.4.1. | The scan order now determines which scan is placed at Z = 0. |
| `ReadH5OinaDataFilter-D10` | H5OINA reader error messages were constructed but stored as empty strings. | DREAM3D-NX 7.0.0 through 7.4.1. | EbsdLib stores and forwards the complete error message. |
| `ReadH5OinaDataFilter-D11` | The reader did not validate both cell counts and returned an error code that differed from its stored code. | DREAM3D-NX 7.0.0 through 7.4.1. | EbsdLib validates rows and columns and returns the stored error code. |
| `ReadH5OinaDataFilter-D12` | A non-numeric phase-group name caused `std::stoi()` to throw and left an HDF5 group open. | DREAM3D-NX 7.0.0 through 7.4.1. | EbsdLib validates the complete group name before it opens the group. |
| `ReadH5OinaDataFilter-D13` | Non-positive or non-finite scan spacing could create an invalid Image Geometry. | DREAM3D-NX 7.0.0 through 7.4.1. | Preflight requires finite, positive X, Y, and Z spacing. |

## Code path coverage

26 of 33 paths are exercised.

Source: `Filters/ReadH5OinaDataFilter.cpp`, `Filters/Algorithms/ReadH5OinaData.cpp`, and the shared `utilities/IEbsdOemReader.hpp` read and ensemble-fill path.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | Preflight | Reject non-positive or non-finite Z spacing (`-9580`) | `Parameter Rejections` |
| 2 | Preflight | Reject an empty scan selection (`-9581`) | `Parameter Rejections` |
| 3 | Preflight | Reject pattern import (`-9583`) | `Parameter Rejections` |
| 4 | Preflight | Fail while listing scans (`-9582`) | *Not directly tested. Requires a present file that cannot be read.* |
| 5 | Preflight | Reject a selected scan that is not in the file (`-9586`) | `Missing Scan Name rejected` |
| 6 | Preflight | Fail while reading the first scan header (`-9582`) | `Missing Lattice Angles`; `Invalid Phase Group Name` |
| 7 | Preflight | Reject cell counts below one (`-9584`) | `Invalid Cell Counts rejected` |
| 8 | Preflight | Reject non-positive or non-finite X or Y spacing (`-9591`) | `Invalid Scan Spacing rejected` |
| 9 | Preflight | Reject a phase index outside the ensemble bounds (`-9587`) | `Phase Index Out Of Range rejected` |
| 10 | Preflight | Fail while reading a later scan header (`-9582`) | `Missing Lattice Angles` |
| 11 | Preflight | Reject a later scan with a different grid (`-9585`) | `Scan Header Mismatch rejected` |
| 12 | Preflight | Reject a later scan with a different phase count (`-9589`) | `Scan Phase Count Mismatch rejected` |
| 13 | Preflight | Reject a later scan with different phase definitions (`-9590`) | `Scan Phase Definition Mismatch rejected` |
| 14 | Preflight | Create geometry dimensions, spacing, origin, and cell matrix | Class 1 oracle; multi-scan tests |
| 15 | Preflight | Create the ensemble matrix and three ensemble arrays | Class 1 oracle |
| 16 | Preflight | Create nine cell arrays and select the Phase type | `Conversion Option Combinations` |
| 17 | Preflight | Generate display-only scan and phase information | *Not directly tested. The values are shown only in preflight.* |
| 18 | Execute | Propagate an EbsdLib data-read failure (`-8970`) | `Missing Data Column` |
| 19 | Execute | Reject an empty phase vector (`-8971`) | *Not directly tested. H5OINAReader rejects this file during preflight first.* |
| 20 | Execute | Initialize ensemble tuple 0 and fill phase tuples | Class 1 oracle; real-file readback |
| 21 | Execute | Apply Low-to-High or High-to-Low stacking | `Stacking Order` |
| 22 | Copy | Fail when the file cannot be reopened (`-34971`) | *Not directly tested. Requires file replacement during execution.* |
| 23 | Copy | Skip an extent probe that cannot inspect a dataset | *Not directly tested. The reader rejects a missing required dataset first.* |
| 24 | Copy | Reject a dataset extent that differs from the header (`-34971`) | `Dataset Extent Mismatch rejected` |
| 25 | Copy | Copy the four `uint8` arrays into the current scan slab | Class 1 oracle; multi-scan placement |
| 26 | Copy | Copy Euler values with a three-component offset | `Multi-Scan Slab Placement` |
| 27 | Copy | Reject a Phase value outside the ensemble bounds (`-34972`) | `Out-of-Range Phase Value rejected` |
| 28 | Copy | Widen Phase to `int32` or retain `uint8` | `Conversion Option Combinations` |
| 29 | Copy | Copy MAD, X, and Y into the current scan slab | Class 1 oracle; multi-scan placement |
| 30 | Copy | Apply hexagonal alignment only to Hexagonal-High points in the current slab | Class 1 oracle; option sweep; multi-scan alignment |
| 31 | Execute/Copy | Return at the three cancellation checks | *Not directly tested. Requires cancel-signal injection.* |
| 32 | EbsdLib | Reject a noncanonical phase-group name (`-90034`, surfaced as `-9582`) | `Invalid Phase Group Name rejected` |
| 33 | EbsdLib | Reject an unreadable phase group, lattice dimensions, or Laue group | *Not directly tested. The fixture writer cannot create the unreadable-group case; the other required-dataset cases use the same checked path as lattice angles.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `Class 1 Analytical Oracle` | new-for-V&V | Verifies geometry, all cell arrays, all ensemble arrays, alignment, and invariants. |
| `Conversion Option Combinations` | new-for-V&V | Covers the 2 x 2 alignment and Phase-type option matrix. |
| `Multi-Scan Slab Placement` | new-for-V&V | Verifies all scan slabs with disjoint values. |
| `Multi-Scan Hexagonal Alignment` | new-for-V&V | Verifies alignment on each scan slab. |
| `Format Version Variants` | new-for-V&V | Covers version 5.0, version 2.0, and no version dataset. |
| `Parameter Rejections` | new-for-V&V | Covers Z spacing, empty selection, and unsupported pattern import. |
| `Invalid Cell Counts rejected` | new-for-V&V | Covers zero and negative cell counts. |
| `Scan Header Mismatch rejected` | new-for-V&V | Covers grid dimensions and step-size differences. |
| `Missing Scan Name rejected` | new-for-V&V | Covers missing first and later scan names. |
| `Phase Index Out Of Range rejected` | new-for-V&V | Covers invalid phase indices in the first and later scans. |
| `Invalid Phase Group Name rejected` | new-for-V&V | Verifies that EbsdLib reports a non-numeric group name without throwing. |
| `Scan Phase Count Mismatch rejected` | new-for-V&V | Covers later scans with more or fewer phase groups. |
| `Scan Phase Definition Mismatch rejected` | new-for-V&V | Covers material, Laue, space-group, lattice-dimension, and lattice-angle differences. |
| `Invalid Scan Spacing rejected` | new-for-V&V | Covers zero, negative, and non-finite scan spacing. |
| `Dataset Extent Mismatch rejected` | new-for-V&V | Verifies header and dataset extent agreement. |
| `Out-of-Range Phase Value rejected` | new-for-V&V | Verifies Phase values before ensemble indexing. |
| `Missing Data Column` | new-for-V&V | Verifies EbsdLib execute error propagation. |
| `Missing Lattice Angles` | new-for-V&V | Verifies EbsdLib preflight error propagation for first and later scans. |
| `Stacking Order` | new-for-V&V | Verifies both scan orders. |
| `Real AZtec File Readback` | kept, modified | Uses the vendor H5OINA input and independent readback. The generated DREAM3D exemplar is not used. |
| `InValid Filter Execution` | retired | The old test used a missing file and did not verify the intended error. |

All 20 test cases pass. The ctest run reports 9,943 assertions.

## Test sensitivity verification

Test sensitivity verification introduces one temporary defect at a time and confirms that the applicable test fails.

Eleven temporary defects were evaluated. Each defect caused the expected V&V test to fail.

| Temporary defect | Test that detected the defect | Result |
|---|---|---|
| Restore the 30-radian alignment value. | Class 1 oracle; option sweep; multi-scan alignment; format variants | Detected |
| Use a float32 intermediate for alignment. | Class 1 oracle; option sweep; multi-scan alignment; format variants | Detected |
| Use the tuple offset for the Euler element offset. | Multi-scan slab placement; multi-scan alignment | Detected |
| Apply alignment from tuple 0 for every scan. | Multi-scan alignment | Detected |
| Copy beta into the gamma lattice slot. | Class 1 oracle | Detected |
| Leave lattice angles in radians. | Class 1 oracle; real-file readback | Detected |
| Remove the dataset-extent validation. | Dataset extent mismatch | Detected |
| Pass an invalid phase-group name to `std::stoi()`. | Invalid phase group name | Detected |
| Disable phase-definition validation. | Scan phase definition mismatch | Detected |
| Disable spacing validation. | Parameter rejections; invalid scan spacing | Detected |
| Remove the file path from the Phase-range error. | Out-of-range Phase value | Detected |

## Exemplar archive

`H5Oina_Test_Data.tar.gz` is retained because it contains a vendor H5OINA input. The generated DREAM3D output was a circular oracle and was retired for this filter. The replacement uses inline Class 1 analytical data, Class 4 invariants, and the Class 2 h5py readback.

SHA512: `346573ac6b96983680078e8b0a401aa25bd9302dff382ca86ae4e503ded6db3947c4c5611ee603db519d8a8dc6ed35b044a7bfea9880fade5ab54479d140ea03`.

## Deviations from DREAM.3D 6.5.171

DREAM.3D 6.5.171 has no H5OINA importer. Therefore, no legacy comparison is possible.

The deviations document compares this verified version with `ReadH5OinaDataFilter` as shipped in DREAM3D-NX 7.0.0 through 7.4.1. See `vv/deviations/ReadH5OinaDataFilter.md` for the root cause, affected users, and recommendation for each defect.

| Deviation | Observed difference |
|---|---|
| `ReadH5OinaDataFilter-D1` | Hexagonal `phi2` alignment uses 30 degrees instead of 30 radians. |
| `ReadH5OinaDataFilter-D2` | Every scan writes its Euler values to the correct tuple slab. |
| `ReadH5OinaDataFilter-D3` | Hexagonal alignment modifies each scan exactly once. |
| `ReadH5OinaDataFilter-D4` | Unsupported pattern import reports a clear preflight error. |
| `ReadH5OinaDataFilter-D5` | The gamma lattice slot contains gamma instead of beta. |
| `ReadH5OinaDataFilter-D6` | Lattice angles use degrees instead of radians. |
| `ReadH5OinaDataFilter-D7` | Missing phase datasets produce an error instead of invalid access. |
| `ReadH5OinaDataFilter-D8` | Multi-scan inputs must have safe phase indices and identical phase definitions. |
| `ReadH5OinaDataFilter-D9` | Stacking Order controls the scan order. |
| `ReadH5OinaDataFilter-D10` | Reader error messages reach the user. |
| `ReadH5OinaDataFilter-D11` | Both cell counts are validated and the returned error code is consistent. |
| `ReadH5OinaDataFilter-D12` | Invalid phase-group names produce an error instead of an exception. |
| `ReadH5OinaDataFilter-D13` | Geometry spacing must be finite and positive. |
