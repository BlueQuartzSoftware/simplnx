# Exemplar Provenance: RotateSampleRefFrame (no archive — inline Class 1 oracle)

This filter has **no exemplar archive**. The prior golden-file archives were retired this V&V cycle and replaced by an inlined Class 1 (analytical) + Class 4 (invariant) oracle that lives entirely in the test source. This sidecar records that decision and the provenance of the legacy A/B inputs.

---

## Retired archive

| Field | Value |
|---|---|
| **Retired archives** | `Rotate_Sample_Ref_Frame_Test_v2.tar.gz`, `Rotate_Sample_Ref_Frame_Test_v3.tar.gz` |
| **Retired on** | 2026-07-02 |
| **Where they were referenced** | `src/Plugins/SimplnxCore/test/CMakeLists.txt` `download_test_data()` (entries removed) and the old `SimplnxCore::RotateSampleRefFrame` exemplar test (rewritten) |
| **Why retired** | Golden-file / regression oracle: the test compared filter output against a bundled `.dream3d` of previously-captured output, which cannot establish correctness independently (circular-oracle pattern per `oracle_classes.md`). The prior test also included 45-degree rotation cases that are now (correctly) rejected by the preflight guard. |

## Canonical oracle output (inline, no archive)

| Oracle | Source of expected values |
|---|---|
| 180@Z on a 3×2×1 volume → `[6,5,4,3,2,1]` | Class 1 hand derivation (a 180-degree rotation about Z reverses a single Z-slice's row-major order) — inline in `test/RotateSampleRefFrameTest.cpp` |
| Output dimensions for all 9 principal-90 rotations | Class 1 (deterministic axis permutation of the input dims) — inline |
| Value-multiset conservation, zero background, full-circle composition = identity | Class 4 invariants — inline predicates |
| `KeepInputGeometryOrigin` origin values `(0,0,0)` / `(-3,0,0)` | Class 1 hand derivation (rotated bounding-box min for 90@Z of a 4×3×2 at origin 0) — inline |

Classes 1 and 4 require no external provenance block — the oracle lives in the test code.

## Legacy A/B comparison inputs (provenance)

The DREAM3D 6.5.171 comparison used a shared input minted with the `legacy_dream3d` h5py writer (a legacy v7/SIMPL `.dream3d` readable by both PipelineRunner and nxrunner):

- **Generator:** `vv/comparisons/RotateSampleRefFrameFilter/make_input.py` — 4×3×2 Image Geometry, origin (0,0,0), spacing (1,1,1), Int32 `Data = 1..24` (ZYX).
- **Pipelines:** `gen_simpl_pipeline.py` (legacy `.json`) and `gen_nx_pipeline.py` (NX `.d3dpipeline`), 4 cases each (90@Z, 180@Z, 90@X, 180@Y).
- **Result:** bit-identical on all four fixtures — `vv/comparisons/RotateSampleRefFrameFilter/results.md`.

These A/B artifacts are working files, not a test-consumed archive; they are kept in the source tree for reproducibility only.

## Second-engineer oracle review

- **Reviewer:** Michael Jackson (technical authority)
- **Date:** 2026-07-16

## Regenerated to fix a circular-oracle situation?

Yes — this replaces the retired `Rotate_Sample_Ref_Frame_Test_v2/v3.tar.gz` golden-file archives with an independently-derived Class 1 analytical oracle. The new oracle is *not* derived from SIMPLNX (or legacy) output; it is hand-derived from the definition of a principal-90 reference-frame rotation.
