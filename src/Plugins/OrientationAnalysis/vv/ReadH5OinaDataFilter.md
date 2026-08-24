# V&V Report: ReadH5OinaDataFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| SIMPLNX Human Name | Read Oxford Aztec Data (.h5oina) |
| DREAM3D 6.5.171 equivalent | **None.** DREAM3D 6.5.171 has no H5OINA/AZtec importer of any kind (verified by a case-insensitive search of `DREAM3D/Source`, `SIMPL/Source` and `DREAM3D_Plugins` in the 6.5.171 tree at `/Users/mjackson/Workspace/D3D_v6.5.171`: zero source hits — the one match is a byte sequence inside a ZeissImport sample `.bmp`). The filter was written in SIMPLNX (PR #700, `a51dd5f3d`, 2024-03-25) and carries no `FromSIMPLJson` and no legacy-UUID mapping entry. |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Michael A. Jackson <mike.jackson@bluequartz.net> — 2026-08-24. Second engineer: *<pending PR review>*. |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **New filter, no legacy equivalent.** Nothing in DREAM3D 6.5.171 imports H5OINA, so there is no port to classify and no legacy behavior to inherit or defend. The filter is one of three siblings (`ReadH5OimData`, `ReadH5EspritData`) built on the shared `IEbsdOemReader` template; EbsdLib's `H5OINAReader` does the parsing. |
| Oracle (confirmed)     | **Confirmed. Class 1 (analytical) + Class 4 (invariant), with a Class 2 independent readback for production data.** EbsdLib's `H5OINAReader` is the trusted Class 2 boundary and is not re-tested. Three toy fixture specifications (A, B, C) are written by the test itself with H5Lite and materialise into nineteen `.h5oina` files at run time — eight that are imported successfully and eleven that back a rejection or passthrough case — and every expected value is derived from the fixture specification. The archived production AZtec file is compared against a readback of its own data sets. Encoded as 16 TEST_CASEs (8,890 assertions) in `test/ReadH5OinaDataTest.cpp`; all pass. SIMPLNX matched the oracle once the seven defects below were corrected. |
| Code paths enumerated  | 21 of 26 paths exercised (see Code path coverage). The five gaps are two `-9582` return sites that share a statement with a covered row or need permission manipulation, the display-only preflight information values, the shared `-8971` empty-phase path (unreachable from this filter), and the cancel-signal early returns. |
| Tests today            | 16 test cases: the Class 1+4 analytical oracle, a 2×2 conversion-option sweep, two multi-scan cases, a three-way Format Version sweep, seven value-add rejection cases with the error code pinned per section, a stacking-order warning case, two EbsdLib error passthroughs, and the Class 2 readback of the production AZtec file. Fixtures are written at test time; no new archive. |
| Exemplar archive       | **`H5Oina_Test_Data.tar.gz` retained, SHA512 `346573ac…d140ea03`, unchanged.** Its genuine Oxford AZtec `.h5oina` is kept as an irreplaceable production input. Its `H5Oina_Test_Data.dream3d` exemplar is no longer consulted: that file was written by this filter, so comparing against it pinned the filter to itself. Documented in `vv/provenance/ReadH5OinaDataFilter.md`. |
| Legacy comparison      | **Not run — no legacy equivalent (verified against the 6.5.171 tree).** Its place is taken by the Class 2 independent readback described under Oracle. |
| Bug flags              | SIMPLNX, all releases 7.0.0 through 7.4.1: hexagonal φ2 shifted by 30 radians instead of 30 degrees (D1), multi-scan Euler slabs misplaced (D2), the hexagonal shift confined to the first scan and repeated (D3), pattern import advertised but impossible (D4), the third lattice angle discarded (D5), lattice angles left in radians while every other importer reports degrees (D6), and a crash on a phase group missing its lattice angles (D7). All resolved. |
| V&V phase              | Discovery, relationship, oracle, reconciliation, algorithm review (fixes applied), tests, deviations, provenance, docs — **complete**. Tests pass 16/16 in the EbsdLib preset build. **Release dependency:** the EbsdLib-side corrections (D5/D6/D7) live on `topic/3_1_1_staging` and reach users only through EbsdLib 3.1.1; the `vcpkg.json` pin is raised to `>= 3.1.1` and the pull request is merge-blocked until that release exists. OOC waived for this batch. Second-engineer sign-off outstanding (PR review). |

## Summary

`ReadH5OinaDataFilter` ("Read Oxford Aztec Data (.h5oina)") imports one or more scans from an Oxford Instruments AZtec `.h5oina` file into a single Image Geometry: it builds the geometry from the first selected scan's header, creates the nine cell arrays and the three ensemble arrays, copies each scan's data into its own tuple slab, optionally widens the file's `uint8` Phase column to `int32`, and optionally applies the EDAX/TSL hexagonal x-axis alignment to φ2. Verification is Class 1 analytical plus Class 4 invariant on hand-authored `.h5oina` fixtures, with a Class 2 independent readback standing in for the legacy A/B comparison that cannot exist — DREAM3D 6.5.171 has no H5OINA importer. Headline result: seven defects were found, four in the filter and three in EbsdLib's `H5OINAReader`, including a crash and a silently wrong orientation for every hexagonal point at the shipped default settings; all are corrected and pinned, and all 16 tests pass.

## Algorithm Relationship

*Classification:* **New filter.**

*Evidence:* A case-insensitive search for `oina` across `D3D_v6.5.171/DREAM3D/Source`, `.../SIMPL/Source` and `.../DREAM3D_Plugins` returns zero source hits; the single match is a byte sequence inside `DREAM3D_Plugins/ZeissImport/Data/ZeissImport/SampleMosaic/SampleMosaic_p0.bmp`. The `aztec` hits in that tree are the `H5Aztec` file-version constant belonging to DREAM3D's own HDF5-CTF archive format, which is unrelated to Oxford's H5OINA. The filter has no `FromSIMPLJson`, no `SIMPLConversion` include, no entry in the plugin's legacy-UUID mapping and no SIMPL conversion fixtures — all consistent with a filter that never existed in SIMPL. It was added by PR #700 (`a51dd5f3d`, 2024-03-25).

Because there is no legacy equivalent, the same-UUID equivalence claim that drives the Deviations gate does not apply; the Deviations file instead records the differences between what DREAM3D-NX 7.0.0–7.4.1 shipped and the corrected behavior.

*Material PRs since introduction:* #996 (OEM reader error messages), #1088 (parameter versioning), #1152 (spacing/origin ordering), #1263 (phase info in preflight values), #1438 (microtexture cleanup), #1472 (EbsdLib 2.0.0 API migration), #1576 (error-message sweep). None of them touched the hexagonal-alignment constant, the multi-scan offsets or the pattern path; those three carried their defects from the initial import through every subsequent change.

## Oracle

*Class:* **1 (Analytical) + 4 (Invariant)**, plus **2 (Independent readback)** for the production file; EbsdLib parsing = **Class 2 boundary (trusted, not re-tested)**.

### The EbsdLib boundary (what we do NOT re-test)

EbsdLib's `H5OINAReader` owns HDF5 traversal, the header and phase-group parsing, the nine required data-set reads, the Laue-group-to-crystal-structure mapping in `CtfPhase`, and its own error codes. Those behaviors are upstream's to verify, and the tests pin only that their codes and messages reach the user. The filter's value-add — everything this oracle covers — is the deterministic plumbing on top: geometry construction from the first scan's header, array creation and typing, per-scan slab offsets, the verbatim column copies, the `uint8`→`int32` Phase widening, the hexagonal φ2 alignment, ensemble slot-0 defaults, and the value-add rejection paths.

Three defects found during this work sit *inside* that boundary but corrupt user-visible SIMPLNX output or crash the process (D5, D6, D7). They were corrected upstream on `topic/3_1_1_staging` rather than worked around in the filter, which is what creates the EbsdLib 3.1.1 release dependency.

### Applied

Toy `.h5oina` files are written by the test itself with `H5Support::H5Lite` into the binary test-output directory, from a fixture specification declared as C++ structs at the top of `test/ReadH5OinaDataTest.cpp`. The eight fixtures that are imported successfully carry exactly the dataset set `H5OINAReader` requires — the four `Header` scalars, one or more `Phases/<n>` groups with `Phase Name` / `Lattice Dimensions` / `Lattice Angles` / `Laue Group` / `Space Group`, and the nine `Data` datasets — plus the inert root `Manufacturer` / `Software Version` / `Index` datasets for realism. (`Format Version` is not in that required set, which is why the variant that omits it still imports.) Of the eleven fixtures behind the rejection and passthrough cases, two omit a required dataset outright — `Data/Bands` and `Phases/1/Lattice Angles` — while the other nine carry the full set and are rejected on their values or on how they are selected.

- **Fixture A** — one scan, 3 × 2 cells, steps 0.25 / 0.5, a hexagonal phase (Laue 9) and a cubic phase (Laue 11), with Phase values `{1, 2, 0, 2, 1, 1}` so an unindexed point is present. Points 0, 1 and 2 all carry φ2 = `0.1F` on a hexagonal, a cubic and an unindexed point respectively, so the three expectations differ only through the alignment branch. The hexagonal phase's lattice angles are 90/90/120 degrees stored as radians, so γ ≠ β.
- **Fixture B** — two scans, 2 × 2 each, cubic only, with disjoint values per scan. With no hexagonal point present the Euler array must be a pure verbatim copy, which isolates slab placement.
- **Fixture C** — two scans, 2 × 2 each, every point hexagonal, so a shift applied to the wrong slab or applied twice changes pinned values.
- **Guard fixtures** — zero / negative cell counts, mismatched scan grids, a missing scan name in first and second position, phase groups not numbered 1..N, an 8-row data set behind a 16-cell header, an out-of-range phase byte, a missing `Data/Bands`, and a phase group missing `Lattice Angles`.
- **Format Version variants** — `"5.0"`, `"2.0"` and absent, which must all produce identical output.

Expected values are derived from the fixture specification, not from observed output. Every value written into a toy fixture is a float32 literal stored as float32, so the file round-trips it bit-for-bit and verbatim copies are asserted with exact equality. (The production file's values are whatever AZtec wrote; that test asserts exact equality too, but against an independent readback of the file rather than against literals.) The hexagonal expectations are the correctly-rounded float32 results of `double(φ2) + 30 × (π/180)`, derived independently with IEEE-754 float32/float64 semantics in NumPy (`ww_work/ReadH5OinaData/h5oina_oracle.py`, recorded output `oracle_spec.txt`) and embedded as literals with derivation comments.

**Precision pinning.** Following the ReadCtfData lesson, φ2 values were chosen where the float32 result *differs* between a double-precision intermediate and a float32 intermediate: `0.1F` and `0.34F` in Fixture A and Fixture C slab 0, and `0.09F`, `0.22F`, `0.35F` in Fixture C slab 1. None of those five is an exactly representable decimal, which is deliberate — a dyadic value has trailing zero mantissa bits, so the sum rounds to the same float32 under either intermediate and cannot separate the two paths. `0.25F`, `0.5F` and `0.75F` are exactly representable and are carried alongside on hexagonal points as controls that round identically either way, so the suite distinguishes magnitude errors from arithmetic-shape errors. The discriminating values were found by an exhaustive sweep of the float32 values in `[0.25, 6.5)`, recorded in `pi_over_6_discriminator.txt`.

Class 4 invariants encoded: the ensemble matrix always carries exactly one more tuple than the file has phase groups; slot 0 always holds `UnknownCrystalStructure` / `"Invalid Phase"` / zeroed lattice constants; the Phase values are unchanged by either conversion option; and the hexagonal shift never reaches a cubic or an unindexed point.

### The Class 2 independent readback (substitute for the legacy A/B)

There is no DREAM3D 6.5.171 H5OINA importer, so the comparison that would normally establish behavioral continuity does not exist. Its place is taken by an independent readback of the production AZtec file:

- `test/ReadH5OinaDataTest.cpp::"Real AZtec File Readback"` reads the archived `H5Oina_Test_Data.h5oina`'s own `Data` datasets with `H5Lite` — the file bytes, bypassing `H5OINAReader` entirely — and compares them element-wise against the filter's output. The file is a single 25 × 25 cubic scan with 625 points, both indexed and unindexed, and the comparison covers all nine cell arrays (6,966 assertions in that test case).
- `ww_work/ReadH5OinaData/h5oina_oracle.py` performs the equivalent readback with h5py in a second language and a second HDF5 binding, re-deriving the geometry, the ensemble values and the cell arrays from the documented rules. Its recorded output is `readback_real_file.txt`; it is the source of the ensemble literals pinned in that test case (`CrystalStructures {999, 1}`, `MaterialName {"Invalid Phase", "Titanium cubic"}`, `LatticeConstants {3.192, 3.192, 3.192, 90, 90, 90}`).

This file cannot exercise the hexagonal alignment (its only phase is cubic) or the multi-scan slab offsets (it holds one scan); the toy fixtures carry those paths.

*Second-engineer review:* Outstanding — to be recorded at PR review. The oracle design is auditable in the test source: the fixture specification, the derivation of every literal and the Python cross-check are all committed or recorded in the evidence folder.

## Algorithm review

Line-by-line review of `Algorithms/ReadH5OinaData.cpp` and the filter's `preflightImpl` after oracle reconciliation. All findings applied; all 16 tests pass afterwards.

- **Correctness:** the hexagonal alignment now adds 30 degrees expressed in radians on a double intermediate (D1); the Euler slab offset is now an element offset rather than a tuple offset (D2); the alignment loop now walks the scan's own slab (D3).
- **Robustness:** six malformed-input rejections added (`-9584`, `-9585`, `-9586`, `-9587`, `-34971`, `-34972`), each naming the offending value, the scan and the file.
- **Dead code:** the execute-side pattern block was unreachable — preflight always failed first — and internally inconsistent, creating a `uint16` array that execute fetched as `UInt8Array`. It is removed along with its error code `-34970`, and preflight now rejects the parameter honestly (D4).
- **Progress and cancel:** the scan loop moved from the shared `IEbsdOemReader::execute()` into `ReadH5OinaData::operator()` so that per-scan progress messages and three cancel checks could be added without changing the sibling filters. The shared `readData()` is still used.
- **Message quality:** `-9582` now carries the file path and the scan name; `-8970`'s message already carried both and is pinned by a test.
- **Documentation:** the page claimed a Format Version restriction the reader does not have, named the wrong Euler angle for the hexagonal alignment, and listed no created outputs. It also carried the `.ctf` convention that `Error` = 0 marks a good point, which is wrong for this format: in the bundled AZtec export every one of the 587 indexed points carries `Error` = 1 and every one of the 38 un-indexed points carries `Error` = 2, and no point carries 0, so the masking recipe it recommended would have selected nothing. The page now recommends thresholding `Phase` > 0 and states the observation. Evidence: `ww_work/ReadH5OinaData/error_column_check.txt`.
- **Not changed:** `utilities/IEbsdOemReader.hpp` is untouched, so `ReadH5OimData` and `ReadH5EspritData` are unaffected by this work. See Follow-ups.

## Code path coverage

*21 of 26 enumerated paths exercised. Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadH5OinaData.cpp` (256 lines) + preflight in `Filters/ReadH5OinaDataFilter.cpp` (317 lines).* Logical phases: **(a)** preflight, **(b)** execute read + ensemble population (shared `IEbsdOemReader::readData`), **(c)** per-scan cell-data copy.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | `z_spacing <= 0` → `-9580` | `Parameter Rejections` (section "Non-positive Z Spacing") |
| 2  | (a) | empty scan-name list → `-9581` | `Parameter Rejections` (section "No Scan Names Selected") |
| 3  | (a) | `read_pattern_data` on → `-9583` | `Parameter Rejections` (section "Pattern Import Not Supported") |
| 4  | (a) | `readScanNames` failure → `-9582` | *Not directly tested — needs an unreadable-but-present file (permission manipulation). The same `-9582` return statement family is covered by rows 5 and 6.* |
| 5  | (a) | selected scan absent from the file → `-9586` | `Missing Scan Name rejected (-9586)` (sections: missing first scan; missing second scan) |
| 6  | (a) | first scan `readHeaderOnly` failure → `-9582` | `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` |
| 7  | (a) | `X Cells`/`Y Cells` < 1 → `-9584` | `Invalid Cell Counts rejected (-9584)` (sections: zero X; zero Y; negative X) |
| 8  | (a) | phase index outside `[1, phase count]` → `-9587` | `Phase Index Out Of Range rejected (-9587)` |
| 9  | (a) | later scan `readHeaderOnly` failure → `-9582` | *Not separately tested — same return statement as row 6, reached from the per-scan loop.* |
| 10 | (a) | later scan grid differs from the first → `-9585` | `Scan Header Mismatch rejected (-9585)` (sections: differing cell counts; differing step size) |
| 11 | (a) | stacking order not Low-to-High → warning `-9588` | `Stacking Order Warning (-9588)` (both branches: warning raised, and no warning for Low-to-High) |
| 12 | (a) | geometry action: dims (X, Y, scan count), spacing (X Step, Y Step, z_spacing), user origin | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement`; `Multi-Scan Hexagonal Alignment` |
| 13 | (a) | ensemble matrix sized phase count + 1; three ensemble array actions | `Class 1 Analytical Oracle` (3 tuples, 2 phases) |
| 14 | (a) | nine cell-array actions with the Phase type chosen by `convert_phase_to_int32` | `Conversion Option Combinations` (both branches assert the Phase array's type) |
| 15 | (a) | preflight scan/phase information values | *Exercised implicitly by every preflight; display-only, not asserted.* |
| 16 | (b) | `readFile()` failure → `-8970` | `EbsdLib Error Passthrough - Missing Data Column (-8970)` (code and message content pinned) |
| 17 | (b) | empty phase vector → `-8971` | *Not directly tested — `H5OINAReader` rejects a file with no phase groups at header-read time with `-90009`, so preflight `-9582` fires first and this shared-code path is unreachable from this filter.* |
| 18 | (b) | ensemble slot-0 defaults and per-phase fill (Laue mapping, name, lattice constants) | `Class 1 Analytical Oracle`; `Real AZtec File Readback` |
| 19 | (c) | data-set extent disagrees with the header → `-34971` | `Dataset Extent Mismatch rejected (-34971)` |
| 20 | (c) | four `uint8` verbatim copies into the scan's slab | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 21 | (c) | Euler copy at three times the tuple offset | `Multi-Scan Slab Placement` (24 values across two slabs) |
| 22 | (c) | phase value outside `[0, phase count]` → `-34972` | `Out-of-Range Phase Value rejected (-34972)` |
| 23 | (c) | Phase widened to `int32` / copied verbatim as `uint8` | `Conversion Option Combinations` (both branches) |
| 24 | (c) | three `float32` verbatim copies (MAD, X, Y) into the scan's slab | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 25 | (c) | hexagonal alignment on the scan's own slab, Hexagonal-High points only | `Class 1 Analytical Oracle`; `Conversion Option Combinations`; `Multi-Scan Hexagonal Alignment` |
| 26 | (b)/(c) | cancel checks (3 sites) | *Not directly tested. Requires cancel-signal injection; standard early-return pattern. Excluded from scope by direction.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ReadH5OinaDataFilter: Class 1 Analytical Oracle` | new-for-V&V | Class 1 + 4 over Fixture A. Geometry, all nine cell arrays and all three ensemble arrays asserted element-wise; the ensemble tuple-count invariant. |
| `…: Conversion Option Combinations` | new-for-V&V | Class 1 + 4. `GENERATE` over the 2 × 2 hexagonal-alignment × phase-conversion grid; pins the Phase array's type in each branch and that the alignment never reaches cubic or unindexed points. |
| `…: Multi-Scan Slab Placement` | new-for-V&V | Class 1 over Fixture B (cubic, two scans). 24 Euler values plus five other arrays across two slabs; regression pin for D2. |
| `…: Multi-Scan Hexagonal Alignment` | new-for-V&V | Class 1 over Fixture C (hexagonal, two scans). Regression pin for D3; 24 Euler values, eight of which carry the precision discrimination. |
| `…: Format Version Variants` | new-for-V&V | `GENERATE` over `"5.0"` / `"2.0"` / absent; pins that the reader does not gate on the version and that the dataset is optional. |
| `…: Parameter Rejections` | new-for-V&V | Three sections pinning `-9580`, `-9581`, `-9583`. Replaces the previous invalid-execution test, whose sections asserted only "some error" and whose input file was in neither extracted archive. |
| `…: Invalid Cell Counts rejected (-9584)` | new-for-V&V | Three sections: zero X Cells, zero Y Cells, negative X Cells. |
| `…: Scan Header Mismatch rejected (-9585)` | new-for-V&V | Two sections: differing cell counts, differing step size. |
| `…: Missing Scan Name rejected (-9586)` | new-for-V&V | Two sections: the missing name in first and in second position. |
| `…: Phase Index Out Of Range rejected (-9587)` | new-for-V&V | A single-phase file whose phase group is named `7`. |
| `…: Dataset Extent Mismatch rejected (-34971)` | new-for-V&V | A 4 × 4 header over 8-row data sets; deterministic, no file-mutation injection needed. |
| `…: Out-of-Range Phase Value rejected (-34972)` | new-for-V&V | A one-phase file with a Phase byte of 5. |
| `…: EbsdLib Error Passthrough - Missing Data Column (-8970)` | new-for-V&V | Pins the code and that the message names the scan and the file. |
| `…: EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` | new-for-V&V | Pins the code and the message content. This fixture crashed the process before D7 was corrected. |
| `…: Stacking Order Warning (-9588)` | new-for-V&V | Both branches: the warning for High-to-Low, and no warning for Low-to-High. |
| `…: Real AZtec File Readback` | modified | Class 2. Was `Valid Filter Execution`, which compared against the archive's `.dream3d` exemplar — a file this filter had written. Now compares all nine cell arrays element-wise against an `H5Lite` readback of the `.h5oina`'s own data sets, with the geometry and the ensemble values pinned from the h5py derivation. |
| *(retired)* `…: InValid Filter Execution` | retired | Its sentinel extracted `6_6_ImportH5Data.tar.gz` while its paths pointed into `H5Oina_Test_Data/`, and the file it named exists in neither archive, so the "incompatible manufacturer" section passed on file-not-found. Replaced by `Parameter Rejections` and the nine code-pinned rejection cases. |

All 16 pass in the EbsdLib preset build `NX-Com-Qt69-Vtk96-Rel-EbsdLib` — 8,890 assertions in total. OOC is waived for this batch.

## Exemplar archive

- **Archive:** `H5Oina_Test_Data.tar.gz`, SHA512 `346573ac6b96983680078e8b0a401aa25bd9302dff382ca86ae4e503ded6db3947c4c5611ee603db519d8a8dc6ed35b044a7bfea9880fade5ab54479d140ea03`, matching the `download_test_data()` entry at `test/CMakeLists.txt:146`. Unchanged — no re-upload.
- **Retained:** `H5Oina_Test_Data.h5oina`, a genuine Oxford AZtec export (Format Version 5.0, 25 × 25, 4 µm steps, one cubic titanium phase). Irreplaceable production realism, used as the input to the Class 2 readback.
- **No longer consulted:** `H5Oina_Test_Data.dream3d`. It was produced by running this filter on the sibling `.h5oina`, so it is a self-oracle. It also had the pre-correction radian lattice angles baked into `LatticeConstants`, which means the previous test actively enforced D6.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ReadH5OinaDataFilter.md`.

## Deviations from DREAM3D 6.5.171

**Not applicable — DREAM3D 6.5.171 has no H5OINA importer** (evidence in Algorithm Relationship). No legacy comparison was run and none is possible.

`vv/deviations/ReadH5OinaDataFilter.md` instead records, in the same structured form, the seven differences between what DREAM3D-NX 7.0.0 through 7.4.1 shipped and the corrected behavior:

- `ReadH5OinaDataFilter-D1` — the hexagonal alignment added 30 radians to a radian-valued φ2 instead of 30 degrees; the option ships ON.
- `ReadH5OinaDataFilter-D2` — in a multi-scan import the Euler block of scan 2 onward landed a third of the way into its slab.
- `ReadH5OinaDataFilter-D3` — the hexagonal alignment was applied to the first scan once per scan and never to the others.
- `ReadH5OinaDataFilter-D4` — "Import Pattern Data" could never succeed and reported a misleading reason.
- `ReadH5OinaDataFilter-D5` — the third lattice angle was discarded and γ echoed β.
- `ReadH5OinaDataFilter-D6` — lattice angles were reported in radians while every other importer reports degrees.
- `ReadH5OinaDataFilter-D7` — a phase group missing `Lattice Angles` crashed the process.

D5, D6 and D7 are corrected in EbsdLib and reach users only through EbsdLib 3.1.1.

## Follow-ups for the engineering team

1. **Sibling exposure (not fixed here, by direction).** `ReadH5OimData` and `ReadH5EspritData` share `utilities/IEbsdOemReader.hpp` and several of the same defect shapes. Specifically: `ReadH5OimData::copyRawEbsdData` has the same pattern-array tuple-offset bug in its pattern copy loop; neither sibling validates that every selected scan exists or that later scans match the first scan's grid; neither range-checks the phase column; and the ensemble fill in the shared `IEbsdOemReader::readData` writes `crystalStructures[phaseId]` with no bounds check, which this filter now prevents from its own preflight but the siblings do not. `utilities/IEbsdOemReader.hpp` was deliberately left untouched so this work changes no sibling behavior.
2. **`stackingOrder` is still not implemented** (see the HO-9 proposal in the task report). It now warns rather than being silently ignored; implementing it would require a member on the shared `ReadH5DataInputValues` and a change to the shared scan loop, which is sibling-affecting work.
3. **EbsdLib 3.1.1 release gate.** This pull request joins #1723 behind the same gate.
4. **`H5OINAReader::getPatternDims(std::array<int32_t,2>)` takes its argument by value** and `getPatternData()` returns `nullptr`. Implementing pattern import for H5OINA is a feature, not a fix, and is out of scope here.
