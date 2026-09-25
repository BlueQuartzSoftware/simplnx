The independent V&V Catch2 assertions remain preserved. Sections 4.2 and 4.3 are closed: all 40 audited filters now have representative, output-sensitive real-HDF5 boundary evidence. Further semantic-test and report-review items are recorded separately below.

## Initial audit results (historical)

- Full in-core build: passed.
- Full OOC build: passed.
- In-core CTest: 503/503 passed in 63.82 seconds.
- OOC CTest: 530/531 passed in 286.30 seconds.
- Hidden real-OOC contracts passed:
    - FeatureSizes: 3 cases / 129 assertions
    - CopyFeatureArray and MultiThreshold store tests: 5 cases / 368 assertions
    - GroupMicroTextureRegions 200³: 642 assertions
    - CreateFeatureArrayFromElementArray 200³: 3,495 assertions
    - MultiThresholdObjects 200³: 11 assertions

The initial audit changed no files. The subsequent Section 1 remediation is retained in the current branch. The Section 4.2 tests, reports, and CTest registration are local working-tree changes.

## Five-gate assessment

1. Oracle preservation [FIXED]

All 40 filters retain their independent-oracle assertions. No oracle tolerance using Approx().margin() or epsilon() was removed or relaxed.

Two reorganizations were confirmed rather than counted as losses:

- [X] MultiThresholdObjects’ V&V cases moved verbatim into MultiThresholdObjectsVvTest.cpp, gaining dispatch witnesses.
- [X] NeighborOrientationCorrelation’s Small IN100 Class-4 test is restored to the public dual-build suite, and its duplicate private implementation is removed. The latest focused runs pass in-core in 4.41 seconds and with explicitly asserted HDF5-OOC storage in 202.30 seconds. The complete selections pass 19/19 in-core and 20/20 OOC; the OOC selection retains the separate 200³ boundary case.

2. Dual-build runtime [FIXED]

The initial in-core suite was clean. The initial OOC suite had one deterministic failure, since corrected by reapplying the guard fix:

- /Users/mjackson/Workspace5/SimplnxOoc/test/PluginTests/OrientationAnalysis/ComputeAvgOrientationsOocTest.cpp:1083 segfaults in the “in-range
  feature without a selected operation” section.

- Isolated reproduction exits 139, with 22/23 assertions passing before SIGSEGV.
- src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgOrientations.cpp:1182 dereferences featureOps[feature]
  before the intended -54677 validation.

- Local fix commit 04292a66a moves that guard ahead of the dereference. It was reapplied as 180c2cf05; the focused selections subsequently passed 15/15 in-core and 22/22 OOC.

This was an observable crash regression, not an oracle-output mismatch or flaky test.

3. Material algorithm implementations [PASS]

Twelve filters have distinct Direct/Scanline, BFS/CCL, Worklist/Scanline, or manual in-core/OOC implementations. All twelve execute an oracle-
backed test through runtime dispatch witnesses:

BadDataNeighborOrientationCheck, ComputeAvgOrientations, ComputeIPFColors, ComputeKernelAvgMisorientations, ComputeFeatureNeighbors,
ComputeFeatureSizes, CopyFeatureArrayToElementArray, DBSCAN, FillBadData, IdentifySample, MultiThresholdObjects, and RotateSampleRefFrame.

The normal oracle comparisons pass for both implementations. The ComputeAvgOrientations guard crash above is fixed.

4. Real-HDF5 boundary evidence

4.1: Confirmed meaningful boundary/full-scale evidence exists for 40 of 40: [PASS]

