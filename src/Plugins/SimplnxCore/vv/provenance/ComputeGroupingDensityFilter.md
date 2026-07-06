# Exemplar Archive Provenance: compute_grouping_densities_v2.tar.gz

This sidecar records how the exemplar archive used in `ComputeGroupingDensityFilter`'s Class 1 + Class 4 unit tests was generated. It is the answer to "where did this gold-standard data come from?"

Pairs with the archive's inline `README.md` (Mike's hand sign-off) and the V&V report at `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter.md`. The inline `README.md` is the materially-complete provenance document; this sidecar exposes the same information at the in-source location the V&V policy expects (`src/Plugins/<Plugin>/vv/provenance/<FilterName>.md`).

---

## Archive identity

| Field                  | Value                                                                                                                              |
|------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| **Archive**            | `compute_grouping_densities_v2.tar.gz`                                                                                             |
| **SHA512**             | `3aaabb63c4fa16f7fa192ae4ee9dbba9394ec7f1cd19aff55e399a624d495a3a778c7f6f282911f681e85cea99e4c6d15344274e9107f337af7d4a19f93784ff` |
| **Used by tests**      | `SimplnxCore::ComputeGroupingDensityFilter: Exemplar A/B — all 4 configurations` (DYNAMIC_SECTION, 4 fixtures `NC0_CF0`, `NC0_CF1`, `NC1_CF0`, `NC1_CF1`) |
|                        | `SimplnxCore::ComputeGroupingDensityFilter: Empty-parent edge case (-1.0f sentinel)` (Class 4 inline-invariant test)               |
| **CMakeLists.txt**     | `src/Plugins/SimplnxCore/test/CMakeLists.txt` line 291                                                                             |
| **Hand-review sign-off** | Michael A. Jackson (BlueQuartz Software), **2026-05-27**, recorded in the archive's inline `README.md` §"Sign-off"                |

---

## Authorship chain

The exemplar dataset went through four distinct hands before publication. This filter is the originating use-case for the BlueQuartz LLM-attribution provenance policy; the chain is recorded explicitly because more than one LLM session touched the dataset and the boundaries between human design, LLM scaffold, and human validation matter for downstream auditors.

| Step | Actor                                                                  | Contribution                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|------|------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1    | **Claude (Opus 4.6, Anthropic)** under direction of Michael A. Jackson | Created the initial input `.dream3d` test dataset at `data/compute_grouping_density_inputs.dream3d` — the 20×5×1 `ImageGeometry` carrying the 5-feature / 2-parent hand-built microstructure (`FeatureIds`, `ParentIds` at the cell level; `Volumes`, `ParentVolumes`, `ParentIds` at the feature level; `ContiguousNL`, `NonContiguousNL` NeighborLists) designed to exercise all 4 `(UseNonContiguousNeighbors, FindCheckedFeatures)` template specializations of `FindDensityGrouping`. |
| 2    | **Michael A. Jackson** (BlueQuartz Software)                           | Manual structural review of the input dataset — confirmed that the `.dream3d` file contains every input array the filter consumes (`ParentIds`, `Volumes`, `ParentVolumes`, contiguous NeighborList, non-contiguous NeighborList) at the correct `DataPath`s with the correct types and shapes. Verified that the cell-level `FeatureIds` / `ParentIds` layout is consistent with the feature-level volumes (cell counts per feature: 0, 10, 20, 15, 25, 30; cell counts per parent: 0, 45, 55). Signed off as structurally correct from a filter-input-completeness point of view. |
| 3    | **Claude (Opus 4.7, Anthropic)** under direction of Michael A. Jackson | Independent test-coverage review of the same input dataset — confirmed the chosen `Volumes`, `ParentIds`, `ParentVolumes`, and neighbor-list topology are *sufficient* to exercise all 4 template specializations AND deterministically resolve the `CheckedFeatures` last-writer-wins-on-greater-volume tie-break path. Validated the Class 1 analytical hand-derivation against the dataset (see §"Canonical oracle output" below).                              |
| 4    | **Claude (Opus 4.7, Anthropic)** under direction of Michael A. Jackson | Translated the input dataset into a fully-reproducible Python generator script (`generate_inputs.py` in the archive) that emits the same input file as a legacy v7.0-format `.dream3d` consumable by **both** the DREAM3D 6.5.171-era `PipelineRunner` *and* SIMPLNX `nxrunner`. Also authored the `compare_outputs.py` byte-diff script. These two scripts enabled the empirical A/B comparison against the locally-rebuilt legacy `FindGroupingDensity` binary that produced the **bit-identical, zero-deviation** result documented in `vv/deviations/ComputeGroupingDensityFilter.md` and `results/ab_comparison_report.txt`. |

