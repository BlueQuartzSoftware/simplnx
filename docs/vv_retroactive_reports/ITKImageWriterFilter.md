# Retroactive V&V: ITKImageWriterFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `a181ee3e-1678-4133-b9c5-a9dd7bfec62f` |
| SIMPLNX ClassName | `ITKImageWriterFilter` |
| SIMPLNX Human Name | Write Image (ITK) |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL/ITKImageProcessing repo)* |
| SIMPL ClassName | `ITKImageWriter` *(presumed; confirm in legacy)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | ITKImageProcessing |

### Source files scanned

- `src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters/ITKImageWriterFilter.{hpp,cpp}`
- `src/Plugins/ITKImageProcessing/test/ITKImageWriterTest.cpp`
- `src/Plugins/ITKImageProcessing/test/simpl_conversion/6_5/ITKImageWriterFilter.json`
- `src/Plugins/ITKImageProcessing/test/simpl_conversion/6_4/ITKImageWriterFilter.json`
- `src/Plugins/ITKImageProcessing/docs/ITKImageWriterFilter.md`
- `src/Plugins/ITKImageProcessing/test/CMakeLists.txt`

There is **no** `Algorithms/ITKImageWriter.{hpp,cpp}` file — the writer logic lives entirely in the filter `.cpp` inside the anonymous-namespace helpers `cxITKImageWriterFilter::WriteAsOneFile`, `WriteAs2DStack`, `WriteImage`, `CopyTuple`, and `SaveImageData`. The file dispatches to `itk::ImageFileWriter` (single-file output) or `itk::ImageSeriesWriter` (per-slice stack output) via `ITK::ArraySwitchFunc<WriteImageFunctor, ITK::ScalarVectorPixelIdTypeList>`.

## Algorithm Relationship

