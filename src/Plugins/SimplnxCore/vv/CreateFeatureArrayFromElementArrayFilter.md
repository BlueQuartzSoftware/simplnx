# V&V Report: CreateFeatureArrayFromElementArrayFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| SIMPLNX Human Name | Create Feature Array from Element Array |
| DREAM3D 6.5.171 equivalent | `CreateFeatureArrayFromElementArray` — SIMPL UUID `94438019-21bb-5b61-a7c3-66974b9a34dc` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *pending second-engineer review* |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Minor changes** — same copy semantics as SIMPL `CreateFeatureArrayFromElementArray` (first-instance comparison, once-only -1000 warning, last-write-wins, gap ids keep fill value 0), with the legacy AM-size errors (-5555/-5556) deliberately replaced by an auto-resize, three new safety validations, and a performance-restructured kernel that preserves bit-identical output. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — hand-derived expected feature arrays on inline `BuildAnalyticalFixture` toy data, with **Class 4 (Invariant)** companions (tuple count == max(FeatureIds)+1; gap ids keep fill value; warning-count semantics). Encoded in `test/CreateFeatureArrayFromElementArrayTest.cpp`; all fixtures pass. |
| Code paths enumerated  | 16 of 17 exercised; the cancel check is untestable (no cancel-signal injection). |
| Tests today            | 5 TEST_CASEs — analytical oracle (6 SECTIONs), 11-type dispatch sweep, 3 error conditions, large (100³/5000-feature, OOC-preferenced) analytical, SIMPL backwards-compat (2 DYNAMIC_SECTIONs). Dual-build pass: 5/5 in-core + 5/5 OOC. |
| Exemplar archive       | None needed — all fixtures are inline analytical data. The prior use of `6_5_test_data_1_v2.tar.gz` (Small IN100) in this test was retired as a legacy/circular oracle; the archive itself remains for other filters' tests. |
| Legacy comparison      | **Run (2026-07-23, re-run post-review-rewrite)** on the toy fixture (int32 1-comp, float32 3-comp, inconsistent-values case): **bit-identical**, both outputs equal the Class 1 oracle, identical once-only -1000 warning. Oversized-AM fixture confirmed legacy errors -5556 where SIMPLNX resizes (D1). No surgical legacy patch needed. |
| Bug flags              | `-D2`, `-D3`, `-D4` — legacy 6.5.171 undefined behavior (negative FeatureIds OOB write; mismatched tuple-count OOB read) and a spurious NaN warning. SIMPLNX errors/handles all three deterministically. A pre-V&V SIMPLNX defect (self-referential destination AM corrupting inputs) was found by adversarial review and fixed with preflight error -5572. |
| V&V phase              | All phases complete, including a five-perspective specialist review battery (adversarial, senior-engineer, CPU, memory, out-of-core) whose accepted findings are implemented and covered by tests. Outstanding: second-engineer review (= PR review). |

## Summary

`CreateFeatureArrayFromElementArrayFilter` promotes an Element/Cell-level array to the Feature level: for each cell it writes the cell's tuple into the output at index FeatureId (last cell of each feature wins) and resizes the destination Feature AttributeMatrix to max(FeatureIds)+1. Verification used a Class 1 analytical oracle (hand-derived expected arrays on inline fixtures covering gap ids, feature 0, multi-component tuples, NaN handling, and warning semantics) plus Class 4 invariants, then an A/B diff against DREAM3D 6.5.171 that was bit-identical on the happy path. Four deviations are documented (one deliberate usability change, three legacy bugs), and a specialist review battery drove one critical SIMPLNX fix (self-referential AM guard) plus a semantics-preserving performance restructuring of the kernel.

## Algorithm Relationship

*Classification:* **Minor changes**

