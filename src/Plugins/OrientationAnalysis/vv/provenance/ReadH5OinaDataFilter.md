# Exemplar Archive Provenance: ReadH5OinaDataFilter

This sidecar documents the test data behind `ReadH5OinaDataFilter` and the disposition
of each part of it.

---

## Archive identity

| Field | Value |
|---|---|
| **Archive** | `H5Oina_Test_Data.tar.gz` |
| **SHA512** | `346573ac6b96983680078e8b0a401aa25bd9302dff382ca86ae4e503ded6db3947c4c5611ee603db519d8a8dc6ed35b044a7bfea9880fade5ab54479d140ea03` |
| **`download_test_data()` entry** | `src/Plugins/OrientationAnalysis/test/CMakeLists.txt:146` |
| **Contents** | `H5Oina_Test_Data.h5oina`, `H5Oina_Test_Data.dream3d`, `H5Oina_Test_Data.xdmf`, `Acknowledgements.md` |
| **Used by** | `test/ReadH5OinaDataTest.cpp::"Real AZtec File Readback"` (the `.h5oina` only) |
| **Changed?** | **No.** The archive is unchanged and was not re-uploaded; the SHA512 above is the one already in `CMakeLists.txt`. |

## The input file: retained

`H5Oina_Test_Data.h5oina` is a genuine Oxford Instruments AZtec export and is retained as
an irreplaceable piece of production realism. Its properties, read directly with h5py:

| Property | Value |
|---|---|
| `Format Version` | `5.0` |
| Scans | one, named `1` |
| Grid | `X Cells` 25, `Y Cells` 25 (625 points), `X Step` = `Y Step` = 4.0 |
| Phases | one group, named `1`: "Titanium cubic", Laue group 11 (Cubic-High), space group 229 |
| Lattice | dimensions 3.192, 3.192, 3.192; angles 1.5707964 rad each (90 degrees) |
| Phase column | values {0, 1} — the file contains both indexed and unindexed points |
| Euler | float32 (625, 3) in radians, observed range [0, 6.2773] |
| Extra columns | `Beam Position X`/`Y`, `Detector Distance`, `Pattern Center X`/`Y`, `Pattern Quality`, and a 625 × 512 × 622 `Processed Patterns` dataset — none of which this filter reads |

Recorded probe output: `ww_work/ReadH5OinaData/probe_real_file.txt`.

Note that the file carries `Processed Patterns` and **no** `Unprocessed Patterns`, while the
filter only ever targeted the latter — one of the observations behind deviation D4.

## The exemplar file: no longer consulted

`H5Oina_Test_Data.dream3d` is **no longer used as a comparison target**. It remains inside
the archive because the archive is unchanged, but no test reads it.

**Why.** The file was produced by running this filter on the sibling `.h5oina` in the same
archive: its cell arrays are bit-identical to the raw file data. It is therefore a
self-oracle. It is *not* a forbidden legacy oracle — no legacy H5OINA importer has ever
existed, so no version of DREAM3D could have generated it — but it pins "the filter keeps
doing what it did", not "the filter is right", which is the same failure mode.

It also had the pre-correction behavior baked in: its `LatticeConstants` reads
`[3.192, 3.192, 3.192, 1.5707964, 1.5707964, 1.5707964]`, the radian angles of deviation
D6. A test comparing against it did not merely fail to detect D6 — it actively enforced it.

Its discriminating power against the defects found in this work was near zero: the file has
a single cubic phase, so the hexagonal alignment (D1, D3) never executes; it holds one scan,
so the slab offsets (D2) are always zero; pattern import was off, so D4 and its latent type
mismatch were never reached; and its angles are all equal, so the gamma slot (D5) is
invisible. What it did pin — single-scan copy plumbing, geometry wiring, the phase widening
and the ensemble slot-0 defaults against a real-world file — is preserved and strengthened by
its replacement.

## Replacement oracle

**Toy fixtures, written by the test at run time.** `test/ReadH5OinaDataTest.cpp` declares a
fixture specification as C++ structs and writes `.h5oina` files with `H5Support::H5Lite`
into the binary test-output directory. Three fixture specifications (A, B, C) materialise
into nineteen files: nine positive-path files and ten guard files. Nothing is committed as
binary test data and no archive upload was needed. Each fixture carries exactly the dataset set `H5OINAReader`
requires, plus the inert root datasets for realism.