- **Tentative classification:** **Port (thin wrapper).** This filter is a translation of the legacy SIMPL `ITKImageWriter` filter. The bulk of the work is delegated to ITK's `itk::ImageFileWriter` / `itk::ImageSeriesWriter`. The DREAM3DNX-only logic is: (1) plane selection (XY/XZ/YZ) and per-slice tuple re-shuffling via `CopyTuple`; (2) `Is2DFormat` extension lookup that decides single-file-vs-stack; (3) auto-creation of missing parent directories; (4) `AtomicFile` wrapping for safe writes; (5) the `IndexOffset` / `TotalIndexDigits` / `LeadingDigitCharacter` filename-generation parameters added in PR #1489.
- **Evidence:** UUID is preserved across SIMPL→SIMPLNX (the same UUID appears in 6_4 and 6_5 SIMPL conversion fixtures). The pixel-type dispatch uses the standard simplnx `ITK::ScalarVectorPixelIdTypeList`. The plane-loop / slice-extraction / `CopyTuple` block is hand-written in this `.cpp`, not delegated to ITK — that part is a candidate for an algorithm-level review.
- **Action required:** Compare the `CopyTuple` slice extraction logic against the legacy SIMPL implementation to determine whether the indexing arithmetic was carried over verbatim (Port) or re-derived (Minor rewrite). Note: the `uint32` and `uint64` cases of `CopyTupleTyped` both call `CopyTupleTyped<int32>` / `CopyTupleTyped<int64>` — see Deviation candidate D2 below.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 `static inline`) and broad refactor PRs that did not touch this filter (#1301, #1439, #1472, #1501, #1521, #1523, #1524, #1535, #1538, #1544, #1582, #1590) are listed at the bottom of this section. **PR #1590 (Standardize 2D Image Handling)** was carefully checked because of the policy guidance — **it does not touch ITKImageWriterFilter.cpp/.hpp/test/.md or ITKArrayHelper**; #1590's 2D scope is on neighbor utilities, `ImageGeom::findElementSizes`, and Identify-Sample / FillBadData / Compute-Feature-Neighbors. Truly pruned for this filter.

### PR #1449 — *"ENH: Crop Geometry Parameter"* — merged 2025-10-16

- **Files in this filter:** test (.cpp), +4 / -4 lines
- **Diff size:** trivial — only changed the `ITKImportImageStackFilter` setup block in the test from `float32` origin/spacing to `float64`. No behavioral change to ITKImageWriter.
- **Change nature:** Test-fixture update riding along with the `CropGeometryParameter` PR. The actual cropping changes were on the reader side.
- **V&V content:** None for the writer.

### PR #1457 — *"STY: Clean up 'static inline' from filter headers"* — merged 2025-10-22

- **Files in this filter:** filter (.hpp), +5 / -5 lines
- **Change nature:** Pure style — removed redundant `inline` from `static constexpr` parameter-key declarations.
- **V&V content:** None.

### PR #1489 — *"ENH: ItkImageWriter allow user to set the number of padding digits and the fill char"* — merged 2025-12-17

- **Files in this filter:** filter (.cpp), filter (.hpp), test (.cpp). +64 / -21 across the trio.
- **Change nature:** **Material feature addition.** Added two new parameters: `k_TotalIndexDigits_Key` (Int32, default 3) and `k_LeadingDigitCharacter_Key` (StringParameter, default `"0"`). Behavior changes:
  - Previously the writer computed `totalDigits = log10(maxSlice) + 1` automatically and hard-coded `'0'` as the fill character. Now the user controls both.
  - Added a preflight error code `-25601` for fill-character size > 1.
  - Replaced the old preflight error code `-1` with `-25600` for the dimension-mismatch case.
  - Added a `preflightUpdatedValues` entry showing an "Example Output File" preview string.
  - Bumped `parametersVersion()` from 1 → 2.
  - Removed a commented-out in-memory-only check.
- **V&V content:** Test file got 8 new lines (the new args were added to the existing three plane-direction sub-blocks). The new error codes are not directly exercised by the test. The parameter-version bump is correct policy.

### PR #1490 — *"STY: Fix warnings about unintended slicing of object"* — merged 2026-01-09

- **Files in this filter:** filter (.cpp), 1 line changed.
- **Change nature:** Pure style — `Result<>` / `ResultVoid` slicing fix. No behavioral change.
- **V&V content:** None.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2026-01-06

- **Files in this filter:** filter (.cpp), +12 / -2 lines (in the `FromSIMPLJson` block).
- **Change nature:** **SIMPL conversion bug fix.** The `Plane` and `IndexOffset` SIMPL parameters do not appear in some 6.5 pipelines; the previous code hard-pushed both `Result<>`s into the result vector even when the JSON keys were absent, which would mark the whole conversion invalid. Fixed by guarding both with `if(result.valid())` so the absent-key cases now fall through to default values silently. (Identical pattern was applied across many filters in this PR.)
- **V&V content:** **Material for SIMPL-conversion correctness.** This PR is the reason the corresponding `simpl_conversion/6_4/ITKImageWriterFilter.json` fixture is permitted to omit the `Plane` parameter without failing — see the per-fixture branch in the test file (`if(label == "SIMPL 6.5 (UUID)")`).

### PR #1555 — *"ENH: Again require in-memory data for ITK filters"* — merged 2026-03-05

- **Files in this filter:** filter (.cpp), 1 line changed.
- **Change nature:** Changed `dynamic_cast<AbstractDataStore<...>&>` to `dynamic_cast<DataStore<...>&>` in `WriteImage`. The cast target is now the concrete in-memory `DataStore<T>` rather than the abstract base. **Effective behavior:** if the input array is in an out-of-core `DataStore` subclass, this `dynamic_cast` will throw `std::bad_cast` at execute time, effectively re-asserting the in-memory requirement that an earlier change had relaxed.
- **V&V content:** Material for **OOC compatibility** — this filter now silently fails on OOC inputs. The previous commented-out explicit check (`// DataArray must be in memory`) was deleted by PR #1489, so the only remaining gate is the `dynamic_cast` itself, which produces a less-helpful error. Candidate for an explicit preflight check + clearer error message; see Deviation D3 below.

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md), +8 lines.
- **Change nature:** Documentation hygiene — added a `### Plane` subsection enumerating the XY / XZ / YZ choices.
- **V&V content:** Doc currency improvement. Not algorithmic.