Same copy semantics as SIMPL `CreateFeatureArrayFromElementArray` (SIMPL UUID `94438019-21bb-5b61-a7c3-66974b9a34dc`, mapped in `SimplnxCoreLegacyUUIDMapping.hpp`): cells visited in index order, per-feature first-instance consistency check, at most one -1000 warning, last cell of each feature provides its value, gap feature ids keep the zero fill value. Verified bit-identical to 6.5.171 on the A/B fixture.

*Port-time deltas:*

1. **AM sizing policy (behavioral — D1):** legacy validated the pre-existing Feature AM tuple count and errored (-5555/-5556); SIMPLNX resizes the AM (and every array in it) to max(FeatureIds)+1 at execute time, warning (-5573) when this truncates existing data.
2. **Negative FeatureIds guard (behavioral — D2):** error -5570 replaces undefined behavior present in both legacy and pre-V&V SIMPLNX.
3. **Tuple-count preflight check (behavioral — D3):** error -5571 replaces a potential out-of-bounds read present in both legacy and pre-V&V SIMPLNX.
4. **NaN consistency (behavioral — D4):** two NaNs are treated as consistent, so NaN-padded features no longer consume the single warning (legacy warns spuriously). Output unchanged.
5. **Self-referential AM guard (SIMPLNX fix, no legacy analogue):** preflight error -5572 when the destination AM contains the input arrays — in pre-V&V SIMPLNX the execute-time resize silently corrupted the inputs (adversarial-review finding); legacy usually errored via its size validation instead.
6. **Sparse-id warning:** warning -5574 when max(FeatureIds) exceeds the cell count, flagging suspicious ids before the allocation they drive.
7. **Kernel restructuring (performance, output-identical):** dense `seenFeature`/`firstValues` vectors replace the legacy-style per-cell map lookups and the random read-back into the cell array; output is staged in a feature-level buffer and written to the store once, sequentially. Cells are still visited in increasing index order, so last-write-wins and the warning's first-mismatch attribution are unchanged (A/B re-run confirmed bit-identical). On cancel, the output array now holds its fill values instead of a partial copy (cancelled results are discarded by the pipeline).
8. **Type dispatch** — `ExecuteDataFunction` functor vs legacy's `CanDynamicCast` if/else chain over the same 11 primitive types. No output change.
9. **Empty-name validation** — legacy dataCheck error -11002 is covered by `DataObjectNameParameter` validation at the framework level.
10. **Progress reporting** — throttled progress messages added (legacy had none). UX only.

*Material PRs since baseline:* #1301 / #1521 / #1544 (Algorithm-class extraction refactors, no behavioral change identified).

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. Classes 2, 3, 5 N/A — the operation is a pure indirection copy with no external library equivalent worth pinning and no published algorithm.

*Applied:* Expected feature arrays are hand-derived in comments beside each fixture. Canonical fixture: FeatureIds `{1,3,1,4,3,1,0,4}` with element values `{10,30,10,40,30,10,5,40}` → expected `{5,10,0,30,40}` (feature 0 from cell 6; gap feature 2 keeps fill value 0; max id 4 grows the AM from 1 to 5 tuples). Companion fixtures pin the once-only -1000 warning with last-value-wins, multi-component copies, NaN handling, AM shrink and sparse-id growth. A 100³/5000-feature fixture uses the closed form `element = featureId*0.5+0.25` so `feature[f] = f*0.5+0.25` exactly.

*Invariants (Class 4):* output tuple count == max(FeatureIds)+1 regardless of the AM's starting size (grow and shrink both pinned); gap ids keep the fill value; consistent data produces zero -1000 warnings; inconsistent data produces exactly one; truncation and sparse ids each produce exactly one warning of their code.

*Encoded:* `src/Plugins/SimplnxCore/test/CreateFeatureArrayFromElementArrayTest.cpp` — TEST_CASEs `Analytical oracle` (6 SECTIONs), `All DataTypes dispatch` (11 DYNAMIC_SECTIONs), `Large analytical`. All pass at the verified commit in both builds.

*Second-engineer review:* pending (this report is the request); the PR review constitutes the sign-off.

## Code path coverage