Every expected value is derived from the fixture specification. The hexagonal-alignment
expectations are the correctly-rounded float32 results of `double(φ2) + 30 × (π/180)`,
derived independently with IEEE-754 semantics in NumPy and embedded as literals with
derivation comments beside them.

**Class 2 independent readback for the production file.**
`…::"Real AZtec File Readback"` compares all nine cell arrays element-wise against a readback
of the `.h5oina`'s own `Data` datasets performed with `H5Lite` — the file bytes, bypassing
`H5OINAReader` entirely — for 6,966 assertions. The geometry and the three ensemble arrays
are pinned as literals derived from the h5py readback.

## Independent-derivation script

| Field | Value |
|---|---|
| **Script** | `ww_work/ReadH5OinaData/h5oina_oracle.py` (not committed; archived to the V&V working-folder remote) |
| **Interpreter** | `/opt/local/anaconda3/envs/dream3d/bin/python`, h5py 3.16.0, NumPy |
| **Author** | Michael Jackson <mike.jackson@bluequartz.net> |
| **Date** | 2026-08-24 |
| **`spec` mode** | Prints the fixture specification and every derived expectation as C++-ready float32 literals. Recorded output: `oracle_spec.txt`. This is the source of the pinned hexagonal-alignment constants. |
| **`readback` mode** | Re-reads an `.h5oina` with h5py and re-derives the expected NX arrays from the documented rules, without EbsdLib or simplnx. Recorded output for the production file: `readback_real_file.txt`. |

The script encodes the derivation rules read out of the filter and algorithm source — geometry
from the header, slab placement, verbatim copies, the phase widening, the Laue-group mapping,
the ensemble slot-0 defaults, the radians-to-degrees lattice-angle conversion and the
hexagonal φ2 alignment — and applies them to the fixture specification or to a file's bytes.
It never runs the filter.

Supporting evidence in the same folder:

| File | What it records |
|---|---|
| `pi_over_6_discriminator.txt` | The exhaustive sweep of float32 values in `[0.25, 6.5)` that identified which φ2 values separate a double-precision intermediate from a float32 one (8,289,627 of 38,797,312 do), and confirmation that `30 × k_PiOver180D` is bit-identical to the nearest double to π/6 |
| `probe_real_file.txt` | The h5py structure dump of the production `.h5oina` |
| `red_baseline.log` | The full test suite run at the batch base commit: 14 of 16 failing, including the SEGFAULT that is deviation D7 |
| `mutation_table.md` | Seven mutations, each killing exactly its claimed test cases, each reverted to an empty diff |
| `base_oa_suite.log` / `full_oa_suite.log` | The OrientationAnalysis suite at base sources and at branch head, used to establish that all 29 suite failures are pre-existing |
| `ebsdlib_suite.log`, `simplnxcore_suite.log` | The EbsdLib and SimplnxCore suite runs |

## Oracle-before-comparison ordering

There was no comparison to order against: DREAM3D 6.5.171 has no H5OINA importer. The oracle
was nonetheless established before any filter output was observed — the fixture
specification, the derivation script and every pinned literal were written and recorded
(`oracle_spec.txt`, timestamped before the first build) ahead of the first test run, and the
first run of the suite against the batch-base code is the RED baseline in `red_baseline.log`.
No expected value in the test file was taken from observed output.

## Second-engineer oracle review

Outstanding — to be recorded at PR review. The oracle design is fully auditable from the
committed test source: the fixture specification, the derivation of every pinned literal and
the reason each discriminating φ2 value was chosen are all stated in comments beside the
values.

## Archive disposition

`H5Oina_Test_Data.tar.gz` stays in the GitHub
[Data_Archive release](https://github.com/BlueQuartzSoftware/simplnx/releases/tag/Data_Archive)
and on the www.dream3d.io mirror unchanged, and its `download_test_data()` entry stays in
`test/CMakeLists.txt` because the `.h5oina` input is still needed. Should the archive ever be
regenerated, the `.dream3d` and `.xdmf` members can be dropped: nothing reads them.