- ComputeAvgCAxes — 4,097-cell real-HDF5 fixture with a 4,096-tuple block plus one-cell tail and literal +30° output.
- BadDataNeighborOrientationCheck — real-HDF5 Bool/UInt8 masks propagate backward across five 3×3 slices; exact output masks and Scanline dispatch are asserted.
- ComputeFeatureCentroids — real-HDF5 block-plus-tail contributions yield exact physical centroids with non-unit spacing and origin; periodic off/on pass.
- ComputeFeatureNeighbors — existing 5×5×5 analytical fixture runs on HDF5 FeatureIds and all five OOC outputs across five 25-cell slices.
- ComputeGroupingDensity — HDF5 neighbor-list chunks straddle features 2/3; all four option combinations retain exact union-volume ratios and feature claims.
- ComputeIPFColors — Bool/UInt8 HDF5 masks and exact black/red/green colors cross a 65,536-cell block and partial tail.
- ComputeNeighborhoods — HDF5 output list chunks straddle features 2/3; both radius modes retain exact directed neighbors and counts.
- ComputeSchmids — physical 65,536-quaternion chunk plus one-feature tail gives analytical factors 0.5/0.48 and distinct poles.
- ConvertOrientations — 4,096-quaternion block plus one-tuple tail gives stereographic [0,0,1/3] and [1/3,0,0].
- FillBadData — connected defect spans slices at the retention threshold; an isolated cell copies the correct feature, phase, and payload, with new-phase option off/on.
- ReadAngData — 65,537 input rows preserve exact Euler interleaving, scalar values, and phase-zero remapping into HDF5 output.
- ReadCtfData — 65,537 input rows preserve exact double-intermediate Euler conversions and unindexed phase zero across the block tail.
- RegularizeZSpacing — HDF5 plane copies implement the analytical [0,1,2,2,3] source-plane map in new-geometry and in-place modes.
- RotateEulerRefFrame — HDF5 full-block end and one-tuple tail preserve independent identity/wraparound Euler rotation values.
- RotateSampleRefFrame — full output permutation crosses a 1 MiB input page and partial second page in both whole-volume and slice modes.
- WriteDREAM3D — raw HDF5 readback preserves every uint32 value across a physical 1 MiB input chunk and tail, with compression off/on.
- WritePoleFigure — two orientations straddle the extraction block; exact discrete family counts and a full analytical <001> image verify both contributions.
- ComputeCAxisLocations — 65,537-cell real-HDF5 fixture with distinct literal outputs at tuple 65,535 and the one-cell tail.
- ComputeFaceIPFColoring — 65,537-face real-HDF5 fixture with distinct exact cubic-red and hex-green outputs at tuple 65,535 and the one-face tail.
- ComputeFeatureFaceMisorientations — 65,537-face real-HDF5 fixture with exact 15-degree and 30-degree outputs at tuple 65,535 and the one-face tail.
- ComputeFeatureNeighborCAxisMisalignments — 65,537 features; verified 65,536-tuple quaternion chunk shape and independent 15-degree/30-degree pair and average results across the tail.
- ComputeFeatureNeighborMisorientations — 65,537 features; verified 65,536-tuple quaternion chunk shape and independent cubic 15-degree/30-degree pair and average results across the tail.
- ComputeFeatureReferenceCAxisMisorientations — 17×19×4 real-HDF5 cells; contributors across 323-cell slice buffers verify cell angles, mean, and population standard deviation.
- ComputeFeatureReferenceMisorientations — both modes cross a 65,536-cell block plus one-cell tail; analytical angles, means, and tied-center coordinates pass.
- CopyFeatureArrayToElementArray — 65,537 real-HDF5 cells; literal three-component tuples verify the full-block end and one-cell tail.
- IdentifySample — 129×129×3 HDF5 mask creates over 8,000 labels, crosses 4,096-record equivalence pages and rolling slices, and checks the full expected mask with hole filling off/on.
- ReadH5OinaData — two 65,537-point scans; exact values in all nine HDF5 cell arrays verify batch tails, scan offsets, phase widening, and hexagonal alignment.
- DBSCAN — 65,538 real-HDF5 points; adjacent core grids span the 65,536-point batch boundary and two-point tail. The complete expected cluster-label array passes.
- CAxisSegmentFeatures
- ComputeAvgOrientations
- ComputeKernelAvgMisorientations
- GroupMicroTextureRegions
- NeighborOrientationCorrelation
- ComputeFeaturePhases
- ComputeFeatureSizes
- CreateFeatureArrayFromElementArray
- ErodeDilateBadData
- MultiThresholdObjects
- ReplaceElementAttributesWithNeighborValues
- RequireMinNumNeighbors

