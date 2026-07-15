# V&V Report: RegularizeZSpacingFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `d6599986-1932-4bfc-993d-71eafefe6db0` |
| DREAM3D 6.5.171 equivalent | `RegularizeZSpacing` — `Source/Plugins/Sampling/SamplingFilters/RegularizeZSpacing.{h,cpp}` (legacy UUID `bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *Michael Jackson <mike.jackson@bluequartz.net> (port + V&V cycle, 2026-07-08); second-engineer oracle review pending* |

## At a glance

| Aspect                 | Current state                                                                                                                |
|------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Port** of legacy `RegularizeZSpacing::execute()` — identical Z-plane mapping rule and `floor(extent/newZRes)` dim math (independently hand-traced, including the strict-`>` boundary and clamp cases). Five port-time deltas (bulk copy, new-geometry output mode, preflight validation, parallelization, cell-AM binding); none change output for valid input. |
| Oracle (confirmed)     | **Class 1 (Analytical)** primary + **Class 4 (Invariant)** companion — closed-form indirection map `out[i] = in[map[i]]`. Element-wise Class 1 assertions in 2 fixtures (`Valid Execution (New Geometry)`, `Valid Execution (Spacing Exceeds Extent)`); Class 4 invariants across the valid fixtures. All pass in-core + OOC. |
| Code paths enumerated  | 12 of 14 exercised; the 2 uncovered are the redundant file-open guard and the cancel check (reasons below).                  |
| Tests today            | 5 TEST_CASEs (2 Class-1 valid + 1 in-place valid + 1 invalid-parameters with 6 SECTIONs + 1 SIMPL backwards-compat with 6.5/6.4 fixtures); every documented preflight error code asserted explicitly. |
| Exemplar archive       | **None** — inline analytical fixtures (no `download_test_data`; avoids a circular oracle per project policy).                 |
| Legacy comparison      | **Run** vs DREAM3D 6.5.171 on a synthetic multi-type fixture (int32 + bool + 3-component float). Bit-identical: legacy == SIMPLNX == Class-1 oracle on every array and on geometry (dims/spacing/origin). |
| Bug flags              | None.                                                                                                                        |
| V&V phase              | Oracle applied and reconciled; algorithm review + three independent adversarial reviews applied (fixes folded in); dual-build green; legacy A/B bit-identical with 0 deviations. **Outstanding:** second-engineer oracle review before COMPLETE. |

## Summary

`RegularizeZSpacingFilter` resamples an Image Geometry with irregular Z slice spacing onto a uniform Z spacing: it reads the physical Z position of each original slice boundary from a text file (`ZPoints + 1` values), computes `newZ = floor(totalZExtent / newZSpacing)` (min 1), and copies each new plane's cell data from the original plane whose boundary interval contains it. Verification used a **Class 1 (Analytical)** oracle — the algorithm is a closed-form indirection lookup `output[i] = input[map[i]]` — with **Class 4 (Invariant)** companions (X/Y dims and spacing preserved, origin preserved, Z spacing becomes the requested value). A direct A/B against DREAM3D 6.5.171 on a synthetic multi-type dataset is **bit-identical** (0 deviations); legacy, SIMPLNX, and the analytical oracle agree exactly.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* Near line-by-line translation of legacy `RegularizeZSpacing::execute()`. Same SIMPL UUID retained and mapped in `SimplnxCoreLegacyUUIDMapping.hpp`. The new→old Z-plane mapping loop (`plane` = largest `iter` in `[1, origZ)` with `i*newZRes > zBound[iter]`, else 0) and the `newZ = (size_t)(lastZBound / newZRes)` (min 1) computation are reproduced exactly; an independent hand-trace confirmed the arithmetic types match legacy expression-by-expression (float32 throughout, no double promotion), so even float-boundary cases agree.

*Port-time deltas (none change output for valid input — confirmed bit-identical in the legacy A/B):*

1. **Data copy**: legacy per-tuple `memcpy` via a rebuilt `newindicies` table → SIMPLNX bulk `AbstractDataStore::copyFrom` of one contiguous plane-slab per destination plane. Same bytes moved; more efficient and out-of-core friendly.
2. **Output mode**: legacy replaces the cell AttributeMatrix in place → SIMPLNX uses the modern new-geometry pattern (`CreateImageGeometryAction` + rename + deferred delete) with a "Perform In Place" toggle (default true). In-place mode is behaviorally equivalent to legacy.
3. **Preflight validation added**: positive `new_z_spacing`; input file has ≥ `ZPoints + 1` parseable values; values monotonically non-decreasing; positive total Z extent; only DataArray members in the cell AM. These reject bad input that legacy silently consumed (short files reused the last-read value; non-monotonic files produced a garbage mapping; zero extent produced a clamped 1-plane output) but do not alter valid-input output. The `.txt` extension is a dialog hint only (`acceptAllExtensions`), so converted legacy pipelines referencing `.dat`/`.csv`/extensionless files still validate.
4. **Parallelization**: per-array parallel task runner (one task per cell array, distinct stores) vs legacy serial loop. No output effect.
5. **Cell-AM binding**: legacy took an explicit AttributeMatrix path; SIMPLNX takes the geometry and uses its assigned cell AttributeMatrix. `FromSIMPLJson` keeps only the DataContainer name from the legacy `CellAttributeMatrixPath` (the AM name is discarded). For the overwhelmingly common single-cell-AM case this is identity; corner-case consequences for converted pipelines are documented in the deviations file's scope note. Related accept/reject divergences: a geometry with no cell AM now errors (`-5560`, impossible to express in legacy), and a stale AM name that legacy rejected converts to the geometry's actual cell AM and proceeds.

*Material PRs since baseline:* new filter port (this branch); no prior history.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion.

*Applied (Class 1):* The output cell data is a closed-form indirection lookup `output_tuple[t] = input_tuple[map[t]]`, where `map` is derived by hand from the Z-positions file and `new_z_spacing`. Primary fixture (origZ=4, zBound `{0,1,3,6,10}`, newZRes 2.0): new→old plane map `[0,1,2,2,3]`, giving a fully hand-derived 10-tuple expected output. Clamp fixture (same bounds, newZRes 20.0): `newZ = floor(10/20) = 0 → 1`, expected output = source plane 0. Both derivations are embedded as comments beside the `REQUIRE`s.

*Applied (Class 4):* X and Y dimensions and spacing preserved, origin preserved, Z spacing equals `new_z_spacing`, `newZ == max(1, floor(lastZBound / newZRes))`, and loose (non-cell-AM) children are carried over intact. Asserted in the valid-execution fixtures.

*Encoded:* `test/RegularizeZSpacingTest.cpp::SimplnxCore::RegularizeZSpacingFilter: Valid Execution (New Geometry)` (element-wise Class 1 + invariants), `...::Valid Execution (Spacing Exceeds Extent)` (clamp-path Class 1), `...::Valid Execution (In Place)` (in-place invariants) — all pass in-core (`NX-Com-Qt69-Vtk95-Rel`) and out-of-core (`NX-OOC-Qt69-Vtk95-Rel`).

*Second-engineer review:* **Pending.** The oracle is a deterministic index remap (lowest-risk oracle class); review still recommended before promotion to COMPLETE.

## Code path coverage

`12 of 14 paths exercised.` The 2 uncovered are a redundant file-open guard and the cancel check, listed with reasons.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RegularizeZSpacing.cpp` (217 lines) + `Filters/RegularizeZSpacingFilter.cpp` (309 lines, preflight). Logical phases: **(a) preflight validation**, **(b) output-geometry construction**, **(c) mapping + copy**.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | `new_z_spacing <= 0` → error `-5555` | `Invalid Parameters` / "Non-positive Z spacing" (code asserted) |
| 2  | (a) | input file cannot be opened → error `-5556` | *Not directly tested. File existence is validated by the `FileSystemPathParameter` before preflight; this is a redundant safety net reachable only by a filesystem race.* |
| 3  | (a) | file has fewer than `ZPoints + 1` parseable values → error `-5557` | `Invalid Parameters` / "File with too few values" (code asserted) |
| 4  | (a) | Z boundary values not monotonically non-decreasing → error `-5558` | `Invalid Parameters` / "Non-monotonic Z boundary values" (code asserted) |
| 5  | (a) | total Z extent (last value) `<= 0` → error `-5559` | `Invalid Parameters` / "Zero total Z extent" (code asserted) |
| 6  | (b) | geometry has no cell AttributeMatrix → error `-5560` | `Invalid Parameters` / "Missing cell Attribute Matrix" (code asserted) |
| 7  | (b) | cell AM member is not a DataArray → error `-5561` | `Invalid Parameters` / "Non-DataArray member in cell Attribute Matrix" (code asserted) |
| 8  | (b) | `remove_original_geometry == true` → rename src, create temp geom, deferred delete + rename | `Valid Execution (In Place)` |
| 9  | (b) | `remove_original_geometry == false` → create new geometry | `Valid Execution (New Geometry)` |
| 10 | (b) | copy loose child data objects from source geometry (component-wise path rebase) | `Valid Execution (New Geometry)` — `LooseData` presence + values asserted |
| 11 | (c) | `newZ == 0` → clamp to 1 (`ComputeRegularizedZDim`) | `Valid Execution (Spacing Exceeds Extent)` — Class 1 expected output asserted |
| 12 | (c) | build `newToOldZPlane` mapping (nearest-below, strict `>`) | `Valid Execution (New Geometry)` — verified against oracle map `[0,1,2,2,3]` |
| 13 | (c) | per-array bulk `copyFrom` of each plane slab | all three valid fixtures + legacy A/B (int32/bool/float3) |
| 14 | (c) | cancel check (per-array, per-plane) | *Not directly tested. Requires cancel-signal injection; branch is a simple early return.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Valid Execution (New Geometry)` | new-for-V&V | Class 1 oracle: asserts output Z dim (5), preserved X/Y dims + spacing, preserved origin, Z spacing = 2.0, the 10-tuple expected `Data` array element-wise, loose-child (`LooseData`) copy with values, and source-geometry retention. |
| `Valid Execution (In Place)` | new-for-V&V | Asserts in-place result replaces the source geometry (same name), no output-name geometry created, output Z dim + Z spacing correct. |
| `Valid Execution (Spacing Exceeds Extent)` | new-for-V&V | Class 1 clamp fixture: `newZRes` (20) > extent (10) → 1 output plane sourced from plane 0; dims (2,1,1), Z spacing 20, `Data` = {0,1} asserted. |
| `Invalid Parameters` | new-for-V&V | 6 SECTIONs — non-positive spacing (`-5555`), too-few file values (`-5557`), non-monotonic values (`-5558`), zero total extent (`-5559`), missing cell AM (`-5560`), non-DataArray cell member (`-5561`); each asserts the specific error code. |
| `SIMPL Backwards Compatibility` | new-for-V&V | 2 DYNAMIC_SECTIONs — converts the 6.5 (UUID-keyed) and 6.4 (Filter_Name-keyed) fixtures in `test/simpl_conversion/`; asserts converted geometry path, input file, Z spacing, and the in-place default. The 6.4 path exercises the `k_LegacySimplFilterUuidMap` name resolution. |

## Exemplar archive

- **Archive:** None — inline analytical fixtures.
- **SHA512:** N/A (no `download_test_data`).
- **Provenance:** N/A — Class 1/4 oracles live in the test source directly; no exemplar archive to provenance.

## Deviations from DREAM3D 6.5.171

No deviations observed. Comparison run on a synthetic Image Geometry (dims 3×2×4; int32 `Data`, `DataArray<bool>` `Mask`, 3-component float `Vec`) authored in legacy v7 format so both runners read identical input; Z-positions `{0,1,3,6,10}`, `new_z_spacing = 2.0`. Legacy 6.5.171 output, SIMPLNX output, and the Class-1 oracle are bit-identical on every cell array and on geometry (dimensions, spacing, origin). A conversion-semantics scope note (cell-AM binding, delta 5) is recorded in `vv/deviations/RegularizeZSpacingFilter.md`.
