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
| Oracle (confirmed)     | **Confirmed. Class 1 (analytical) + Class 4 (invariant), with a Class 2 independent readback for production data.** EbsdLib's `H5OINAReader` is the trusted Class 2 boundary and is not re-tested. Three toy fixture specifications are written by the test itself with H5Lite and materialise into twenty-three `.h5oina` files at run time, and the archived production AZtec file is compared against a readback of its own data sets. Encoded as 17 TEST_CASEs in `test/ReadH5OinaDataTest.cpp`; all pass. |
| Code paths enumerated  | 23 of 30 paths exercised (see Code path coverage). The seven gaps are one `-9582` return site needing permission manipulation, the display-only preflight information values, the shared `-8971` empty-phase path (unreachable from this filter), two defensive branches of the data-set extent probe, the cancel-signal early returns, and three EbsdLib phase-read codes with no constructible fixture. |
| Tests today            | 17 test cases: the Class 1+4 analytical oracle, a 2×2 conversion-option sweep, two multi-scan cases, a stacking-order case, a three-way Format Version sweep, eight value-add rejection cases with the error code pinned per section, two EbsdLib error passthroughs, and the Class 2 readback of the production AZtec file. Fixtures are written at test time; no new archive. |
| Exemplar archive       | **`H5Oina_Test_Data.tar.gz` retained, SHA512 `346573ac…d140ea03`, unchanged.** Its genuine Oxford AZtec `.h5oina` is kept as an irreplaceable production input. Its `H5Oina_Test_Data.dream3d` exemplar is no longer consulted: that file was written by this filter, so comparing against it pinned the filter to itself. Documented in `vv/provenance/ReadH5OinaDataFilter.md`. |
| Legacy comparison      | **Not run — no legacy equivalent (verified against the 6.5.171 tree).** Its place is taken by the Class 2 independent readback described under Oracle. |
| Bug flags              | D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11 — every one a bug under the root-cause taxonomy, every one present in DREAM3D-NX 7.0.0 through 7.4.1, all resolved. Two of them (D1, D6) change the value of a published output; D6 is labelled a breaking change. |
| V&V phase              | Discovery, relationship, oracle, reconciliation, algorithm review, tests, deviations, provenance, docs — **complete**. The EbsdLib-side corrections (D5, D6, D7, D10, D11) reach users only through EbsdLib 3.1.1, which the `vcpkg.json` pin requires. In-core build and tests pass in `NX-Com-Qt69-Vtk96-Rel-EbsdLib`; OOC build skipped — the filter's writes are single-pass forward-sequential into freshly created arrays, and the plan for this batch scopes verification to in-core reader plumbing (approved in the batch plan for this filter). Second-engineer sign-off outstanding. |

## Summary

`ReadH5OinaDataFilter` ("Read Oxford Aztec Data (.h5oina)") imports one or more scans from an Oxford Instruments AZtec `.h5oina` file into a single Image Geometry: it builds the geometry from the first selected scan's header, creates the nine cell arrays and the three ensemble arrays, copies each scan's data into its own tuple slab in the order the stacking-order setting asks for, optionally widens the file's `uint8` Phase column to `int32`, and optionally applies the EDAX/TSL hexagonal x-axis alignment to φ2. Verification is Class 1 analytical plus Class 4 invariant on hand-authored `.h5oina` fixtures, with a Class 2 independent readback standing in for the legacy A/B comparison that cannot exist — DREAM3D 6.5.171 has no H5OINA importer. Headline result: eleven defects were found, six in the filter and five in EbsdLib's `H5OINAReader`, including two crashes and a silently wrong orientation for every hexagonal point at the shipped default settings; all are corrected and pinned, and all 17 tests pass.

## Algorithm Relationship

*Classification:* **New filter.**

*Evidence:* A case-insensitive search for `oina` across `D3D_v6.5.171/DREAM3D/Source`, `.../SIMPL/Source` and `.../DREAM3D_Plugins` returns zero source hits; the single match is a byte sequence inside `DREAM3D_Plugins/ZeissImport/Data/ZeissImport/SampleMosaic/SampleMosaic_p0.bmp`. The `aztec` hits in that tree are the `H5Aztec` file-version constant belonging to DREAM3D's own HDF5-CTF archive format, which is unrelated to Oxford's H5OINA. The filter has no `FromSIMPLJson`, no `SIMPLConversion` include, no entry in the plugin's legacy-UUID mapping and no SIMPL conversion fixtures — all consistent with a filter that never existed in SIMPL. It was added by PR #700 (`a51dd5f3d`, 2024-03-25).