4.2: Closed for all 12 filters in this group (2026-09-18). ComputeAvgCAxes was the first case; all 11 subsequent cases now have an independent, output-sensitive HDF5 block, chunk, page, or slice-boundary regression. [PASS]

| Filter | In-core CTest | OOC CTest | New hidden boundary case |
|--------|---------------|-----------|--------------------------|
| ComputeAvgCAxes | 5/5 | 5/5 | 38 assertions |
| ComputeCAxisLocations | 7/7 | 7/7 | 25 assertions |
| ComputeFaceIPFColoring | 5/5 | 5/5 | 38 assertions |
| ComputeFeatureFaceMisorientations | 3/3 | 3/3 | 25 assertions |
| ComputeFeatureNeighborCAxisMisalignments | 7/7 | 7/7 | 37 assertions |
| ComputeFeatureNeighborMisorientations | 7/7 | 7/7 | 37 assertions |
| ComputeFeatureReferenceCAxisMisorientations | 8/8 | 8/8 | 34 assertions |
| ComputeFeatureReferenceMisorientations | 9/9 | 9/9 | 63 assertions; both reference modes |
| CopyFeatureArrayToElementArray | 24/24 | 24/24 | 22 assertions |
| IdentifySample | 5/5 | 6/6 | 99,875 assertions; hole filling off/on |
| ReadH5OinaData | 20/20 | 21/21 | 166 assertions; all nine imported cell arrays |
| DBSCAN | 12/12 | 19/19 | 65,555 assertions; ExternalGDCF runtime witness |

The per-filter counts exclude the hidden tests. The hidden tests require the HDF5-OOC manager and are included in the OOC-only `OrientationAnalysisOocStoreContracts` and `SimplnxCoreOocStoreContracts` CTest entries. The latter also contains three pre-existing contracts. Run them serially:

```sh
ctest --test-dir /Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk96-OoC-Rel -j 1 --output-on-failure -R '^(OrientationAnalysis|SimplnxCore)OocStoreContracts$'
```

The in-core build is `NX-Com-Qt69-Vtk96-Rel`. The OOC build is `NX-Com-Qt69-Vtk96-OoC-Rel`. Final combined CTest logs are under each build's `Testing/Section42-20260918.log`.

Final combined serial selections: **112/112 passed in-core (18.87 s)** and **123/123 passed OOC (162.31 s)**. The OOC total includes both contract groups, so all 12 new boundary tests are covered through CTest. The OrientationAnalysis group passed all nine cases in 144.61 s. Both test targets built successfully in both configurations.

No production filter algorithm changed in this campaign. Existing oracle values and tolerances remain intact. The reference-misorientation scaffold now bulk-initializes the same zero/identity values to keep the large HDF5 fixture inexpensive. Reports retain their historical statuses and sign-offs and use the report template's dashboard, coverage, inventory, and defect sections.

Final preservation check: all 91 upstream/develop test-case declarations in the 12 touched test files remain present. The removed result-macro lines in the branch diff are equivalent named-result wrappers; no numerical oracle assertion or tolerance was removed or relaxed. All 12 reports contain the template's eight dashboard rows and required main sections. Their status and sign-off rows match HEAD exactly. Pinned clang-format 19.1.1 and `git diff --check` pass.

Additional semantic gaps are recorded, without expanding this boundary campaign: the two feature-neighbor filters can reuse their smallest analytical pair fixtures with optional averaging disabled, check unchanged neighbor lists, and assert that no average array is created. These are separate from the now-closed Section 4.2 storage-boundary gaps.

4.3: Closed for all 16 filters (2026-09-21). The tests were handled in the listed order. Existing Section 4.2 working-tree changes are preserved.

