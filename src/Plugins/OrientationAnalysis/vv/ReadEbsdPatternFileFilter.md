# V&V Report: ReadEbsdPatternFileFilter

| | |
|---|---|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `cd975a45-53de-4ace-ac16-d99638a44d6a` |
| DREAM3D 6.5.171 equivalent | None. This is a new filter. |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | DRAFT |
| Sign-off | Pending second-engineer review. |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **New filter.** No DREAM3D 6.5.171 filter reads EDAX UP pattern stacks into a general DataArray. |
| Oracle (confirmed) | **Class 1 (Analytical) primary and Class 4 (Invariant) companion.** The test writes fixed little-endian headers and payload values. It checks the parsed metadata, output shapes, output types, and exact pixel values. |
| Code paths enumerated | 17 of 19 paths are exercised. The out-of-core store fallback and execution-time short-read recovery path need an OOC build or a file-mutation harness. |
| Tests today | 14 test cases cover UP versions 1 and 3, `.up1` and `.up2`, output placement, warnings, malformed files, extension mismatches, and cancellation. All pass in the full 1,753-test in-core run. |
| Exemplar archive | None. The Class 1 fixtures are small files that the test creates from independent literal bytes. The test does not use a saved output from SIMPLNX or DREAM3D. |
| Legacy comparison | Not run. The filter is new and has no DREAM3D 6.5.171 equivalent. |
| Bug flags | None. |
| V&V phase | Oracle reconciliation, the full in-core build, all 1,753 in-core tests, and the clang-format gate are complete. OOC evidence and second-engineer review are pending. |

## Summary

This filter reads EDAX `.up1` and `.up2` EBSD detector patterns into a SIMPLNX DataArray. Class 1 fixtures verify exact header and payload behavior. Class 4 checks verify tuple-count and shape contracts. No legacy comparison applies because this is a new filter.

## Algorithm Relationship

*Classification:* **New filter**

*Evidence:* Search of the available DREAM3D 6.5.171 source trees found no filter or utility that reads `.up1` or `.up2` pattern files. This filter has a new SIMPLNX UUID and no SIMPL conversion mapping.

## Oracle

*Class:* **1 (Analytical) primary and 4 (Invariant) companion**

