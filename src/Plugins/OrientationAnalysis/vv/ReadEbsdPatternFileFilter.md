# V&V Report: ReadEbsdPatternFileFilter

| | |
|---|---|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `cd975a45-53de-4ace-ac16-d99638a44d6a` |
| SIMPLNX Human Name | Read EBSD Pattern File |
| DREAM.3D v6.5.171 equivalent | None (new filter) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | DRAFT |
| Sign-off | Michael Jackson, 2026-08-27; second engineer: Jared Duffey, 2026-09-15 |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | New filter. DREAM.3D v6.5.171 has no equivalent EDAX UP pattern-file reader. |
| Oracle (confirmed) | Class 1 byte fixtures and Class 4 shape checks are encoded in `test/ReadEbsdPatternFileTest.cpp`. All pass in CI. |
| Code paths enumerated | 19 of 42 exercised. Gaps include input guards, warnings, file changes, chunk boundaries, and pixel assertions for two format combinations. |
| Tests today | 14 test cases cover headers, output shapes, selected pixel values, malformed inputs, warnings, and cancellation. All pass in CI. |
| Exemplar archive | None. Tests construct synthetic files and expected values in C++. |
| Legacy comparison | Not run — DREAM.3D v6.5.171 has no equivalent filter. |
| Bug flags | None |
| V&V phase | Analytical and invariant checks are encoded and pass in CI. Second-engineer review is complete. |

## Summary

`ReadEbsdPatternFileFilter` reads EDAX `.up1` and `.up2` EBSD patterns into a DataArray. Class 1 fixtures check metadata, errors, warnings, and selected endpoint pixels; Class 4 checks verify tuple and component shapes. All pass in CI.

## Algorithm Relationship

*Classification:* New filter

*Evidence:* The DREAM.3D v6.5.171 source and corresponding SIMPL and plugin sources contain no EDAX UP reader; `ReadEbsdPatternFileFilter.hpp` declares the new UUID above.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) checks.