| Filter | In-core CTest | OOC CTest | New hidden case assertions |
|--------|---------------|-----------|----------------------------|
| BadDataNeighborOrientationCheck | 33/33 | 33/33 | 139 (two mask types) |
| ComputeFeatureCentroids | 5/5 | 5/5 | 35 |
| ComputeFeatureNeighbors | 37/37 | 37/37 | 73 |
| ComputeGroupingDensity | 7/7 | 7/7 | 143 |
| ComputeIPFColors | 9/9 | 9/9 | 87 |
| ComputeNeighborhoods | 5/5 | 5/5 | 79 |
| ComputeSchmids | 6/6 | 6/6 | 37 |
| ConvertOrientations | 5/5 | 5/5 | 16 |
| FillBadData | 15/15 | 15/15 | 1,053 |
| ReadAngData | 8/8 | 8/8 | 36 |
| ReadCtfData | 12/12 | 12/12 | 38 |
| RegularizeZSpacing | 5/5 | 5/5 | 67 |
| RotateEulerRefFrame | 5/5 | 5/5 | 18 |
| RotateSampleRefFrame | 12/12 | 12/12 | 528,425 |
| WriteDREAM3D | 22/22 | 22/22 | 27 |
| WritePoleFigure | 4/4 | 4/4 | 29 |

The 16 additions instantiate 17 hidden cases because BadDataNeighborOrientationCheck covers both Bool and UInt8. They are included in the existing OOC-only store-contract groups. The final serial selection passes **190/190 in-core (30.69 s)** and **192/192 OOC (143.12 s)**; the OOC total includes both groups and therefore all new HDF5 cases. Logs are `Testing/Section43-20260921.log` in the two DREAM3DNX build trees.

Both test targets compile in both builds. All **186 test declarations from the original pre-OOC baseline** and all **192 declarations from the current local upstream/develop reference** in the 16 touched files remain present. No pre-existing REQUIRE/CHECK/Approx assertion was removed by this campaign. Historical DREAM3D reader-result checks moved into the checked UnitTest loader or separate pipeline/DataStructure Result checks; their numerical comparisons remain intact. The apparent IPF red-corner assertion change is comment-only. No production filter or algorithm changed. All 16 reports retain their exact historical status and sign-off rows.

Boundary evidence uses the implementation's relevant transfer size: numeric blocks and partial tails, rolling image slices, HDF5 NeighborList chunks, or bounded source-cache pages. These are correctness regressions, not cache-eviction performance benchmarks. New expected values come from existing independent fixtures or explicit analytical constructions, never from observed filter output. Existing legacy deviations are retained; no fresh legacy binary comparison is claimed.

There are **no remaining Section 4.3 storage gaps**. The two optional-average semantic regressions identified under Section 4.2 remain possible follow-ups.

5. Observable changes and deviations

- The initial audit removed or changed no V&V report or deviation sidecar. The Section 4.2 and 4.3 campaigns update 28 reports with current inventories and boundary evidence; sidecars are retained.
- No covered numerical output discrepancy was found.
- ComputeAvgOrientations' initial crash is corrected as described above.
- The reference-c-axis report now cites D2 for its precision deviation, matching the existing sidecar; its former D4 cross-reference was stale.
- Historical legacy comparison results were retained; this campaign does not claim fresh legacy binary runs. The DBSCAN report now distinguishes its local legacy proof build from a stock DREAM3D 6.5.171 comparison.

- MultiThresholdObjects needs a report inventory refresh for its relocated and renamed tests.

The initial audit used tip e6ca39e1c and baseline 286dfa90b. Section 4.2 used HEAD efb036625. Section 4.3 runs on develop at squashed HEAD 271a72192, whose committed tree is identical, plus local test/report changes. The original pre-OOC comparison baseline is 3e026500e7d8cf8e0debedd9a249ce887f0ad334. The current local upstream/develop reference is 4ec21e93f2d848159905aca95cde037c1ce0db08; its committed tree also matches HEAD. No remote fetch was performed.

Recommended next step: review the remaining meaningful semantic gaps and the MultiThresholdObjects report inventory. Storage-boundary work in Sections 4.2 and 4.3 is complete for all 40 filters; historical sign-offs have not been renewed by this test campaign.
