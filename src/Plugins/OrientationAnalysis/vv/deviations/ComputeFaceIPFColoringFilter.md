# Deviations from DREAM.3D v6.5.171: ComputeFaceIPFColoringFilter

Part 1 lists differences from `GenerateFaceIPFColoring` in DREAM.3D v6.5.171 (D1, D2, D4 and D5). Part 2 lists the fixed SIMPLNX phase-bounds defect (D3). Both parts use one stable ID sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFaceIPFColoringFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D1` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |

**Symptom:** The second face side uses the wrong symmetry on a mixed-phase boundary, or stays black when the first side is exterior and phase zero has an unknown crystal structure.

**Root cause:** bug. The second-side calculation reads the second feature's Euler angles and reverses the normal, but its crystal-structure guard and operator lookup use the first phase. For a mixed-phase face, this selects the wrong Laue operator. For an exterior first side, the guard reads the phase-zero sentinel and skips the second color. The corrected calculation uses the second phase in both lookups.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.1 or DREAM.3D v6.5.171 with adjacent features of different crystal symmetries, or exterior faces whose first label is non-positive. The exterior-face defect also affects single-phase data.

**Recommendation:** trust SIMPLNX. DREAM3D-NX v7.4.2 contains the fix. Each face side uses its own feature's phase and orientation.

**Evidence:** `Class 1 Oracle - mixed-phase analytical` asserts all 30 output bytes on five faces, including both defect triggers. `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFaceIPFColoring.cpp` uses `phase1` for the second side from `upstream/release/v7.0.0` through `upstream/release/v7.4.1`; v7.4.2 uses `phase2` after PR #1646 (`f5b383be6`). DREAM.3D tag `v6.5.171` has the wrong index in `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/GenerateFaceIPFColoring.cpp`. `vv_work:ComputeFaceIPFColoringFilter/legacy_comparison_summary.md` records the 2026-06-30 comparison of stock DREAM.3D v6.5.171, a local proof version with D1 corrected, and SIMPLNX with D1 corrected. Its inputs are `vv_work:ComputeFaceIPFColoringFilter/build_fixture.py` and `vv_work:ComputeFaceIPFColoringFilter/fx_6_5_171.json`. Two of four complete face-color cases match stock DREAM.3D v6.5.171; all four first-side colors match. The summary also records 120,000 changed faces among 756,474 archived Small IN100 faces, all with non-positive first labels; the other 636,474 match. That comparison uses archived colors, not a new legacy execution. These results predate the algorithm changes in PR #1646 and PR #1568 (`4ec21e93f`); they describe the 2026-06-30 binaries.

---

### ComputeFaceIPFColoringFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D2` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |

**Symptom:** The tested hexagonal basal direction is green `(0,255,0)` in SIMPLNX and blue `(0,0,255)` with the legacy library after correction of D1 in a local proof version.

**Root cause:** library. The libraries assign different hues to the tested basal corner. The difference remains when both implementations use the correct hexagonal operator. Both outputs have a zero red channel.

**Affected users:** Users who compare hexagonal basal-plane IPF colors between DREAM3D-NX and DREAM.3D v6.5.171. The cubic controls in this comparison match.

**Recommendation:** trust SIMPLNX. Use its color key when interpreting its colors. The current analytical test encodes the green basal corner; the zero-red invariant does not depend on the green/blue assignment.

**Evidence:** `vv_work:ComputeFaceIPFColoringFilter/legacy_comparison_summary.md` records the 2026-06-30 comparison with D1 corrected in a local proof version. It separates the library hue difference from the wrong-phase defect and records the same green expectation with EbsdLib 3.0.0 and 3.1.0.

---

### ComputeFaceIPFColoringFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D4` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |

**Symptom:** SIMPLNX produces two three-component RGB arrays instead of one six-component array.

**Root cause:** algorithmic choice. SIMPLNX separates first-side and second-side output. `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFaceIPFColoringFilter.cpp` creates both arrays with component shape `{3}`. The DREAM.3D v6.5.171 source creates one array with six components.

**Affected users:** Users who import pipelines or downstream scripts that expect `SurfaceMeshFaceIPFColors` to contain both sides.

**Recommendation:** trust SIMPLNX. Read the first and second RGB arrays separately. `FromSIMPLJson()` maps the legacy output name to the first array and leaves the second output name at its default.

**Evidence:** PR #1427 (`0b81a0d59`) separates the two output arrays. `Class 1 Oracle - mixed-phase analytical` checks both arrays; `SIMPL Backwards Compatibility` checks the converted first output name.

---

### ComputeFaceIPFColoringFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D5` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |

**Symptom:** SIMPLNX permits PUCM and Nolze-Hielscher IPF colors in addition to TSL.

**Root cause:** algorithmic choice. SIMPLNX passes the selected Color Key value to EbsdLib. The DREAM.3D v6.5.171 filter has no color-key parameter. `executeImpl()` maps choices 0, 1 and 2 to TSL, PUCM and Nolze-Hielscher.

**Affected users:** Users who select a nondefault color key. TSL remains the default, with the separate D1 and D2 differences described above.

**Recommendation:** trust SIMPLNX. Interpret output with the selected key.

**Evidence:** PR #1631 (`ec22ac375`) adds the Color Key parameter. `ColorKey choice reaches algorithm` checks that each nondefault first-side array differs from TSL; it does not supply independent expected RGB arrays for those keys.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFaceIPFColoringFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D3` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A referenced positive feature phase at or above the Crystal Structures tuple count can cause an out-of-bounds read.

**Root cause:** bug. Released algorithms index Crystal Structures with a feature phase without checking that phase against the ensemble size. Their filters validate array types and tuple counts, not phase values. Current `ValidateReferencedFeaturePhases()` in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFaceIPFColoring.cpp` reads feature phases and face labels, checks both sides, and returns `-24341` before coloring. It skips non-positive labels and ignores unreferenced feature phases.

**Affected users:** Users whose Face Labels reference a feature with a positive phase outside the ensemble range.

**Recommendation:** trust SIMPLNX. Correct invalid phase values or supply the missing ensemble tuples. The error identifies the feature, face side, phase value and available ensemble size.

**Evidence:** PR #1568 (`4ec21e93f`) adds referenced-phase validation. `Phase Index Bounds` checks `-24341` for each side and success when the invalid phase belongs to an unreferenced feature. Source at every release branch from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` lacks this validation. DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/GenerateFaceIPFColoring.cpp`, also reads Crystal Structures through an unchecked phase index. Its `dataCheckVoxel()` checks array types and tuple counts only.