*Applied:* The test writes each integer and floating-point header field as fixed little-endian bytes. It writes deterministic pixel literals after the header. Expected metadata, shapes, types, warnings, error branches, and output pixels are derived directly from those bytes.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ReadEbsdPatternFileTest.cpp` contains 14 test cases. The suite checks version 1 and 3 headers, both pixel widths, exact payload values, AttributeMatrix tuple rules, manual version 1 dimensions, extra-pattern exclusion, future-version and unknown-step warnings, malformed inputs, both extension-mismatch directions, case-insensitive extensions, and cancellation. All 14 tests pass in the full 1,753-test in-core run.

*Second-engineer review:* Pending.

## Bugs found and fixed

None.

## Code path coverage

17 of 19 paths are exercised.

Sources:

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadEbsdPatternFile.cpp` (72 lines).
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EdaxUpPatternFileReader.cpp` (354 lines).

| # | Phase | Path | Test case |
|---:|---|---|---|
| 1 | Factory | `.up1` or `.up2` selects the EDAX reader | `Case Insensitive Extension`; all valid filter cases |
| 2 | Factory | Unsupported extension returns an error | `Factory Rejects Unsupported Extension` |
| 3 | Header | Version 1 derives a flat pattern count from file size | `Version 1 Flat Preflight` |
| 4 | Header | Version 3 reads packed fields at unaligned offsets | `Version 3 Metadata` |
| 5 | Header | Version 2 returns an explicit error | `Malformed Files` — Version 2 section |
| 6 | Header | Version above 3 uses the version 3 layout and warns | `Version 3 Warnings` — future-version section |
| 7 | Header | Invalid pattern dimensions or data offsets return errors | `Malformed Files` — dimension and offset sections |
| 8 | Header | Residual version 1 payload bytes return an error | `Malformed Files` — residual-byte section |
| 9 | Header | Version 3 grid payload is incomplete | `Malformed Files` — truncated-grid section |
| 10 | Header | Pixel width conflicts with `.up1` or `.up2` | `Probable Extension Mismatch`; `Probable Wide Payload Extension Mismatch` |
| 11 | Header | Extra patterns are present and match the trailing payload | `Version 3 Extra Patterns Are Skipped` |
| 12 | Header | Extra-pattern count and trailing payload disagree | `Version 3 Warnings` — extra-count section |
| 13 | Preflight | Existing AttributeMatrix has the required tuple count | `Attribute Matrix Tuple Contract` — matching section |
| 14 | Preflight | Existing AttributeMatrix has the wrong tuple count | `Attribute Matrix Tuple Contract` — mismatch section |
| 15 | Preflight | Version 1 manual rows and columns match or mismatch | `Version 1 UP2 Manual Scan Dimensions`; `Invalid Version 1 Scan Dimensions` |
| 16 | Execute | Read `uint8` and `uint16` payloads in file order | `Version 1 Flat Preflight`; `Version 1 UP2 Manual Scan Dimensions`; `Version 3 Header Geometry` |
| 17 | Execute | Cancellation occurs before payload transfer | `Cancellation Before Payload Read` |
| 18 | Execute | Non-contiguous or out-of-core store uses sequential `setValue` writes | *Not directly tested. No DREAM3D-NX OOC build is available in this workspace.* |
| 19 | Execute | File becomes short after preflight and before a chunk read | *Not directly tested. This needs deterministic file mutation during the execution read.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `Version 1 UP2 Manual Scan Dimensions` | new-for-V&V | Checks little-endian `uint16` values and `{2, 2}` manual tuple shape. |
| `Version 3 Header Geometry` | new-for-V&V | Checks file-derived tuple shape and exact `uint8` values. |
| `Version 3 Metadata` | new-for-V&V | Checks packed fields, steps, grid counts, type, and pattern size. |
| `Version 3 Extra Patterns Are Skipped` | new-for-V&V | Checks warning propagation and excludes two appended patterns. |
| `Attribute Matrix Tuple Contract` | new-for-V&V | Checks matching shape inheritance and mismatch rejection. |
| `Invalid Version 1 Scan Dimensions` | new-for-V&V | Checks row-column product validation. |
| `Malformed Files` | new-for-V&V | Checks version 2, residual bytes, offsets, dimensions, truncation, and overflow. |
| `Factory Rejects Unsupported Extension` | new-for-V&V | Checks the extension-only factory contract. |
| `Probable Extension Mismatch` | new-for-V&V | Checks `.up2` with a one-byte payload. |
| `Probable Wide Payload Extension Mismatch` | new-for-V&V | Checks `.up1` with a two-byte payload. |
| `Cancellation Before Payload Read` | new-for-V&V | Checks prompt pre-cancel return and unchanged output. |
| `Version 3 Warnings` | new-for-V&V | Checks future version, zero step, and extra-size warnings. |
| `Case Insensitive Extension` | new-for-V&V | Checks uppercase `.UP1`. |
| `Version 1 Flat Preflight` | new-for-V&V | Checks default flat shape, type, component shape, and payload import. |

## Exemplar archive

No archive is required. The test creates all Class 1 inputs from literal bytes and stores all expected values in the test source. A supplementary local ctest imported all 25 patterns from a version 1, 480 x 480 file and confirmed its first and last bytes. The same test parsed a 60 x 60 version 3 file with 1,001 columns, 854 rows, and 854,854 patterns without loading its 3.1 GB payload. The machine-specific test was removed after it passed. These files are not oracle data.

## Deviations from DREAM3D 6.5.171

Not applicable. DREAM3D 6.5.171 has no equivalent filter.
