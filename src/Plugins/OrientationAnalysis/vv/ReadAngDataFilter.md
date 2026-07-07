# V&V Report: ReadAngDataFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| SIMPLNX Human Name | Read EDAX EBSD Data (.ang) |
| DREAM3D 6.5.171 equivalent | `ReadAngData` (SIMPL UUID `b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40`) — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ReadAngData.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW — all V&V phases complete; verified-correct against independent oracle; legacy comparison bit-identical on numerics. Pending: sign-off. |
| Sign-off | *Michael Jackson <mike.jackson@bluequartz.net> — in progress, 2026-07-07* |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Minor changes.** Faithful port of legacy `ReadAngData` control flow with 4 deliberate deltas: material-name trim added (D1), TEM/ACOM Nanometer-units detection dropped (D2, obsolete file variants), non-contiguous phase-index handling fixed (D3, legacy crashes), and error-code renumbering (D4). The legacy PIMPL file-cache was dropped (no output effect). |
| Oracle (confirmed)     | **Confirmed.** **Class 1 (analytical) + Class 4 (invariant)**, scoped to the filter's value-add per the "don't re-test upstream" rule — EbsdLib (vcpkg 3.0.0) owns `.ang` parsing and is trusted (Class 2 boundary). A hand-authored inline toy `.ang` (3×2 grid, 2 phases, phase-0 points, all values float32-exact) with every expected value hand-derived from the fixture text. Encoded as 7 TEST_CASEs in `test/ReadAngDataTest.cpp`; all pass. SIMPLNX matched the oracle on every fixture with zero discrepancies. |
| Code paths enumerated  | 13 of 15 paths exercised (see Code path coverage); the two gaps are cancel-signal paths and the file-changed-between-preflight-and-execute guard, both untestable without injection. |
| Tests today            | 7 test cases: Class 1+4 analytical oracle, non-contiguous phase-index invariant (regression pin for D3), 2 value-add preflight error paths (-19500/-19501), 2 EbsdLib error passthroughs (-150/-600), and SIMPL 6.4/6.5 backwards-compat (DYNAMIC_SECTION, new — the filter previously had no conversion test). All inline hand-built fixtures — no exemplar archive. |
| Exemplar archive       | **None — retired `read_ang_test.tar.gz`** (circular oracle: the exemplar `.dream3d` was generated from this filter's own output). `download_test_data()` entry removed from `test/CMakeLists.txt`; retirement documented in `vv/provenance/read_ang_test.md`. |
| Legacy comparison      | **Run (2026-07-07) vs the official DREAM3D 6.5.171 release.** Three fixtures: hand-authored toy, Small IN100 `Slice_1.ang` (189×201 production scan), non-contiguous-phase toy. On the two supported-format fixtures **all numeric outputs are bit-identical** (cell arrays, ensemble arrays, geometry). Differences: MaterialName trailing space (D1) and the non-contiguous-phase fixture, where **6.5.171 segfaults** (D3). |
| Bug flags              | One **legacy** bug, empirically confirmed: `ReadAngDataFilter-D3` (6.5.171 out-of-bounds ensemble write → SIGSEGV on non-contiguous phase indices; SIMPLNX fixed during this pass and pinned by test). **No SIMPLNX bugs** — the same latent OOB existed in the NX port and was found by the algorithm review and fixed before the comparison. |
| V&V phase              | Discovery, oracle, reconciliation, algorithm review (fixes applied), tests, legacy comparison, deviations, provenance, docs — **complete**. In-core build/tests pass (`simplnx-rel`); OOC build skipped (no OOC build configured in this workspace, per maintainer precedent). Outstanding: sign-off. |

## Summary

`ReadAngDataFilter` ("Read EDAX EBSD Data (.ang)") imports a single EDAX TSL `.ang` file into a new Image Geometry: it builds the geometry from the header (dims/step, z=1, origin 0, Micrometer), creates one cell array per data column plus the condensed 3-component `EulerAngles` and remapped `Phases` arrays, and populates the ensemble arrays (`CrystalStructures` via EbsdLib's symmetry mapping, trimmed `MaterialName`, `LatticeConstants`) with slot 0 reserved for the "Invalid Phase". Verification is Class 1 analytical + Class 4 invariant on a hand-authored inline toy `.ang` whose expected outputs were fully hand-derived (EbsdLib parsing itself is trusted upstream and not re-tested). Headline result: SIMPLNX matches the oracle exactly; against DREAM3D 6.5.171 all numeric outputs are bit-identical on supported files, with 4 documented deviations — including one empirically confirmed legacy crash bug (D3) whose latent NX twin was found and fixed during this pass. All 7 unit tests pass; the circular-oracle exemplar archive `read_ang_test.tar.gz` is retired.

## Algorithm Relationship

*Classification:* **Minor changes.**

*Evidence:* Same SIMPL UUID inherited (`b8e128a8-…` → SIMPLNX `5b062816-…`) with `FromSIMPLJson` conversion and 6.4/6.5 fixtures at `test/simpl_conversion/{6_4,6_5}/ReadAngDataFilter.json`. The legacy `ReadAngData::copyRawEbsdData()` (705-line `.cpp`) is preserved essentially line-for-line in `Algorithms/ReadAngData.cpp` (phase remap loop, Euler interleave, verbatim column copies), and `loadMaterialInfo()` keeps the same slot-0-defaults + per-phase-fill structure.

### Port-time deltas (each mapped to a Deviation entry where user-visible)

1. **Material-name trim added** (D1). NX applies `StringUtilities::trimmed()`; legacy stores EbsdLib's raw token-rejoin which carries a trailing space. Changes output (string arrays only).
2. **TEM/ACOM units detection dropped** (D2). Legacy scans the header for `# TEM data` / ACOM markers and sets Nanometer units; NX hard-codes Micrometer. Deliberately not restored — EDAX retired those file variants 10+ years ago. Changes output metadata only for obsolete files.
3. **Non-contiguous phase-index handling** (D3, fixed this pass). Preflight now sizes ensemble arrays by `maxPhaseIndex + 1` (identical to `phases.size() + 1` for well-formed files), `loadMaterialInfo` initializes every slot to Invalid-Phase defaults and guards the write range (`-19502`). Legacy has an out-of-bounds write here (segfault, demonstrated).
4. **`determineLaueGroup()` → `determineOrientationOpsIndex()`** — the EbsdLib symmetry→structure mapping was diffed function-body-for-function-body: **byte-identical** (pure rename). No output effect.
5. **Error-code renumbering** (D4) and framework-level file validation (extension/existence moved to `FileSystemPathParameter`). Rejection paths only.
6. **Legacy PIMPL file-cache dropped** (`ReadAngDataPrivate`/`Ang_Private_Data` — ported as dead declarations, removed during this pass). No output effect; NX re-reads the file on each execute.

*Material PRs since baseline:* none identified for this filter beyond cross-cutting EbsdLib version bumps (the `ebsdlib` namespace migration visible in the current source); the May-2026 touch (`FromSIMPLJson`/conversion fixture refresh) does not alter the algorithm.

## Oracle

*Class:* **1 (Analytical) + 4 (Invariant)**; EbsdLib parsing = **Class 2 boundary (trusted, not re-tested)**.

### The EbsdLib boundary (what we do NOT re-test)

EbsdLib's `AngReader` owns: header parsing (keys, phase sections, colon-chopping), data-column parsing, square-grid order fix-up, hex-grid rejection at read time, and its own error codes (`-150` no phases, `-600` truncated data, etc.). Those behaviors are upstream's to verify. The filter's value-add — everything this oracle covers — is the deterministic plumbing on top: geometry construction, array creation/typing, phase `<1 → 1` remap, Euler interleave, ensemble sizing + slot-0 defaults, symmetry-index placement, material-name trim, lattice-constant copy, and the value-add error paths (`-19500` HexGrid at preflight, `-19501` missing GRID, `-19502` phase-index range).

### Applied

A hand-authored toy `.ang` (3 cols × 2 rows, XSTEP 0.25 / YSTEP 0.5, Cubic Nickel Symmetry 43 + Hexagonal "Titanium (Alpha)" Symmetry 62, points 2 and 5 carrying Phase 0) lives as a string literal in the test source; every fixture value is a multiple of 1/8 so all float32 comparisons are exact. Expected outputs — geometry (3,2,1)/(0.25,0.5,1.0)/(0,0,0)/Micrometer, `Phases {1,2,1,1,2,1}` (remap), the 18-value Euler interleave, 6 verbatim pass-through columns, `CrystalStructures {999,1,0}` (Unknown / Cubic_High / Hexagonal_High), trimmed material names, lattice constants — were derived by hand from the fixture text and the documented TSL symmetry codes, never by running any DREAM3D version. Class 4 invariants: ensemble tuple count = maxPhaseIndex+1, slot-0 (and any uncovered slot's) Invalid-Phase defaults.

*Encoded:*
- `test/ReadAngDataTest.cpp::"OrientationAnalysis::ReadAngDataFilter: Class 1 Analytical Oracle"` — the full Class 1 + Class 4 assertion set (geometry, 8 cell arrays element-wise, 3 ensemble arrays element-wise).
- `…::"Non-Contiguous Phase Index"` — Class 4 sizing/defaults invariant on the sparse-phase fixture (regression pin for D3).
- `…::"HexGrid Preflight Error (-19500)"`, `…::"Missing GRID Preflight Error (-19501)"` — value-add error paths.
- `…::"EbsdLib Error Passthrough - No Phase (-150)"`, `…::"EbsdLib Error Passthrough - Truncated Data (-600)"` — error propagation from the trusted boundary.
- `…::"SIMPL Backwards Compatibility"` — UUID + argument conversion (6.4 and 6.5 fixtures).

All 7 pass in the in-core `simplnx-rel` build. Reconciliation found zero SIMPLNX-vs-oracle discrepancies.

*Second-engineer review:* skipped — documented reason: the filter's value-add is pure data plumbing with no numerical algorithm content; every oracle value is mechanically derivable from the fixture text (grid math, verbatim copies, a documented enum mapping), leaving no design freedom for the author-bias failure mode the review guards against. See `vv/provenance/read_ang_test.md`.

## Algorithm review

Line-by-line review performed via the `review-algorithm` skill after oracle reconciliation. All findings applied (all 7 tests still pass after rebuild):

- **Robustness (Critical):** fixed the latent out-of-bounds ensemble write on non-contiguous phase indices (preflight `maxPhaseIndex+1` sizing; full default initialization of every slot; `-19502` range guard in `loadMaterialInfo`). Legacy twin of this bug segfaults (Deviation D3).
- **Dead code:** removed the vestigial legacy PIMPL (`ReadAngDataPrivate`, `Ang_Private_Data`, `m_AngDataPrivate` member, forward declaration), the never-firing `replace("MaterialName", "")`, unused `tDims`/`cDims` locals, the unused `FloatVec3Type` alias, and scaffold-generator boilerplate comments.
- **API consistency:** `loadMaterialInfo` now returns `Result<>` instead of `std::pair<int32, std::string>`.
- **Progress messaging:** added status messages before the EbsdLib read and the cell-data copy (the loops themselves are memcpy-speed; no throttled messenger warranted).
- **Naming:** local `CamelCase` path variables renamed to `camelBack`; doc `@brief`s filled in.

## Code path coverage

*13 of 15 paths exercised. Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadAngData.cpp` (198 lines) + preflight in `Filters/ReadAngDataFilter.cpp` (~265 lines).* Logical phases: **(a)** preflight (header-only read → output actions), **(b)** execute read + ensemble population (`loadMaterialInfo`), **(c)** cell-data copy (`copyRawEbsdData`).

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) Preflight | `readHeaderOnly` error → passthrough error | *Not directly tested as a distinct case — `FileSystemPathParameter` rejects missing files first; unreadable-header content surfaces via the same return as paths 2/3.* Exercised implicitly by every error fixture. |
| 2  | (a) Preflight | `getGrid()` empty → `-19501` | `Missing GRID Preflight Error (-19501)` |
| 3  | (a) Preflight | `getGrid() == "HexGrid"` → `-19500` (after actions are built, so scan info still reaches the UI) | `HexGrid Preflight Error (-19500)` |
| 4  | (a) Preflight | Geometry action: dims (X,Y,1), spacing (XStEP,YSTEP,1), origin (0,0,0), Micrometer | `Class 1 Analytical Oracle` |
| 5  | (a) Preflight | Per-column array creation with int32/float32 type dispatch from `AngFields` | `Class 1 Analytical Oracle` (all 8 cell arrays exist with right types) |
| 6  | (a) Preflight | Ensemble AM sized `maxPhaseIndex + 1`; CrystalStructures/LatticeConstants/MaterialName actions | `Class 1 Analytical Oracle` (3 tuples) + `Non-Contiguous Phase Index` (3 tuples from max index 2) |
| 7  | (b) Execute | `readFile()` error → passthrough | `EbsdLib Error Passthrough - No Phase (-150)` *(−110/−100 variants are the same return statement)* and `…Truncated Data (-600)` |
| 8  | (b) Execute | `phases.empty()` → error passthrough | `EbsdLib Error Passthrough - No Phase (-150)` |
| 9  | (b) Execute | All-slot Invalid-Phase default initialization | `Non-Contiguous Phase Index` (slot 1 keeps defaults) + `Class 1 Analytical Oracle` (slot 0) |
| 10 | (b) Execute | Per-phase fill: symmetry→structure index, trimmed name, lattice constants | `Class 1 Analytical Oracle` (2 phases, 2 Laue classes) |
| 11 | (b) Execute | `phaseID` out of ensemble range → `-19502` | *Not directly tested. Guard only trips if the file changes between preflight and execute; requires file-mutation injection.* |
| 12 | (c) Copy | Phase remap `< 1 → 1` then copy | `Class 1 Analytical Oracle` (points 2, 5) |
| 13 | (c) Copy | Euler interleave `3i/3i+1/3i+2` | `Class 1 Analytical Oracle` (18 values) |
| 14 | (c) Copy | Verbatim copies: IQ, CI, SEM Signal, Fit, X/Y Position | `Class 1 Analytical Oracle` (6 arrays element-wise) |
| 15 | (b)/(c) | Cancel checks (4 sites) | *Not directly tested. Requires cancel-signal injection; standard early-return pattern.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ReadAngDataFilter: Class 1 Analytical Oracle` | new-for-V&V | Class 1 + 4. Geometry (dims/spacing/origin/units), 8 cell arrays and 3 ensemble arrays asserted element-wise (≈70 assertions), all expected values hand-derived, float32-exact fixture. Inline data. |
| `OrientationAnalysis::ReadAngDataFilter: Non-Contiguous Phase Index` | new-for-V&V | Class 4. Sparse `# Phase 2`-only file → ensemble sized to 3 tuples, uncovered slot keeps Invalid-Phase defaults. Regression pin for the OOB fix (Deviation D3). |
| `OrientationAnalysis::ReadAngDataFilter: HexGrid Preflight Error (-19500)` | new-for-V&V | Value-add preflight rejection. |
| `OrientationAnalysis::ReadAngDataFilter: Missing GRID Preflight Error (-19501)` | new-for-V&V | Value-add preflight rejection. |
| `OrientationAnalysis::ReadAngDataFilter: EbsdLib Error Passthrough - No Phase (-150)` | new-for-V&V | Replaces the retired archive-based "Invalid Phase" test with an inline no-phase-header fixture; same error code asserted. |
| `OrientationAnalysis::ReadAngDataFilter: EbsdLib Error Passthrough - Truncated Data (-600)` | new-for-V&V | Replaces the retired archive-based "Invalid Columns & Rows" test with an inline truncated-data fixture; same error code asserted. |
| `OrientationAnalysis::ReadAngDataFilter: SIMPL Backwards Compatibility` | new-for-V&V | `DYNAMIC_SECTION` over the existing 6.4/6.5 conversion fixtures — the filter previously had **no** conversion test despite shipping the fixtures. |
| *(retired)* `OrientationAnalysis::ReadAngData: Exemplary Test` | retired | Exemplar comparison against `read_ang_test.dream3d` — **circular oracle** (exemplar generated from this filter's own output). Replaced by the Class 1 oracle above. |
| *(retired)* `OrientationAnalysis::ReadAngData: Invalid Phase` | retired | Archive-based `-150` test; superseded by the inline passthrough test. |
| *(retired)* `OrientationAnalysis::ReadAngData: Invalid Columns & Rows` | retired | Archive-based `-600` test; superseded by the inline passthrough test. |

## Exemplar archive

- **Archive:** None. **`read_ang_test.tar.gz` retired** this pass (SHA512 was `de7cd89d…e4b236`); its `download_test_data()` entry is removed from `test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/read_ang_test.md` — documents the circular-oracle status and the inline replacement.
- All current oracle data is inline in `test/ReadAngDataTest.cpp`; there is nothing to archive.

## Deviations from DREAM3D 6.5.171

*Comparison run 2026-07-07 against the official DREAM3D 6.5.171 release on three byte-identical-input fixtures (toy `.ang`, Small IN100 `Slice_1.ang`, sparse-phase toy). All numeric outputs bit-identical on the supported-format fixtures. Full write-ups: `vv/deviations/ReadAngDataFilter.md`; working artifacts in `Code_Review/ReadAngDataFilter/`.*

- `ReadAngDataFilter-D1` — MaterialName trailing space: legacy stores `"Nickel "`, SIMPLNX trims to `"Nickel"`. Demonstrated on both fixtures. Trust SIMPLNX.
- `ReadAngDataFilter-D2` — TEM/ACOM `.ang` variants get Nanometer units in legacy, Micrometer in SIMPLNX. Document-only: EDAX retired those files 10+ years ago. Either acceptable.
- `ReadAngDataFilter-D3` — **Legacy crash bug, empirically confirmed:** non-contiguous phase indices segfault 6.5.171 (OOB ensemble write, exit 139); SIMPLNX imports correctly (fixed this pass, test-pinned). Trust SIMPLNX.
- `ReadAngDataFilter-D4` — Error-code renumbering on rejection paths (`-1000`→`-19500` HexGrid, etc.). No data effect. Either acceptable.
