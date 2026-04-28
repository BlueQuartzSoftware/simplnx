# Oracle Classes — Quick Reference

One-page cheat sheet for filter V&V oracles. Policy source: [`mtr_filter_verification_validation.md`](./mtr_filter_verification_validation.md).

## The five classes

| # | Class | What it is | Drift risk |
|---|---|---|---|
| 1 | **Analytical** | Closed-form expected output on toy input. Threshold, crop, rotate, color conversion, array arithmetic. | None |
| 2 | **Reference implementation** | Trusted external library produces the expected output. NumPy/SciPy, MTEX/OrientationLib, Eigen. | **High** — library version drift |
| 3 | **Paper-based** | Filter reproduces a published figure, table, or equation from a named reference. | Low–Medium |
| 4 | **Invariant** | Derivable properties the output must satisfy. FeatureIds start at 1 and are contiguous; sum of phase fractions = 1; etc. | None |
| 5 | **Expert-visual** | Domain expert signs off on output for canonical test cases. Last resort. | **High** — social drift |

## Where each class lives

| # | Artifact location | Form |
|---|---|---|
| 1 | Unit test code | `REQUIRE(result == X)` + derivation comment |
| 2 | Archive (script) + test (cached exemplar `.dream3d`) | Script → exemplar; test compares against it |
| 3 | Unit test code + archive (paper ref) | Numeric `REQUIRE`s with citation comment; DOI + figure # in ReadMe |
| 4 | Unit test code | Property assertions on output |
| 5 | Archive (screenshots + sign-off) + test (cached exemplar) | Screenshots are the record; test compares against approved exemplar |

Summary: **1, 3, 4 → inline test assertions. 2, 5 → cached exemplar files.**

## Provenance required in archive ReadMe

- **Class 2:** library + exact version + seed (if any) + script filename. Optional: hash of script's output for drift detection.
- **Class 3:** DOI + edition + figure/table/equation # + page #. Embed the paper PDF (or the specific figure) in the archive.
- **Class 5:** named expert + date + signed-off screenshots + class-5-only justification.
- **Classes 1 and 4:** no provenance block needed — oracle lives in the test code.

## Policy gates

- Every filter needs at least one class **1–4** oracle, **or** a documented justification for class-5-only in the archive ReadMe.
- "Legacy 6.5.172 produced this output" is **never** a valid oracle for correctness.
- Oracle design **really should** be reviewed by a second engineer before use. Skips must be recorded in the archive ReadMe with a reason.

## Workflow reminder (Step 0, a–e)

1. **Classify** — pick oracle class, record in archive ReadMe.
2. **Design toy data** — minimum-size synthetic inputs that exercise each code path *and* that the oracle can answer.
3. **Compute expected output independently** — do not run any DREAM3D version. Save the oracle computation (script / spreadsheet / paper ref / expert notes) in the archive.
4. **Run SIMPLNX** on toy data. Resolve any discrepancy vs. the oracle before moving on.
5. **Run 6.5.172** on the same toy data. Diff against verified SIMPLNX output. Each difference → one Deviation entry (`<FilterName>-D<N>`) with a named root cause.

Refactor (readability / performance / memory) only *after* steps 1–5 are complete — a correctness safety net has to exist before it's safe to refactor.