*16 of 17 paths exercised; the cancel check is the one untestable gap.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CreateFeatureArrayFromElementArray.cpp` (~200 lines). Logical phases: (a) filter preflight, (b) scan/validate/resize in `operator()`, (c) per-cell copy in `CopyCellDataFunctor`, (d) staged write-back.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) Preflight | FeatureIds vs element array tuple-count mismatch → error -5571 | `Error conditions` — "mismatched ... tuple counts fail preflight" |
| 2  | (a) Preflight | Destination AM contains the input arrays → error -5572 | `Error conditions` — "destination Feature AttributeMatrix containing the inputs fails preflight" |
| 3  | (a) Preflight | Created array takes the AM's current tuple shape + fill value "0" | All positive TEST_CASEs (gap-id checks read the fill value) |
| 4  | (b) Scan | Min/max FeatureId scan over all tuples | All positive TEST_CASEs |
| 5  | (b) Scan | Negative FeatureId → error -5570 | `Error conditions` — "negative feature ids fail execution" |
| 6  | (b) Validate | max(FeatureIds) > cell count → warning -5574, proceed | `Analytical oracle` — "sparse feature ids grow ... and warn" |
| 7  | (b) Resize | Shrink to max(FeatureIds)+1 → warning -5573, all AM arrays truncated | `Analytical oracle` — "oversized Feature AttributeMatrix is resized down ... with a warning" (10 → 3 tuples) |
| 8  | (b) Resize | Grow to max(FeatureIds)+1 (no warning) | `Analytical oracle` — "single component..." (1 → 5 tuples) |
| 9  | (c) Copy | Cancel check every 1024 cells | *Not directly tested — requires cancel-signal injection (excluded by scope).* |
| 10 | (c) Copy | Output stage initialized from post-resize store (gap ids keep fill value) | `Analytical oracle` — "single component..." (gap feature 2 == 0); sparse SECTION (ids 0-4 == 0) |
| 11 | (c) Copy | First cell of a feature → record firstValues, stage tuple | All positive TEST_CASEs |
| 12 | (c) Copy | Repeat cell, consistent → no warning, stage tuple | `Analytical oracle` — "single component..." (0 warnings asserted) |
| 13 | (c) Copy | Repeat cell, inconsistent → single -1000 warning, last value wins; further inconsistencies suppressed | `Analytical oracle` — "inconsistent feature values warn exactly once" (features 1 AND 2 inconsistent, warning count == 1) |
| 14 | (c) Copy | NaN vs NaN treated as consistent (float types) | `Analytical oracle` — "NaN element values do not consume the consistency warning" |
| 15 | (c) Copy | Multi-component tuple copy | `Analytical oracle` — "multi component tuples copy component-wise" (3-comp uint8) |
| 16 | (c) Copy | Type dispatch over all 11 DataTypes | `All DataTypes dispatch` — one DYNAMIC_SECTION per type incl. boolean |
| 17 | (d) Write-back | Sequential staged write into the created store | All positive TEST_CASEs (every expected-value check reads the store after write-back) |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Analytical oracle` (6 SECTIONs) | new-for-V&V | Class 1 + 4; element-wise REQUIREs with hand-derivation comments; warning-count assertions for -1000/-5573/-5574; AM grow + shrink + sparse growth + NaN handling pinned. |
| `All DataTypes dispatch` (11 DYNAMIC_SECTIONs) | new-for-V&V | Class 1 per type; {0,2,4} expected values; exercises boolean store. |
| `Error conditions` (3 SECTIONs) | new-for-V&V | Pins -5570 (negative ids, execute), -5571 (tuple mismatch, preflight), -5572 (self-referential AM, preflight). |
| `Large analytical` | new-for-V&V | 100³ cells / 5000 features, closed-form expected values, OOC-preferenced stores (`PreferencesSentinel("Zarr", 1 slab)`); doubles as the OOC/perf benchmark. |
| `SIMPL Backwards Compatibility` (2 DYNAMIC_SECTIONs) | kept | UUID + argument round-trip via `Pipeline::FromSIMPLFile` for 6.4 and 6.5 fixture JSONs. Not an oracle test. |
| `Valid filter execution - 1 Component` | retired | Compared against `CellFeatureData/Confidence Index` inside the legacy-generated Small IN100 file — a legacy/circular oracle, banned under v2 policy. |
| `Valid filter execution - 3 Component` | retired | Same circular-oracle pattern (IPFColors). |

