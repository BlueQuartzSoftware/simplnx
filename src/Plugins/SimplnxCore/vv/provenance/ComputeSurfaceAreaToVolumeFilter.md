# Exemplar Provenance: ComputeSurfaceAreaToVolumeFilter

**No exemplar archive.** As of this V&V cycle (2026-08-20) the filter's correctness tests are **inline Class 1 (Analytical) + Class 4 (Invariant) fixtures** built directly in `test/ComputeSurfaceAreaToVolumeTest.cpp` (`namespace SavToy`). There is no `.dream3d` gold master to hash — the oracle lives in the test code as hand-derived `REQUIRE` values, each accompanied by its derivation as a comment.

## Retired test / circular-oracle removal

The prior test `SimplnxCore::ComputeSurfaceAreaToVolume` compared freshly computed `SurfaceAreaVolumeRatioNX` / `SphericityNX` arrays against **sibling `SurfaceAreaVolumeRatio` / `Sphericity` arrays in the same `6_6_stats_test_v2.dream3d` file**. Those reference arrays were produced by an earlier DREAM3D run of the same algorithm, making the check a **consistency-with-self / circular oracle** (see `docs/vv_templates/oracle_classes.md`, "What is NOT an oracle"). It was **retired** and replaced by the analytical fixtures below.

Two **executed** facts establish that the retired test was not merely circular but incapable of doing its job (script `blind_suite.py` and output `results_blind_suite.txt` in the V&V working folder; re-runnable against any extraction of the archive):

1. **It could not see the face-area bug.** The archive's `_SIMPL_GEOMETRY/SPACING` is `(0.25, 0.25, 0.25)`. Deviation D1 swaps the ±X face area (`dy·dz`) with the ±Y face area (`dx·dz`); at isotropic spacing those products are equal, so the swap is a no-op on this dataset. No number of additional assertions against this file could have detected D1.
2. **Its reference data carried the sphericity bug.** Reconstructing `A` from the archive's own `SurfaceAreaVolumeRatio` and `NumElements` and recomputing sphericity over its 619 non-background features: the archived `Sphericity` matches the **truncated**-exponent expression (`π^0.333333 (6V)^0.66666 / A`) to 1.6e-7 relative, and differs from the exact-exponent expression by up to **4.9e-5** relative. After the D2 fix the retired test would have begun failing against its own reference arrays.

A third limitation, relevant to the mutation sweep: the archive's `NumElements` array equals `bincount(FeatureIds)` exactly for all 620 features (executed check), so it cannot distinguish "volume from the user-supplied `NumCells`" from "volume recounted from `FeatureIds`". Inline fixture F2b exists to close that gap.

### Archive disposition

- `6_6_stats_test_v2.tar.gz` — this filter's tests no longer consume it. The `download_test_data()` entry in `src/Plugins/SimplnxCore/test/CMakeLists.txt` (SHA512 `e84999de…089723`) is **left untouched**: `ComputeEuclideanDistMapTest` and `ComputeFeatureNeighborsTest` still consume it in SimplnxCore, and `ComputeShapesFilterTest`, `ComputeSchmidsTest` and `AlignSectionsMutualInformationTest` consume it in OrientationAnalysis. Any future regeneration must account for all five consumers.

## Canonical oracle output (Class 1 — hand derivation)

`A(f)` = Σ over each cell of feature `f` and each of its six face neighbors of the shared face area, counted iff the neighbor is inside the grid **and** carries a different id (id 0 included as "different"). ±Z face = `dx·dy`; ±Y face = `dx·dz`; ±X face = `dy·dz`. `V(f) = NumCells[f]·dx·dy·dz`. `SAVR = A/V`. `Sphericity = π^(1/3)(6V)^(2/3)/A`.

F1–F7 (eight TEST_CASE sections) were derived by an independent brute-force face enumeration (`derive.py`) written from those rules and RED-run before either the SIMPLNX or the DREAM3D binary was invoked on any fixture. F1b and F2b were added afterward — F1b for deviation D3's over-provisioned Feature AM case, F2b to close the volume-recount blind spot — with expected values likewise hand-derived from the same rules rather than read off any observed run (F2b's was also checked post-hoc against `derive.py`, but that check confirms a hand-derived value, not a recount: the expected value is specifically not what a recount would produce). Ratio values are exactly representable in float32 and are asserted as exact equalities; sphericity is asserted to 1e-6 relative, a bound justified by a float32 simulation of the fixed expression (`f32check.py`, worst case 1.3e-7).