### PR #1576 — *"ENH: Improve error messages across the codebase"* — merged 2026-04-06

- **Files in this filter:** filter (.cpp), +5 / -1 lines.
- **Change nature:** Replaced the dimension-mismatch error string `"Image Array dimensions must match ImageGeometry"` with a fully-formatted message using the new `StringUtilities::formatTupleShape3D` and `formatDimensions3D` helpers — now lists the array path, its tuple shape, the geometry path, and its dimensions with axis labels.
- **V&V content:** Error-message clarity only. No behavioral change. Useful when verifying the dimension-mismatch path in a unit test (the test currently doesn't exercise this error path).

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +52 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ITKImageWriterFilter.json`
  - `test/simpl_conversion/6_5/ITKImageWriterFilter.json`
- **Change nature:** **Test addition.** Added the per-filter SIMPL→SIMPLNX backwards-compatibility test. Test name: `"ITKImageProcessing::ITKImageWriterFilter: SIMPL Backwards Compatibility"`. Uses `DYNAMIC_SECTION` over the 6.4 (`Filter_Name` lookup) and 6.5 (UUID lookup) fixtures. The 6.5 branch additionally checks that `Plane == 0` and `IndexOffset == 5` survive the conversion; the 6.4 fixture omits those parameters and the test correctly skips those checks.
- **V&V content:** **Pipeline-conversion correctness only.** Verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. Does **not** verify that the writer's *output file* matches a file written by SIMPL.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1449 | Crop Geometry Parameter | Test float32→float64 fixture-update only |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1490 | Fix warnings about unintended slicing of object | Style |
| #1571 | Add standardized ChoicesParameter descriptions to filter docs | Documentation |
| #1590 | Standardize 2D Image Handling | **Did not touch this filter or ITKArrayHelper** — 2D-handling work was on neighbor utilities and `ImageGeom::findElementSizes` |

## Test coverage detected

`ITKImageWriterTest.cpp` contains **2** `TEST_CASE`s:

1. `ITKImageProcessing::ITKImageWriterFilter: Write Stack` — End-to-end test. Reads a 3-image TIFF stack via `ITKImportImageStackFilter`, then writes it back out three separate times — once per plane (XY, XZ, YZ) — using a randomly-named temp output directory. Verifies that the expected number of slice files (Z-count, Y-count, X-count respectively) exist on disk with the correct numeric suffix (offset = 100, 3-digit padding, '0' fill), removes each, and confirms no extra files were written. **Does not** read the written files back and compare pixel values to the input — only the existence/count/naming of output files is verified.
2. `ITKImageProcessing::ITKImageWriterFilter: SIMPL Backwards Compatibility` *(added by PR #1588)* — `DYNAMIC_SECTION` over SIMPL 6.4 (`Filter_Name`) and 6.5 (UUID) conversion fixtures. Verifies parameter mapping; does not run the filter.

**Coverage gaps:**

- Plane is exercised, but only with an in-memory grayscale TIFF stack. **No coverage of:** RGB/RGBA multi-component inputs, non-TIFF formats (PNG/JPG/BMP/NRRD/MetaImage/NIfTI), 2D inputs (i.e. `Dimensions==3` with `size[2]==1` is not tested through `WriteAsOneFile`), single-slice writes (the `maxSlice == 1` branch in `SaveImageData` is not exercised by Test 1, which always has 3 slices), `IndexOffset==0`, fill-character variations (the test only uses `"0"`), `TotalIndexDigits` variations.
- **No round-trip pixel comparison** — the test verifies file *presence* but not file *content*.
- **No error-path tests** — the dimension-mismatch (-25600), fill-char-size>1 (-25601), 2D-format-but-only-one-Z-slice (-21012), and ITK-exception (-21011) paths are all uncovered.
- **No OOC-input test** that would exercise the post-#1555 `dynamic_cast<DataStore<T>&>` behavior.

## Exemplar archive

- **Archive name(s) referenced in `test/CMakeLists.txt`:**
  - `fiji_montage.tar.gz`
  - `image_flip_test_images.tar.gz`
  - `import_image_stack_test.tar.gz`
  - `import_image_stack_test_v2.tar.gz`
  - `itk_image_reader_test.tar.gz`
- **Used by ITKImageWriter test:** The `Write Stack` test reads from `${unit_test::k_DataDir}/ImageStack` — that directory comes from one of the `import_image_stack_test*.tar.gz` archives (used by `ITKImportImageStackTest`). There is **no exemplar dataset specifically for ITKImageWriter**: the test produces fresh files in a randomly-named temp directory and deletes them after checking existence.
- **Provenance:** Not applicable — no golden output files are checked in for the writer's output.
- **Action required:** If V&V is to be done at the file-content level, a new exemplar set of "expected ITK-written files" must be produced (e.g., reference TIFF / PNG / NRRD outputs generated by Python ITK on a known input volume) and uploaded to the GitHub data archive.

## Oracle classification (tentative)

The retroactive policy brief proposed Class 2 (Reference-implementation) as the natural oracle, with Class 4 (Invariant) and Class 1 (Analytical) as companions. The DRAFT recommendation here:

- **Recommended primary class:** **Class 2 (Reference-implementation)** — for each supported file format (TIFF, PNG, JPG, BMP, NRRD, MetaImage, NIfTI), construct a small in-memory ImageGeom + cell DataArray with known content, run ITKImageWriterFilter to produce file `A`, and run a Python script using `itk.imwrite` (or C++ `itk::ImageFileWriter` directly) on the same numpy/in-memory data to produce file `B`. Compare:
  - **Lossless formats** (TIFF without compression, BMP, MetaImage `.mha`, NRRD, NIfTI uncompressed): bit-exact comparison of the pixel-data sections, plus header field equality on `dimensions`, `spacing`, `origin`. Header byte-for-byte equality is unlikely (timestamps, library version strings) and should not be required.
  - **Lossy formats** (JPG, optionally TIFF with lossy compression): comparison via decoded pixel-buffer L∞ norm with a tolerance.
- **Recommended companion class:** **Class 4 (Invariant-based).**
  - **Round-trip invariant:** `ITKImageWriter → ITKImageReader → equality with original` for lossless formats. Already partially testable today using the existing `ITKImportImageStackFilter` + `ITKImageWriterFilter` pair.
  - **Metadata invariant:** Written file's reported dimensions / spacing / origin equal the input ImageGeom's, modulo the per-slice case where Z-dim collapses to 1.
  - **File-count invariant:** When writing a 3D volume to a 2D-only format, the number of output files equals the size along the chosen plane axis. This invariant is what the existing `validateOutputFiles()` helper already checks — promote it to an explicit invariant assertion.
- **Optional Class 1 (Analytical):** A 2×2 grayscale uint8 image with hand-picked pixel values produces a TIFF whose binary content can be computed from the TIFF specification by hand. Useful as a sanity check; arguably overkill given that Class 2 already pins behavior to ITK.
- **Action required:** Developer to confirm the format matrix actually exercised in production pipelines (TIFF dominates in EBSD pipelines per the example pipelines `Edax_IPF_Colors`, `TxCopper_Exposed/Unexposed`, `aptr12_Analysis`, etc. — these all write `.tif`) and decide which formats merit explicit Class-2 testing.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | The `cxITKImageWriterFilter::CopyTuple` plane-extraction arithmetic should be reviewed line-by-line; it is the non-ITK-delegated portion. The `uint32`/`uint64` typo in `CopyTuple` (see Deviation D2) suggests the dispatcher was not reviewed. |
| Code path coverage (algorithmic) | Partial | Three planes covered. Single-file output, RGB inputs, all non-TIFF formats, 2D inputs, and the `maxSlice==1` branch are uncovered. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added 6.4 + 6.5 conversion test. Notably exercises the PR #1476 fix (Plane/IndexOffset absent in 6.4 fixture). |
| Exemplar data in Data_Archive | **No** | Writer has no per-test exemplar archive; only the input image stack from `import_image_stack_test*.tar.gz` is downloaded. |
| Exemplar provenance documented | N/A | None to document. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No Python ITK reference script on file. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. |
| Deviation entries (`ITKImageWriter-D<N>`) | None | Not yet written. Candidates: D1 (OOC support gap), D2 (`uint32`/`uint64` mis-dispatch in `CopyTuple`), D3 (silent `dynamic_cast` failure on OOC inputs), D4 (`WriteAs2DStack` off-by-one loop bound). |
| Documentation currency | Partial | PR #1571 added `### Plane`. Missing: documentation for the new (PR #1489) `Total Number of Index Digits` / `Fill Character` parameters; the description still claims only "TIF, BMP, or PNG" but the code dispatches to whatever ITK supports including NRRD/MetaImage/NIfTI. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 2 (reference-implementation against Python ITK / direct ITK C++) is the recommended primary; Class 4 invariants are the natural companion. Defend or replace.
2. **Build the format matrix.** Decide which file formats are formally validated. Production EBSD pipelines write `.tif`, so TIFF is the minimum bar. Recommend at minimum: TIFF (lossless), PNG (lossless), NRRD (lossless 3D), and one lossy (JPG with tolerance).
3. **Add a content-comparison test.** Today the test only checks file existence and count. Add a round-trip test (Class 4 invariant): write → read back → compare. This is cheap and immediately exercises the lossless guarantee.
4. **Add the missing error-path tests.** `-25600` (dim mismatch), `-25601` (multi-char fill), `-21012` (3D-to-2D-format with Z<2). The fill-char check is a one-line test.
5. **Decide the OOC story.** Either (a) add an explicit preflight check that rejects out-of-core stores with a clear message, restoring the spirit of the deleted in-memory check, or (b) implement chunk-by-chunk writing for OOC inputs. Currently the filter throws `std::bad_cast` at execute time on OOC inputs, which is the worst of both worlds.
6. **Update the docs** to reflect (a) the new `TotalIndexDigits` / `LeadingDigitCharacter` parameters added in PR #1489 and (b) the actual format support beyond TIFF/BMP/PNG.
7. **Run the legacy comparison.** Use `compare-legacy-dream3d` to write the same input image with SIMPL `ITKImageWriter` and SIMPLNX `ITKImageWriterFilter` and diff the output files.
8. **Algorithm review** of `cxITKImageWriterFilter::CopyTuple` / `CopyTupleTyped<uint32>` / `CopyTupleTyped<uint64>` to address the dispatch bug noted in D2.
9. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ITKImageWriter-D1`
> **Filter UUID:** `a181ee3e-1678-4133-b9c5-a9dd7bfec62f`
> **Symptom:** Filter requires the input array to be backed by an in-memory `DataStore<T>`. Out-of-core inputs cause `std::bad_cast` at execute time with no clear error message.
> **Root cause:** PR #1555 reverted the cast in `WriteImage` from `AbstractDataStore<T>` to the concrete `DataStore<T>`. PR #1489 had previously deleted the explicit in-memory preflight check (the commented-out `if(currentData.getStoreType() != IDataStore::StoreType::InMemory)` block).
> **Affected users:** Anyone running this filter on a pipeline that uses OOC storage for the image array.
> **Recommendation:** Restore an explicit preflight check and a user-facing error code, or implement chunk-aware writing through `ITK::WrapDataStoreInImage`'s OOC variant.
> **Status:** Proposed.

---

> **Deviation ID:** `ITKImageWriter-D2`
> **Filter UUID:** `a181ee3e-1678-4133-b9c5-a9dd7bfec62f`
> **Symptom:** When the input array's `DataType` is `uint32`, `CopyTuple` dispatches to `CopyTupleTyped<int32>` (signed). Same issue for `uint64` → `CopyTupleTyped<int64>`. See `ITKImageWriterFilter.cpp:209-211, 217-219`.
> **Root cause:** Apparent copy/paste typo in the type-dispatch `switch` inside `CopyTuple`.
> **Practical impact:** The body of `CopyTupleTyped` calls `AbstractDataStore<T>::copyFrom`, which copies bytes via the `T` interface. Because `int32` and `uint32` have the same in-memory size and `copyFrom` does not interpret the values, this is **likely** a benign byte-copy with no observable correctness impact at the slice-extraction step. **However**, the subsequent ITK pixel-type dispatch in `WriteImageFunctor` keys off the array's actual `DataType`, not off the `CopyTupleTyped<T>` parameter, so this typo does not propagate downstream. **It is still a bug** and should be fixed for code clarity and to defend against future refactoring.
> **Affected users:** None observable today, assuming the byte-copy reasoning above holds. A regression here would silently corrupt unsigned-32/64-bit images.
> **Recommendation:** Trivial fix: change `CopyTupleTyped<int32>` → `CopyTupleTyped<uint32>` and `CopyTupleTyped<int64>` → `CopyTupleTyped<uint64>` in the corresponding case branches. Add a unit test that writes a `uint32` and `uint64` image.
> **Status:** Proposed — code-review finding, no legacy comparison needed.

---

> **Deviation ID:** `ITKImageWriter-D3`
> **Filter UUID:** `a181ee3e-1678-4133-b9c5-a9dd7bfec62f`
> **Symptom:** In `WriteAs2DStack`, the file-generation loop is `for(uint64 index = indexOffset; index < (z_size - 1); index++)` — note the `- 1`. If a 3D volume has `Z = 5` and `indexOffset = 0`, this writes only 4 files (indices 0..3), not 5. **However**, this stack writer is invoked only from `WriteImage` when `Is2DFormat(filePath) && Dimensions == 3`, and `WriteImage` is only ever called on the post-extraction `*sliceData` whose `Dimensions == 2`. In the current `executeImpl` flow, `WriteAs2DStack` is therefore **dead code** — the slice loop runs in the filter, each slice is passed to `SaveImageData` → `WriteImage` with a 2D `sliceData` and `Dimensions==2`, and `WriteAs2DStack`'s `Dimensions==3` guard is never triggered.
> **Root cause:** The `WriteAs2DStack` path appears to be a leftover from an earlier design where the filter delegated multi-slice writing to ITK's `ImageSeriesWriter` directly, before the per-plane `CopyTuple` slice-extraction was introduced.
> **Affected users:** None today (dead code).
> **Recommendation:** Either delete `WriteAs2DStack` and the `Is2DFormat`-and-`Dimensions==3` branch in `WriteImage`, or fix the loop bound (`index < z_size + indexOffset`) and add a code path that actually exercises it. Document the intended behavior.
> **Status:** Proposed — code-review finding.

---

> **Deviation ID:** `ITKImageWriter-D4`
> **Filter UUID:** `a181ee3e-1678-4133-b9c5-a9dd7bfec62f`
> **Symptom:** Documentation claims output formats are "TIF, BMP, or PNG", but the actual format dispatch is whatever `ITKImageProcessingPlugin::GetList2DSupportedFileExtensions()` returns (and `itk::ImageFileWriter` for non-2D-only formats), which includes NRRD, MetaImage `.mha`/`.mhd`, NIfTI `.nii`/`.nii.gz`, and others.
> **Root cause:** Doc string never updated as ITK format support grew.
> **Affected users:** Anyone reading the docs to find out what formats are supported.
> **Recommendation:** Update `ITKImageWriterFilter.md` to enumerate the actual 2D format list from `GetList2DSupportedFileExtensions()` and the volumetric formats supported by the single-file path. Cross-link to `ITKImageProcessingPlugin::GetList2DSupportedFileExtensions()` so the doc stays accurate.
> **Status:** Proposed — documentation hygiene.