The original hand-built fixture *design* (the choice of which configurations to cover and the `(Volumes, ParentIds, ParentVolumes, NeighborList)` tuple that achieves the coverage) was directed by Michael A. Jackson; the LLM contributions are scaffolding (initial `.dream3d` materialization, coverage cross-check, Python-script back-translation for cross-binary consumption, byte-diff comparison harness). Every numerical value was independently re-derived in the Class 1 analytical hand-derivation embedded in the V&V report.

---

## How the archive was generated

The published `compute_grouping_densities_v2.tar.gz` archive contains:

1. **Input file** (`data/compute_grouping_density_inputs.dream3d`) — the 20×5×1 `ImageGeometry` designed in Step 1 of the authorship chain. Legacy v7.0 format (FileVersion 7.0) so the *same* file is consumed by both the locally-rebuilt legacy DREAM3D binary and SIMPLNX. Contents:
   - `CellData/{FeatureIds, ParentIds}` — 100-voxel layout where each feature occupies a contiguous slab matching its volume (10 voxels for F1, 20 for F2, 15 for F3, 25 for F4, 30 for F5)
   - `FeatureData`: `Volumes = [0, 10, 20, 15, 25, 30]`, `ParentIds = [0, 1, 1, 1, 2, 2]`, plus the two `NeighborList<int32>` arrays:
     - `ContiguousNL    = [[], [2], [1,3], [2,4], [3,5], [4]]`
     - `NonContiguousNL = [[], [4], [5], [], [1], [2]]`
   - `ParentData`: `ParentVolumes = [0, 45, 55]`

2. **Generator script** (`generate_inputs.py`) — Python+h5py reproducer for the input file. The script docstring documents every SIMPL HDF5 attribute (`@FileVersion`, `@TupleDimensions`, `@ComponentDimensions`, `@ObjectType`, `@DataArrayVersion`, `@Tuple Axis Dimensions`) and the `NeighborList<T>` two-dataset convention (main 1D array + `_NumNeighbors` companion). Any auditor can rebuild the input file byte-for-byte from source.

3. **SIMPLNX outputs** (`output_simplnx/simplnx_compute_grouping_density_ab.dream3d`) — single output file with all 4 template-specialization outputs as separate arrays:
   - `/DataStructure/DataContainer/ParentData/GroupingDensities_NC<n>_CF<c>` (all 4 configs)
   - `/DataStructure/DataContainer/FeatureData/CheckedFeatures_NC<n>_CF<c>` (2 configs with CF=1)

4. **Legacy outputs** (`output_legacy/6_5_find_grouping_density_NC<n>_CF<c>.dream3d`) — 4 files, one per `(NC, CF)` configuration, produced by the locally-rebuilt `FindGroupingDensity` binary — a local build of the legacy DREAM3D 6.5 source with the `tuks188/DREAM3D` `feature/770_Grouping_Density` sources pulled in. Output arrays are prefixed `6_5_` so they can be co-loaded with the SIMPLNX outputs in DREAM3D-NX for side-by-side visual diff.

5. **Pipelines** (`pipelines/`) — `6_5_find_grouping_density_NC<n>_CF<c>.json` (4 SIMPL JSON pipelines, one per config) + `simplnx_compute_grouping_density_ab.d3dpipeline` (1 SIMPLNX pipeline running all 4 configs sequentially against the same input).

6. **Comparison script + report** (`compare_outputs.py` + `results/ab_comparison_report.txt`) — Python+h5py reader that opens all 4 legacy outputs and the single SIMPLNX combined output and prints per-array byte-comparison results. Result is checked in for reproducibility audit.

7. **Inline `README.md`** — Michael A. Jackson's hand-review sign-off (2026-05-27) capturing the structural correctness review (Step 2), the Class 1 analytical expected-output table, the reproduction instructions, and the headline result (all 4 configurations bit-identical, zero deviations).

---

## Canonical oracle output