| Fixture | Grid / spacing | Purpose | Key expected value |
|---|---|---|---|
| F1 | 3³, spacing 0.5 | single interior voxel; index-0 sentinel | `SAVR[1] = 12.0`; `Ψ[1] = 0.8059959770`; `SAVR[0] = Ψ[0] = 0` |
| F1b | 3³, 4 feature tuples | over-provisioned Feature AM (deviation D3) | `SAVR = [0, 12, 0, 0]`; `Ψ[2] = Ψ[3] = +inf` |
| F2 | 4³, spacing 0.5 | 2×2×2 cube; sphericity scale invariance | `SAVR[1] = 6.0`; `Ψ[1] = 0.8059959770` (same as F1) |
| F2b | 3³, spacing 0.5, `NumCells[1] = 8` | volume from `NumCells`, not a recount | `SAVR[1] = 1.5`; `Ψ[1] = 3.2239839080` |
| F3 | 6×8×3, spacing (1, 2, 4) | anisotropic X-rod + Y-rod; the D1 discriminator | `A = [_, 64, 88]` → `SAVR = [_, 2.0, 2.75]`; `Ψ = [_, 0.7616184728, 0.5539043438]` |
| F4 | 3³, spacing 0.5, corner cell | outer-boundary faces not counted | `A = 3a² = 0.75`; `SAVR[1] = 6.0`; `Ψ[1] = 1.6119919540` (> 1, expected) |
| F5 | 3³, all cells id 1 | degenerate divide | `SAVR[1] = 0`; `Ψ[1] = +inf` |
| F6 | 3³, shell id ∈ {0, 2} | id 0 counts as surface | `SAVR[1] = 12.0` for both shells; shell 2: `SAVR[2] = 1.5/3.25`, `Ψ[2] = 7.0737293550` |
| F7 | 7×3×1, spacing (1, 2, 4) | 2D slab, no ±Z faces; D1 in the 2D path | `SAVR = [_, 3.0, 2.0]`; `Ψ = [_, 0.8059959770, 0.9595791455]` |
| F8 | 3³, sphericity off | array not created (no legacy counterpart) | `getDataAs<Float32Array>(sphericityPath) == nullptr` |
| errors | 3³ variants | `-5355`, `-5351`, `-12802`, `-12803` | `preflight`/`execute` invalid with the exact code |

**Pre-fix values, for anyone reconciling stored output.** The same derivation, evaluated with the two shipped defects (swapped ±X/±Y areas, exponents `0.333333`/`0.66666`), predicted the pre-fix output before it was observed; the observed pre-fix values matched to nine decimal places. `Ψ`: F1 `0.805997133`, F2 `0.805986106`, F2b `3.223944167`, F3 `[0.553884745, 0.761591554]` (also exchanged), F4 `1.611994267`, F6 shell 2 `7.073586941`, F7 `[0.805974901, 0.767639]`. `SAVR`: F3 `[2.75, 2.0]` (exchanged), F7 feature 2 `2.5`.

## Second-engineer oracle review

- **Delegated to the PR reviewer** (requester decision, 2026-08-19).
- Suggested review focus: the F3 area arithmetic (`A₁ = 2·(dy·dz) + 8·(dx·dz) + 8·(dx·dy) = 16 + 32 + 16 = 64`, and its mirror `A₂ = 2·4 + 8·8 + 8·2 = 88`), on which the entire D1 argument rests; and the deliberate `Ψ > 1` expectation in F4, which reads like a defect and is the documented consequence of the boundary-face rule.

## Reproduction

The fixtures require no external data:

```
cmake --build <build-dir> --target SimplnxCoreUnitTest
ctest -R "SimplnxCore::ComputeSurfaceAreaToVolume" --verbose
```

The A/B and alignment evidence (fixture writer, both pipeline sets, comparison and alignment scripts, the mutation transcript, and the legacy patch diff) lives in the V&V working folder `ww_work/ComputeSurfaceAreaToVolume/`, which is intentionally not committed to this repository. All hand derivations are also in the code comments beside each fixture and in `src/Plugins/SimplnxCore/vv/ComputeSurfaceAreaToVolumeFilter.md`.
