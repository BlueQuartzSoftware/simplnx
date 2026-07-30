# V&V Report: ITKImageWriterFilter

| | |
|---|---|
| Plugin | ITKImageProcessing |
| SIMPLNX UUID | `a181ee3e-1678-4133-b9c5-a9dd7bfec62f` |
| DREAM3D 6.5.171 equivalent | `ITKImageWriter` (SIMPL UUID `11473711-f94d-5d96-b749-ec36a81ad338`) - `Source/Plugins/ITKImageProcessing/ITKImageProcessingFilters/ITKImageWriter.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | DRAFT |
| Sign-off | Pending second-engineer review |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | Minor changes - same XY/XZ/YZ pixel extraction; NX uses SIMPLNX stores, AtomicFile, and current ITK APIs. Plane spacing and origin now follow the selected physical axes (D2). |
| Oracle (confirmed) | Classes 1 + 4 - one 3x2x2 scalar fixture (`value(x,y,z)=x+10y+100z`) has same XY/XZ/YZ pixels for all ten accepted scalar types; it verifies selected-plane spacing for all types and origin for MetaImage output. uint8 uses TIFF and the remaining types use MetaImage. |
| Code paths enumerated | 14 of 19 explicit paths exercised; OOC, unsupported-component, cancellation, write-failure, and defensive tuple-copy failure paths remain untested. |
| Tests today | 7 named test cases - 1 Class 1+4 Oracle, 2 preflight error-path tests, 1 single-slice exact-name test, 1 RGBA output test, 1 stack-writing test, and 1 SIMPL backwards-compatibility test |
| Exemplar archive | None - Class 1+4 oracle uses inline data |
| Legacy comparison | Run against DREAM3D 6.5.171 with the inline test data for the Class 1+4 Oracle. Decoded pixels match; D1 and D2 record filename-formatting and plane-metadata differences. |
| Bug flags | Unsigned uint32/uint64 dispatch was corrected for this V&V cycle. Current changes add invalid fill-character validation, component-count validation, RGBA dispatch, selected-plane physical metadata, and tuple-copy error propagation. |
| V&V phase | Regression tests and report amendments in progress |

## Summary

ITKImageWriterFilter exports ImageGeom cell data as an ITK image or a 2D image stack. A hand-derived non-square fixture verifies decoded pixel orientation and plane metadata independently of legacy, while the legacy comparison checks migration output on the toy and production fixtures.

## Algorithm Relationship

**Minor changes.** Port-time changes are SIMPLNX DataStructure stores, AtomicFile writes, and current ITK APIs. Scalar XY/XZ/YZ decoded pixels match the legacy behavior. The NX writer deliberately preserves the selected physical axes in XZ and YZ output spacing/origin; legacy output used identity 2D metadata (D2). The parameter set is unchanged, so `parametersVersion()` remains `2`; validation has been tightened without a parameter-schema change.

**PR(s):**

**2026**

- [PR #1626](https://github.com/BlueQuartzSoftware/simplnx/pull/1626) ("ENH: Various small doc, bug and enhancement fixes.") - Small documentation, bug, and enhancement fixes.
- [PR #1585](https://github.com/BlueQuartzSoftware/simplnx/pull/1585) ("ENH: Add Image Reader/Writer that depend on Tiff and Stb libraries.") - Integrates TIFF/STB reader-writer support.
- [PR #1576](https://github.com/BlueQuartzSoftware/simplnx/pull/1576) ("ENH: Improve error messages across the codebase") - Improves error messages.
- [PR #1555](https://github.com/BlueQuartzSoftware/simplnx/pull/1555) ("ENH: Again require in-memory data for ITK filters") - Restores the ITK in-memory data requirement.
- [PR #1490](https://github.com/BlueQuartzSoftware/simplnx/pull/1490) ("STY: Fix warnings about unintended slicing of object") - Removes object-slicing warnings.
- [PR #1476](https://github.com/BlueQuartzSoftware/simplnx/pull/1476) ("BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing") - Fixes backward-pipeline compatibility.

**2025**

- [PR #1489](https://github.com/BlueQuartzSoftware/simplnx/pull/1489) ("ENH: ItkImageWriter allow user to set the number of padding digits and the fill char") - Adds output-index padding and fill-character controls.
- [PR #1457](https://github.com/BlueQuartzSoftware/simplnx/pull/1457) ("STY: Clean up 'static inline' from filter headers") - Cleans up static-inline header declarations.
- [PR #1377](https://github.com/BlueQuartzSoftware/simplnx/pull/1377) ("STY: Ensure all code arguments are consistent across filters") - Standardizes argument style.
- [PR #1257](https://github.com/BlueQuartzSoftware/simplnx/pull/1257) ("ENH: Add missing documentation comments for preflight and execute methods in filters") - Adds preflight and execute documentation.
- [PR #1253](https://github.com/BlueQuartzSoftware/simplnx/pull/1253) ("ENH: Merge Initial Out-of-Core infrastructure") - Introduces the initial OOC infrastructure.
- [PR #1238](https://github.com/BlueQuartzSoftware/simplnx/pull/1238) ("ENH: Added pipeline relative path support") - Adds pipeline-relative path support.
- [PR #1232](https://github.com/BlueQuartzSoftware/simplnx/pull/1232) ("DOC: General code clean up to make parameters consistent and docs consistent") - Aligns parameter and documentation conventions.

**2024**

- [PR #1088](https://github.com/BlueQuartzSoftware/simplnx/pull/1088) ("Added versioning to filter parameters and json") - Adds filter and parameter versioning, including backwards-compatible parameter JSON reading.
- [PR #1082](https://github.com/BlueQuartzSoftware/simplnx/pull/1082) ("SIMPLConversion header optimization") - Optimizes SIMPL-conversion headers.
- [PR #941](https://github.com/BlueQuartzSoftware/simplnx/pull/941) ("Moved Result handling outside of AtomicFile") - Moves result handling outside AtomicFile.
- [PR #934](https://github.com/BlueQuartzSoftware/simplnx/pull/934) ("BUG: Pipeline and Filter human facing label cleanup") - Cleans up human-facing labels.
- [PR #931](https://github.com/BlueQuartzSoftware/simplnx/pull/931) ("ENH: All filter's class names end with \"Filter\".") - Applies the Filter class-name suffix.
- [PR #918](https://github.com/BlueQuartzSoftware/simplnx/pull/918) ("BUG: Update ITK Image Writer to Use Atomic File API") - Migrates the writer to the AtomicFile API.
- [PR #874](https://github.com/BlueQuartzSoftware/simplnx/pull/874) ("ENH: Refactor the Parameter Keys to make them consistent and easy to learn") - Refactors parameter keys.

**2023**

- [PR #801](https://github.com/BlueQuartzSoftware/simplnx/pull/801) ("ENH: Rename complex to simplnx") - Renames the framework namespace and repository identity.
- [PR #790](https://github.com/BlueQuartzSoftware/simplnx/pull/790) ("ENH: Write Temp Files for All Writers") - Adds temporary-file writing for writers.
- [PR #779](https://github.com/BlueQuartzSoftware/simplnx/pull/779) ("ENH: Implement SIMPL pipeline conversion") - Implements SIMPL pipeline conversion.
- [PR #753](https://github.com/BlueQuartzSoftware/simplnx/pull/753) ("API: Standardize I/O Naming to Read/Write") - Standardizes I/O naming.
- [PR #703](https://github.com/BlueQuartzSoftware/simplnx/pull/703) ("ENH: Enable Out-of-Core functionality") - Adds OOC infrastructure compatibility.
- [PR #671](https://github.com/BlueQuartzSoftware/simplnx/pull/671) ("API: Add C++ Class Name to All Default Tags") - Adds C++ class names to default tags.
- [PR #593](https://github.com/BlueQuartzSoftware/simplnx/pull/593) ("ENH: Update ITK filters to follow naming and parameter layout conventions") - Aligns ITK filter naming and parameter layout.
- [PR #575](https://github.com/BlueQuartzSoftware/simplnx/pull/575) ("BUG: Fix issue where ITKImageWriter is double looping over the Z dim") - Fixes double iteration over Z.
- [PR #86](https://github.com/BlueQuartzSoftware/simplnx/pull/86) ("STYLE: Clean Up Includes") - Cleans up includes.

**2022**

- [PR #80](https://github.com/BlueQuartzSoftware/simplnx/pull/80) ("STYLE: Short Variable Patching") - Shortens variable names.
- [PR #56](https://github.com/BlueQuartzSoftware/simplnx/pull/56) ("Add Filter Comments") - Adds filter comments.
- [PR #54](https://github.com/BlueQuartzSoftware/simplnx/pull/54) ("DOCS: Update paths and CMake codes to prepare for documentation updates") - Updates documentation paths and CMake configuration.
- [PR #50](https://github.com/BlueQuartzSoftware/simplnx/pull/50) ("All Parameter keys should be snake_case") - Converts parameter keys to snake_case.
- [PR #37](https://github.com/BlueQuartzSoftware/simplnx/pull/37) ("Refactored geometry hierarchy") - Refactors the geometry hierarchy.
- [PR #30](https://github.com/BlueQuartzSoftware/simplnx/pull/30) ("UUID cleaning") - Cleans up filter UUIDs.
- [PR #2](https://github.com/BlueQuartzSoftware/simplnx/pull/2) ("Newly Created ITK Wrapped Filters") - Adds the newly generated ITK-wrapped filter implementation.

## Oracle

**Class:** 1 (Analytical) + 4 (Invariant).

**Applied:** The 3x2x2 scalar fixture has `value(x,y,z)=x+10y+100z`; exact XY, XZ, and YZ pixel matrices are hand-derived and decoded for all ten accepted types. Its non-uniform origin `(10,20,40)` and spacing `(1,2,4)` verify that each written plane has the correct two physical axes: spacing is decoded for every type and origin for MetaImage output. TIFF does not preserve ITK origin metadata. File creation and decoded dimensions are companion invariants.

**Encoded:** `test/ITKImageWriterTest.cpp::ITKImageProcessing::ITKImageWriterFilter: Analytical Pixel Order` - templated over all ten accepted scalar types.

**Second-engineer review:** pending second-engineer review.

## Code path coverage

14 of 19 explicit paths exercised. Source: `src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters/ITKImageWriterFilter.cpp` (511 lines).

| # | Path | Test case |
|---|---|---|
| 1 | Valid preflight. | `Analytical Pixel Order`, `Write Stack` |
| 2 | Dimension-mismatch preflight error. | `Dimension Mismatch Validation` - a 1x1x1 array and 1x1x2 geometry return `-25600`. |
| 3 | Empty fill-character validation. | `Fill Character Validation` - empty string returns `-25601`. |
| 4 | Invalid fill-character validation. | `Fill Character Validation` - `{` and `/` return `-25602`. |
| 5 | OOC input rejection. | *Not directly tested; not able to run on CI* |
| 6 | Unsupported component-count rejection. | *Not directly tested.* |
| 7 | Preflight example-output-file value, including offset multi-slice and unsuffixed single-slice output. | `Write Stack` and `Single Slice Keeps Exact Output Name`. |
| 8 | XY output pixels and physical metadata. | `Analytical Pixel Order` - hand-derived pixels and spacing `(1,2)` for all types, plus origin `(10,20)` for MetaImage output. |
| 9 | XZ output pixels and physical metadata. | `Analytical Pixel Order` - hand-derived pixels and spacing `(1,4)` for all types, plus origin `(10,40)` for MetaImage output. |
| 10 | YZ output pixels and physical metadata. | `Analytical Pixel Order` - hand-derived pixels and spacing `(2,4)` for all types, plus origin `(20,40)` for MetaImage output. |
| 11 | All accepted scalar dispatch arms: int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, and float64. | `Analytical Pixel Order` - TIFF pixels for uint8 and MetaImage pixels for every other type. |
| 12 | RGBA dispatch arm. | `RGBA Image Output` - one uint8 RGBA pixel decodes as `(10,20,30,40)`. |
| 13 | Cancellation between slices. | *Not directly tested; requires cancel signal infrastructure* |
| 14 | Filesystem/ITK write-failure propagation. | *Not directly tested; requires failure injection.* |
| 15 | Tuple-copy failure propagation. | *Not directly tested. This is a defensive check whose failure preconditions are excluded by construction: `sliceData` is created with the input store's data type and component shape and exactly the selected plane's tuple shape; preflight requires the input tuple dimensions to match the selected geometry, so the computed source and destination ranges are valid.* |
| 16 | Single-slice output keeps the exact output name. | `Single Slice Keeps Exact Output Name` - `maxSlice == 1` suppresses the index suffix, and a 3x1x2 ImageGeom XZ plane writes one unsuffixed MetaImage file with exact decoded pixels. |
| 17 | Multi-file stack output. | `Write Stack`. |
| 18 | SIMPL JSON conversion with optional legacy parameters. | `SIMPL Backwards Compatibility`. |
| 19 | Atomic-file commit after a successful ITK write. | Covered by successful output tests; no injected commit failure. |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `ITKImageProcessing::ITKImageWriterFilter: Analytical Pixel Order` | new-for-V&V | Tests Class 1+4 Oracle over all accepted scalar types, including decoded XY/XZ/YZ pixels, spacing, and MetaImage origin. |
| `ITKImageProcessing::ITKImageWriterFilter: Fill Character Validation` | new-for-V&V | Empty fill character is rejected with `-25601`; format-control and path-separator characters are rejected with `-25602`. |
| `ITKImageProcessing::ITKImageWriterFilter: Dimension Mismatch Validation` | new-for-V&V | Mismatched array and geometry input is rejected during preflight with error `-25600`. |
| `ITKImageProcessing::ITKImageWriterFilter: Single Slice Keeps Exact Output Name` | new-for-V&V | A 3x1x2 ImageGeom XZ plane previews and writes an unsuffixed `.mha` file with exact decoded pixels. |
| `ITKImageProcessing::ITKImageWriterFilter: RGBA Image Output` | new-for-V&V | A uint8 RGBA pixel is dispatched and decodes as `(10,20,30,40)`. |
| `ITKImageProcessing::ITKImageWriterFilter: Write Stack` | kept | Checks that the preflight preview starts at the configured index offset and that the image is written as a stack of files. |
| `ITKImageProcessing::ITKImageWriterFilter: SIMPL Backwards Compatibility` | kept | Covers SIMPL json backwards compatibility. |

## Exemplar archive

No new exemplar archive was created for this V&V cycle: the Class 1 oracle is encoded entirely as inline expected values in the test source.

## Deviations from DREAM3D 6.5.171

- `ITKImageWriterFilter-D1` - NX defaults to zero-padded slice indices while 6.5.171 does not - see `vv/deviations/ITKImageWriterFilter.md`.
- `ITKImageWriterFilter-D2` - NX writes the selected plane's physical spacing and origin, while 6.5.171 used identity 2D metadata for XZ/YZ output - see `vv/deviations/ITKImageWriterFilter.md`.

All pixels otherwise match: the analytical fixture has 2 XY, 2 XZ, and 3 YZ slices; Small IN100 has 117 XY, 201 XZ, and 189 YZ float32 TIFF slices.
