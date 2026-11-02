# V&V Report: WritePoleFigureFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| SIMPLNX Human Name | Generate and Write Pole Figure Images |
| DREAM.3D v6.5.171 equivalent | `WritePoleFigure` (SIMPL UUID `a10bb78e-fcff-553d-97d6-830a43c85385`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-30; second engineer: Jared Duffey, 2026-07-09 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `WritePoleFigure` in DREAM.3D v6.5.171. SIMPLNX uses the EbsdLib compositor and adds image geometry and count-array outputs. |
| Oracle (confirmed) | Class 4 output-change invariants are encoded in three tests in `test/WritePoleFigureTest.cpp`. All pass in CI. |
| Code paths enumerated | 14 of 59 exercised. Other layouts, additional crystal classes, invalid inputs and failure returns lack direct coverage in the cited CI tests. |
| Tests today | Five test cases: three rendering invariants, one SIMPL conversion case and one hidden analytical count fixture. The four default test cases all pass in CI. |
| Exemplar archive | `Pole_Figure_Exemplars_v6.tar.gz` supplies orientation, phase, mask and ensemble inputs. Tests derive expectations without a stored image baseline. |
| Legacy comparison | Not run for the current algorithm; the available DREAM.3D v6.5.171 comparison is dated 2026-07-08 and predates algorithm changes. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`WritePoleFigureFilter` generates pole figures from Euler angles, phase IDs and crystal structures, with an optional mask. Class 4 tests check the effect of masking, the hexagonal basis convention and discrete marker size. All pass in CI.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; the PDF renderer is replaced by an EbsdLib compositor with PNG, RGB image geometry and optional count-array outputs.

## Oracle

*Class:* 4 (Invariant); Class 5 visual comparison artifacts and a Class 1 analytical fixture are described in provenance.

*Applied:* Paired runs must change more than 1% of RGB components when the mask, hexagonal basis, rendering mode or marker radius changes; the basis test also checks intensity differences.

*Encoded:* `src/Plugins/OrientationAnalysis/test/WritePoleFigureTest.cpp` — `Mask filter changes the rendered pole figure`, `HexConvention choice reaches algorithm`, and `Discrete mode and marker radius reach algorithm`: three paired-output fixtures. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-07-16, as recorded for the oracle review.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains these fixes.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `WritePoleFigureFilter-D6` | Preflight accepted non-positive image sizes; the new marker-radius setting also lacked a lower bound during development. | DREAM3D-NX v7.0.0 through v7.4.1 lacked the image-size check. DREAM.3D v6.5.171 was affected by that omission. No release contained the marker-radius omission. | Reject image sizes below 1 with `-680002` and discrete marker radii below 1 with `-680003`. |
| `WritePoleFigureFilter-D7` | SIMPL conversion reused an input container name for the new output geometry and could fail preflight. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Keep the default `PoleFigure` output geometry path. |
| `WritePoleFigureFilter-D8` | PNG output had a TIFF example extension and TIFF error text during development. | No released version. DREAM.3D v6.5.171 was not affected. | The example extension and writer error text identify PNG. |
| `WritePoleFigureFilter-D9` | A temporary default selected Discrete instead of Color Intensity. | No released version. DREAM.3D v6.5.171 was not affected. | Default to Color Intensity. |

## Code path coverage

14 of 59 paths exercised. Counts include only paths mapped to the four default test cases. Each remaining row states its coverage gap.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/WritePoleFigure.cpp` (535 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/WritePoleFigureFilter.cpp` (398 lines).

Test names omit the common `OrientationAnalysis::WritePoleFigureFilter: ` prefix. The algorithm has no cancellation check.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Render | Color Intensity mode | `Mask filter changes the rendered pole figure` |
| 2 | Render | Discrete mode; marker radii 1 and 10 pixels | `Discrete mode and marker radius reach algorithm` |
| 3 | Basis | X parallel to a | `HexConvention choice reaches algorithm` |
| 4 | Basis | X parallel to a* | `HexConvention choice reaches algorithm` |
| 5 | Layout | Horizontal composite | `Mask filter changes the rendered pole figure` |
| 6 | Layout | Vertical composite | *Not directly tested. No fixture selects Vertical.* |
| 7 | Layout | Square composite | *Not directly tested. No fixture selects Square.* |
| 8 | Selection | Mask disabled | `Mask filter changes the rendered pole figure` |
| 9 | Selection | Mask enabled | `Mask filter changes the rendered pole figure` |
| 10 | Selection | Count and extract phase-one Euler angles | `Mask filter changes the rendered pole figure` |
| 11 | Selection | No selected Euler angles in a phase; skip | *Not directly tested. No default fixture masks all orientations or has an empty phase.* |
| 12 | Selection | No phase after background; empty outer loop | *Not directly tested. No fixture supplies only the background ensemble.* |
| 13 | Selection | Extraction crosses the 65,536-tuple boundary | *Not directly tested. The hidden count fixture encodes this path; it is outside the cited CI selection.* |
| 14 | Intensity | Count-array output disabled | `Mask filter changes the rendered pole figure` |
| 15 | Intensity | Color count arrays with MRD normalization | `HexConvention choice reaches algorithm` |
| 16 | Intensity | Color count arrays without MRD normalization | *Not directly tested. No Color fixture disables normalization.* |
| 17 | Intensity | Discrete counts and southern-hemisphere reflection | *Not directly tested. The hidden count fixture encodes exact counts; it is outside the cited CI selection.* |
| 18 | Dispatch | Hexagonal_High intensity and RGB output | `HexConvention choice reaches algorithm` |
| 19 | Dispatch | Cubic_High intensity and RGB output | *Not directly tested. The numerical fixture is hidden; the visual comparison predates algorithm changes.* |
| 20 | Dispatch | Cubic_Low intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 21 | Dispatch | Hexagonal_Low intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 22 | Dispatch | Trigonal_High intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 23 | Dispatch | Trigonal_Low intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 24 | Dispatch | Tetragonal_High intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 25 | Dispatch | Tetragonal_Low intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 26 | Dispatch | OrthoRhombic intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 27 | Dispatch | Monoclinic intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 28 | Dispatch | Triclinic intensity and composite dispatch | *Not directly tested. No current fixture selects this crystal class.* |
| 29 | Dispatch | Unknown crystal structure; warn and omit intensity | *Not directly tested. Requires a selected phase with an unsupported crystal-structure value.* |
| 30 | Intensity | Create arrays for phase numbers above 1 | *Not directly tested. The fixtures have one material phase.* |
| 31 | Intensity | Resize and populate phase metadata | *Not directly tested. Tests do not assert metadata values.* |
| 32 | Output | Save RGB image geometry; resize and pack RGBA to RGB | `Discrete mode and marker radius reach algorithm` |
| 33 | Output | Discard temporary image geometry | *Not directly tested. Only the hidden count fixture disables image geometry output.* |
| 34 | Output | Disk output disabled | `Mask filter changes the rendered pole figure` |
| 35 | Output | Disk output enabled; existing or newly created directory | *Not directly tested. Default fixtures disable disk output.* |
| 36 | Output | Both composite outputs disabled | *Not directly tested. Only the hidden count fixture requests counts alone.* |
| 37 | Output | Compositor returns no image; skip | *Not directly tested. Requires a compositor failure fixture.* |
| 38 | Validation | Image size below 1; error -680002 | *Not directly tested. No invalid-size fixture exists.* |
| 39 | Validation | Discrete marker radius below 1; error -680003 | *Not directly tested. No invalid-radius fixture exists.* |
| 40 | Validation | Material name input is not a StringArray; error -680000 | *Not directly tested. No wrong-type fixture exists.* |
| 41 | Validation | Mask instantiation fails; error -53900 | *Not directly tested. Requires a direct algorithm call that bypasses parameter validation.* |
| 42 | Validation | Invalid hexagonal convention; error -680010 | *Not directly tested. Requires a value outside the two parameter choices.* |
| 43 | Failure | Initial image AttributeMatrix resize fails | *Not directly tested. Requires an injected resize failure.* |
| 44 | Failure | Phase read fails during the count scan | *Not directly tested. Requires an injected store read failure.* |
| 45 | Failure | Phase read fails during the extraction scan | *Not directly tested. Requires an injected store read failure.* |
| 46 | Failure | Euler read fails during the extraction scan | *Not directly tested. Requires an injected store read failure.* |
| 47 | Failure | Creation of later-phase intensity array 1 fails | *Not directly tested. Requires a second material phase and an injected creation failure.* |
| 48 | Failure | Creation of later-phase intensity array 2 fails | *Not directly tested. Requires a second material phase and an injected creation failure.* |
| 49 | Failure | Creation of later-phase intensity array 3 fails | *Not directly tested. Requires a second material phase and an injected creation failure.* |
| 50 | Failure | Transfer of intensity array 1 fails | *Not directly tested. Requires an injected store write failure.* |
| 51 | Failure | Transfer of intensity array 2 fails | *Not directly tested. Requires an injected store write failure.* |
| 52 | Failure | Transfer of intensity array 3 fails | *Not directly tested. Requires an injected store write failure.* |
| 53 | Failure | Metadata resize fails | *Not directly tested. Requires an injected StringArray resize failure.* |
| 54 | Failure | Composite AttributeMatrix resize fails | *Not directly tested. Requires an injected allocation or creation failure.* |
| 55 | Failure | RGB array creation fails | *Not directly tested. Requires an injected allocation or creation failure.* |
| 56 | Failure | RGB transfer fails | *Not directly tested. Requires an injected store write failure.* |
| 57 | Failure | Output directory creation fails; error -67020 | *Not directly tested. Requires a filesystem failure fixture.* |
| 58 | Failure | PNG writing fails; error -53900 | *Not directly tested. Requires a filesystem or writer failure fixture.* |
| 59 | Conversion | SIMPL JSON conversion; preserve default output geometry | `SIMPL Backwards Compatibility` |

## Test inventory

All five `TEST_CASE` definitions are in `src/Plugins/OrientationAnalysis/test/WritePoleFigureTest.cpp`. The four default test cases run in CI. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::WritePoleFigureFilter: Mask filter changes the rendered pole figure` | kept | Compares masked and unmasked RGB arrays. Requires equal nonzero sizes and more than 1% differing components. |
| `OrientationAnalysis::WritePoleFigureFilter: HexConvention choice reaches algorithm` | kept | Compares both basis choices. Requires more than 1% differing basal-intensity pixels and RGB components; does not measure an exact rotation angle. |
| `OrientationAnalysis::WritePoleFigureFilter: Discrete mode and marker radius reach algorithm` | new-for-V&V | Compares Color with Discrete, and discrete radii of 1 and 10 pixels. Requires more than 1% differing RGB components in each comparison. |
| `OrientationAnalysis::WritePoleFigureFilter: real HDF5 65536-block discrete-count oracle` | new-for-V&V | Hidden fixture. Constructs two selected cubic orientations across the 65,536-tuple boundary and masks a third. Asserts count sums [12, 24, 16] and the full 32×32 <001> histogram. |
| `OrientationAnalysis::WritePoleFigureFilter: SIMPL Backwards Compatibility` | kept | Variants `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check identity and converted arguments. The output-geometry assertion checks `PoleFigure` (D7); the unused ImageFormat assertion was removed (D5). |

## Exemplar archive

- **Archive:** `Pole_Figure_Exemplars_v6.tar.gz`
- **SHA512:** `23ad8853c38c66f498c7a9c43d6152419506b2bcb6c7e4854728dd3ad81efd309541981bf703ad9b0a1e9b506a0098da918599d5209cb06353281d9cecf39223`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/WritePoleFigureFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/WritePoleFigureFilter.md` for the root cause, affected users and recommendation. This table lists Part 1 only.

| Deviation | Observed difference |
|-----------|---------------------|
| `WritePoleFigureFilter-D1` | Sample-axis labels change from X/Y to A1/A2. |
| `WritePoleFigureFilter-D2` | Font shapes and text spacing differ. |
| `WritePoleFigureFilter-D3` | Hexagonal and trigonal family-label conventions differ. |
| `WritePoleFigureFilter-D4` | Discrete plots use filled-circle markers with a configurable radius. |
| `WritePoleFigureFilter-D5` | Disk output changes from PDF to PNG; SIMPLNX also offers image geometry and count arrays. |
