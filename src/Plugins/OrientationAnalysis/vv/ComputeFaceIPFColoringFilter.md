# V&V Report: ComputeFaceIPFColoringFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| SIMPLNX Human Name | Compute IPF Colors (Face) |
| DREAM.3D v6.5.171 equivalent | `GenerateFaceIPFColoring` (SIMPL UUID `0a121e03-3922-5c29-962d-40d88653f4b6`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-30; second engineer: Nathan Young, 2026-07-10 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `GenerateFaceIPFColoring` in DREAM.3D v6.5.171. SIMPLNX separates the two RGB outputs, adds color keys and corrects phase handling. |
| Oracle (confirmed) | Classes 1 and 4: exact TSL corner colors and a zero-red basal-plane invariant. One five-face fixture in `test/ComputeFaceIPFColoringTest.cpp` encodes these checks. All pass in CI. |
| Code paths enumerated | 15 of 25 exercised by the mapped ordinary tests. Gaps include tuple-count guards, invalid choices, read failures and unsupported phase or symmetry inputs. |
| Tests today | Six test cases: five ordinary cases and one hidden boundary case. They cover analytical colors, phase bounds, invalid selections, color-key routing and SIMPL conversion. |
| Exemplar archive | `6_6_Small_IN100_GBCD.tar.gz` supplies inputs for validation and color-key tests. The archived face colors are no longer a correctness oracle. |
| Legacy comparison | Not run on the current algorithm. The available comparison is dated 2026-06-30; algorithm source changed after that run. |
| Bug flags | None |
| V&V phase | COMPLETE. The five ordinary test cases cover the analytical oracle and both fixed defects. All pass in CI. |

## Summary

`ComputeFaceIPFColoringFilter` calculates an IPF color for each side of a surface-mesh face. A five-face analytical fixture and a basal-plane invariant check the corrections for wrong-phase coloring; phase-bounds tests check the correction for out-of-range phase access. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; the same per-side orientation calculation now uses separate RGB arrays, selectable color keys and validated phase indices.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Hand-derived TSL corner colors specify both RGB arrays on five cubic/hexagonal faces with distinct feature orientations; the hexagonal basal direction must have a zero red channel.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeFaceIPFColoringTest.cpp::OrientationAnalysis::ComputeFaceIPFColoringFilter: Class 1 Oracle - mixed-phase analytical` — one five-face fixture, 30 exact color-byte checks and one invariant. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-07-10.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `ComputeFaceIPFColoringFilter-D1`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFaceIPFColoringFilter-D1` | The second face side used the first side's symmetry operator. Mixed-phase colors were wrong, and some exterior sides stayed black. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Each side uses its own phase and orientation. The five-face oracle checks exact colors. |
| `ComputeFaceIPFColoringFilter-D3` | A referenced positive phase outside the ensemble range caused an out-of-bounds read. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | Validation returns `-24341` before coloring either side; it ignores unreferenced feature phases. |

## Code path coverage

15 of 25 paths exercised. Each uncovered path has a reason below.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFaceIPFColoring.cpp` (265 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFaceIPFColoringFilter.cpp` (216 lines).

Test names below omit `OrientationAnalysis::ComputeFaceIPFColoringFilter:`. The negative selection test fails parameter validation before the two filter-specific tuple-count guards.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Reject selected arrays with incompatible types or component shapes | `Invalid filter execution` — both sections |
| 2 | Preflight | Face Labels and Face Normals have different tuple counts → `-2430` | *Not directly tested. The negative fixture selects an invalid normal type and shape before this guard.* |
| 3 | Preflight | Missing Face Labels → `-2431` | *Not directly tested. Array-selection validation rejects the missing array first.* |
| 4 | Preflight | Feature Euler Angles and Feature Phases have different tuple counts → `-2432` | *Not directly tested. The negative fixture selects a float64 phases array before this guard.* |
| 5 | Preflight | Create two uint8 RGB arrays with the face tuple shape | `Class 1 Oracle - mixed-phase analytical` |
| 6 | Routing | Color Key 0 selects TSL | `Class 1 Oracle - mixed-phase analytical` — exact RGB values |
| 7 | Routing | Color Key 1 selects PUCM | `ColorKey choice reaches algorithm` — first-side array differs from TSL; no independent PUCM values |
| 8 | Routing | Color Key 2 selects Nolze-Hielscher | `ColorKey choice reaches algorithm` — first-side array differs from TSL; no independent Nolze-Hielscher values |
| 9 | Routing | Invalid color-key index → `-24340` | *Not directly tested. ChoicesParameter rejects an invalid index before execution.* |
| 10 | Validation | Propagate a Feature Phases bulk-read failure | *Not directly tested. Requires an injected store read failure.* |
| 11 | Validation | Propagate a Face Labels bulk-read failure | *Not directly tested. Requires an injected store read failure.* |
| 12 | Validation | Skip non-positive face labels | `Class 1 Oracle - mixed-phase analytical`; `Phase Index Bounds` |
| 13 | Validation | Accept referenced positive phases within the ensemble range | `Class 1 Oracle - mixed-phase analytical` |
| 14 | Validation | Reject an out-of-range phase on the first side → `-24341` | `Phase Index Bounds` / `First Feature Phase returns an error` |
| 15 | Validation | Reject an out-of-range phase on the second side → `-24341` | `Phase Index Bounds` / `Second Feature Phase returns an error` |
| 16 | Validation | Ignore an invalid phase on an unreferenced feature | `Phase Index Bounds` / `Unreferenced Feature Phase is ignored` |
| 17 | First color | Use the first feature's phase, Euler angles and face normal | `Class 1 Oracle - mixed-phase analytical` — cubic red/blue and hexagonal c-axis red |
| 18 | First color | Non-positive first label gives phase zero and black | `Class 1 Oracle - mixed-phase analytical` — face 1 |
| 19 | First color | Positive phase has an unsupported Laue index → skip color write | *Not directly tested. No fixture assigns an unsupported structure to a referenced positive phase.* |
| 20 | Second color | Use the second feature's phase, Euler angles and reversed normal | `Class 1 Oracle - mixed-phase analytical` — hexagonal green and cubic red/blue |
| 21 | Second color | Non-positive second label gives phase zero and black | `Class 1 Oracle - mixed-phase analytical` — face 2 |
| 22 | Second color | Positive phase has an unsupported Laue index → skip color write | *Not directly tested. No fixture assigns an unsupported structure to a referenced positive phase.* |
| 23 | Both colors | Positive feature label maps to a zero or negative phase → black | *Not directly tested. The black-color fixtures use non-positive feature labels.* |
| 24 | Validation | Read full 65,536-face blocks and a partial final block | `ColorKey choice reaches algorithm` — the 756,474-face input spans both block sizes |
| 25 | Loops | Empty face array skips validation and coloring loops | *Not directly tested. All fixtures contain faces.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeFaceIPFColoringFilter: Class 1 Oracle - mixed-phase analytical` | new-for-V&V | Checks 30 RGB bytes and one zero-red invariant on five faces with distinct feature orientations. Covers D1 and the current hexagonal hue convention. |
| `OrientationAnalysis::ComputeFaceIPFColoringFilter: Phase Index Bounds` | kept | Checks `-24341` for a referenced invalid phase on each side and success for an invalid unreferenced phase. Covers D3. |
| `OrientationAnalysis::ComputeFaceIPFColoringFilter: genuine HDF5 65536-block tail oracle` | new-for-V&V | Hidden; requires HDF5 stores. Asserts red/black at tuple 65,535 and black/green at tuple 65,536, plus store formats and tuple dimensions. |
| `OrientationAnalysis::ComputeFaceIPFColoringFilter: Invalid filter execution` | kept | Two sections assert invalid preflight and execution for incompatible selected arrays. They do not assert `-2430` or `-2432`. |
| `OrientationAnalysis::ComputeFaceIPFColoringFilter: ColorKey choice reaches algorithm` | kept | Runs all three color keys on Small IN100. Checks equal first-side array sizes and that each nondefault first-side array differs from TSL. |
| `OrientationAnalysis::ComputeFaceIPFColoringFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check UUID, comments, five input paths and the first output name. |

## Exemplar archive

- **Archive:** `6_6_Small_IN100_GBCD.tar.gz`
- **SHA512:** `543e3bdcee24ff9e5cd80dfdedc39ceef1529e7172cebd01d8e5518292ffdf4e0eb2e79d75854cb3eaca5c60e19c861ca67f369e21b81c306edb66327f47a1e3`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFaceIPFColoringFilter-circular-oracle.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFaceIPFColoringFilter.md` for the root cause, affected users and recommendation for each difference. Only Part 1 entries appear here.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFaceIPFColoringFilter-D1` | SIMPLNX uses each side's own symmetry operator; DREAM.3D v6.5.171 uses the first phase on the second side. |
| `ComputeFaceIPFColoringFilter-D2` | The tested hexagonal basal direction is green in SIMPLNX and blue with the legacy library after correction of D1 in a local proof version. |
| `ComputeFaceIPFColoringFilter-D4` | SIMPLNX writes two three-component RGB arrays; DREAM.3D v6.5.171 writes one six-component array. |
| `ComputeFaceIPFColoringFilter-D5` | SIMPLNX adds PUCM and Nolze-Hielscher color keys; DREAM.3D v6.5.171 uses TSL only. |
