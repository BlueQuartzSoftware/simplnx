# V&V Report: ITKImageWriterFilter

| | |
|---|---|
| Plugin | ITKImageProcessing |
| SIMPLNX UUID | `a181ee3e-1678-4133-b9c5-a9dd7bfec62f` |
| DREAM3D 6.5.171 equivalent | `ITKImageWriter` (SIMPL UUID `11473711-f94d-5d96-b749-ec36a81ad338`) - `Source/Plugins/ITKImageProcessing/ITKImageProcessingFilters/ITKImageWriter.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | <engineer(s), date> |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | Minor changes - same XY/XZ/YZ options; NX uses SIMPLNX stores, AtomicFile, and current ITK APIs. |
| Oracle (confirmed) | Classes 1 + 4 - one 3x2x2 scalar fixture (`value(x,y,z)=x+10y+100z`) has same XY/XZ/YZ pixels for all ten accepted types; uint8 uses TIFF and the remaining types use MetaImage. |
| Code paths enumerated | 17 of 18 branches exercised; only ITK write-failure propagation remains untested. |
| Tests today | 6 test cases - 1 Class 1+4 Oracle, 2 preflight error path tests, 1 image format supports write as 1 file test, 1 stack writing test, 1 SIMPL backwards compatibility test |
| Exemplar archive | None - Class 1+4 oracle uses inline data |
| Legacy comparison | Run against DREAM3D 6.5.171 with the inline test data for the Class 1+4 Oracle. Each slice matches exactly; D1 records filename formatting difference only. |
| Bug flags | Unsigned uint32/uint64 dispatch was corrected for this V&V cycle. |
| V&V phase | Ready for review |

## Summary

ITKImageWriterFilter exports scalar ImageGeom cell data as an ITK image or a 2D image stack. A hand-derived non-square fixture verifies decoded pixel orientation independently of legacy, while the legacy comparison checks that migration preserves decoded output on the toy and production fixtures.

## Algorithm Relationship

**Minor changes.** Port-time changes are SIMPLNX DataStructure stores, AtomicFile writes, and current ITK APIs; the pixel comparisons show no output change from these substitutions.

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

**Applied:** The 3x2x2 scalar fixture has `value(x,y,z)=x+10y+100z`; exact XY, XZ, and YZ pixel matrices are hand-derived and decoded for all ten accepted types. uint8 uses TIFF; the remaining types use MetaImage. File creation and decoded dimensions are companion invariants.

**Encoded:** `test/ITKImageWriterTest.cpp::ITKImageProcessing::ITKImageWriterFilter: Analytical TIFF Pixel Order` - templated over all ten accepted scalar types; all checks pass.

**Second-engineer review:** pending second-engineer review.

## Code path coverage

17 of 18 branches exercised. Source: `src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters/ITKImageWriterFilter.cpp` (533 lines).

| # | Path | Test case |
|---|---|---|
| 1 | Valid preflight. | `Analytical TIFF Pixel Order`, `Write Stack` |
| 2 | Dimension-mismatch preflight error. | `Dimension Mismatch Validation` - a 1x1x1 array and 1x1x2 geometry return `-25600`. |
| 3 | Fill-character validation, including empty input. | `Fill Character Validation` - empty string returns `-25601`. |
| 4 | XY output for all accepted scalar types. | `Analytical TIFF Pixel Order` - same pixels; uint8 TIFF, other types MetaImage. |
| 5 | XZ output for all accepted scalar types. | `Analytical TIFF Pixel Order` - same pixels; uint8 TIFF, other types MetaImage. |
| 6 | YZ output for all accepted scalar types. | `Analytical TIFF Pixel Order` - same pixels; uint8 TIFF, other types MetaImage. |
| 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 | All accepted scalar dispatch arms: int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, and float64. | `Analytical TIFF Pixel Order` - same TIFF pixels for uint8 and MetaImage pixels for every other type. |
| 17 | Single-file output for a non-2D format. | `3D Image Single-File Output` - a 3x1x2 ImageGeom XZ plane writes one unsuffixed MetaImage file with exact decoded pixels. |
| 18 | Filesystem/ITK write failure. | *Not directly tested; requires failure injection.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `ITKImageProcessing::ITKImageWriterFilter: Analytical TIFF Pixel Order` | new-for-V&V | Tests Class 1+4 Oracle over all accepted types. |
| `ITKImageProcessing::ITKImageWriterFilter: Fill Character Validation` | new-for-V&V | Empty fill character is rejected during preflight with error `-25601`. |
| `ITKImageProcessing::ITKImageWriterFilter: Dimension Mismatch Validation` | new-for-V&V | Mismatched array and geometry input is rejected during preflight with error `-25600`. |
| `ITKImageProcessing::ITKImageWriterFilter: 3D Image Single-File Output` | new-for-V&V | A 3x1x2 ImageGeom XZ plane writes an unsuffixed `.mha` file with exact decoded pixels. |
| `ITKImageProcessing::ITKImageWriterFilter: Write Stack` | kept | Checks that an image is written as an image stack of files. |
| `ITKImageProcessing::ITKImageWriterFilter: SIMPL Backwards Compatibility` | kept | Covers SIMPL json backwards compatibility. |

## Exemplar archive

No new exemplar archive was created for this V&V cycle: the Class 1 oracle is encoded entirely as inline expected values in the test source.

## Deviations from DREAM3D 6.5.171

- `ITKImageWriterFilter-D1` - NX defaults to zero-padded slice indices while 6.5.171 does not - see `vv/deviations/ITKImageWriterFilter.md`.

All pixels otherwise match: the analytical fixture has 2 XY, 2 XZ, and 3 YZ slices; Small IN100 has 117 XY, 201 XZ, and 189 YZ float32 TIFF slices.
