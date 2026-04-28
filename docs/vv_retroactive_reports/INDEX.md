# MTR Retroactive V&V Reports — Index

*Status:* **All 22 Tier-1 filter reports are DRAFT.** Tentative Algorithm Relationship and Oracle classifications need developer confirmation. Material PR counts and bug flags are derived from git history and source-tree inspection — they are observations, not verified claims.

This index is the entry point for the SBIR MTR retroactive audit covering filters with PRs since 2025-10-01. Each row links to the filter-specific report. The audit follows the policy in `.claude/mtr_filter_verification_validation.md` (gitignored) and used `CAxisSegmentFeaturesFilter.md` as the structural template-of-record.

## At a glance

| Metric | Count |
|---|---|
| Tier-1 filters audited | 22 (+ 1 sample) |
| Total report lines | ~4,800 |
| Suspected real bugs surfaced | **8** across **5 filters** |
| Confirmed legacy 6.5 defects (per PR descriptions) | 2 (PR #1499 iteration guard, PR #1569 SSA) |
| Filters with thin coverage (1 happy-path test) | 6 |
| Filters with circular / NX-self-generated oracles | 5 |
| Algorithm Relationships: Port / Rewrite / Other | 21 / 1 / 0 |
| Filters delegating math to EbsdLib | 11 |

## Master table

Plugins: **SC** = SimplnxCore, **OA** = OrientationAnalysis, **ITK** = ITKImageProcessing.

The "Bugs" column flags filters where the audit surfaced suspected real bugs in source (not just deviation candidates vs. legacy). See [Suspected real bugs](#suspected-real-bugs-surfaced) below.

### Template-of-record

| Filter | Plugin | Algorithm Relationship (tentative) | Oracle class (tentative) | Material PRs | Bugs |
|---|---|---|---|---|---|
| [CAxisSegmentFeaturesFilter](CAxisSegmentFeaturesFilter.md) | OA | Port | 4 (+ optional 3) | 6 | — |

### Batch A — Counting / centroid / phase / copy / sample

| Filter | Plugin | Algorithm Relationship (tentative) | Oracle class (tentative) | Material PRs | Bugs |
|---|---|---|---|---|---|
| [ComputeFeatureCentroidsFilter](ComputeFeatureCentroidsFilter.md) | SC | Port + small additions | 1 | 3 | — |
| [ComputeFeatureSizesFilter](ComputeFeatureSizesFilter.md) | SC | Port | 1 | 6 | — |
| [ComputeFeaturePhasesFilter](ComputeFeaturePhasesFilter.md) | SC | Port | 1 + 4 | 4 | — |
| [CopyFeatureArrayToElementArrayFilter](CopyFeatureArrayToElementArrayFilter.md) | SC | Port | 1 + 4 | 4 | — |
| [IdentifySampleFilter](IdentifySampleFilter.md) | SC | Port + capability additions | 4 (+ extended 1) | 7 | — |

### Batch B — Rotations + IPF + orientation conversion + threshold

| Filter | Plugin | Algorithm Relationship (tentative) | Oracle class (tentative) | Material PRs | Bugs |
|---|---|---|---|---|---|
| [RotateSampleRefFrameFilter](RotateSampleRefFrameFilter.md) | SC | Port | 1 (axis-aligned) + 4 (arbitrary) | 6 | — |
| [RotateEulerRefFrameFilter](RotateEulerRefFrameFilter.md) | OA | Port | 3 (+ 1/4 companions) | 3 | — |
| [ComputeIPFColorsFilter](ComputeIPFColorsFilter.md) | OA | Port | 3 + 4 | 4 | — |
| [ConvertOrientationsFilter](ConvertOrientationsFilter.md) | OA | Port | 3 + 4 + 1 | 5 | — |
| [MultiThresholdObjectsFilter](MultiThresholdObjectsFilter.md) | SC | Port (dual-source: v1+v2 consolidated) | 1 + 4 | 8 | **`std::reverse` vs NOT** |

### Batch C — Orientation / C-axis math (all OrientationAnalysis)

| Filter | Plugin | Algorithm Relationship (tentative) | Oracle class (tentative) | Material PRs | Bugs |
|---|---|---|---|---|---|
| [ComputeAvgOrientationsFilter](ComputeAvgOrientationsFilter.md) | OA | Port + new methods (vMF, Watson) | per-method: 3+4 (Rodrigues), 3+2 (vMF/Watson) | 5 | — |
| [ComputeAvgCAxesFilter](ComputeAvgCAxesFilter.md) | OA | Port w/ 3 silent changes | 3 + 4 + 1 | 6 | — |
| [ComputeCAxisLocationsFilter](ComputeCAxisLocationsFilter.md) | OA | Port | 3 + 4 + 1 | 4 | — |
| [ComputeFeatureReferenceCAxisMisorientationsFilter](ComputeFeatureReferenceCAxisMisorientationsFilter.md) | OA | Port | 3 + 4 + 1 | 5 | — |
| [ComputeFeatureNeighborMisorientationsFilter](ComputeFeatureNeighborMisorientationsFilter.md) | OA | Port | 3 + 4 + 1 | 5 | **Divisor reassigned inside inner loop** |

### Batch D — C-axis misalignments + neighbor work

| Filter | Plugin | Algorithm Relationship (tentative) | Oracle class (tentative) | Material PRs | Bugs |
|---|---|---|---|---|---|
| [ComputeFeatureNeighborCAxisMisalignmentsFilter](ComputeFeatureNeighborCAxisMisalignmentsFilter.md) | OA | Port | 3 + 4 | 6 | **Divisor reassigned inside inner loop (copy-paste of Batch C bug)** |
| [ComputeFeatureNeighborsFilter](ComputeFeatureNeighborsFilter.md) | SC | Port (with explicit legacy bug fix) | 1 + 4 | 6 | — |
| [BadDataNeighborOrientationCheckFilter](BadDataNeighborOrientationCheckFilter.md) | OA | Port (with explicit legacy bug fix) | 4 + 3 (+ 1 inline) | 11 | — |
| [NeighborOrientationCorrelationFilter](NeighborOrientationCorrelationFilter.md) | OA | Port | 4 + 3 (+ 1 spot-check) | 6 | — |

### Batch E — I/O + remaining

| Filter | Plugin | Algorithm Relationship (tentative) | Oracle class (tentative) | Material PRs | Bugs |
|---|---|---|---|---|---|
| [FillBadDataFilter](FillBadDataFilter.md) | SC | **Rewrite** (PR #1515 AI-generated, never formally reviewed) | 4 (+ Test11 → 1) | 5 | **int32 overflow at 2.1B voxels** + **potential infinite loop** |
| [ReadAngDataFilter](ReadAngDataFilter.md) | OA | Port | 5 (de-facto, after PR #1462 oracle replacement) — recommend re-baseline to 2 + 4 | 7 | — |
| [ITKImageWriterFilter](ITKImageWriterFilter.md) | ITK | Port | 2 + 4 | 8 | **OOC `dynamic_cast` break (no preflight gate)** |
| [WritePoleFigureFilter](WritePoleFigureFilter.md) | OA | Port (with two compounding EbsdLib bumps) | 3 + 2 + 4 | 6 | — |
| [ReplaceElementAttributesWithNeighborValuesFilter](ReplaceElementAttributesWithNeighborValuesFilter.md) | SC | Port | 1 + 4 | 4 | **`bestNeighbor` stale state** + **`float32 best` truncation** |
| [ComputeGroupingDensityFilter](ComputeGroupingDensityFilter.md) | SC | Port (renamed `FindGroupingDensity` → `Compute…`) | 1 (gold standard inline) | 2 | — |

## Suspected real bugs surfaced

8 distinct bugs in 5 filters. Full triage details and exact line/file references are tracked in the policy document under "SUSPECTED REAL BUGS surfaced during retroactive audit". This list is the public-report summary.

| # | Filter | Bug | Severity | Production-relevant? |
|---|---|---|---|---|
| 1 | MultiThresholdObjectsFilter | `std::reverse(tempResultVector)` used in `replaceInput && inverse` branch where boolean NOT was the intended operation | High | Possibly — depends on whether any shipping pipeline uses both flags together |
| 2a | ComputeFeatureNeighborMisorientationsFilter | `tempMisoList = featureNeighborList.size()` reassigned inside inner j-loop, divisor reflects only last neighbor's match state | High | The `ComputeAvgMisors=true` test is `[.][UNIMPLEMENTED][!mayfail]` — zero CI coverage |
| 2b | ComputeFeatureNeighborCAxisMisalignmentsFilter | Same shape: `hexNeighborListSize` reassigned line 111 clobbers `--` line 150 — copy-paste sibling | High | **Yes** — `EBSD_Hexagonal_Data_Analysis.d3dpipeline` ships with `find_avg_misals: true`. Existing test exemplar is hex-only, cannot trigger the bug |
| 3a | FillBadDataFilter | `static_cast<int32>(size) < threshold` wraps for components > 2.1B voxels — mis-classifies huge bad regions as small | High | Yes for large datasets |
| 3b | FillBadDataFilter | Phase 4 `while(count != 0)` has no no-progress termination — potential infinite loop | Medium | Yes, on pathological input |
| 4a | ReplaceElementAttributesWithNeighborValuesFilter | `bestNeighbor` vector allocated once before `while(keepGoing)` loop and never reset — voxel replaced in iteration N can be silently re-replaced in N+1 from stale state | High | Yes, on multi-iteration cleanup |
| 4b | ReplaceElementAttributesWithNeighborValuesFilter | `float32 best` is used regardless of templated type `T` — truncates int64/uint64/float64 values | Medium | Yes for non-float arrays |
| 5 | ITKImageWriterFilter | PR #1555 introduced `dynamic_cast<DataStore<T>>` with no preflight gate — runtime `bad_cast` for OOC-stored arrays instead of a clean preflight error | Lower | Yes if anyone writes OOC data |

The two-filter divisor pattern (#2a / #2b) is a confirmed copy-paste origin. Recommend a screening grep for any other `*NeighborList::size()` reassignment-inside-inner-loop usage.

## Cross-cutting findings

These showed up in multiple reports and are likely worth amending into the policy doc:

1. **PR #1438 ("ENH: Microtexture related filter cleanup") is the largest hidden-deviation hotspot in the audit.** Its "ENH" label is misleading — for at least 4 filters in Batches B+C it silently changed math, error codes, or precision. The policy should require per-filter inspection of #1438 wherever it touched the file.
2. **PR #1472 (EbsdLib 2.0.0 bump) is heterogeneous** — for some filters it's pure namespace renames (`ConvertOrientations` initially looked like a rewrite but `ComputeAvgOrientations` and `ReadAng` are pure renames), for others it's a wholesale rewrite (`ConvertOrientations` itself, `ComputeIPFColors` semantic suspect). Always inspect the scoped diff before pruning.
3. **The model V&V pattern is now visible** — hand-derived `expected*` arrays inline in test source, dozens of small fixtures, no exemplar archive dependency for correctness. Filters with this pattern (`BadDataNeighbor` PR #1499, `ComputeFeatureNeighbors` PR #1569, `ComputeFeatureSizes` PR #1540, `ComputeGroupingDensity`) are measurably stronger than filters with one happy-path exemplar test.
4. **The "circular oracle" pattern is real and pervasive.** At least 5 filters have replaced or supplemented their legacy oracle with NX-self-generated exemplars (`6_5_fill_bad_data.tar.gz`, `read_ang_test.tar.gz`, the v5 pole-figure archive, `7_2_AvgCAxis.tar.gz`, ComputeAvgOrientations v2). Each effectively shifts the de-facto oracle to Class 5 (regression / golden-master). Recommend the policy require an explicit Oracle Provenance ReadMe entry for any such archive.
5. **Two audit-confirmed legacy 6.5 defects** (PR #1499 iteration guard, PR #1569 Shared Surface Area) — the first instances where legacy is provably wrong rather than SIMPLNX merely diverging. These are clean Deviation entries to ship in the migration guide.
6. **Shared archive `6_6_stats_test_v2.tar.gz` contagion** — used by Centroids/Sizes/Phases/Neighbors. Its SSA values are now confirmed bad-from-legacy. Tests that don't exercise SSA are still safe, but consumers should know.
7. **AI-generated rewrite without formal review (PR #1515 → FillBadData) is a V&V red flag.** Two real bugs were surfaced in this rewrite during the audit. The policy should explicitly require independent algorithm review + independent oracle for any AI-generated material change, not just "passes existing tests".

## Next steps (per resumption protocol step 7+)

1. **Bug triage** (Task #19) — for each of the 8 suspected bugs: independent code review, write a targeted unit test that *fails* on current `develop`, then fix. Coordinate the `ReplaceElementAttributesWithNeighborValues` work with the existing remote branches `joey/ooc-filter-optimizations` and `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation`.
2. **Skill design** — extract patterns from the 22 reports and design the `mtr-retroactive-report` skill so any of the ~290 simplnx filters can run through this flow. Pattern data is now sufficient.
3. **SimplnxReview-plugin reports** (deferred, separate repo) — `GroupMicroTextureRegionsFilter` and `MergeColoniesFilter`.
4. **Policy amendments** — incorporate the 7 cross-cutting findings above into `.claude/mtr_filter_verification_validation.md`.
