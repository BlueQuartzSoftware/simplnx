# Deviations: ReadH5OinaDataFilter

**There is no DREAM3D 6.5.171 equivalent of this filter**, so there is no legacy comparison and no legacy-versus-SIMPLNX deviation to record. The filter was written in SIMPLNX (PR #700, `a51dd5f3d`, 2024-03-25) and has no `FromSIMPLJson`, no legacy-UUID mapping entry and no SIMPL conversion fixtures.

This file therefore records, in the same structured form, every difference between the behavior DREAM3D-NX shipped and the correct behavior. Entries are referenced by stable ID (`ReadH5OinaDataFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

The filter first shipped in DREAM3D-NX 7.0.0. D1 through D13 affect every released version from 7.0.0 through 7.4.1. D5, D6, D7, D10, D11, and D12 are corrections to EbsdLib's `H5OINAReader` and require EbsdLib 3.1.2.

---

## ReadH5OinaDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D1` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** With "Convert Hexagonal X-Axis to EDAX Standard" on — its shipped default — every scan point whose phase maps to `Hexagonal_High` had 30.0 added to its φ2. The file's Euler angles are radians, so this added **thirty radians**, not thirty degrees. Thirty radians is 4.867 radians modulo 2π, so the resulting orientation bears no relation to either convention: the correction is 57.3 times the intended one (the ratio is exactly 180/π) and lands at an arbitrary angle. Every downstream product of those orientations — pole figures, IPF colors, misorientations, grain segmentation — was wrong for every hexagonal point of every H5OINA file imported at default settings. Cubic and unindexed points were unaffected.

**Root cause:** Bug. The correction is a 30 degree rotation about [0001] applied to φ2, and the `.ctf` importer, whose files store degrees, correctly adds the literal `30.0`. The H5OINA path reused that literal even though H5OINA stores radians and the filter performs no degrees-to-radians conversion anywhere.

**Affected users:** Anyone importing an `.h5oina` file containing a hexagonal phase — titanium, magnesium, zirconium, zinc and hcp alloys generally — without turning the option off. Because the option defaults to on, this was the default outcome. Files with no hexagonal phase, including the archived 25 × 25 titanium-cubic test file, were never affected, which is why the pre-existing test suite could not see it.

**Correct behavior:** The value added is 30 degrees expressed in radians, `30 × (π/180)`, computed on a double-precision intermediate so the stored float32 is the correctly-rounded result — the same arithmetic shape the `.ctf` importer uses. Pinned by `test/ReadH5OinaDataTest.cpp::"Class 1 Analytical Oracle"`, `…::"Conversion Option Combinations"` and `…::"Multi-Scan Hexagonal Alignment"`, whose φ2 expectations distinguish the correct value both from the literal-30 result and from a float32-intermediate result.

**Recommendation:** Trust the corrected behavior. Results produced by 7.0.0 through 7.4.1 from `.h5oina` files with a hexagonal phase and the option left on must be regenerated; there is no post-hoc correction, because the shift was applied before any downstream analysis. Users who cannot upgrade should turn the option off and apply the 30 degree rotation with [Rotate Euler Reference Frame](../../docs/RotateEulerRefFrameFilter.md).

---

## ReadH5OinaDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D2` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** When two or more scans were selected and stacked into one Image Geometry, the `Euler` array of every scan after the first landed at the wrong place. Scan *k* occupies the tuple slab starting at *k*·X·Y, so its Euler block belongs at element 3·*k*·X·Y; it was written at element *k*·X·Y instead — one third of the correct offset, because the offset was counted in tuples rather than in elements. Scan 2's Euler block therefore overwrote the last two thirds of scan 1's Euler data, and the last two thirds of scan 2's own slab were left at their zero-initialized values. Every other cell array was placed correctly, so the corruption was confined to orientations and was silent: no error, no warning, and an output whose array sizes and geometry were all correct. Single-scan imports were unaffected, because the offset is zero.

**Root cause:** Bug. The `Euler` copy correctly passes an element count of `totalPoints × 3` but passed the destination offset in tuples. The sibling `ReadH5OimData` writes the same interleave correctly as `(sliceTupleStart + i) * 3`.

**Affected users:** Anyone importing more than one scan from an `.h5oina` file in a single filter invocation. Single-scan imports — the common case, and the only case the pre-existing test covered — were never affected.

**Correct behavior:** The destination offset is three times the tuple offset. Pinned by `test/ReadH5OinaDataTest.cpp::"Multi-Scan Slab Placement"`, which uses a cubic-only two-scan fixture so no alignment transform can mask a placement error, and asserts all 24 Euler values across both slabs.

**Recommendation:** Trust the corrected behavior. Multi-scan H5OINA imports produced by 7.0.0 through 7.4.1 must be regenerated.

---

## ReadH5OinaDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D3` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** In a multi-scan import with the hexagonal alignment on, the alignment loop always walked scan-point indices 0 through X·Y — the first scan's tuples — regardless of which scan was being copied. In an *S*-scan stack, the first scan's hexagonal points therefore received the shift *S* times over, and no point of any later scan received it at all. Single-scan imports were unaffected.

**Root cause:** Bug. `copyRawEbsdData` computed the slab offset for the copies but the alignment helper it called neither took nor applied that offset.

**Affected users:** Anyone importing more than one scan from an `.h5oina` file containing a hexagonal phase. Compounds with D1 and D2 on the same imports.

**Correct behavior:** The alignment loop iterates the scan's own slab, so each point is visited exactly once. Pinned by `test/ReadH5OinaDataTest.cpp::"Multi-Scan Hexagonal Alignment"`, in which every point of both scans is hexagonal, so a shift applied to the wrong slab or applied twice changes a pinned value.

**Recommendation:** Trust the corrected behavior.

---

## ReadH5OinaDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D4` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — as a limitation, honestly reported — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** Turning on "Import Pattern Data" always failed, on every file, with "The parameter 'Read Pattern Data' has been enabled but there does not seem to be any pattern data in the file for the scan name selected" — including for files that plainly do contain pattern data. The archived production AZtec file, for instance, carries a 625 × 512 × 622 `Processed Patterns` dataset and still produced that message.

**Root cause:** Bug in the reported reason, over a missing feature. `H5OINAReader::getPatternData()` returns `nullptr` unconditionally, `getPatternDims(std::array<int32_t,2>)` takes its argument **by value** with an empty body, and the reader's pattern-reading block is commented out. Preflight's pattern dimensions therefore stayed `{0, 0}` and the failure fired regardless of file content, attributing a missing library feature to the user's file. Behind that failure the execute-side plumbing was itself inconsistent: preflight created `Unprocessed Patterns` as `uint16` while execute fetched it as `UInt8Array`, which would have thrown had it ever been reached, and its copy loop used the tuple offset where it needed the element offset. The filter also only ever targeted `Unprocessed Patterns`, while real AZtec exports may carry only `Processed Patterns`.

**Affected users:** Anyone who turned the parameter on. No user ever obtained pattern data from an `.h5oina` file through this filter.

**Correct behavior:** The parameter is retained for pipeline compatibility, and preflight now reports the true reason — pattern import is not yet supported for H5OINA files — naming the file and pointing at the SimplnxCore **Read HDF5 Dataset** filter as the way to read the patterns a file does contain. The unreachable, inconsistent execute block and its error code `-34970` are removed. The parameter's help text and the filter documentation state the limitation. Pinned by `test/ReadH5OinaDataTest.cpp::"Parameter Rejections"`, section "Pattern Import Not Supported (-9583)".

**Recommendation:** Trust the corrected behavior. Implementing H5OINA pattern import is a feature request against EbsdLib, not a fix.

---

## ReadH5OinaDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D5` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved in EbsdLib — requires EbsdLib 3.1.2 — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** The `LatticeConstants` ensemble array reported each phase's γ angle as a copy of its β angle. The file's third lattice angle was never read. For a cubic phase, where α = β = γ, the error is invisible; for a hexagonal phase, whose angles are 90/90/120, the reported γ was 90 instead of 120, and the same applies to any monoclinic, triclinic, trigonal or rhombohedral cell whose γ differs from its β.

**Root cause:** Bug in the trusted parsing boundary. `H5OINAReader::readHeader()` assembled the six lattice constants as `{a, b, c, angles[0], angles[1], angles[1]}`.

**Affected users:** Anyone importing an `.h5oina` file whose phases are not cubic or tetragonal, and who reads `LatticeConstants` downstream. It does not affect orientations, phase indices or crystal-structure symmetry, which come from the Laue group and not from the lattice angles.

**Correct behavior:** The gamma slot receives the third angle. Corrected in EbsdLib 3.1.2. Pinned by `test/ReadH5OinaDataTest.cpp::"Class 1 Analytical Oracle"`, whose hexagonal fixture phase has γ ≠ β specifically so that a gamma slot echoing beta is visible.

**Recommendation:** Trust the corrected behavior. The correction requires EbsdLib 3.1.2.

---

## ReadH5OinaDataFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D6` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved in EbsdLib — requires EbsdLib 3.1.2 — fixed in DREAM3D-NX **7.4.2** |
| **Breaking change** | **Yes.** The value of a published output array changes for every H5OINA import. |

**Symptom:** The three angle slots of `LatticeConstants` were reported in radians for H5OINA imports and in degrees for every other EBSD importer. A cubic phase imported from an `.h5oina` file reported `1.5707964, 1.5707964, 1.5707964`, while the same phase imported from a `.ctf` or `.ang` file reported `90, 90, 90`. The array's meaning therefore depended on which file format the phase happened to come from, with nothing in the data to say which.

**Root cause:** Library inconsistency, not a misreading. An H5OINA file stores its lattice angles in radians, and that is correct for the format; `H5OINAReader` copied them through unchanged, while the `.ang` and `.ctf` importers populate the same slots from degree-valued file fields. **The H5OINA files themselves are correct**; what differed was the unit contract at the importer boundary.

**Affected users:** Anyone comparing or combining phase information across file formats, and anyone reading `LatticeConstants` from an H5OINA import while assuming the degrees convention that the rest of DREAM3D-NX uses.

**Correct behavior:** `H5OINAReader` converts the angles from radians to degrees on import, on a double-precision intermediate, so the array carries the same unit no matter which format the phase came from. Corrected in EbsdLib 3.1.2. Pinned by `test/ReadH5OinaDataTest.cpp::"Class 1 Analytical Oracle"` (90, 90, 120 for the hexagonal fixture phase) and `…::"Real AZtec File Readback"` (90, 90, 90 for the production file's titanium-cubic phase).

**Release note and migration:** This is a **breaking change to a published output**. The first release that carries it is DREAM3D-NX 7.5.0 with EbsdLib 3.1.2. Every `.dream3d` file written by 7.0.0 through 7.4.1 has radians in components 3, 4 and 5 of `LatticeConstants`, so any saved exemplar, regression baseline or pipeline comparison that reads those components changes value on upgrade. To compare a stored radian value against a new import, multiply it by 180/π. Consumers that compensated by converting H5OINA lattice angles themselves must stop doing so. Nothing else in the import changes unit: `Euler` is still radians and the three lattice dimensions are still unconverted. The user-facing migration note is in `docs/ReadH5OinaDataFilter.md` under "Migration Notes". The archived `H5Oina_Test_Data.dream3d` exemplar has the pre-correction radian values baked in, which is one of the reasons it is no longer used as a comparison target (see `vv/provenance/ReadH5OinaDataFilter.md`).

**Recommendation:** Trust the corrected behavior — the degrees convention is the one the rest of the toolkit uses and the one the other importers already produced.

---

## ReadH5OinaDataFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D7` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved in EbsdLib — requires EbsdLib 3.1.2 — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** An `.h5oina` file whose phase group was missing its `Lattice Dimensions` or `Lattice Angles` dataset **crashed the process**. This is empirically demonstrated, not inferred: running the fixture against a build with the corrected filter sources but the pre-correction `H5OINAReader` reports the regression test as `OrientationAnalysis::ReadH5OinaDataFilter: EbsdLib Error Passthrough - Missing Lattice Angles (-9582) (SEGFAULT)` rather than as a failure. The suite log recording that run is listed in `vv/provenance/ReadH5OinaDataFilter.md`.

**Root cause:** Bug in the trusted parsing boundary. `H5OINAReader::readHeader()` read the two vector datasets while discarding their error codes, then indexed elements 0 through 2 of the resulting vectors, which are empty when the dataset is absent. The `Laue Group` and `Space Group` reads discarded their error codes as well, and a failed phase-group open was not checked at all.

**Affected users:** Anyone opening a truncated, partially written or otherwise malformed `.h5oina` file. The crash occurs during preflight, so it takes down the application as the file is selected, before any pipeline runs.

**Correct behavior:** The four required phase reads are checked and reported with EbsdLib error codes `-90030` through `-90033`, each naming the phase and the dataset; `Space Group` remains optional because the reader only passes it through. The filter surfaces the failure as `-9582` with the file path and the scan name. Corrected in EbsdLib 3.1.2. Pinned by `test/ReadH5OinaDataTest.cpp::"EbsdLib Error Passthrough - Missing Lattice Angles (-9582)"`, which is the fixture that used to crash.

**Recommendation:** Trust the corrected behavior. The correction requires EbsdLib 3.1.2.

---

## ReadH5OinaDataFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D8` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** A later scan with a phase index above the first scan's phase count wrote past the ensemble arrays and could crash the process. Scans with the same indices but different phase definitions silently used one shared definition for all scan slices.

**Root cause:** Bug. The single Ensemble Attribute Matrix that all of the stacked scans share is sized from the first selected scan's phase count, `phases.size() + 1`. The shared ensemble fill in `utilities/IEbsdOemReader.hpp` then runs once per selected scan and writes `crystalStructures[phaseId]`, `materialNames[phaseId]` and `latticeConstants` component `phaseId`, where `phaseId` is the integer in that scan's HDF5 phase group name. Any later scan whose phase index exceeds the first scan's phase count writes past the end of all three arrays. Preflight range-checked those indices for the first selected scan only, and the execute-side `-34972` check inspects the phase *column values*, not the phase *group names*, and runs after the ensemble fill has already happened.

**Affected users:** Anyone selecting more than one scan from an `.h5oina` file whose scans differ in their phase lists. Single-scan imports and multi-scan imports of files with one uniform phase list — which is every file in the shipped test data — were never affected, which is why the pre-existing suite could not see it.

**Correct behavior:** Preflight validates the phase indices (`-9587`), phase count (`-9589`), and phase definitions (`-9590`) of every selected scan. The definitions must match because the scans become slices of one 3D microstructure and share one Ensemble Attribute Matrix.

**Recommendation:** Trust the corrected behavior. Re-import any multi-scan file whose phase definitions differ between scans. Import those scans separately if they describe different microstructures.

---

## ReadH5OinaDataFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D9` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** The **Stacking Order** setting carried by the scan selection had no effect. Choosing *High To Low* produced exactly the same output as *Low To High*, with no error and no warning, so the Z order of a multi-scan stack could not be changed from the parameter that appears to control it.

**Root cause:** Bug. The scan loop iterated `SelectedScanNames.scanNames` in list order unconditionally and never read `SelectedScanNames.stackingOrder`. The setting reached the algorithm — it is a member of `OEMEbsdScanSelectionParameter::ValueType` — and was simply not consulted.

**Affected users:** Anyone importing more than one scan who set the stacking order to *High To Low*. Single-scan imports are unaffected, because the two orders coincide.

**Correct behavior:** *Low To High* stacks the scans in the order they are listed; *High To Low* stacks them in the reverse of that order, so the last selected scan occupies tuple slab 0. Pinned by `test/ReadH5OinaDataTest.cpp::"Stacking Order"`, whose two sections assert cell values that only the corresponding order can produce.

**Recommendation:** Trust the corrected behavior. A pipeline saved under 7.0.0 through 7.4.1 with *High To Low* selected now produces a Z-reversed stack relative to what it used to produce, which is what it always asked for; check any such pipeline before re-running it. The two sibling filters `ReadH5OimData` and `ReadH5EspritData` still ignore the same setting — see the V&V report's Follow-ups.

---

## ReadH5OinaDataFilter-D10

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D10` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved in EbsdLib — requires EbsdLib 3.1.2 — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** Every failure reported out of `H5OINAReader` reached the user with a blank `Message:` field. A malformed `.h5oina` produced an error code and an empty explanation, so nothing in the message said which dataset, phase or scan was at fault.

**Root cause:** Bug in the trusted parsing boundary. Ten failure paths in `H5OINAReader` were written as `std::string str; std::stringstream ss(str); ss << …; setErrorMessage(str);`. `std::stringstream(str)` copies `str` into the stream's own buffer, so everything composed went into that copy and the still-empty original was handed to `setErrorMessage()`. A related defect sat one level up: `readFile()` replaced whatever `readHeader()` or `readData()` had set with a generic "could not read header" / "could not read data", discarding the specific reason even where one had been composed correctly.

**Affected users:** Anyone who hit any `H5OINAReader` error — a malformed file, a missing dataset, an absent scan. The filter's own `-8970` and `-9582` messages name the file and the scan, so the file was identifiable; the reason was not.

**Correct behavior:** Each of the ten sites composes into a stream of its own and reports that stream's contents, and `readFile()`'s wrappers name the scan and append the inner message rather than replacing it. Corrected in EbsdLib 3.1.2.

**Recommendation:** Trust the corrected behavior. The correction requires EbsdLib 3.1.2.

---

## ReadH5OinaDataFilter-D11

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D11` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved in EbsdLib — requires EbsdLib 3.1.2 — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** Two defects in one code path. A scan whose header declared zero rows was rejected with error code `-90301` recorded on the reader but `-301` handed back as the return value, so a caller reporting the return value and a caller reading `getErrorCode()` disagreed about what had happened. A scan whose header declared a **negative** column count was not rejected at all: the count was widened to `size_t`, producing an enormous allocation request.

**Root cause:** Bug in the trusted parsing boundary. `H5OINAReader::readData()` validated the row count only, and its rejection returned a literal that did not match the code it had just set.

**Affected users:** Anyone opening a truncated or otherwise malformed `.h5oina` file. DREAM3D-NX reports the code it receives, so the mismatched value was the one the user saw.

**Correct behavior:** Both the row and the column count are validated and the rejection returns the code it sets. Corrected in EbsdLib 3.1.2. The filter rejects the same shape earlier and independently: `-9584` rejects `X Cells` or `Y Cells` below 1 at preflight, which is pinned by `test/ReadH5OinaDataTest.cpp::"Invalid Cell Counts rejected (-9584)"`, including the negative case.

**Recommendation:** Trust the corrected behavior. The filter's `-9584` guard fires first for a file selected through DREAM3D-NX, so this entry matters to other `H5OINAReader` callers.

---

## ReadH5OinaDataFilter-D12

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D12` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved in EbsdLib — requires EbsdLib 3.1.2 — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** A non-numeric or noncanonical phase-group name caused `std::stoi()` to throw during preflight. The exception stopped the import and left the opened HDF5 phase group unclosed.

**Root cause:** Bug. `H5OINAReader::readHeader()` passed each HDF5 group name directly to `std::stoi()` after it opened the group. The code did not validate that the complete name was a positive integer.

**Affected users:** Users with a malformed or manually edited H5OINA file whose phase group is not named with a canonical positive integer.

**Correct behavior:** EbsdLib parses the complete group name before it opens the group. It returns `-90034` for a noncanonical name. The filter reports the error through `-9582` and identifies the file, scan, and group name.

**Recommendation:** Trust the corrected behavior. Correct the phase-group names or export the file again from AZtec.

---

## ReadH5OinaDataFilter-D13

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D13` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Affected releases** | 7.0.0 through 7.4.1 |
| **Status** | resolved — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** The filter accepted non-positive or non-finite X, Y, or Z spacing and created an invalid Image Geometry.

**Root cause:** Bug. Preflight checked only whether Z spacing was less than or equal to zero. It did not reject NaN, and it did not validate the X and Y step values from the file.

**Affected users:** Users with malformed H5OINA step values or a non-finite Z Spacing parameter.

**Correct behavior:** Preflight requires finite, positive X, Y, and Z spacing. It reports `-9580` for Z spacing and `-9591` for scan spacing, with the actual values and file context.

**Recommendation:** Trust the corrected behavior. Correct the spacing values or export the file again from AZtec.

---

## Malformed-input rejections

These are not behavioral deviations on well-formed files — every one of them is a rejection path that stands where 7.0.0 through 7.4.1 performed an out-of-range read, an out-of-range write, or produced a silently useless output. They are listed here so a reader auditing the error-code series has one place to find them. The multi-scan phase-group cases are the exception and are recorded as D8, because they are reachable from a well-formed file.

| Code | Rejects | Previously |
|---|---|---|
| `-9584` | `X Cells` or `Y Cells` below 1 in the first selected scan | The counts were cast to `usize` unchecked, producing a zero-sized geometry, or an enormous one from a negative count |
| `-9585` | A selected scan whose grid or step sizes differ from the first selected scan's | Only the first scan's header was ever read; the copy then spanned the later scan's reader buffers using the first scan's point count |
| `-9586` | A selected scan name that is not in the file | Only the first name was checked, at preflight; a bad later name failed part-way through execute with the earlier scans already written into the output arrays and no rollback |
| `-9587` | A phase group of **any** selected scan named outside 1 through N, where N is the first selected scan's phase count | The check covered the first selected scan only, so a group named `7` in a later scan wrote past the end of the ensemble arrays — see D8 |
| `-9589` | A selected scan whose phase-group count differs from the first selected scan's | The ensemble arrays were sized from the first scan alone and filled from every scan — see D8 |
| `-9590` | A selected scan whose phase definitions differ from the first selected scan's | One shared Ensemble Attribute Matrix silently kept one scan's phase definitions for every scan slice — see D8 |
| `-9591` | Non-positive or non-finite X or Y spacing | The invalid values were assigned to the Image Geometry — see D13 |
| `-34971` | A `Data` dataset whose extent disagrees with the header's cell counts, in either direction | The reader sizes its buffers to the actual extent while the copy spans the header's point count, reading past the end of those buffers when the dataset is short and silently dropping the surplus rows when it is long |
| `-34972` | A phase value outside `[0, phase count]` | The value indexed `CrystalStructures` unchecked in the alignment loop, reading past the end of the ensemble array |

Error codes `-34970` (null pattern data) and `-9588` (stacking order not applied) are retired: the first with the unreachable execute-side pattern block described in D4, the second with the stacking-order implementation described in D9.