Because there is no legacy equivalent, the same-UUID equivalence claim that drives the Deviations gate does not apply; the Deviations file instead records the differences between what DREAM3D-NX 7.0.0–7.4.1 shipped and the corrected behavior.

*PRs since introduction that touched these files:* the full list between `a51dd5f3d` and `d65d859b5` is #874, #934, #937, #996, #1088, #1152, #1187, #1238, #1263, #1438, #1439, #1472 and #1576. The ones that changed behavior rather than form are #996 (OEM reader error messages), #1088 (parameter versioning), #1152 (spacing/origin ordering), #1263 (phase info in preflight values), #1472 (EbsdLib 2.0.0 API migration) and #1576 (error-message sweep). None of them touched the hexagonal-alignment constant, the multi-scan offsets or the pattern path; those three carried their defects from the initial import through every subsequent change.

## Oracle

*Class:* **1 (Analytical) + 4 (Invariant)**, plus **2 (Independent readback)** for the production file; EbsdLib parsing = **Class 2 boundary (trusted, not re-tested)**.

### The EbsdLib boundary (what we do NOT re-test)

EbsdLib's `H5OINAReader` owns HDF5 traversal, the header and phase-group parsing, the nine required data-set reads, the Laue-group-to-crystal-structure mapping in `CtfPhase`, and its own error codes. Those behaviors are upstream's to verify, and the tests pin only that their codes and messages reach the user. The filter's value-add — everything this oracle covers — is the deterministic plumbing on top: geometry construction from the first scan's header, array creation and typing, per-scan slab offsets and their stacking order, the verbatim column copies, the `uint8`→`int32` Phase widening, the hexagonal φ2 alignment, ensemble slot-0 defaults, and the value-add rejection paths.

Five of the eleven deviations sit *inside* that boundary but corrupt user-visible SIMPLNX output, crash the process, or blank out the reason a file was rejected (D5, D6, D7, D10, D11). They are corrected upstream on `topic/3_1_1_staging` rather than worked around in the filter, which is what creates the EbsdLib 3.1.1 release dependency.

### Applied

Toy `.h5oina` files are written by the test itself with `H5Support::H5Lite` into the binary test-output directory, from a fixture specification declared as C++ structs at the top of `test/ReadH5OinaDataTest.cpp`. Three fixture specifications materialise into twenty-three files at run time: **eight** that are imported successfully and **fifteen** that back a rejection or passthrough case. The eight that import carry exactly the dataset set `H5OINAReader` requires — the four `Header` scalars, one or more `Phases/<n>` groups with `Phase Name` / `Lattice Dimensions` / `Lattice Angles` / `Laue Group` / `Space Group`, and the nine `Data` datasets — plus the inert root `Manufacturer` / `Software Version` / `Index` datasets for realism, and all but one of them also carry `Format Version`, which is not in the required set (which is why the variant that omits it still imports). Of the fifteen behind the rejection and passthrough cases, three omit a required dataset outright — `Data/Bands` once and `Phases/<n>/Lattice Angles` twice — while the other twelve carry the full dataset set and are rejected on a parameter value, on a header or phase value, or on how the scans are selected.

- **Fixture A** — one scan, 3 × 2 cells, steps 0.25 / 0.5, a hexagonal phase (Laue 9) and a cubic phase (Laue 11), with Phase values `{1, 2, 0, 2, 1, 1}` so an unindexed point is present. Points 0, 1 and 2 all carry φ2 = `0.1F` on a hexagonal, a cubic and an unindexed point respectively, so the three expectations differ only through the alignment branch. The hexagonal phase's lattice angles are 90/90/120 degrees stored as radians, so γ ≠ β.
- **Fixture B** — two scans, 2 × 2 each, cubic only, with every one of the nine columns disjoint between the two scans. With no hexagonal point present the Euler array must be a pure verbatim copy, which isolates slab placement and stacking order.
- **Fixture C** — two scans, 2 × 2 each, every point hexagonal, so a shift applied to the wrong slab or applied twice changes pinned values.
- **Guard fixtures** — zero / negative cell counts, mismatched scan grids, a missing scan name in first and second position, a phase group named outside 1..N in the first and in a later scan, a later scan whose phase-group count differs from the first scan's in both directions, an 8-row data set behind a 16-cell header, an out-of-range phase byte, a missing `Data/Bands`, and a phase group missing `Lattice Angles` in the first and in a later scan.
- **Format Version variants** — `"5.0"`, `"2.0"` and absent, which must all produce identical output.