*Applied:* Independent little-endian writers define header fields and pixel sequences; assertions check metadata, shapes, diagnostics, and the first and last pixels for version 1 `.up2` and version 3 `.up1`.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ReadEbsdPatternFileTest.cpp::OrientationAnalysis::ReadEbsdPatternFileFilter: Version 1 UP2 Manual Scan Dimensions` and 13 companion cases use three valid import layouts plus invalid-input and warning variants. All pass in CI.

*Second-engineer review:* Jared Duffey, 2026-09-15.

## Bugs found and fixed

None.

## Code path coverage

19 of 42 paths exercised. Each uncovered row states the missing fixture or assertion.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadEbsdPatternFile.cpp` (72 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ReadEbsdPatternFileFilter.cpp` (223 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EdaxUpPatternFileReader.cpp` (351 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EbsdPatternFileReaderFactory.cpp` (21 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EbsdPatternFileUtilities.cpp` (90 lines).

Test names below omit their `OrientationAnalysis::ReadEbsdPatternFileFilter: ` or `OrientationAnalysis::EBSD Pattern Reader: ` prefix. The tests use square patterns and square scan grids; they do not distinguish swapped axes.

| # | Phase | Path | Test case |
|---:|---|---|---|
| 1 | Factory | `.up1` or `.up2` selects the EDAX reader | `Version 1 UP2 Manual Scan Dimensions`; `Version 3 Header Geometry` |
| 2 | Factory | Unsupported extension returns an error | `Factory Rejects Unsupported Extension` |
| 3 | Header | Version 1 derives a flat pattern count from file size | `Version 1 Flat Preflight` |
| 4 | Header | Version 3 reads packed fields at unaligned offsets | `Version 3 Metadata` |
| 5 | Header | Version 2 returns an explicit error | `Malformed Files` — Version 2 section |
| 6 | Header | Version above 3 uses the version 3 layout and warns | `Version 3 Warnings` — future-version section |
| 7 | Header | Invalid pattern dimensions or data offsets return errors | `Malformed Files` — dimension and offset sections |
| 8 | Header | Residual version 1 payload bytes return an error | `Malformed Files` — residual-byte section |
| 9 | Header | Version 3 grid payload is incomplete | `Malformed Files` — truncated-grid section |
| 10 | Header | Pixel width conflicts with `.up1` or `.up2` | `Probable Extension Mismatch`; `Probable Wide Payload Extension Mismatch` |
| 11 | Header | Extra patterns are present and match the trailing payload | `Version 3 Extra Patterns Are Skipped` — warning and output size only |
| 12 | Header | Extra-pattern count and trailing payload disagree | `Version 3 Warnings` — extra-count section |
| 13 | Preflight | Existing AttributeMatrix has the required tuple count | `Attribute Matrix Tuple Contract` — matching section |
| 14 | Preflight | Existing AttributeMatrix has the wrong tuple count | `Attribute Matrix Tuple Contract` — mismatch section |
| 15 | Preflight | Version 1 manual rows and columns match or mismatch | `Version 1 UP2 Manual Scan Dimensions`; `Invalid Version 1 Scan Dimensions` |
| 16 | Execute | Read `uint8` and `uint16` payloads in file order | `Version 1 UP2 Manual Scan Dimensions`; `Version 3 Header Geometry` — first and last pixels only |
| 17 | Execute | Cancellation occurs before payload transfer | `Cancellation Before Payload Read` |
| 18 | Execute | A store other than `DataStore<T>` uses sequential `setValue` writes | *Not directly tested. The tests do not select this store path.* |
| 19 | Execute | File becomes short after preflight and before a chunk read | *Not directly tested. This needs deterministic file mutation during the execution read.* |
| 20 | Header | Zero scan steps produce absent step values and a warning | `Version 3 Warnings` — zero-step section |
| 21 | Preflight | Manual row-column multiplication overflows | `Malformed Files` — overflowing manual dimensions section |
| 22 | Preflight | Manual rows or columns are zero | *Not directly tested. No fixture supplies zero manual dimensions.* |
| 23 | Preflight | An AttributeMatrix overrides supplied manual dimensions and warns | *Not directly tested. No fixture enables manual dimensions inside an AttributeMatrix.* |
| 24 | Preflight | Version 3 overrides supplied manual dimensions and warns | *Not directly tested. No fixture supplies manual dimensions for a version 3 file.* |
| 25 | Header I/O | File-size lookup, open, or seek fails | *Not directly tested. The tests create readable files and do not inject I/O failures.* |
| 26 | Header I/O | The initial header is too short or an exact header read fails | *Not directly tested. No fixture truncates a header or changes a file during header reading.* |
| 27 | Header | Version 0 is rejected; the later unsupported-version guard is unreachable | *Not directly tested. No version 0 fixture exists; the earlier check handles 0 and 2, leaving 1 or values of at least 3.* |
| 28 | Header | Data offset is at or beyond the end of the file | *Not directly tested. The offset fixture only overlaps the header.* |
| 29 | Header | A valid nonstandard data offset produces a warning | *Not directly tested. No fixture pads the header before a valid payload.* |
| 30 | Header | Version 1 has zero complete patterns | *Not directly tested. The earlier offset and residual-byte checks reject such files.* |
| 31 | Header | A scan step is negative or not finite | *Not directly tested. Step fixtures contain only positive values or zero.* |
| 32 | Header | Version 3 has an empty grid | *Not directly tested. No fixture sets a stored row or column count to zero.* |
| 33 | Header | Undeclared trailing bytes do not match the opposite pixel width | *Not directly tested. The trailing-byte fixture only tests the opposite-width error.* |
| 34 | Size checks | Header size arithmetic overflows or metadata cannot fit the platform array size | *Not directly tested. Only manual-dimension multiplication overflow has a fixture; dimension limits prevent some header overflows.* |
| 35 | Execute | Current metadata requires a different output tuple shape | *Not directly tested. No test changes metadata after preflight or calls the algorithm with a mismatched tuple shape.* |
| 36 | Execute | Output type, tuple count, or component shape fails the reader contract | *Not directly tested. No test calls the reader with an incompatible destination.* |
| 37 | Payload I/O | Opening or seeking the payload fails | *Not directly tested. No test injects failure after metadata validation.* |
| 38 | Execute | Cancellation occurs at a payload chunk boundary | *Not directly tested. The cancellation test returns before the reader is created.* |
| 39 | Execute | Import spans multiple chunks | *Not directly tested. All synthetic payloads fit in one 4 MiB chunk.* |
| 40 | Execute | A valid version 3 `.up2` file imports `uint16` pixels | *Not directly tested. The version 3 wide-pixel fixture is used only with an incorrect `.up1` extension.* |
| 41 | Execute | A valid version 1 `.up1` file imports `uint8` pixels | *Not directly tested. The tests execute this mode but assert type and shape, not pixel values.* |
| 42 | Header | A nonzero grid flag identifies hexagonal metadata | *Not directly tested. All version 3 fixtures write a zero grid flag.* |

## Test inventory

All 14 cases are in `src/Plugins/OrientationAnalysis/test/ReadEbsdPatternFileTest.cpp`. All pass in CI.

| Test case | Status | Notes |
|---|---|---|
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Version 1 UP2 Manual Scan Dimensions` | new-for-V&V | Checks `uint16` type, `{2, 2}` tuple shape, `{12, 12}` component shape, and endpoint pixels 1000 and 1575. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Version 3 Header Geometry` | new-for-V&V | Checks file-derived shapes, `uint8` type, and first and last pixel values. |
| `OrientationAnalysis::EBSD Pattern Reader: Version 3 Metadata` | new-for-V&V | Checks version, pixel type, dimensions, pattern count, distinct X/Y steps, and square-grid flag. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Version 3 Extra Patterns Are Skipped` | new-for-V&V | Checks warnings, four output tuples, and 576 elements after two extra patterns; does not assert pixels. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Attribute Matrix Tuple Contract` | new-for-V&V | Checks AttributeMatrix shape inheritance and mismatch rejection with `-78040`. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Invalid Version 1 Scan Dimensions` | new-for-V&V | Checks rejection of a manual row-column product that differs from the pattern count (`-78043`). |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Malformed Files` | new-for-V&V | Checks version 2 rejection, residual bytes, overlapping data offset, invalid width, truncated grid, and manual-dimension overflow. |
| `OrientationAnalysis::EBSD Pattern Reader: Factory Rejects Unsupported Extension` | new-for-V&V | Checks rejection of `.ebsp` by the reader factory. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Probable Extension Mismatch` | new-for-V&V | Checks rejection of a one-byte version 3 payload named `.up2`. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Probable Wide Payload Extension Mismatch` | new-for-V&V | Checks rejection of a two-byte version 3 payload named `.up1`. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Cancellation Before Payload Read` | new-for-V&V | Calls the algorithm with cancellation already set; checks success and zero endpoint pixels. |
| `OrientationAnalysis::EBSD Pattern Reader: Version 3 Warnings` | new-for-V&V | Checks future-version warning, absent zero-valued steps, and inconsistent extra-pattern count warnings. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Case Insensitive Extension` | new-for-V&V | Checks uppercase `.UP1` acceptance and output type; does not assert pixels. |
| `OrientationAnalysis::ReadEbsdPatternFileFilter: Version 1 Flat Preflight` | new-for-V&V | Checks flat tuple shape, component shape, type, tuple count, and component count; does not assert pixels. |

## Exemplar archive

- **Archive:** None. The tests create all input files in C++.
- **SHA512:** Not applicable. `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` registers no archive for these tests.
- **Provenance:** `src/Plugins/OrientationAnalysis/test/ReadEbsdPatternFileTest.cpp` contains the fixture writers and expected values; no archive sidecar is required.

## Deviations from DREAM.3D v6.5.171

Not applicable. DREAM.3D v6.5.171 has no equivalent filter. There are no Part 1 entries in `vv/deviations/ReadEbsdPatternFileFilter.md`.

| Deviation | Observed difference |
|---|---|
| None | No equivalent filter exists in DREAM.3D v6.5.171. |
