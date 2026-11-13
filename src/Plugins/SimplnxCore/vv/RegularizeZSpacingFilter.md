# V&V Report: RegularizeZSpacingFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID | `d6599986-1932-4bfc-993d-71eafefe6db0` |
| DREAM3D 6.5.171 equivalent | `RegularizeZSpacing` — `Source/Plugins/Sampling/SamplingFilters/RegularizeZSpacing.{h,cpp}` (legacy UUID `bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | *Michael Jackson <mike.jackson@bluequartz.net> (port + V&V cycle, 2026-07-08); |

*Updated 2026-10-29 for the Clear Feature Attribute Matrix change on `topic/correct_feature_am_updates`. Added the Feature Attribute Matrix paths, 12 new tests, the oracle for them, and deviation D1.*

*Updated 2026-11-03: added one test for a Feature Attribute Matrix that is also linked inside another child group of the geometry (`Feature AM nested alias regression`).*

*Updated 2026-11-04: reconciled the test inventory after the rebase onto upstream #1897. Added inventory rows for the three upstream extension tests and set their Feature options explicitly for the cell-only fixture.*

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** of legacy `RegularizeZSpacing::execute()` — identical Z-plane mapping rule and `floor(extent/newZRes)` dim math (independently hand-traced, including the strict-`>` boundary and clamp cases). Six port-time deltas. Five (bulk copy, new-geometry output mode, preflight validation, parallelization, cell-AM binding) do not change output for valid input. The sixth, *Clear Feature Attribute Matrix* (default ON), empties the Feature Attribute Matrix in native pipelines (D1). |
| Oracle (confirmed)     | **Class 1 (Analytical)** primary + **Class 4 (Invariant)** companion — closed-form indirection map `out[i] = in[map[i]]`. Element-wise Class 1 assertions in 2 fixtures (`Valid Execution (New Geometry)`, `Valid Execution (Spacing Exceeds Extent)`); Class 4 invariants across the valid fixtures. All pass in-core + OOC. The Feature Attribute Matrix tests use hand-computed Class 1 expectations from a 1×1×4 Feature fixture (see Oracle). The new tests have been run in-core only so far. |
| Code paths enumerated  | 23 of 25 exercised; the 2 uncovered are the redundant file-open guard and the cancel checks (reasons below).                  |
| Tests today | 22 `TEST_CASE`s: 21 registered cases plus one hidden HDF5 boundary test. 6 are kept from the V&V cycle (all 6 modified on `topic/correct_feature_am_updates`), 13 are new on that branch, and 3 are from upstream #1897. The in-core CTest selection passes 21/21 (2026-11-04). |
| Exemplar archive       | **None** — inline analytical fixtures (no `download_test_data`; avoids a circular oracle per project policy).                 |
| Legacy comparison      | **Run** vs DREAM3D 6.5.171 on a synthetic multi-type fixture (int32 + bool + 3-component float). Bit-identical: legacy == SIMPLNX == Class-1 oracle on every array and on geometry (dims/spacing/origin). The fixture had no Feature Attribute Matrix. One documented deviation: `RegularizeZSpacingFilter-D1` (Feature Attribute Matrix cleared by default in native pipelines). |
| Bug flags              | None. D1 is an intentional design choice, not a bug.              |
| V&V phase | Historical status and sign-off retained. Section 4.3 adds real-HDF5 boundary verification and paired CTest evidence. |

## Summary

`RegularizeZSpacingFilter` resamples an Image Geometry with irregular Z slice spacing onto a uniform Z spacing: it reads the physical Z position of each original slice boundary from a text file (`ZPoints + 1` values), computes `newZ = floor(totalZExtent / newZSpacing)` (min 1), and copies each new plane's cell data from the original plane whose boundary interval contains it. Verification used a **Class 1 (Analytical)** oracle — the algorithm is a closed-form indirection lookup `output[i] = input[map[i]]` — with **Class 4 (Invariant)** companions (X/Y dims and spacing preserved, origin preserved, Z spacing becomes the requested value). A direct A/B against DREAM3D 6.5.171 on a synthetic multi-type dataset is **bit-identical** on cell data and geometry; legacy, SIMPLNX, and the analytical oracle agree exactly. Since 2026-10-29 the filter also clears the selected Feature Attribute Matrix by default, with optional renumbering of Feature Ids. Feature values are stale after Z resampling. This is one documented deviation from 6.5.171 (D1). Converted 6.x pipelines start with Clear off and still match legacy.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* Near line-by-line translation of legacy `RegularizeZSpacing::execute()`. Same SIMPL UUID retained and mapped in `SimplnxCoreLegacyUUIDMapping.hpp`. The new→old Z-plane mapping loop (`plane` = largest `iter` in `[1, origZ)` with `i*newZRes > zBound[iter]`, else 0) and the `newZ = (size_t)(lastZBound / newZRes)` (min 1) computation are reproduced exactly; an independent hand-trace confirmed the arithmetic types match legacy expression-by-expression (float32 throughout, no double promotion), so even float-boundary cases agree.

*Port-time deltas (deltas 1-5 do not change output for valid input — confirmed bit-identical in the legacy A/B; delta 6 is deviation D1):*

1. **Data copy**: legacy per-tuple `memcpy` via a rebuilt `newindicies` table → SIMPLNX bulk `AbstractDataStore::copyFrom` of one contiguous plane-slab per destination plane. Same bytes moved; more efficient and out-of-core friendly.
2. **Output mode**: legacy replaces the cell AttributeMatrix in place → SIMPLNX uses the modern new-geometry pattern (`CreateImageGeometryAction` + rename + deferred delete) with a "Perform In Place" toggle (default true). In-place mode is behaviorally equivalent to legacy.
3. **Preflight validation added**: positive `new_z_spacing`; input file has ≥ `ZPoints + 1` parseable values; values monotonically non-decreasing; positive total Z extent; only DataArray members in the cell AM. These reject bad input that legacy silently consumed (short files reused the last-read value; non-monotonic files produced a garbage mapping; zero extent produced a clamped 1-plane output) but do not alter valid-input output. The input file must have a `.txt`, `.dat`, or `.csv` extension. Converted legacy pipelines that reference other extensions now fail preflight with error `-3003`; files without an extension fail with error `-3002`. Rename these files and update the paths in the pipelines.
4. **Parallelization**: per-array parallel task runner (one task per cell array, distinct stores) vs legacy serial loop. No output effect.
5. **Cell-AM binding**: legacy took an explicit AttributeMatrix path; SIMPLNX takes the geometry and uses its assigned cell AttributeMatrix. `FromSIMPLJson` keeps only the DataContainer name from the legacy `CellAttributeMatrixPath` (the AM name is discarded). For the overwhelmingly common single-cell-AM case this is identity; corner-case consequences for converted pipelines are documented in the deviations file's scope note. Related accept/reject divergences: a geometry with no cell AM now errors (`-5560`, impossible to express in legacy), and a stale AM name that legacy rejected converts to the geometry's actual cell AM and proceeds.
6. **Clear Feature Attribute Matrix** (added 2026-10-29): when on (the default), the selected Feature Attribute Matrix is recreated in the output geometry with its source tuple shape and no arrays. Optional *Renumber Features* compacts the Feature Ids and resizes the empty matrix. When off, every non-cell Attribute Matrix is copied unchanged, as in legacy. Older native pipelines also load with Clear ON. They must select a valid Feature Attribute Matrix and Cell Feature Ids array, or turn Clear off, before this filter can pass preflight. `FromSIMPLJson` sets Clear to off. Legacy left the Feature Attribute Matrix unchanged, so native pipelines differ from 6.5.171. See `RegularizeZSpacingFilter-D1`.

*Material PRs since baseline:* new filter port; then the Clear Feature Attribute Matrix change on `topic/correct_feature_am_updates` (delta 6).

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion.

*Applied (Class 1):* The output cell data is a closed-form indirection lookup `output_tuple[t] = input_tuple[map[t]]`, where `map` is derived by hand from the Z-positions file and `new_z_spacing`. Primary fixture (origZ=4, zBound `{0,1,3,6,10}`, newZRes 2.0): new→old plane map `[0,1,2,2,3]`, giving a fully hand-derived 10-tuple expected output. Clamp fixture (same bounds, newZRes 20.0): `newZ = floor(10/20) = 0 → 1`, expected output = source plane 0. Both derivations are embedded as comments beside the `REQUIRE`s.

*Applied (Class 4):* X and Y dimensions and spacing preserved, origin preserved, Z spacing equals `new_z_spacing`, `newZ == max(1, floor(lastZBound / newZRes))`, and loose (non-cell-AM) children are carried over intact. Asserted in the valid-execution fixtures.

*Encoded:* `test/RegularizeZSpacingTest.cpp::SimplnxCore::RegularizeZSpacingFilter: Valid Execution (New Geometry)` (element-wise Class 1 + invariants), `...::Valid Execution (Spacing Exceeds Extent)` (clamp-path Class 1), `...::Valid Execution (In Place)` (in-place invariants) — all pass in-core (`NX-Com-Qt69-Vtk95-Rel`) and out-of-core (`NX-OOC-Qt69-Vtk95-Rel`).

*Applied (Feature Attribute Matrix tests, added 2026-10-29):* Class 1, hand-computed. The fixture is a 1×1×4 Image Geometry. Each slice `z` has Feature Id `z + 1`, so the source Ids are {1,2,3,4}. The Feature Attribute Matrix has 5 rows (row 0 plus features 1-4). It holds an int32 array, a NeighborList and a StringArray. A separate Ensemble Attribute Matrix holds `CrystalStructures` {999, 1}. The Z bounds are {0,1,3,6,10} and the new Z spacing is 4. Then `newZ = floor(10/4) = 2`. Plane 0 is at position 0, which is not > 1, so it comes from source slice 0. Plane 1 is at position 4, which is > 1 and > 3 but not > 6, so it comes from source slice 2. The output Feature Ids are {1,3}. Features 2 and 4 disappear. With Renumber, the Ids become {1,2} and the matrix has 3 rows (row 0 plus two features). Without Renumber, the matrix keeps shape {5}. With Clear off, all three Feature arrays are copied unchanged. The Ensemble data is {999, 1} in every mode. The source-Id check test puts an invalid Id (-1 or 5) in slice 1, which the resample does not use, so only the source check can reject it.

*Second-engineer review:* **Pending.** The oracle is a deterministic index remap (lowest-risk oracle class); review still recommended before promotion to COMPLETE.

## Bugs found and fixed

No new defect was found during this storage recertification. Existing fixes and deviation dispositions remain documented above and in the sidecar.

## Code path coverage

`23 of 25 paths exercised.` The 2 uncovered are a redundant file-open guard and the cancel checks, listed with reasons.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RegularizeZSpacing.cpp` (235 lines) + `Filters/RegularizeZSpacingFilter.cpp` (401 lines, preflight). Logical phases: **(a) preflight validation**, **(b) output-geometry construction**, **(c) mapping + copy**, **(d) Feature Attribute Matrix** (added 2026-10-29).

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | `new_z_spacing <= 0` → error `-5555` | `Invalid Parameters` / "Non-positive Z spacing" (code asserted) |
| 2  | (a) | input file cannot be opened → error `-5556` | *Not directly tested. File existence is validated by the `FileSystemPathParameter` before preflight; this is a redundant safety net reachable only by a filesystem race.* |
| 3  | (a) | file has fewer than `ZPoints + 1` parseable values → error `-5557` | `Invalid Parameters` / "File with too few values" (code asserted) |
| 4  | (a) | Z boundary values not monotonically non-decreasing → error `-5558` | `Invalid Parameters` / "Non-monotonic Z boundary values" (code asserted) |
| 5  | (a) | total Z extent (last value) `<= 0` → error `-5559` | `Invalid Parameters` / "Zero total Z extent" (code asserted) |
| 6  | (b) | geometry has no cell AttributeMatrix → error `-5560` | `Invalid Parameters` / "Missing cell Attribute Matrix" (code asserted) |
| 7  | (b) | cell AM member is not a DataArray → error `-5561` | `Invalid Parameters` / "Non-DataArray member in cell Attribute Matrix" (code asserted) |
| 8  | (b) | `remove_original_geometry == true` → rename src, create temp geom, deferred delete + rename | `Valid Execution (In Place)` |
| 9  | (b) | `remove_original_geometry == false` → create new geometry | `Valid Execution (New Geometry)` |
| 10 | (b) | copy loose child data objects from source geometry (component-wise path rebase) | `Valid Execution (New Geometry)` — `LooseData` presence + values asserted |
| 11 | (c) | `newZ == 0` → clamp to 1 (`ComputeRegularizedZDim`) | `Valid Execution (Spacing Exceeds Extent)` — Class 1 expected output asserted |
| 12 | (c) | build `newToOldZPlane` mapping (nearest-below, strict `>`) | `Valid Execution (New Geometry)` — verified against oracle map `[0,1,2,2,3]` |
| 13 | (c) | per-array bulk `copyFrom` of each plane slab | all three valid fixtures + legacy A/B (int32/bool/float3) |
| 14 | (c) | cancel check (per-array, per-plane, and after the copy before renumbering) | *Not directly tested. Requires cancel-signal injection; each branch is a simple early return.* |
| 15 | OOC boundary | slice-copy oracle | `real HDF5 slice-copy oracle` — independent expected outputs on actual HDF5 stores |
| 16 | (d) | Clear OFF → every non-cell Attribute Matrix copied unchanged; preflight value "Feature Data Not Cleared"; stored Renumber and Feature selections ignored | `Clear OFF preflight`, `Clear OFF execute copies Feature AM unchanged`, `Clear OFF ignores stored renumber` |
| 17 | (d) | Clear ON → Feature Attribute Matrix recreated with source shape and no arrays; preflight value "Cleared Feature Data" lists the cleared arrays; Ensemble data copied unchanged | `Clear ON, renumber OFF preflight`, `Clear ON, renumber OFF execute` |
| 18 | (d) | Clear ON, Feature Attribute Matrix and Feature Ids selected through a second parent → accepted; no second copy, also when the second parent is inside another child group | `Clear ON accepts shared parent paths`, `StrictFeatureIds preflight` (alias section), `Feature AM nested alias regression` |
| 19 | (d) | Feature Attribute Matrix is not a direct child of the geometry → error `-67431` | `Clear ON Feature AM errors` (code and corrective hint asserted). A missing path is rejected earlier by parameter validation, so the null branch of `-67431` is defensive. |
| 20 | (d) | Feature Attribute Matrix is the Cell Attribute Matrix → error `-67432` | `Clear ON Feature AM errors` (code asserted) |
| 21 | (d) | Clear + Renumber, Cell Feature Ids not in the Cell Attribute Matrix → error `-67433`; accepted with Renumber off | `StrictFeatureIds preflight` (code and message asserted) |
| 22 | (d) | Clear + Renumber, Cell Feature Ids tuple count ≠ cell count → error `-67434` | `Renumber requires one Feature Id per cell` (code asserted) |
| 23 | (d) | Clear + Renumber, source Feature Id negative → `-5355`, or ≥ Feature Attribute Matrix tuple count → `-5351`; unused slices are checked too | `Renumber validates source Feature Id range` (both codes asserted) |
| 24 | (d) | Clear + Renumber → Feature Ids compacted, empty matrix resized (new geometry and in place) | `Clear ON, renumber ON execute`, `Clear ON in place`, `Clear ON accepts shared parent paths`, `StrictFeatureIds preflight` (alias section) |
| 25 | (d) | SIMPL 6.x conversion sets Clear off; converted pipeline preflights on a geometry with only cell data | `SIMPL Backwards Compatibility` |
| 26 | (d) | Feature Attribute Matrix is registered as another geometry's element data → error `-67460` | `Reject another geometry element Feature AM` checks a linked Vertex Attribute Matrix. |
| 27 | (c) | Slice transfer fails → return the store error before compaction or deferred source replacement. | `Transfer failure preserves source` injects `-95120` on the second slice; checks source identity, dimensions, arrays and uncompacted partial output. |
| 28 | (b) | Feature Ids is a StringArray or NeighborList → existing error `-5561`, without an exception. | `Reject nonnumeric Feature Ids` checks both types. |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Valid Execution (New Geometry)` | new-for-V&V; modified on `topic/correct_feature_am_updates` | Class 1 oracle: asserts output Z dim (5), preserved X/Y dims + spacing, preserved origin, Z spacing = 2.0, the 10-tuple expected `Data` array element-wise, loose-child (`LooseData`) copy with values, and source-geometry retention. Modified: sets Clear off and the other three new arguments explicitly. |
| `Valid Execution (In Place)` | new-for-V&V; modified on `topic/correct_feature_am_updates` | Asserts in-place result replaces the source geometry (same name), no output-name geometry created, output Z dim + Z spacing correct. Modified: sets the four new arguments explicitly. |
| `Valid Execution (Spacing Exceeds Extent)` | new-for-V&V; modified on `topic/correct_feature_am_updates` | Class 1 clamp fixture: `newZRes` (20) > extent (10) → 1 output plane sourced from plane 0; dims (2,1,1), Z spacing 20, `Data` = {0,1} asserted. Modified: sets the four new arguments explicitly. |
| `Invalid Parameters` | new-for-V&V; modified on `topic/correct_feature_am_updates` | 6 SECTIONs — non-positive spacing (`-5555`), too-few file values (`-5557`), non-monotonic values (`-5558`), zero total extent (`-5559`), missing cell AM (`-5560`), non-DataArray cell member (`-5561`); each asserts the specific error code. Modified: each section sets the four new arguments explicitly. |
| `Supported Extensions` | from upstream #1897; modified on `topic/correct_feature_am_updates` | `.dat` and `.csv` inputs pass preflight and execute; output arrays inherit the geometry tuple dimensions. Modified: sets the four new arguments explicitly (Clear and Renumber off, empty Feature paths). |
| `Unsupported Extension` | from upstream #1897; modified on `topic/correct_feature_am_updates` | An existing `.zpos` file fails preflight with `-3003`. Modified: sets the four new arguments explicitly (Clear and Renumber off, empty Feature paths). |
| `Missing Extension` | from upstream #1897; modified on `topic/correct_feature_am_updates` | An existing file without an extension fails preflight with `-3002`. Modified: sets the four new arguments explicitly (Clear and Renumber off, empty Feature paths). |
| `SIMPL Backwards Compatibility` | new-for-V&V; modified on `topic/correct_feature_am_updates` | 2 DYNAMIC_SECTIONs — converts the 6.5 (UUID-keyed) and 6.4 (Filter_Name-keyed) fixtures in `test/simpl_conversion/`; asserts converted geometry path, input file, Z spacing, and the in-place default. The 6.4 path exercises the `k_LegacySimplFilterUuidMap` name resolution. Modified: also asserts Clear off, Renumber off and the default Feature paths, and preflights the converted pipeline on a geometry with only cell data. |
| `real HDF5 slice-copy oracle` | new-for-V&V; modified on `topic/correct_feature_am_updates` | 67 assertions; The existing source fixture and Z-boundary writer produce the analytical source-plane map [0,1,2,2,3]. HDF5 input and output stores preserve [0,1,2,3,4,5,4,5,6,7] across plane-copy boundaries. In-place and new-geometry modes both pass, including the loose child array. Modified: sets the four new arguments explicitly. Hidden (`[.OocStoreContract]`). |
| `Clear ON, renumber OFF preflight` | new on `topic/correct_feature_am_updates` | Asserts "Cleared Feature Data" lists the three Feature arrays and no Ensemble array, no "Feature Data Not Cleared", parameters version 2, Clear default ON, Renumber default off. After the actions run, the new matrix has shape {5} and no arrays, and the Ensemble array is {999, 1}. |
| `Clear OFF preflight` | new on `topic/correct_feature_am_updates` | Asserts "Feature Data Not Cleared" is reported, "Cleared Feature Data" is not, and no warnings. |
| `Clear OFF ignores stored renumber` | new on `topic/correct_feature_am_updates` | Clear off with Renumber on and nonexistent Feature paths. Asserts preflight and execute succeed and the copied matrix keeps shape {5} and its 3 arrays. |
| `StrictFeatureIds preflight` | new on `topic/correct_feature_am_updates` | Feature Ids in another Attribute Matrix: `-67433` with the exact message when Renumber is on; accepted when Renumber is off. Accepted from the Cell Attribute Matrix. Through a second parent, execute gives Ids {1,2}. |
| `Renumber requires one Feature Id per cell` | new on `topic/correct_feature_am_updates` | Cell Attribute Matrix resized to 3 tuples for a 4-cell geometry. Asserts one error, `-67434`. |
| `Renumber validates source Feature Id range` | new on `topic/correct_feature_am_updates` | Puts -1 or 5 in slice 1, which the resample does not use. Asserts preflight passes and execute fails with `-5355` or `-5351`. |
| `Clear ON Feature AM errors` | new on `topic/correct_feature_am_updates` | Asserts `-67431` and the corrective hint for a matrix outside the geometry, `-67432` for the Cell Attribute Matrix, and an error for a missing path. |
| `Clear ON accepts shared parent paths` | new on `topic/correct_feature_am_updates` | Feature matrix and Feature Ids selected through a second parent, Renumber off and on. Asserts an empty matrix with 5 or 3 rows, Ids {1,3} or {1,2}, and the source matrix unchanged. |
| `Clear ON, renumber OFF execute` | new on `topic/correct_feature_am_updates` | Asserts an empty matrix with shape {5}, Ids {1,3}, Ensemble {999, 1}, and the source matrix still has 3 arrays. |
| `Clear ON, renumber ON execute` | new on `topic/correct_feature_am_updates` | Asserts the renumber text in "Cleared Feature Data", an empty matrix with shape {3}, Ids {1,2}, and Ensemble {999, 1}. |
| `Clear OFF execute copies Feature AM unchanged` | new on `topic/correct_feature_am_updates` | Renumber off and on (ignored). Asserts the copied matrix has shape {5} and 3 arrays, `NumElements` {0,1,1,1,1}, the NeighborList and names unchanged, Ids {1,3}, and Ensemble {999, 1}. |
| `Clear ON in place` | new on `topic/correct_feature_am_updates` | Clear and Renumber on, in place. Asserts `Image/Cell Feature Data` is empty with shape {3}, dims (1,1,2), Ids {1,2}, no temporary or output geometry left, and Ensemble {999, 1}. |
| `Feature AM nested alias regression` | new on `topic/correct_feature_am_updates` | The Feature matrix is also linked in `Image/Misc`, and `Misc` is also linked in `Outside`. Clear, Renumber and the selected path (direct or nested) are each on and off. With Clear on, the output matrix has no arrays and `Misc` has no copy of the matrix; with Clear off, it has 3 arrays and the nested copy keeps the original counts. `Misc/Keep` is 123 in every case. The source matrix keeps its ID, parents, shape, 3 arrays, counts and the `Outside` link. |
| `Reject another geometry element Feature AM` | review regression | Rejects clearing a linked matrix registered by a second geometry; asserts `-67460`. |
| `Transfer failure preserves source` | review regression | One successful slice precedes the injected read error. Asserts `-95120`, unchanged source data and no destination feature compaction. |
| `Reject nonnumeric Feature Ids` | review regression | StringArray and NeighborList inputs return the existing `-5561` error without throwing. |

There are 25 `TEST_CASE`s: 6 from the V&V cycle (all modified on `topic/correct_feature_am_updates`), 13 new on that branch, 3 from upstream #1897, and 3 review regressions. 24 are registered with CTest; `real HDF5 slice-copy oracle` is hidden. The earlier 21-case in-core result is from 2026-11-04. The review validation results are recorded separately.

OOC recertification (2026-09-21): serial CTest passed 5/5 target entries in each DREAM3DNX build. The hidden `real HDF5 slice-copy oracle` passed 67 assertions in the OOC binary. The existing source fixture and Z-boundary writer produce the analytical source-plane map [0,1,2,2,3]. HDF5 input and output stores preserve [0,1,2,3,4,5,4,5,6,7] across plane-copy boundaries. In-place and new-geometry modes both pass, including the loose child array. The new case is included in the plugin's OOC store-contract CTest group. Upstream oracle assertions and tolerances remain intact. No new legacy binary comparison is claimed.


## Exemplar archive

- **Archive:** None — inline analytical fixtures.
- **SHA512:** N/A (no `download_test_data`).
- **Provenance:** N/A — Class 1/4 oracles live in the test source directly; no exemplar archive to provenance.

## Deviations from DREAM3D 6.5.171

One documented deviation: `RegularizeZSpacingFilter-D1`. Native pipelines clear the Feature Attribute Matrix by default (Clear ON), while DREAM3D 6.5.171 left it unchanged. Converted 6.x pipelines start with Clear off and still match legacy. Reason: Feature values are stale after Z resampling (Mike Jackson's decision, 2026-10).

No deviations in cell data or geometry. Comparison run on a synthetic Image Geometry (dims 3×2×4; int32 `Data`, `DataArray<bool>` `Mask`, 3-component float `Vec`) authored in legacy v7 format so both runners read identical input; Z-positions `{0,1,3,6,10}`, `new_z_spacing = 2.0`. Legacy 6.5.171 output, SIMPLNX output, and the Class-1 oracle are bit-identical on every cell array and on geometry (dimensions, spacing, origin). A conversion-semantics scope note (cell-AM binding, delta 5) is recorded in `vv/deviations/RegularizeZSpacingFilter.md`.