| DataPath                                                | Source of expected values                                                                                                                                                                                                          |
|---------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `/DataContainer/ParentData/GroupingDensities_NC0_CF0`   | **Class 1 analytical** — `Parent1 = 45/70 ≈ 0.6428571`, `Parent2 = 55/70 ≈ 0.7857143` (touched-set = assigned ∪ contiguous-NL neighbors of assigned)                                                                                |
| `/DataContainer/ParentData/GroupingDensities_NC0_CF1`   | **Class 1 analytical** — identical density values to `NC0_CF0`                                                                                                                                                                       |
| `/DataContainer/ParentData/GroupingDensities_NC1_CF0`   | **Class 1 analytical** — `Parent1 = 45/100 = 0.45`, `Parent2 = 55/100 = 0.55` (touched-set = assigned ∪ contiguous-NL ∪ non-contiguous-NL neighbors of assigned; both parents touch all 5 features)                                 |
| `/DataContainer/ParentData/GroupingDensities_NC1_CF1`   | **Class 1 analytical** — identical density values to `NC1_CF0`                                                                                                                                                                       |
| `/DataContainer/FeatureData/CheckedFeatures_NC0_CF1`    | **Class 1 analytical** — `[0, 1, 1, 2, 2, 2]` (parent 2 wins the tie at features 3 & 4 because `ParentVolumes[2] = 55 > ParentVolumes[1] = 45` and the algorithm uses last-writer-wins-on-greater-volume semantics)                |
| `/DataContainer/FeatureData/CheckedFeatures_NC1_CF1`    | **Class 1 analytical** — `[0, 2, 2, 2, 2, 2]` (both parents touch every feature; parent 2 wins on every feature for the same volume-tie reason)                                                                                      |
| All `GroupingDensities_*` arrays (invariant predicates) | **Class 4 invariant** — `density[0] == 0.0f`; for `i ≥ 1`, `density[i] > 0 ∨ == -1.0f`; for `i ≥ 1`, `density[i] ≤ 1.0f`. Asserted inline in the test for every fixture.                                                            |
| `CheckedFeatures_*` arrays (invariant predicates)       | **Class 4 invariant** — `CheckedFeatures[k] ∈ {0, …, numParents-1}`; `CheckedFeatures[0] == 0`. Asserted inline in the test for the two `_CF1` fixtures.                                                                            |

Bit-exact float32 values produced by both binaries (from `ab_comparison_report.txt`):
- `0.6428571343421936` (45/70), `0.7857142686843872` (55/70)
- `0.44999998807907104` (0.45), `0.550000011920929` (0.55)

The unit test uses `UnitTest::CompareDataArrays<float32>` and `<int32>` to verify bit-exact equality between the SIMPLNX-generated arrays and the archived exemplar arrays. Class 4 invariant predicates are asserted inline in the same test, independent of the exemplar.

Full Class 1 hand-derivation table is embedded in `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter.md` §"Oracle".

---

## Oracle provenance (Classes 2, 3, 5 only)

N/A — Class 1 and Class 4 oracles only. No reference-library invocation, no paper-figure reproduction, no expert-visual sign-off needed.

---

## Second-engineer oracle review

- **Reviewer:** *Skipped*
- **Date:** N/A
- **Skip reason:** The Class 1 derivation is set-union sums + ratio division on a 5-feature hand-built fixture (high-school arithmetic). External cross-validation was obtained via the independently-authored legacy `FindGroupingDensity` implementation (`tuks188/DREAM3D` `feature/770_Grouping_Density`, rebuilt locally against the legacy DREAM3D 6.5 source): the A/B comparison produced **bit-identical** agreement across all 4 `(NC, CF)` configurations and both output arrays (`results/ab_comparison_report.txt`). Any oracle-derivation error would have surfaced as a deviation. Formal second-engineer review of a 5-feature analytical oracle was not justified given this cross-check.

---

## Regenerated to fix a circular-oracle situation?

**Yes — v2 supersedes v1 to remove a circular-oracle concern.**

| Field           | v1 (retired)                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Archive name    | `compute_grouping_densities.tar.gz`                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Provenance      | Generated from a single SIMPLNX `ComputeGroupingDensityFilter` run by the filter author (PR #1548), without an independent oracle and without a legacy-binary cross-check. The exemplar arrays in v1 were therefore *whatever SIMPLNX produced at the time the archive was published* — a textbook circular oracle (the test compares SIMPLNX-now against SIMPLNX-then; an algorithmic regression that simultaneously breaks the live code AND the regeneration script would slip through). |
| Retired on      | 2026-05-27 (V&V cycle close)                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| CMakeLists.txt  | `download_test_data(... compute_grouping_densities.tar.gz ...)` removed from this filter's section of `src/Plugins/SimplnxCore/test/CMakeLists.txt` (no other test in this filter family consumed it).                                                                                                                                                                                                                                                                                |

v2 closes the circular-oracle gap by:

1. **Independent oracle** — the Class 1 hand-derivation embedded in the V&V report was carried out from the input definition alone, with no reference to either DREAM3D 6.5.171 or SIMPLNX output.
2. **External cross-validation** — the same input was fed through a locally-rebuilt legacy `FindGroupingDensity` binary; output was bit-identical to SIMPLNX across all 4 configurations (see `results/ab_comparison_report.txt`).
3. **Reproducibility** — `generate_inputs.py` and `compare_outputs.py` let any auditor regenerate both the input and the comparison from source.
4. **Coverage** — v1 carried output for only one `(NC, CF)` configuration; v2 carries all four, exercising every template specialization of `FindDensityGrouping`.

v2 ships with the inline `README.md` sign-off so the provenance travels with the archive even when consumed outside the source tree.