Expected values are derived from the fixture specification, not from observed output. Every floating-point value written into a toy fixture is a float32 literal stored as float32, so the file round-trips it bit-for-bit and verbatim copies are asserted with exact equality; the cell counts and the Laue and space group numbers are `int32` scalars and five of the nine data columns are `uint8`, all of which round-trip exactly as well. (The production file's values are whatever AZtec wrote; that test asserts exact equality too, but against an independent readback of the file rather than against literals.) The hexagonal expectations are the correctly-rounded float32 results of `double(φ2) + 30 × (π/180)`, derived independently with IEEE-754 float32/float64 semantics in NumPy and embedded as literals with derivation comments. The derivation script and its recorded output are described in `vv/provenance/ReadH5OinaDataFilter.md`.

**Precision pinning.** φ2 values are chosen where the float32 result *differs* between a double-precision intermediate and a float32 intermediate: `0.1F` and `0.34F` in Fixture A and Fixture C slab 0, and `0.09F`, `0.22F`, `0.35F` in Fixture C slab 1. None of those five is an exactly representable decimal, which is deliberate — a dyadic value has trailing zero mantissa bits, so the sum rounds to the same float32 under either intermediate and cannot separate the two paths. `0.25F`, `0.5F` and `0.75F` are exactly representable and are carried alongside on hexagonal points as controls that round identically either way, so the suite distinguishes magnitude errors from arithmetic-shape errors. The hexagonal fixture phase's third lattice angle is a sixth discriminator of the same kind, on the EbsdLib radians-to-degrees conversion rather than on the alignment: `2.0943952F → 120.0F` under a double intermediate and `120.00000762939453F` under a float32 one. The discriminating values were found by an exhaustive sweep of the float32 values in `[0.25, 6.5)`, of which 8,289,627 of 38,797,312 separate the two paths.

Class 4 invariants encoded: the ensemble matrix always carries exactly one more tuple than the file has phase groups; slot 0 always holds `UnknownCrystalStructure` / `"Invalid Phase"` / zeroed lattice constants; the Phase values are unchanged by either conversion option; the hexagonal shift never reaches a cubic or an unindexed point; and the geometry is identical under both stacking orders.

### The Class 2 independent readback (substitute for the legacy A/B)

There is no DREAM3D 6.5.171 H5OINA importer, so the comparison that would normally establish behavioral continuity does not exist. Its place is taken by an independent readback of the production AZtec file:

- `test/ReadH5OinaDataTest.cpp::"Real AZtec File Readback"` reads the archived `H5Oina_Test_Data.h5oina`'s own `Data` datasets with `H5Lite` — the file bytes, bypassing `H5OINAReader` entirely — and compares them element-wise against the filter's output. The file is a single 25 × 25 cubic scan with 625 points, both indexed and unindexed, and the comparison covers all nine cell arrays (6,966 assertions in that test case).
- The same readback is performed out of band with h5py, in a second language and a second HDF5 binding, re-deriving the geometry, the ensemble values and the cell arrays from the documented rules. It is the source of the ensemble literals pinned in that test case (`CrystalStructures {999, 1}`, `MaterialName {"Invalid Phase", "Titanium cubic"}`, `LatticeConstants {3.192, 3.192, 3.192, 90, 90, 90}`). The script and its recorded output are described in `vv/provenance/ReadH5OinaDataFilter.md`.

This file cannot exercise the hexagonal alignment (its only phase is cubic), the multi-scan slab offsets or the stacking order (it holds one scan); the toy fixtures carry those paths.

*Second-engineer review:* Outstanding — to be recorded at PR review. The oracle design is auditable in the test source: the fixture specification, the derivation of every literal and the Python cross-check are all committed or recorded in the working folder described in the provenance sidecar.

## Algorithm review

Line-by-line review of `Algorithms/ReadH5OinaData.cpp` and the filter's `preflightImpl`.

- **Correctness:** the hexagonal alignment adds 30 degrees expressed in radians on a double intermediate (D1); the Euler slab offset is an element offset rather than a tuple offset (D2); the alignment loop walks the scan's own slab (D3); the scan iteration honors the stacking order (D9).
- **Robustness:** seven malformed-input rejections (`-9584`, `-9585`, `-9586`, `-9587`, `-9589`, `-34971`, `-34972`), each naming the offending value, the scan and the file. The phase-group checks run for every selected scan, not only the first, because the ensemble arrays are sized from the first scan and filled from all of them (D8).
- **Dead code:** the execute-side pattern block was unreachable — preflight always failed first — and internally inconsistent, creating a `uint16` array that execute fetched as `UInt8Array`. It is removed along with its error code `-34970`, and preflight rejects the parameter honestly (D4).
- **Progress and cancel:** the scan loop lives in `ReadH5OinaData::operator()` rather than in the shared `IEbsdOemReader::execute()`, which is what lets the per-scan progress messages, the three cancel checks and the stacking order apply to this filter without changing the sibling filters. The shared `readData()` is still used.
- **Message quality:** the two `-9582` sites that name a scan carry the file path and the scan name. The third, on `readScanNames()`, carries the file path only, because no scan has been selected at that point. `-8970`'s message carries both and is pinned by a test.
- **Preflight cost:** preflight performs one `readScanNames` open, one first-scan header open and one further open per additional selected scan, and preflight fires on every GUI parameter edit. That is `O(S)` file opens per keystroke for a selection of `S` scans. It is the price of validating every selected scan's header rather than only the first, and it is bounded by the number of scans a file contains.
- **Documentation:** the page states that the reader ignores the file's `Format Version` value, names φ2 as the third Euler angle the hexagonal alignment shifts, lists the created outputs with their types and component counts, and recommends thresholding `Phase` > 0 to mask unindexed points. That last point is format-specific: the `.ctf` convention that `Error` = 0 marks a good point does not hold here. In the bundled AZtec export every one of the 587 indexed points carries `Error` = 1 and every one of the 38 un-indexed points carries `Error` = 2, and no point carries 0, so an `Error` = 0 mask selects nothing; the page says so. A Migration Notes section covers the two deviations that change a value a saved pipeline may compare against.
- **Not changed:** `utilities/IEbsdOemReader.hpp` is untouched, so `ReadH5OimData` and `ReadH5EspritData` are unaffected. See Follow-ups.

## Code path coverage

*23 of 30 enumerated paths exercised. Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadH5OinaData.cpp` (272 lines) + preflight in `Filters/ReadH5OinaDataFilter.cpp` (328 lines) + the shared read and ensemble fill in `utilities/IEbsdOemReader.hpp` (147 lines), which rows 16–18 live in.* Logical phases: **(a)** preflight, **(b)** execute read + ensemble population (shared `IEbsdOemReader::readData`), **(c)** per-scan cell-data copy.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | `z_spacing <= 0` → `-9580` | `Parameter Rejections` (section "Non-positive Z Spacing") |
| 2  | (a) | empty scan-name list → `-9581` | `Parameter Rejections` (section "No Scan Names Selected") |
| 3  | (a) | `read_pattern_data` on → `-9583` | `Parameter Rejections` (section "Pattern Import Not Supported") |
| 4  | (a) | `readScanNames` failure → `-9582` | *Not directly tested — the file has to be present but unreadable, which needs permission manipulation. This is a distinct return statement from rows 6 and 9, and it is the one `-9582` site whose message carries the file path without a scan name.* |
| 5  | (a) | selected scan absent from the file → `-9586` | `Missing Scan Name rejected (-9586)` (sections: missing first scan; missing second scan) |
| 6  | (a) | first scan `readHeaderOnly` failure → `-9582` | `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` (section "First selected scan") |
| 7  | (a) | `X Cells`/`Y Cells` < 1 → `-9584` | `Invalid Cell Counts rejected (-9584)` (sections: zero X; zero Y; negative X) |
| 8  | (a) | phase group of any selected scan named outside `[1, first scan's phase count]` → `-9587` | `Phase Index Out Of Range rejected (-9587)` (sections: first selected scan; later selected scan) |
| 9  | (a) | later scan `readHeaderOnly` failure → `-9582` | `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` (section "Later selected scan"; asserts the scan name this site injects) |
| 10 | (a) | later scan grid differs from the first → `-9585` | `Scan Header Mismatch rejected (-9585)` (sections: differing cell counts; differing step size) |
| 11 | (a) | later scan phase-group count differs from the first → `-9589` | `Scan Phase Count Mismatch rejected (-9589)` (sections: more phases; fewer phases) |
| 12 | (a) | geometry action: dims (X, Y, scan count), spacing (X Step, Y Step, z_spacing), user origin | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement`; `Multi-Scan Hexagonal Alignment` |
| 13 | (a) | ensemble matrix sized phase count + 1; three ensemble array actions | `Class 1 Analytical Oracle` (3 tuples, 2 phases) |
| 14 | (a) | nine cell-array actions with the Phase type chosen by `convert_phase_to_int32` | `Conversion Option Combinations` (both branches assert the Phase array's type) |
| 15 | (a) | preflight scan/phase information values | *Not directly tested — display-only values, exercised implicitly by every preflight and asserted by none.* |
| 16 | (b) | `readFile()` failure → `-8970` | `EbsdLib Error Passthrough - Missing Data Column (-8970)` (code and message content pinned) |
| 17 | (b) | empty phase vector → `-8971` | *Not directly tested — `H5OINAReader` rejects a file with no phase groups at header-read time with `-90009`, so preflight `-9582` fires first and this shared-code path is unreachable from this filter.* |
| 18 | (b) | ensemble slot-0 defaults and per-phase fill (Laue mapping, name, lattice constants) | `Class 1 Analytical Oracle`; `Real AZtec File Readback` |
| 19 | (b)/(c) | stacking order: Low-to-High reads the selection in list order, High-to-Low in reverse | `Stacking Order` (both sections; each pins values only its own order can produce) |
| 20 | (c) | the file cannot be reopened for the extent probe → `-34971` | *Not directly tested — the file was opened successfully moments earlier by the reader, so reaching this needs the file to be deleted or its permissions changed mid-execute.* |
| 21 | (c) | `getDatasetInfo` fails for a `Data` dataset → skip that dataset | *Not directly tested — a missing `Data` dataset is fatal inside `H5OINAReader`, which has already run, so `-8970` fires first (row 16) and this branch is defensive only.* |
| 22 | (c) | data-set extent disagrees with the header, in either direction → `-34971` | `Dataset Extent Mismatch rejected (-34971)` |
| 23 | (c) | four `uint8` verbatim copies into the scan's slab | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 24 | (c) | Euler copy at three times the tuple offset | `Multi-Scan Slab Placement` (24 values across two slabs) |
| 25 | (c) | phase value outside `[0, phase count]` → `-34972` | `Out-of-Range Phase Value rejected (-34972)` |
| 26 | (c) | Phase widened to `int32` / copied verbatim as `uint8` | `Conversion Option Combinations` (both branches) |
| 27 | (c) | three `float32` verbatim copies (MAD, X, Y) into the scan's slab | `Class 1 Analytical Oracle`; `Multi-Scan Slab Placement` |
| 28 | (c) | hexagonal alignment on the scan's own slab, Hexagonal-High points only | `Class 1 Analytical Oracle`; `Conversion Option Combinations`; `Multi-Scan Hexagonal Alignment` |
| 29 | (b)/(c) | cancel checks (3 sites) | *Not directly tested. Requires cancel-signal injection; standard early-return pattern. Excluded from scope by direction.* |
| 30 | (a) | EbsdLib phase-read failures `-90030` (unopenable phase group), `-90031` (`Lattice Dimensions`) and `-90033` (`Laue Group`) surfaced through `-9582` | *Not directly tested — the fixture writer emits every phase dataset or omits `Lattice Angles`, which is `-90032` and is covered by rows 6 and 9. `-90030` needs an HDF5 object that cannot be opened, which `H5Lite` cannot write; the other two need fixture switches this suite does not carry.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ReadH5OinaDataFilter: Class 1 Analytical Oracle` | new-for-V&V | Class 1 + 4 over Fixture A. Geometry, all nine cell arrays and all three ensemble arrays asserted element-wise; the ensemble tuple-count invariant. |
| `…: Conversion Option Combinations` | new-for-V&V | Class 1 + 4. `GENERATE` over the 2 × 2 hexagonal-alignment × phase-conversion grid; pins the Phase array's type in each branch and that the alignment never reaches cubic or unindexed points. |
| `…: Multi-Scan Slab Placement` | new-for-V&V | Class 1 over Fixture B (cubic, two scans). 24 Euler values plus eight other arrays across two slabs; regression pin for D2. |
| `…: Multi-Scan Hexagonal Alignment` | new-for-V&V | Class 1 over Fixture C (hexagonal, two scans). Regression pin for D3; 24 Euler values, five of which carry the precision discrimination and three of which are the dyadic controls. |
| `…: Format Version Variants` | new-for-V&V | `GENERATE` over `"5.0"` / `"2.0"` / absent; pins that the reader does not gate on the version and that the dataset is optional. |
| `…: Parameter Rejections` | new-for-V&V | Three sections pinning `-9580`, `-9581`, `-9583`. Replaces the previous invalid-execution test, whose sections asserted only "some error" and whose input file was in neither extracted archive. |
| `…: Invalid Cell Counts rejected (-9584)` | new-for-V&V | Three sections: zero X Cells, zero Y Cells, negative X Cells. |
| `…: Scan Header Mismatch rejected (-9585)` | new-for-V&V | Two sections: differing cell counts, differing step size. |
| `…: Missing Scan Name rejected (-9586)` | new-for-V&V | Two sections: the missing name in first and in second position. |
| `…: Phase Index Out Of Range rejected (-9587)` | new-for-V&V | Two sections: a phase group named `7` in the first selected scan, and the same in a later selected scan. The second fixture passes preflight and crashes the process against the pre-correction filter; regression pin for D8. |
| `…: Scan Phase Count Mismatch rejected (-9589)` | new-for-V&V | Two sections: a later scan declaring more phase groups than the first, and one declaring fewer. The first fixture passes preflight and crashes the process against the pre-correction filter; regression pin for D8. |
| `…: Dataset Extent Mismatch rejected (-34971)` | new-for-V&V | A 4 × 4 header over 8-row data sets; deterministic, no file-mutation injection needed. |
| `…: Out-of-Range Phase Value rejected (-34972)` | new-for-V&V | A one-phase file with a Phase byte of 5. |
| `…: EbsdLib Error Passthrough - Missing Data Column (-8970)` | new-for-V&V | Pins the code and that the message names the scan and the file. |
| `…: EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` | new-for-V&V | Two sections, one per `-9582` return site, each asserting the file path and the scan name that site injects. This fixture crashed the process before D7 was corrected. |
| `…: Stacking Order` | new-for-V&V | Two sections over Fixture B, one per order, each pinning Euler, Band Contrast and Phase values that only that order can produce; regression pin for D9. Replaces `Stacking Order Warning (-9588)`, which pinned a warning that no longer exists. |
| `…: Real AZtec File Readback` | kept | Class 2. Was `Valid Filter Execution`, which compared against the archive's `.dream3d` exemplar — a file this filter had written. It now compares all nine cell arrays element-wise against an `H5Lite` readback of the `.h5oina`'s own data sets, with the geometry and the ensemble values pinned from the h5py derivation; 6,966 assertions. |
| *(retired)* `…: InValid Filter Execution` | retired | Its sentinel extracted `6_6_ImportH5Data.tar.gz` while its paths pointed into `H5Oina_Test_Data/`, and the file it named exists in neither archive, so the "incompatible manufacturer" section passed on file-not-found. Replaced by `Parameter Rejections` and the eight code-pinned rejection cases plus the two EbsdLib passthroughs. |

All 17 pass in the EbsdLib preset build `NX-Com-Qt69-Vtk96-Rel-EbsdLib`, built and run at the tree state one commit before the `vcpkg.json` pin is raised to EbsdLib 3.1.1 — the pin makes the head unconfigurable until that release exists, so every measurement here is from the pre-pin tree, whose sources are otherwise identical. The whole `OrientationAnalysis::` suite is 308 tests with one failure at that state, `ComputeSchmidsFilter`, which is the known EbsdLib-staging numerical drift and is analysed in `vv/provenance/ReadH5OinaDataFilter.md`. The filter suite reports **9,374 assertions** when the 17 test cases run in one process. A `ctest -V` run sums to **9,390** instead, because `UnitTest::LoadPlugins()` guards itself per process, so its single assertion is counted once in each of ctest's 17 processes rather than once overall; both numbers are correct for how they were measured. OOC build skipped, for the reason recorded in the V&V phase row.

## Exemplar archive

- **Archive:** `H5Oina_Test_Data.tar.gz`, SHA512 `346573ac6b96983680078e8b0a401aa25bd9302dff382ca86ae4e503ded6db3947c4c5611ee603db519d8a8dc6ed35b044a7bfea9880fade5ab54479d140ea03`, matching the `download_test_data()` entry at `test/CMakeLists.txt:146`. Unchanged — no re-upload.
- **Retained:** `H5Oina_Test_Data.h5oina`, a genuine Oxford AZtec export (Format Version 5.0, 25 × 25, 4 µm steps, one cubic titanium phase). Irreplaceable production realism, used as the input to the Class 2 readback.
- **No longer consulted:** `H5Oina_Test_Data.dream3d`. It was produced by running this filter on the sibling `.h5oina`, so it is a self-oracle. It also had the pre-correction radian lattice angles baked into `LatticeConstants`, which means the previous test actively enforced D6.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ReadH5OinaDataFilter.md`.

## Deviations from DREAM3D 6.5.171

**Not applicable — DREAM3D 6.5.171 has no H5OINA importer** (evidence in Algorithm Relationship). No legacy comparison was run and none is possible.

`vv/deviations/ReadH5OinaDataFilter.md` instead records, in the same structured form, the eleven differences between what DREAM3D-NX 7.0.0 through 7.4.1 shipped and the corrected behavior:

- `ReadH5OinaDataFilter-D1` — the hexagonal alignment added 30 radians to a radian-valued φ2 instead of 30 degrees; the option ships ON.
- `ReadH5OinaDataFilter-D2` — in a multi-scan import the Euler block of scan 2 onward landed a third of the way into its slab.
- `ReadH5OinaDataFilter-D3` — the hexagonal alignment was applied to the first scan once per scan and never to the others.
- `ReadH5OinaDataFilter-D4` — "Import Pattern Data" could never succeed and reported a misleading reason.
- `ReadH5OinaDataFilter-D5` — the third lattice angle was discarded and γ echoed β.
- `ReadH5OinaDataFilter-D6` — lattice angles were reported in radians while every other importer reports degrees. **Breaking change**; see its release note and migration section.
- `ReadH5OinaDataFilter-D7` — a phase group missing `Lattice Angles` crashed the process.
- `ReadH5OinaDataFilter-D8` — a multi-scan selection whose scans declared different phase groups wrote past the end of the ensemble arrays and crashed the process.
- `ReadH5OinaDataFilter-D9` — the Stacking Order setting was accepted and never applied.
- `ReadH5OinaDataFilter-D10` — every error message `H5OINAReader` composed was discarded, so failures reached the user with a blank reason.
- `ReadH5OinaDataFilter-D11` — `H5OINAReader::readData()` returned a code that did not match the one it set, and never validated its column count.

D5, D6, D7, D10 and D11 are corrected in EbsdLib and reach users only through EbsdLib 3.1.1.

## Follow-ups for the engineering team

1. **Sibling exposure.** `ReadH5OimData` and `ReadH5EspritData` share `utilities/IEbsdOemReader.hpp` and several of the same defect shapes, `utilities/IEbsdOemReader.hpp` is unchanged, so the two siblings behave exactly as they did. Specifically: `ReadH5OimData::copyRawEbsdData` has the same pattern-array tuple-offset bug in its pattern copy loop; neither sibling validates that every selected scan exists or that later scans match the first scan's grid; neither range-checks the phase column; neither honors the Stacking Order setting the scan-selection parameter carries (D9's shape); and the ensemble fill in the shared `IEbsdOemReader::readData` writes `crystalStructures[phaseId]` with no bounds check, which this filter now prevents from its own preflight but the siblings do not.
2. **`-9587` and `-9589` are preflight checks over a shared write.** Both guard an out-of-range write that happens in the shared `IEbsdOemReader::readData` at execute. A file modified between preflight and execute would still reach that write. Closing it at the point of the write means bounds-checking the shared header, which changes all three filters.
3. **EbsdLib 3.1.1.** The corrections behind D5, D6, D7, D10 and D11 live on `topic/3_1_1_staging` and reach users only through that release; the `vcpkg.json` pin requires it.
4. **`H5OINAReader::getPatternDims(std::array<int32_t,2>)` takes its argument by value** and `getPatternData()` returns `nullptr`. Implementing pattern import for H5OINA is a feature, not a fix.
