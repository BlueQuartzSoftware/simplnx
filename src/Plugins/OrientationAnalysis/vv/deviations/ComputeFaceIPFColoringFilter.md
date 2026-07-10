# Deviations from DREAM3D 6.5.171: ComputeFaceIPFColoringFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`GenerateFaceIPFColoring`).

Entries are referenced by stable ID (`ComputeFaceIPFColoringFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ComputeFaceIPFColoringFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D1` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |

**Symptom:** On a surface-mesh face, the **Phase-2 (second) color** differs between SIMPLNX and DREAM3D 6.5.171 in two situations: (a) when the two adjacent features use **different Laue groups** (e.g., cubic on one side, hexagonal on the other), the 6.5.171 Phase-2 color is computed with the **wrong symmetry operator**; and (b) when the **first** face label is the invalid/exterior side (`feature1 ≤ 0`) while the second is a real feature, 6.5.171 leaves the Phase-2 color **black** instead of coloring it. SIMPLNX colors both correctly.

**Root cause:** **Bug in 6.5.171.** In the Phase-2 branch of `CalculateFaceIPFColorsImpl::generate`, both the crystal-structure validity guard and the IPF-operator lookup indexed `m_CrystalStructures[phase1]` instead of `m_CrystalStructures[phase2]`, even though the same branch correctly read `feature2`'s Euler angles and the flipped face normal (`GenerateFaceIPFColoring.cpp:166,175` in 6.5.171; the same lines were ported verbatim into SIMPLNX `Algorithms/ComputeFaceIPFColoring.cpp:114,123`). Two consequences follow from the single wrong index:

- *Mixed-phase faces:* `ops[m_CrystalStructures[phase1]]` applies Phase 1's symmetry operators to Phase 2's orientation — the output stays inside the IPF triangle so it looks valid but is materially wrong.
- *`feature1`-invalid boundary faces:* `phase1 = 0`, so the guard reads `m_CrystalStructures[0]` (the `UnknownCrystalStructure` sentinel, 999), which is `≥ LaueGroupEnd`; the guard fails and the second color is never written, leaving it black. This affects **even single-phase datasets** — which is why the legacy Small IN100 GBCD exemplar changed when the fix was applied.

The fix (`phase1`→`phase2`) is confirmed against the Class 1 analytical oracle and matches the legacy 6.5.172 backport `1c96b3b8e`.

**Empirical evidence — legacy binary A/B (hand-built mixed cubic/hex mesh).** On a legacy-native
4-face fixture (authored with the `legacy_dream3d` writer; generators + pipelines archived on OneDrive under `vv_work/face_ipf/`), DREAM3D
6.5.171 reproduces the hand-derived *buggy* oracle exactly: the mixed-phase face's Phase-2 color
is cubic red `(255,0,0)` (Phase-1's operator on a hex feature) and the `feature1`-invalid boundary
face's Phase-2 color is black `(0,0,0)`. DREAM3D 6.5.172 and SIMPLNX both apply the hex operator on
those two faces (the fix). The two faces are exactly the ones predicted; cubic-side colors and the
cubic|cubic control face are identical across all three.

**Empirical evidence — real data (756,474-face Small IN100 GBCD mesh).** SIMPLNX (fixed) vs the
buggy `SurfaceMeshFaceIPFColors` baked into the file (authentic 6.5.171-class output) differs on
**exactly 120,000 faces**, every one a `feature1 ≤ 0` boundary face, **zero** elsewhere (single-phase,
so only the boundary manifestation appears). This figure is **reproducible** by re-running the A/B
generators + pipelines archived on OneDrive (`vv_work/face_ipf/`); the rendered snapshots and the
prose `legacy_comparison_summary.md` were not preserved, so it is reproducible rather than archived as
a result file. The `phase1`→`phase2` fix itself is verified independently of this A/B by the Class 1
analytical oracle, so this real-data figure is corroborating.

**Affected users:** (a) Anyone coloring surface meshes of **multi-phase** microstructures where adjacent phases use different Laue groups — alpha/beta titanium, dual-phase steels, IN625 with inclusions. (b) **All** users (including single-phase) on exterior/boundary faces where the mesh ordered the exterior side as `feature1`; those faces' second color was silently black in 6.5.171. Users who only inspect the first color, or whose meshes order the real feature as `feature1`, are unaffected.

**Recommendation:** **Trust SIMPLNX.** The 6.5.171 output was mathematically incorrect on the Phase-2 side; SIMPLNX applies each phase's own symmetry operators to its own orientation. DREAM3D 6.5.172 contains the same fix (`1c96b3b8e`); SIMPLNX and 6.5.172 agree on *which* operator to apply (the residual hex hue difference between them is a separate library deviation, `-D2` below).

---

## ComputeFaceIPFColoringFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFaceIPFColoringFilter-D2` |
| **Filter UUID** | `30759600-7c02-4650-b5ca-e7036d6b568e` |
| **Status** | active |

**Symptom:** For a **hexagonal** feature, the IPF color of a basal-plane reference direction differs between SIMPLNX and legacy DREAM3D (both 6.5.171 and 6.5.172): SIMPLNX produces **green `(0,255,0)`** where legacy produces **blue `(0,0,255)`** (and vice-versa for the other basal corner). Cubic coloring is identical. This is unrelated to the `-D1` wrong-phase bug — it shows up on every hex IPF color, in this filter and in the sibling cell-level `ComputeIPFColors`.

**Root cause:** **Library.** SIMPLNX links EbsdLib 3.x; legacy DREAM3D links an older EbsdLib. The hexagonal Laue ops' assignment of the two basal standard-triangle corners (`[2-1-10]` vs `[10-10]`) to green/blue differs between those EbsdLib generations (equivalently, the cartesian a-axis convention / fundamental-sector reduction differs). Both assignments keep the direction in-gamut and give red-channel 0 (a basal direction is never the red c-axis corner); only the green↔blue hue assignment flips. Surfaced during the legacy A/B on the hand-built cubic/hex fixture (`vv_work/face_ipf/`): on the mixed-phase face, 6.5.171 = `(255,0,0)` (wrong phase), 6.5.172 = `(0,0,255)` (hex, legacy hue), SIMPLNX = `(0,255,0)` (hex, EbsdLib 3.x hue). Confirmed identical (`(0,255,0)`) under **both EbsdLib 3.0.0 and EbsdLib 3.1.0** — the Class 1 oracle test passes unchanged on a from-source 3.1.0 build (`NX-Com-Qt69-Vtk95-Rel-EbsdLib`).

**Affected users:** Anyone comparing **hexagonal**-phase IPF colors between DREAM3D-NX and any legacy DREAM3D — face IPF colors and cell IPF colors alike. Cubic-only datasets are unaffected.

**Recommendation:** **Trust SIMPLNX (EbsdLib).** EbsdLib is the canonical IPF-color authority, and its current output (3.0.0 and 3.1.0 agree: green) is the reference; legacy DREAM3D's blue is the deviation to migrate away from. The difference is a hue-assignment convention that lives in EbsdLib, not in this filter, so it applies uniformly to all hex IPF coloring (face and cell). Migration note for users: hexagonal IPF hues will change vs. legacy DREAM3D; this is expected and correct.