## Exemplar archive

- **Archive:** None. All fixtures are inline analytical data generated in the test file; no `download_test_data()` entry is needed for this filter.
- The retired tests consumed the shared `6_5_test_data_1_v2.tar.gz` (Small IN100) archive, which remains in `test/CMakeLists.txt` for the other four test files that still use it.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/CreateFeatureArrayFromElementArray_toy_ab.md` (A/B comparison input provenance; not a unit-test archive).

## Specialist review battery (2026-07-23)

Five independent reviews were run after the oracle/tests/A-B phases: adversarial, nit-picky senior engineer, CPU performance, memory performance, and out-of-core. Accepted findings, all implemented and re-verified (dual-build 5/5 + A/B re-run bit-identical):

- **Adversarial (critical):** self-referential destination AM corrupted inputs mid-execute → preflight error -5572 + test. NaN spuriously consumed the warning → NaN-consistent check (D4) + test. Silent truncation → warning -5573 + test. Suspicious sparse ids driving huge allocations → warning -5574 + test.
- **CPU/Memory/OOC (convergent):** `std::map` first-instance bookkeeping (≈64 B/feature, 2-3 tree walks/cell) and the per-cell random read-back into the cell array replaced with dense feature-level `seenFeature`/`firstValues` vectors; output staged and written back once sequentially (per-cell scattered store writes eliminated); cancel/progress checks batched to every 1024 cells. All O(features) memory, on par with the created array itself.
- **Senior engineer:** redundant created-array resize removed (AM resize covers it); parameter descriptions corrected; header `@brief` stubs replaced; include hygiene; `Ptr`/`Ref` suffix conventions; `CheckArraysInheritTupleDims` added to error tests. Deliberately rejected: rewording the -1000 warning text (kept identical to 6.5.171 for A/B parity).
- **Known deferred (framework-level):** `resizeTuples` performs no memory-requirement check (an absurd max FeatureId can still OOM after the -5574 warning); `IFilter::execute` does not catch `bad_alloc`. Both belong to the core library, not this filter.

## Deviations from DREAM3D 6.5.171

Comparison run on the toy A/B fixture (`input_toy.dream3d`: 2×2×2 ImageGeom; int32 1-comp, float32 3-comp, and inconsistent-values cases) — see `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md`.

- `CreateFeatureArrayFromElementArrayFilter-D1` — legacy errors (-5555/-5556) on a mis-sized Feature AM; SIMPLNX resizes it instead, warning on truncation (algorithmic choice).
- `CreateFeatureArrayFromElementArrayFilter-D2` — negative FeatureIds: legacy undefined behavior (OOB write); SIMPLNX errors -5570 (bug in 6.5.171).
- `CreateFeatureArrayFromElementArrayFilter-D3` — mismatched tuple counts: legacy undefined behavior (OOB read); SIMPLNX fails preflight -5571 (bug in 6.5.171).
- `CreateFeatureArrayFromElementArrayFilter-D4` — all-NaN features: legacy emits a spurious -1000 warning that can mask real inconsistencies; SIMPLNX treats NaN==NaN as consistent (bug in 6.5.171, cosmetic; outputs identical).

On the correctly-sized happy path, 6.5.171 and SIMPLNX outputs are bit-identical and both match the Class 1 oracle values exactly (verified 2026-07-23 with `compare_ab.py`, re-verified after the review-driven kernel restructuring; both versions emitted the identical single -1000 warning on the inconsistent-values case).
