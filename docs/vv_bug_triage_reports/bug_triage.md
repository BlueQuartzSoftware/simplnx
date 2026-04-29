# MTR Retroactive V&V Audit — Bug Triage Report

*Status:* **DRAFT.** Triage findings derived from source-tree inspection by isolated agent verification. Each bug's evidence has been independently checked against the actual source on `topic/vv_retroactive_audit`. Test code and unified diffs below are proposals — they have not been compiled, run, or merged. **Developer must independently verify each finding and any test/fix before landing.** This document is the deliverable companion to the per-filter retroactive reports in `docs/vv_retroactive_reports/`.

This is a follow-up to the 22-filter retroactive V&V audit (`docs/vv_retroactive_reports/INDEX.md`). The audit surfaced 8 suspected real bugs across 5 filters; this document is the triage of those 8 findings.

## Summary

| # | Filter | Bug | Verdict | Severity | Production-relevant? |
|---|---|---|---|---|---|
| 1 | MultiThresholdObjectsFilter | `std::reverse` used where boolean NOT was intended | **CONFIRMED** | High (when triggered) | No shipping pipeline triggers it |
| 2 | ComputeFeatureNeighborMisorientationsFilter | Divisor reassigned inside inner j-loop, clobbers per-mismatch decrement | **CONFIRMED** | High | `ComputeAvgMisors=true` test is `[.][UNIMPLEMENTED]` stub — zero CI coverage |
| 3 | ComputeFeatureNeighborCAxisMisalignmentsFilter | Same divisor pattern — copy-paste sibling of #2 | **CONFIRMED** | High | **Yes** — `EBSD_Hexagonal_Data_Analysis.d3dpipeline` ships with `find_avg_misals: true`. Existing exemplar is hex-only, cannot trigger the bug |
| 4 | FillBadDataFilter | `static_cast<int32>(uint64 size)` overflow at line 476 | **CONFIRMED** | High | Yes for components ≥ 2^31 voxels (e.g., 2048³ or larger volumes) |
| 5 | FillBadDataFilter | Phase 4 `while(count != 0)` lacks no-progress / cancel guards | **CONFIRMED** | Medium | Yes on pathological input (all-bad volumes, isolated bad regions) |
| 6 | ReplaceElementAttributesWithNeighborValuesFilter | `bestNeighbor` not reset between outer iterations | **NOT A CORRUPTION BUG** (latent code-smell only) | Low | No — invariant proof shows redundant copies are no-ops; performance impact only |
| 7 | ReplaceElementAttributesWithNeighborValuesFilter | `float32 best` truncates int64/uint64/float64 values | **CONFIRMED** | Medium | Yes for non-float32 arrays |
| 8 | ITKImageWriterFilter | PR #1555 `dynamic_cast<DataStore<T>>` with no preflight gate | **CONFIRMED** | Lower | Yes — runtime `bad_cast` for OOC inputs, no clean preflight error |

7 confirmed real bugs across 5 filters; 1 latent code-smell. All ship on current `develop`.

## Recommended PR plan

Bundle the fixes into 4 PRs to minimize merge conflict surface and respect the in-flight `joey/*` branches:

| PR | Bugs | Title (proposed) | Notes |
|---|---|---|---|
| A | #2 + #3 | `BUG: Fix divisor reset in ComputeFeatureNeighbor{C-Axis,}Misalignments average computation` | Sibling fix; bundle with shared 4-feature mixed-phase test helper; `ComputeAvgMisors=true` test currently `[.][UNIMPLEMENTED]` so no regression risk for the misorientations filter |
| B | #1 | `BUG: Fix MultiThresholdObjects per-tuple inversion (std::reverse → element NOT)` | **Review and merge `Matthew/mtr/MultiThresholdObjects` branch (commit `9b3fe3dd3`)** which already removes both `std::reverse` calls; add the 20-element inverted-threshold test |
| C | #4 + #5 | `BUG: FillBadData int32 overflow + Phase 4 progress guard` | Open companion issue for **independent algorithm review of PR #1515** (the AI-generated rewrite has 11 additional suspect patterns surfaced during triage) |
| D | #8 | `BUG: ITKImageWriter preflight gate for out-of-core inputs` | ~7-line addition mirroring PR #1555's existing gate pattern; uses existing `k_OutOfCoreDataNotSupported = -2002` error code |

Bugs #6 and #7 land via the in-flight branches:
- **#7 already fixed** in `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation` (commit `d6ec06d5f`). Push to land that PR. **Forward-port the same `T best` typing fix into `joey/ooc-filter-optimizations`** (still buggy there because branched before `d6ec06d5f`).
- **#6 defensive `std::fill` reset** → fold into the same validation branch as a one-line cleanup before that branch opens its PR. Do not open a separate hotfix against develop.

---

## Bug 1 — MultiThresholdObjectsFilter `std::reverse` vs NOT

### Summary
**File:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.cpp`
**Origin:** PR #642 (commit `9911dc523`, 2023-07-14, "ENH: MultiThreshold Output Type"). Introduced when `std::vector<bool>::flip()` (per-element NOT) was replaced with `std::reverse(...)` during a templated-output-type refactor — the author appears to have mistaken `flip()` for "reverse" when it actually means "toggle every bit". Carried verbatim through PR #1544 (commit `8381d1dd5`, 2026-02-26) when logic moved into the Algorithm class.
**Existing fix:** Unmerged branch `Matthew/mtr/MultiThresholdObjects` commit `9b3fe3dd3` (Matthew Marine, 2026-04-23) removes both occurrences and unifies the apply logic.

### Evidence

`ThresholdValue` — `replaceInput && inverse` branch (line 138):
```cpp
112: template <typename T>
113: void ThresholdValue(const ArrayThreshold& comparisonValue, ..., AbstractDataStore<T>& outputResultStore, ...,
                        bool replaceInput, bool inverse, T trueValue, ...)
118:   std::vector<T> tempResultVector(totalTuples, falseValue);
128:   ThresholdFilterHelper<T> helper(compOperator, compValue, componentIndex, tempResultVector);
132:   ExecuteDataFunction(ExecuteThresholdHelper{}, iDataArray.getDataType(), helper, iDataArray, trueValue, falseValue);
134:   if(replaceInput)
135:   {
136:     if(inverse)
137:     {
138:       std::reverse(tempResultVector.begin(), tempResultVector.end());   // <-- BUG
139:     }
141:     for(size_t i = 0; i < totalTuples; i++)
143:       outputResultStore[i] = tempResultVector[i];
145:   }
```

`ThresholdSet` — same pattern at line 194, **plus a second independent bug**: in this branch the inner loop writes directly to `outputResultStore`, never to `tempResultVector`. Line 198 then copies the all-`falseValue` `tempResultVector` over the computed result, wiping it.

```cpp
170:   std::vector<T> tempResultVector(totalTuples, falseValue);
174-188:   <-- inner loop writes to outputResultStore, NOT tempResultVector
190:   if(replaceInput)
192:     if(inverse)
194:       std::reverse(tempResultVector.begin(), tempResultVector.end());   // <-- BUG (and irrelevant: vector is all-falseValue)
196:     for(size_t i = 0; i < totalTuples; i++)
198:       outputResultStore[i] = tempResultVector[i];                       // <-- catastrophic: wipes computed result
200:   }
```

### User-visible consequence

`std::reverse` on a binary mask only equals NOT for vectors that are symmetric under both reversal AND negation. Generic non-symmetric inputs differ.

Example: input `[1, 2, 3, 4, 5]`, threshold `>3`:
- Raw mask: `[0, 0, 0, 1, 1]`
- Expected NOT: `[1, 1, 1, 0, 0]`
- Actual `std::reverse`: `[1, 1, 0, 0, 0]` — differs at index 2

Cardinality (`count(true)`) is preserved by reverse, so downstream cardinality checks pass and the wrong mask propagates silently.

`grep -r '"inverted": *true' src/Plugins/**/pipelines/*.d3dpipeline` returns zero matches across the repo. **No shipping pipeline currently triggers this branch**, which is why it has gone undetected.

### Failing Catch2 test (proposal)

Add to `src/Plugins/SimplnxCore/test/MultiThresholdObjectsTest.cpp`. Uses the existing `CreateTestDataStructure()` (which builds a 20-element `int32` array `k_TestArrayInt` with values `0..19`).

```cpp
TEST_CASE("SimplnxCore::MultiThresholdObjects: Inverted ArrayThresholdSet performs per-tuple NOT (not reverse)",
          "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();

  // k_TestArrayInt holds 0..19 (20 tuples). Threshold ">10" yields raw mask
  // [0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1] (eleven 0s then nine 1s).
  // NOT of that is        [1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0]
  // REVERSE of that is    [1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0]
  // The two differ at indices 9, 10 — confirming this asserts NOT, not reverse.

  MultiThresholdObjectsFilter filter;
  Arguments args;

  ArrayThresholdSet thresholdSet;
  thresholdSet.setInverted(true);
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(10.0);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint8));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath maskPath = k_ImageCellDataName.createChildPath(k_ThresholdArrayName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(maskPath));
  const auto& maskArray = dataStructure.getDataRefAs<UInt8Array>(maskPath);
  REQUIRE(maskArray.getNumberOfTuples() == 20);

  // Expected: NOT of (i > 10) ==> i <= 10 ==> indices 0..10 are TRUE (1), indices 11..19 are FALSE (0).
  const std::vector<uint8> expectedMask = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for(usize i = 0; i < expectedMask.size(); i++)
  {
    INFO("tuple index " << i);
    REQUIRE(maskArray[i] == expectedMask[i]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

Fails on current `develop` at index 9 (expected 1, gets `mask[19-9] = mask[10] = 0`).

### Proposed minimal fix

```diff
--- a/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.cpp
+++ b/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.cpp
@@ -134,11 +134,15 @@ void ThresholdValue(...)
   if(replaceInput)
   {
     if(inverse)
     {
-      std::reverse(tempResultVector.begin(), tempResultVector.end());
+      // Per-tuple boolean NOT: flip every tuple between trueValue and falseValue.
+      for(size_t i = 0; i < totalTuples; i++)
+      {
+        tempResultVector[i] = (tempResultVector[i] == trueValue) ? falseValue : trueValue;
+      }
     }
     for(size_t i = 0; i < totalTuples; i++)
     {
       outputResultStore[i] = tempResultVector[i];
     }
   }
```

The same per-element NOT fix applies to line 194. **However**, the `ThresholdSet` branch additionally needs the inner loop wired through `tempResultVector` (or operating in-place on `outputResultStore`) — `9b3fe3dd3` already implements that approach. **Recommend reviewing and merging the existing branch rather than reproducing the fix.**

---

## Bug 2 — ComputeFeatureNeighborMisorientationsFilter divisor

### Summary
**File:** `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.cpp`
**Pattern:** Divisor variable reassigned inside inner loop, clobbering an earlier per-mismatch decrement.

### Evidence
- L47: `size_t tempMisoList = 0;` — initialized once before the feature loop.
- **L75: `tempMisoList = featureNeighborList.size();`** ← THE BUG. Runs every j iteration before the phase-match branch is selected.
- L90: `tempMisoList > 0 ? tempMisoList-- : tempMisoList = 0;` — decrement in the else (phase-mismatch) branch. Effect erased by line 75 on the next iteration.
- L99: `(*avgMisorientations)[i] /= static_cast<float>(tempMisoList);` — divisor used here.
- L105: `tempMisoList = 0;` — outer-loop reset.

The author clearly intended `tempMisoList = featureNeighborList.size();` to live *outside* the j-loop, alongside the `tempMisorientationLists[i].assign(...)` initialization at line ~67.

Net rule of the buggy divisor for a feature with N neighbors:
- `N` if the **last** neighbor matched phase
- `N - 1` if the **last** neighbor mismatched
- Earlier mismatches silently ignored (order-sensitive bug)

### User-visible consequence

Feature with phase-1 neighbors yielding misorientations 10° and 20°, plus one phase-2 neighbor (NaN). Order `[mismatch (phase 2), match (10°), match (20°)]`:
- **Expected:** `(10 + 20) / 2 = 15.0°`
- **Buggy:** numerator 30, divisor 3 (last matched) → `10.0°` — 5° low

The `ComputeAvgMisors=true` test (line 84) is `[.][UNIMPLEMENTED][!mayfail]` — zero CI coverage, which is why this went undetected.

### Failing Catch2 test (proposal)

```cpp
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Avg Misorientation Phase Masking",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 4 features (index 0 = unassigned, 1..3 active in phase 1, 4 in phase 2)
  // Feature 1's neighbors: [4 (mismatch), 2 (match), 3 (match)]
  // Buggy divisor will be 3 (last neighbor matched), should be 2.
  DataStructure dataStructure;
  DataPath dcPath({"DC"});
  auto* dcGroup = DataGroup::Create(dataStructure, "DC");
  REQUIRE(dcGroup != nullptr);

  DataPath cellFeatPath = dcPath.createChildPath("CellFeatureData");
  auto* featGroup = DataGroup::Create(dataStructure, "CellFeatureData", dcGroup->getId());
  REQUIRE(featGroup != nullptr);

  DataPath ensemblePath = dcPath.createChildPath("CellEnsembleData");
  auto* ensGroup = DataGroup::Create(dataStructure, "CellEnsembleData", dcGroup->getId());
  REQUIRE(ensGroup != nullptr);

  const usize totalFeatures = 5; // 0..4

  // FeaturePhases: feat0=0, feat1..3=1, feat4=2
  DataPath phasesPath = cellFeatPath.createChildPath("Phases");
  auto* phasesPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Phases",
                                                                  std::vector<usize>{totalFeatures}, std::vector<usize>{1}, featGroup->getId());
  REQUIRE(phasesPtr != nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(phasesPath));
  auto& phases = dataStructure.getDataRefAs<Int32Array>(phasesPath);
  phases[0] = 0; phases[1] = 1; phases[2] = 1; phases[3] = 1; phases[4] = 2;

  // AvgQuats (4 components per feature). Identity quaternion (0,0,0,1) for feat 1, 3, 4.
  // Feature 2: rotated by 10° about z so misor(1,2)=10°.
  DataPath quatsPath = cellFeatPath.createChildPath("AvgQuats");
  auto* quatsPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "AvgQuats",
                                                                     std::vector<usize>{totalFeatures}, std::vector<usize>{4}, featGroup->getId());
  REQUIRE(quatsPtr != nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(quatsPath));
  auto& quats = dataStructure.getDataRefAs<Float32Array>(quatsPath);
  for(usize f = 0; f < totalFeatures; ++f)
  {
    quats[f * 4 + 0] = 0.0f;
    quats[f * 4 + 1] = 0.0f;
    quats[f * 4 + 2] = 0.0f;
    quats[f * 4 + 3] = 1.0f;
  }
  const float32 halfAngleRad = static_cast<float32>(5.0 * nx::core::Constants::k_PiOver180D);
  quats[2 * 4 + 0] = 0.0f;
  quats[2 * 4 + 1] = 0.0f;
  quats[2 * 4 + 2] = std::sin(halfAngleRad);
  quats[2 * 4 + 3] = std::cos(halfAngleRad);

  // CrystalStructures: ensemble[0]=Cubic_High, ensemble[1]=Cubic_High, ensemble[2]=Hexagonal_High
  const usize numEnsembles = 3;
  DataPath xtalPath = ensemblePath.createChildPath("CrystalStructures");
  auto* xtalPtr = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, "CrystalStructures",
                                                                   std::vector<usize>{numEnsembles}, std::vector<usize>{1}, ensGroup->getId());
  REQUIRE(xtalPtr != nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(xtalPath));
  auto& xtal = dataStructure.getDataRefAs<UInt32Array>(xtalPath);
  xtal[0] = ebsdlib::CrystalStructure::Cubic_High;
  xtal[1] = ebsdlib::CrystalStructure::Cubic_High;
  xtal[2] = ebsdlib::CrystalStructure::Hexagonal_High;

  // NeighborList. Crucial order: [4 (mismatch), 2 (match-10°), 3 (match-0°)]
  DataPath neighborPath = cellFeatPath.createChildPath("NeighborList");
  auto* nlPtr = NeighborList<int32>::Create(dataStructure, "NeighborList",
                                            std::vector<usize>{totalFeatures}, featGroup->getId());
  REQUIRE(nlPtr != nullptr);
  auto& nl = dataStructure.getDataRefAs<NeighborList<int32>>(neighborPath);
  nl.setList(0, NeighborList<int32>::SharedVectorType(new std::vector<int32>{}));
  nl.setList(1, NeighborList<int32>::SharedVectorType(new std::vector<int32>{4, 2, 3}));
  nl.setList(2, NeighborList<int32>::SharedVectorType(new std::vector<int32>{1}));
  nl.setList(3, NeighborList<int32>::SharedVectorType(new std::vector<int32>{1}));
  nl.setList(4, NeighborList<int32>::SharedVectorType(new std::vector<int32>{1}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(neighborPath));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(quatsPath));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(xtalPath));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_MisorientationListArrayName_Key, std::make_any<std::string>("CalcMisorList"));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_AvgMisorientationsArrayName_Key, std::make_any<std::string>("AvgMisors"));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_ComputeAvgMisors_Key, std::make_any<bool>(true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Expected: average over 2 phase-matched neighbors (10°, 0°) = 5.0°
  // Buggy:    sum / 3 (last neighbor matched) = 10/3 ≈ 3.333°
  DataPath avgPath = cellFeatPath.createChildPath("AvgMisors");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(avgPath));
  const auto& avgMisors = dataStructure.getDataRefAs<Float32Array>(avgPath);

  REQUIRE(avgMisors[1] == Catch::Approx(5.0f).margin(1.0e-3f));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

### Proposed minimal fix

```diff
--- a/src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.cpp
+++ b/src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.cpp
@@ -65,13 +65,14 @@ Result<> ComputeFeatureNeighborMisorientations::operator()()
     const NeighborList<int32_t>::VectorType featureNeighborList = inNeighborList.at(static_cast<int32_t>(i));

     tempMisorientationLists[i].assign(featureNeighborList.size(), -1.0);
+    // Initialize the matched-phase neighbor count to the full neighbor count;
+    // it is decremented once per phase-mismatched neighbor in the loop below.
+    tempMisoList = featureNeighborList.size();

     for(size_t j = 0; j < featureNeighborList.size(); j++)
     {
       int32_t neighborFeatureId = featureNeighborList[j];
       quatIndex = neighborFeatureId * 4;
       ebsdlib::QuatD q2(inAvgQuats[quatIndex], inAvgQuats[quatIndex + 1], inAvgQuats[quatIndex + 2], inAvgQuats[quatIndex + 3]);
       uint32_t xtalType2 = inXtalStruct[inFeaturePhases[neighborFeatureId]];
-      tempMisoList = featureNeighborList.size();
       if(laueClass1 == xtalType2 && static_cast<int64_t>(laueClass1) < static_cast<int64_t>(orientationOps.size()))
       {
         ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(q1, q2);
```

The line-105 outer reset is harmless and may stay.

---

## Bug 3 — ComputeFeatureNeighborCAxisMisalignmentsFilter divisor (production-relevant)

### Summary
**File:** `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp`
**Pattern:** Same as Bug 2 — copy-paste sibling. Bundle fixes.
**Production relevance:** Ships in `src/Plugins/OrientationAnalysis/pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` with `find_avg_misals: true`.

### Evidence
- L77: `usize hexNeighborListSize = 0;` — initialized once before the outer loop.
- L82: `for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)` — outer loop.
- L107: `for(usize j = 0; j < currentNeighborList.size(); j++)` — inner loop.
- **L111: `hexNeighborListSize = currentNeighborList.size();`** ← THE BUG. Reassigned every j iteration.
- L114: hex/hex match branch (does nothing to `hexNeighborListSize`).
- L150: `hexNeighborListSize--;` in non-hex skip branch (inside `else` of phase check, guarded by `if(m_InputValues->FindAvgMisals)`).
- L162: `double value = avgCAxisMisalignmentPtr->getValue(featureIdx) / static_cast<double>(hexNeighborListSize);` — divisor used here.
- L169: reset to 0 (after divisor use, dead given line 111 will re-clobber).

Production pipeline JSON (lines 694–731 of `EBSD_Hexagonal_Data_Analysis.d3dpipeline`):
```json
{
  "args": {
    "avg_c_axis_misalignments_array_name": { "value": "AvgCAxisMisalignments", "version": 1 },
    "find_avg_misals": { "value": true, "version": 1 },
    ...
  },
  "filter": {
    "name": "nx::core::ComputeFeatureNeighborCAxisMisalignmentsFilter",
    "uuid": "636ee030-9f07-4f16-a4f3-592eff8ef1ee"
  },
  "isDisabled": false
}
```

Existing test exemplar `compute_feature_neighbor_caxis_misalignments.tar.gz` loads `7_5_simplnx_test_file_25x50_Hex.dream3d` — the `_Hex` suffix means all-hex, so the `else` branch on L146-153 never fires under that fixture. Worse: the exemplar `AvgCAxisMisalignments` was generated by either the buggy code itself or by legacy DREAM3D 6.5.171 (which the audit suspects has the same bug) → false confidence.

### User-visible consequence

3-hex + 1-non-hex feature with hex/hex misalignment angles `0°, 30°` and the non-hex neighbor in any position other than last:
- Numerator: `0 + 30 = 30°`
- **Correct** divisor = 2 hex matches → average = **15°**
- **Buggy** divisor = `currentNeighborList.size() - 1 = 4 - 1 = 3` (if non-hex is last) → **10°**
- Or divisor = 4 (if non-hex is not last) → **7.5°**

Result is order-dependent — non-deterministic across pipelines that re-order neighbor lists.

### Failing Catch2 test (proposal)

```cpp
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Mixed-Phase Divisor (D1)",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter][Bug]")
{
  UnitTest::LoadPlugins();

  // 4 features:
  //   feature 0 : ignored (background)
  //   feature 1 : hex, c-axis along [001]
  //   feature 2 : hex, c-axis along [001]                    -> misalign(1,2) = 0 deg
  //   feature 3 : hex, c-axis tilted 30 deg from [001] in xz -> misalign(1,3) = 30 deg
  //   feature 4 : cubic (non-hex)                            -> misalign(1,4) = NaN, skip
  //
  // Feature 1's neighbor list = {4, 2, 3} (non-hex first to expose the bug).
  // Correct AvgCAxisMisalignments[1] = (0 + 30) / 2 = 15.0 deg
  // Buggy code divides by 3 = 10.0 deg

  DataStructure dataStructure;

  AttributeMatrix* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", AttributeMatrix::ShapeType{3});
  auto* crystalStructures = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, "CrystalStructures",
                                                                            std::vector<usize>{3}, std::vector<usize>{1}, ensembleAM->getId());
  (*crystalStructures)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructures)[1] = ebsdlib::CrystalStructure::Hexagonal_High;
  (*crystalStructures)[2] = ebsdlib::CrystalStructure::Cubic_High;

  AttributeMatrix* featureAM = AttributeMatrix::Create(dataStructure, "CellFeatureData", AttributeMatrix::ShapeType{5});

  auto* phases = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Phases",
                                                               std::vector<usize>{5}, std::vector<usize>{1}, featureAM->getId());
  (*phases)[0] = 0; (*phases)[1] = 1; (*phases)[2] = 1; (*phases)[3] = 1; (*phases)[4] = 2;

  auto* avgQuats = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "AvgQuats",
                                                                     std::vector<usize>{5}, std::vector<usize>{4}, featureAM->getId());
  for(int32 fid : {0, 1, 2, 4})
  {
    (*avgQuats)[fid * 4 + 0] = 0.0f;
    (*avgQuats)[fid * 4 + 1] = 0.0f;
    (*avgQuats)[fid * 4 + 2] = 0.0f;
    (*avgQuats)[fid * 4 + 3] = 1.0f;
  }
  // Feature 3: rotation by 30 deg about y-axis -> c-axis tilts 30 deg from [001].
  const double half = 15.0 * Constants::k_PiD / 180.0;
  (*avgQuats)[3 * 4 + 0] = 0.0f;
  (*avgQuats)[3 * 4 + 1] = static_cast<float32>(std::sin(half));
  (*avgQuats)[3 * 4 + 2] = 0.0f;
  (*avgQuats)[3 * 4 + 3] = static_cast<float32>(std::cos(half));

  Int32NeighborList* neighborList = Int32NeighborList::Create(dataStructure, "NeighborList",
                                                              featureAM->getShape(), featureAM->getId());
  neighborList->setList(1, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{4, 2, 3}));
  neighborList->setList(2, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1}));
  neighborList->setList(3, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1}));
  neighborList->setList(4, std::make_shared<Int32NeighborList::VectorType>(Int32NeighborList::VectorType{1}));

  const DataPath k_NeighborListPath{{"NeighborList"}};
  const DataPath k_AvgQuatsPath{{"AvgQuats"}};
  const DataPath k_PhasesPath{{"Phases"}};
  const DataPath k_CrystalStructuresPath{{"CellEnsembleData", "CrystalStructures"}};

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(k_NeighborListPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_AvgQuatsPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key, std::make_any<std::string>("CAxisMisalignmentList"));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key, std::make_any<std::string>("AvgCAxisMisalignments"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(DataPath({"AvgCAxisMisalignments"})));
  const auto& avgArray = dataStructure.getDataRefAs<Float32Array>(DataPath({"AvgCAxisMisalignments"}));

  // Expected: 15.0 deg ((0 + 30) / 2 hex matches)
  // Buggy:    10.0 deg ((0 + 30) / 3 because last neighbor was a hex match)
  REQUIRE(avgArray[1] == Catch::Approx(15.0f).epsilon(1e-4));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<float32>>(DataPath({"CAxisMisalignmentList"})));
  const auto& cAxisList = dataStructure.getDataRefAs<NeighborList<float32>>(DataPath({"CAxisMisalignmentList"}));
  const auto& list1 = cAxisList[1];
  REQUIRE(list1.size() == 3);
  REQUIRE(std::isnan(list1[0]));
  REQUIRE(list1[1] == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(list1[2] == Catch::Approx(30.0f).epsilon(1e-3));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

### Proposed minimal fix

```diff
--- a/src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp
+++ b/src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp
@@ -103,11 +103,12 @@ Result<> ComputeFeatureNeighborCAxisMisalignments::operator()()
     // Allocate enough room based on the current features neighbor list size
     const NeighborList<int>::VectorType& currentNeighborList = neighborList[featureIdx];
     auto& currentMisalignmentList = misalignmentLists[featureIdx];
     currentMisalignmentList.resize(currentNeighborList.size(), -1.0);
+    // Initialize divisor ONCE per feature; the inner loop will decrement it per non-hex neighbor.
+    hexNeighborListSize = currentNeighborList.size();
     for(usize j = 0; j < currentNeighborList.size(); j++)
     {
       int neighborFeatureId = currentNeighborList[j];
       xtalPhase2 = crystalStructures[featurePhases[neighborFeatureId]];
-      hexNeighborListSize = currentNeighborList.size();

       // If both the feature and the neighbor are both Hexagonal Phases
       if(xtalPhase1 == xtalPhase2 && (xtalPhase1 == ebsdlib::CrystalStructure::Hexagonal_High || xtalPhase1 == ebsdlib::CrystalStructure::Hexagonal_Low))
```

### Coordination
**Bundle Bug 2 + Bug 3 in a single PR** with a shared 4-feature mixed-phase test helper. Both filters were ported from the same legacy DREAM3D family (`FindFeatureNeighbor*Misorientation*`); a single PR titled `BUG: Fix divisor reset in ComputeFeatureNeighbor{C-Axis,}Misalignments average computation` keeps the audit trail clean and ensures the two filters do not regress independently.

---

## Bug 4 — FillBadDataFilter int32 overflow

### Summary
**File:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp`
**Origin:** PR #1515 (AI-generated four-phase chunk-aware rewrite, +1139/-194 lines, never formally reviewed).

### Evidence

Lines 472–480:
```cpp
// Classify regions as small (need filling) or large (keep or assign to a new phase)
std::unordered_set<int64> localSmallRegions;
for(const auto& [root, size] : rootSizes)
{
  if(static_cast<int32>(size) < m_InputValues->minAllowedDefectSizeValue)
  {
    localSmallRegions.insert(root);
  }
}
```

Type chain:
- `rootSizes` (line 462): `std::unordered_map<int64, uint64>` → `size` is **uint64**
- Populated from `unionFind.getSize(root)` which returns **uint64** (`FillBadData.hpp:62`, `FillBadData.cpp:210`)
- Sizes accumulated in `ChunkAwareUnionFind::addSize(int64 label, uint64 count)` where `m_Size` is `std::unordered_map<int64, uint64>` (`FillBadData.hpp:72`)
- One `addSize(label, 1)` per bad voxel in Phase 1 (line 404), summed to root in `flatten()` (line 250)
- `m_InputValues->minAllowedDefectSizeValue` is **int32** (`FillBadData.hpp:77`)

The narrowing cast wraps for any `size` value with bit 31 set — interpreted as a negative `int32`, the predicate `< minAllowed` is trivially true, forcing the region into `localSmallRegions`.

### User-visible consequence

A connected bad region of ≥ 2^31 = 2,147,483,648 voxels is mis-classified as "small" and filled in Phase 4 — the **opposite** of the user's intent (`MinAllowedDefectSize` exists to *protect* large defects from being smeared by morphological dilation).

Realistic case: an OOC tomographic reconstruction of size 4096×4096×512 = 8.6e9 voxels with a single very-large bad region (e.g., the entire vacuum/air around a sample). With `MinAllowedDefectSize = 1000`, the user expects the entire vacuum region to be left alone; instead the algorithm decides the multi-billion-voxel vacuum is "smaller than 1000" and fills it with whatever feature happens to border it.

### Failing Catch2 test (proposal)

A direct unit test that *generates* >2 billion voxels is impractical (>8 GB array, ~20 min runtime). Two complementary forms:

**(a) Property assertion** — drop into `FillBadDataTest.cpp`:

```cpp
TEST_CASE("SimplnxCore::FillBadData::SizeComparisonNarrowing", "[Core][FillBadDataFilter]")
{
  // Property: the size of a connected bad-data region (uint64) must be compared
  // to the threshold without narrowing. If the algorithm narrows uint64 -> int32,
  // a region of size INT32_MAX+1 == 2^31 wraps to INT32_MIN and is mis-classified
  // as smaller than ANY positive threshold.
  uint64 hugeSize = static_cast<uint64>(std::numeric_limits<int32>::max()) + 1ULL; // 2^31
  int32  threshold = 1000;

  // What the BUGGY line does:
  bool buggyClassification = (static_cast<int32>(hugeSize) < threshold);
  // What it SHOULD do:
  bool correctClassification = (hugeSize < static_cast<uint64>(threshold));

  REQUIRE(buggyClassification == true);
  REQUIRE(correctClassification == false);

  // Once fixed, the assertion below should pass; on develop it FAILS.
  REQUIRE(buggyClassification == correctClassification);
}
```

**(b) White-box test** — drives the same comparison Phase 3 uses, with a huge synthetic region size injected via `ChunkAwareUnionFind`:

```cpp
TEST_CASE("SimplnxCore::FillBadData::Phase3_HugeRegionMisclassified", "[Core][FillBadDataFilter]")
{
  ChunkAwareUnionFind uf;
  const int64 label = -1;
  uf.addSize(label, static_cast<uint64>(std::numeric_limits<int32>::max()) + 1ULL);
  uf.flatten();
  REQUIRE(uf.getSize(label) > static_cast<uint64>(std::numeric_limits<int32>::max()));

  const int32 threshold = 1000;
  const uint64 size = uf.getSize(label);

  // Mirrors line 476 verbatim:
  bool wouldBeFilled = (static_cast<int32>(size) < threshold);

  REQUIRE_FALSE(wouldBeFilled); // FAILS on develop, PASSES after fix.
}
```

### Proposed minimal fix

```diff
--- a/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp
+++ b/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp
@@ -472,7 +472,9 @@
   // Classify regions as small (need filling) or large (keep or assign to a new phase)
   std::unordered_set<int64> localSmallRegions;
   for(const auto& [root, size] : rootSizes)
   {
-    if(static_cast<int32>(size) < m_InputValues->minAllowedDefectSizeValue)
+    // Compare in uint64 space; widening the threshold is safe because
+    // minAllowedDefectSizeValue is validated >= 0 by the parameter.
+    if(size < static_cast<uint64>(m_InputValues->minAllowedDefectSizeValue))
     {
       localSmallRegions.insert(root);
     }
```

If `minAllowedDefectSizeValue` could ever be negative, an explicit guard or a stronger parameter type (`uint32`/`uint64`) is preferable. `MinAllowedDefectSize` is conceptually unsigned, so the `uint64` cast is safe in practice.

---

## Bug 5 — FillBadDataFilter Phase 4 infinite loop

### Summary
**File:** Same as Bug 4.
**Pattern:** `while(count != 0)` has no progress guard, no max-iteration cap, no cancel check.

### Evidence

Lines 580–679:
```cpp
580:  usize count = 1;
581:  usize iteration = 0;
584:  while(count != 0)
585:  {
586:    iteration++;
587:    count = 0;
591:    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
596:      if(featureName < 0)
597:      {
598:        count++;
628:            if(current > most)
629:            {
630:              most = current;
631:              neighbors[voxelIndex] = static_cast<int32>(neighborPoint);
632:            }
675:    FillBadDataUpdateTuples<int32>(featureIdsStore, featureIdsStore, neighbors);
678:    throttledMessenger.sendThrottledMessage(...);
679:  }
```

`neighbors[voxelIndex]` is initialized to `-1` once *outside* the while loop (line 562). It is only ever **overwritten** when a positive neighbor is found (line 631), and never reset between iterations. `FillBadDataUpdateTuples` is a no-op when `neighbor == -1` or `featureIds[neighbor] <= 0`. So if every −1 voxel is surrounded only by other −1 voxels and image boundaries, **no progress is made, `count` stays unchanged, and the loop never exits**. There is no `m_ShouldCancel` check inside the while loop either, so the user cannot abort the spin.

### User-visible consequence

Pathological scenario: a 4×4×4 cube with FeatureIds entirely `0` (all bad). Phase 1 labels everything as one component of size 64. With `MinAllowedDefectSize > 64`, Phase 3 flips every voxel to `−1`. Phase 4's first iteration sets `count = 64`; nobody has a positive face-neighbor, so `neighbors[i]` stays `-1` for all `i`; nothing is updated; iteration 2 finds the same 64 −1's; **the filter hangs**. Cancel button doesn't help.

Less pathological: any user who accidentally feeds the filter a FeatureIds array of all zeros (e.g., by selecting the wrong array) gets a hung process.

### Failing Catch2 test (proposal)

```cpp
TEST_CASE("SimplnxCore::FillBadData::Phase4_NoProgressTermination", "[Core][FillBadDataFilter]")
{
  UnitTest::LoadPlugins();

  // 4x4x4 ImageGeom with FeatureIds entirely 0.
  // Phase 3 marks every voxel -1 (size 64 < threshold 1000).
  // Phase 4 has zero positive neighbors anywhere -> would spin forever without a guard.
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({4, 4, 4});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAm = AttributeMatrix::Create(dataStructure, "CellData", {4, 4, 4}, imageGeom->getId());
  imageGeom->setCellData(*cellAm);

  auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(
      dataStructure, "FeatureIds", {4, 4, 4}, {1}, cellAm->getId());
  featureIds->fill(0);

  FillBadDataFilter filter;
  Arguments args;
  args.insertOrAssign(FillBadDataFilter::k_MinAllowedDefectSize_Key, std::make_any<int32>(1000));
  args.insertOrAssign(FillBadDataFilter::k_StoreAsNewPhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(FillBadDataFilter::k_CellFeatureIdsArrayPath_Key,
                      std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(FillBadDataFilter::k_CellPhasesArrayPath_Key,
                      std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Phases"})));
  args.insertOrAssign(FillBadDataFilter::k_IgnoredDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(FillBadDataFilter::k_SelectedImageGeometryPath_Key,
                      std::make_any<DataPath>(DataPath({"DataContainer"})));

  std::atomic_bool finished{false};
  IFilter::ExecuteResult executeResult;
  std::thread runner([&]() {
    executeResult = filter.execute(dataStructure, args);
    finished.store(true);
  });

  // 10-second budget for a 64-voxel test; an infinite loop will exceed it.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while(!finished.load() && std::chrono::steady_clock::now() < deadline)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  REQUIRE(finished.load()); // FAILS on develop (filter still spinning).
  if(finished.load())
  {
    runner.join();
    REQUIRE_FALSE(executeResult.result.valid()); // expect a no-progress error
  }
  else
  {
    runner.detach();
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

### Proposed minimal fix

Two-part: add a no-progress guard, a cancel check, and reset `neighbors[]` per iteration.

```diff
--- a/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp
+++ b/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp
@@ -577,14 +577,21 @@
   MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
   auto throttledMessenger = messageHelper.createThrottledMessenger(std::chrono::milliseconds(1000));

-  usize count = 1;
-  usize iteration = 0;
+  usize count = 1;
+  usize previousCount = 0;    // for no-progress detection
+  usize iteration = 0;

   while(count != 0)
   {
+    if(m_ShouldCancel)
+    {
+      return MakeErrorResult(-1, "FillBadData was cancelled during Phase 4 iterative fill.");
+    }
+    previousCount = count;
     iteration++;
     count = 0;
+    // Reset neighbor assignments so stale -1 voxels don't carry over from prior iterations.
+    std::fill(neighbors.begin(), neighbors.end(), -1);

     for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
@@ -676,6 +683,15 @@

     throttledMessenger.sendThrottledMessage([iteration, count]() { return fmt::format("  Iteration {}: {} voxels remaining to fill", iteration, count); });
+
+    // No-progress termination: if this iteration didn't fill any voxels, we
+    // would loop forever. Bail out with a clear error.
+    if(count == previousCount)
+    {
+      return MakeErrorResult(-2,
+        fmt::format("FillBadData Phase 4 made no progress on iteration {} ({} bad voxels remain with no valid positive neighbors). "
+                    "This typically means the input has bad-data regions that cannot be reached from any valid feature.", iteration, count));
+    }
   }
```

Function return type would need to change from `void` to `Result<>` — straightforward signature plumbing. A simpler alternative if the void signature is preferred: log a warning via `m_MessageHandler` and `break` on no-progress.

The `std::fill(neighbors, -1)` reset is also a latent correctness improvement: a voxel that *was* assigned a neighbor in a prior iteration (and successfully filled) leaves its stale neighbor index in the `neighbors` array. The current `FillBadDataUpdateTuples` skip-when-`featureIds[idx] != -1` check papers over it, but the dependency is subtle.

### PR #1515 — additional suspect patterns flagged during triage

While reading `FillBadData.cpp` for Bugs 4 and 5, the triage agent noticed the following additional candidates for follow-up. These are **not triaged in detail** — open companion issue for an independent algorithm review of the entire AI-generated rewrite (PR #1515).

| # | Location | Pattern | Severity |
|---|---|---|---|
| a | Line 522 | `static_cast<int32>(maxPhase) + 1` narrows `usize` → same family as Bug 4 | High |
| b | `storeAsNewPhase` (audit D1) | Single new phase index assigned to *all* large defect regions regardless of spatial disjointness | Medium |
| c | Lines 732–740 | `numFeatures` computed via O(N) full-array scan before Phase 1 even starts — perf regression for OOC volumes | Performance |
| d | Lines 722–728 | `maxPhase` computed via O(N) full linear scan with subscript operator — same perf concern | Performance |
| e | Line 565 | `featureNumber(numFeatures + 1, 0)` indexed by raw `feature` value — fails when FeatureIds are sparse with very large numeric values; risk of buffer overflow or 8 GB allocation for `INT32_MAX`-style sparse fixtures | Medium |
| f | Line 631 | `neighbors[voxelIndex] = static_cast<int32>(neighborPoint);` — `neighborPoint` is int64; cast wraps for `voxelIndex >= 2^31`. Same family as Bug 4. Combined with `neighbors` being `std::vector<int32>`, caps the algorithm at 2.1B voxels regardless of OOC support. | High |
| g | Line 591 | `int64 voxelIndex` vs `usize totalPoints` — mixed signed/unsigned comparison | Smell |
| h | `FillBadDataUpdateTuples` line 68 | `neighbor == tupleIndex` mixes int32 and usize — for `tupleIndex > INT32_MAX` the int32 (potentially negative) is sign-extended to usize, making the comparison wrong. Same 2.1B voxel cap. | High |
| i | Lines 411, 532 | `featureIdsStore.flush()` after Phase 1 and Phase 3 — no flush after Phase 4. Whether intentional (caller flushes) or a leak depends on OOC contract. | Doc |
| j | Throughout | `m_ShouldCancel` is never read by any of the four phases (only the getter at line 274 exists). Long-running OOC runs cannot be aborted. | Medium |
| k | Lines 427, etc. | `phaseTwoGlobalResolution`'s `smallRegions` parameter is dead — never read or written. Same for the equivalent `smallRegions` parameter in `phaseThreeRelabeling`. Refactor footprint. | Smell |
| l | Line 744 | `provisionalLabels` is `std::unordered_map<usize, int64>` keyed by every bad voxel — ~32 bytes/entry. For a 4 GB FeatureIds array that's mostly bad, this map alone consumes 32 GB, defeating the OOC-friendliness claim. | Performance |
| m | Line 591 | Phase 4 still iterates flat `for(voxelIndex = 0; voxelIndex < totalPoints; ...)` with no chunk awareness. Thrashes the chunk cache on every iteration for OOC inputs. (Already noted in the audit report.) | Performance |

---

## Bug 6 — ReplaceElementAttributesWithNeighborValues `bestNeighbor` stale state

### Verdict
**NOT A CORRUPTION BUG** — latent code-smell only. Performance impact, no observable output difference.

### Why the audit overstated this

The audit claimed staleness "**overwrites the replacement just made in iteration 1**". The triage agent did rigorous case analysis on 1×3, 1×5, 1×7, 1×9 and 3×3 fixtures with `Loop=true` over multiple iterations and **proved by induction** that all redundant copies are no-ops:

1. `bestNeighbor[V]` is written only when V's value `< threshold` and a face-neighbor `N1` passes (`>= threshold`).
2. After iter N's transfer, `copyTuple(N1, V)` makes V's tuple identical to N1's. V now passes.
3. In iter N+1's mark phase, V is **skipped** (passes threshold), so `bestNeighbor[V]` is **NOT** updated — it remains `N1` (stale).
4. In iter N+1's transfer, `copyTuple(N1, V)` runs again. **N1's data is invariant** because:
   - N1 was a passing voxel in iter N (it was a source). Passing voxels never enter the "fails" branch.
   - Therefore `bestNeighbor[N1]` stays `-1` and N1 is never a target in any subsequent iteration.
   - By induction, N1's data never changes.
5. Therefore `copyTuple(N1, V)` in iter N+1 is a no-op. V's data was already N1's data.

### User-visible consequence

**Performance only.** Each "passing-but-was-replaced" voxel triggers a redundant `copyTuple` per outer iteration after its replacement. For a 500³ volume with 30% bad voxels and 5 outer iterations: ~5×10⁸ wasted no-op copies. No data corruption.

### In-flight branch status

- `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation` (commit `d6ec06d5f`): `bestNeighbor` is still allocated outside `while(keepGoing)` with no per-iteration reset. **Bug NOT addressed**.
- `joey/ooc-filter-optimizations` (commit `05bbd3277`): re-architected to per-Z-slice processing; `sliceBestNeighbor` lives at slice scope and is cleared after each slice transfer. Cross-iteration staleness is **eliminated by construction**.

### Proposed defensive fix (worth landing for cleanup, not a behavioral change)

```diff
--- a/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp
+++ b/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp
@@ -130,6 +130,9 @@ struct ExecuteTemplate
     while(keepGoing)
     {
       keepGoing = false;
       count = 0;
+      // Reset per-voxel decisions from the previous outer-loop iteration so the transfer
+      // phase below acts only on entries the current mark phase wrote.
+      std::fill(bestNeighbor.begin(), bestNeighbor.end(), int64_t{-1});
       if(shouldCancel)
       {
         break;
       }
```

Removes wasted work, future-proofs against algorithm changes that might allow a passing voxel to become a target. Land in the `joey/worktree-…Validation` branch as a one-line cleanup, not as a separate hotfix.

### Invariant-pinning test (proposal — passes today)

Worth landing alongside Bug 7's failing test as a regression pin against future changes that could turn the smell into an actual bug.

```cpp
TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Loop convergence on isolated bad voxels",
          "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter]")
{
  UnitTest::LoadPlugins();

  // 1x9x1 grid, float32 confidence: [100, 0, 0, 0, 0, 0, 0, 0, 100], threshold=50, less-than, Loop=true.
  // Multi-iteration fill from both ends; expected result is all 100s.
  DataStructure dataStructure;
  const DataPath imageGeomPath({"Image"});
  const std::string cellDataName = "CellData";
  const DataPath cellDataPath = imageGeomPath.createChildPath(cellDataName);
  const DataPath confidencePath = cellDataPath.createChildPath("Confidence");

  CreateImageGeometryAction createGeom(imageGeomPath, {9, 1, 1}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                                       cellDataName, IGeometry::LengthUnit::Micrometer);
  SIMPLNX_RESULT_REQUIRE_VALID(createGeom.apply(dataStructure, IDataAction::Mode::Execute));

  CreateArrayAction createArray(DataType::float32, {1, 1, 9}, {1}, confidencePath);
  SIMPLNX_RESULT_REQUIRE_VALID(createArray.apply(dataStructure, IDataAction::Mode::Execute));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(confidencePath));
  auto& confidence = dataStructure.getDataRefAs<Float32Array>(confidencePath);
  const std::array<float32, 9> input{100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 100.0f};
  for(usize i = 0; i < 9; ++i) { confidence[i] = input[i]; }

  ReplaceElementAttributesWithNeighborValuesFilter filter;
  Arguments args;
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(50.0f));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(confidencePath));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  SIMPLNX_RESULT_REQUIRE_VALID(filter.execute(dataStructure, args).result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(confidencePath));
  const auto& result = dataStructure.getDataRefAs<Float32Array>(confidencePath);
  for(usize i = 0; i < 9; ++i) { REQUIRE(result[i] == 100.0f); }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

---

## Bug 7 — ReplaceElementAttributesWithNeighborValues `float32 best` truncation

### Summary
**File:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp`
**Pattern:** `float32 best` is used regardless of templated `T`, truncating int64/uint64/float64 values during best-neighbor tracking.
**Existing fix:** Already implemented in `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation` commit `d6ec06d5f` ("REV: Improve ReplaceElementAttributesWithNeighborValues algorithm quality"). The commit message explicitly notes: *"Fix float32 precision loss for non-float32 input types by using native type T for threshold comparisons and best-neighbor tracking"*.
**Still buggy on:** `joey/ooc-filter-optimizations` — branched from develop before the validation fix; re-introduces `float32 best` in its slice-buffered loop.

### Evidence

- L85-86: `void CompareValues(... float thresholdValue, float32& best, ...)` — `best` parameter typed `float32&` regardless of T.
- L88: `if(comparator->compare1(inputArray[neighbor], thresholdValue) && comparator->compare2(inputArray[neighbor], best))` — `inputArray[neighbor]` is type T; `best` (float32) is implicitly converted to T at the comparator call site, with the rounded value.
- L90: `best = inputArray[neighbor];` — narrowing assignment from T to float32.
- L151: `float32 best = inputStore[voxelIndex];` — local declared as `float32` regardless of T.

### User-visible consequence

For arrays of type `float64`, `int64`, or `uint64`, the wrong neighbor can be picked when multiple neighbor values map to the same `float32` representation under round-to-nearest-even truncation.

Concrete numerical example (float64, 3-voxel 1-D fixture):
- Comparison values: V=0 = `1.0 + 1e-9`, V=1 = `-1.0` (fails threshold 0.5), V=2 = `1.0 + 5e-10`. Both V=0 and V=2 round to `1.0f` in float32; in float64 V=0 > V=2.
- Buggy: `best = -1.0f`. V=0 wins; `best = 1.0+1e-9` cast to `float32` = `1.0f`. V=2: `compare2(1.0+5e-10, static_cast<float64>(1.0f) = 1.0)` → `(1.0+5e-10) > 1.0` true (strict). best is reassigned, **bestNeighbor[1] = 2**.
- Fixed: `best = -1.0` (float64). V=0 wins; `best = 1.0+1e-9`. V=2: `1.0+5e-10 > 1.0+1e-9` false. **bestNeighbor[1] = 0**.
- Result: buggy copies V=2's tuple (`1.0+5e-10`); fixed copies V=0's tuple (`1.0+1e-9`). Different output.

For `int64`/`uint64` the bug is harder to trigger in 1-D because consecutive integer values differ by exactly 1 and strict `>` plus rounding usually produces correct ordering; multi-neighbor 2-D / 3-D layouts with values clustered around a float32 ULP boundary can also exhibit it.

### Failing Catch2 test (proposal)

Passes on `joey/worktree-…Validation`; **fails on develop**.

```cpp
TEST_CASE("SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: float64 precision is preserved",
          "[SimplnxCore][ReplaceElementAttributesWithNeighborValuesFilter]")
{
  UnitTest::LoadPlugins();

  // 3x1x1 grid, float64 comparison array, threshold=0.5 (less-than, single pass).
  // V=0 = 1.0 + 1e-9 (LARGER), V=1 = -1.0 (FAILS), V=2 = 1.0 + 5e-10 (smaller).
  // Both V=0 and V=2 round to 1.0f in float32, but V=0 > V=2 in float64.
  // Less-than mode chooses the LARGEST passing neighbor; correct winner is V=0.
  // Buggy code with `float32 best` picks V=2.
  DataStructure dataStructure;
  const DataPath imageGeomPath({"Image"});
  const std::string cellDataName = "CellData";
  const DataPath cellDataPath = imageGeomPath.createChildPath(cellDataName);
  const DataPath confidencePath = cellDataPath.createChildPath("Confidence");

  CreateImageGeometryAction createGeom(imageGeomPath, {3, 1, 1}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                                       cellDataName, IGeometry::LengthUnit::Micrometer);
  SIMPLNX_RESULT_REQUIRE_VALID(createGeom.apply(dataStructure, IDataAction::Mode::Execute));

  CreateArrayAction createArray(DataType::float64, {1, 1, 3}, {1}, confidencePath);
  SIMPLNX_RESULT_REQUIRE_VALID(createArray.apply(dataStructure, IDataAction::Mode::Execute));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(confidencePath));
  auto& confidence = dataStructure.getDataRefAs<Float64Array>(confidencePath);
  const float64 v0 = 1.0 + 1.0e-9;   // larger true value
  const float64 v2 = 1.0 + 5.0e-10;  // smaller true value
  confidence[0] = v0;
  confidence[1] = -1.0;
  confidence[2] = v2;

  ReplaceElementAttributesWithNeighborValuesFilter filter;
  Arguments args;
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_MinConfidence_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedComparison_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_Loop_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_ComparisonDataPath, std::make_any<DataPath>(confidencePath));
  args.insertOrAssign(ReplaceElementAttributesWithNeighborValuesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  SIMPLNX_RESULT_REQUIRE_VALID(filter.execute(dataStructure, args).result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(confidencePath));
  const auto& result = dataStructure.getDataRefAs<Float64Array>(confidencePath);
  REQUIRE(result[0] == v0);
  REQUIRE(result[1] == v0);  // FAILS on develop: result[1] == v2
  REQUIRE(result[2] == v2);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

### Proposed minimal fix (mirror the existing `d6ec06d5f` fix)

```diff
--- a/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp
+++ b/src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp
@@ -82,9 +82,9 @@ struct GreaterThanComparison : public IComparisonFunctor<T>
 struct ExecuteTemplate
 {
   template <typename T>
-  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, const AbstractDataStore<T>& inputArray, int64 neighbor, float thresholdValue, float32& best,
+  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, const AbstractDataStore<T>& inputArray, int64 neighbor, T thresholdValue, T& best,
                      std::vector<int64_t>& bestNeighbor, size_t i) const
   {
     if(comparator->compare1(inputArray[neighbor], thresholdValue) && comparator->compare2(inputArray[neighbor], best))
     {
       best = inputArray[neighbor];
       bestNeighbor[i] = neighbor;
     }
   }

   template <typename T>
-  void operator()(const ImageGeom& imageGeom, IDataArray* inputIDataArray, int32 comparisonAlgorithm, float thresholdValue, bool loopUntilDone, ...)
+  void operator()(const ImageGeom& imageGeom, IDataArray* inputIDataArray, int32 comparisonAlgorithm, float thresholdValueFloat, bool loopUntilDone, ...)
   {
     const auto& inputStore = inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
+    const T thresholdValue = static_cast<T>(thresholdValueFloat);
@@ -148,7 +148,7 @@ struct ExecuteTemplate
           row = (voxelIndex / dims[0]) % dims[1];
           plane = voxelIndex / (dims[0] * dims[1]);
           count++;
-          float32 best = inputStore[voxelIndex];
+          T best = inputStore[voxelIndex];
```

### Coordination
**Land via in-flight branches.** Push to merge `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation` (already contains the fix). Forward-port the same `T best` typing fix into `joey/ooc-filter-optimizations` (still buggy there because it branched before `d6ec06d5f`). Add the failing test fixture to the validation branch's test rewrite (commits `56a253826` / `a49187007`) before that branch opens its PR.

---

## Bug 8 — ITKImageWriterFilter unguarded `dynamic_cast`

### Summary
**File:** `src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters/ITKImageWriterFilter.cpp`
**Origin:** PR #1555 (commit `cefee380f`, merged 2026-03-05, "ENH: Again require in-memory data for ITK filters."). The PR's *intent* was correct — re-assert in-memory requirement, with new error code `k_OutOfCoreDataNotSupported = -2002` and preflight gates added to `ITK::DataCheck` and `ITK::Execute`. **The bug:** ITKImageWriter bypasses those helpers (uses its own `SaveImageData` → `ArraySwitchFunc` path), so it never gets the gate.

### Evidence

Line 138:
```cpp
133  template <class PixelT, uint32 Dimensions>
134  Result<> WriteImage(IDataStore& dataStore, const ITK::ImageGeomData& imageGeom, const fs::path& filePath, uint64 indexOffset)
135  {
136    using ImageType = itk::Image<PixelT, Dimensions>;
137
138    auto& typedDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<PixelT>>&>(dataStore);   // <-- PR #1555: was AbstractDataStore<...>, now concrete DataStore<...>
```

PR #1555's diff:
```diff
-  auto& typedDataStore = dynamic_cast<AbstractDataStore<ITK::UnderlyingType_t<PixelT>>&>(dataStore);
+  auto& typedDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<PixelT>>&>(dataStore);
```

PR #1555 *also* added a matching preflight/execute guard in `ITK::DataCheck` (line 858-862) and `ITK::Execute` (line 880-883):
```cpp
858    if(!inputArray.getDataFormat().empty())
859    {
860      return MakeErrorResult<OutputActions>(Constants::k_OutOfCoreDataNotSupported,
861                                            fmt::format("Input Array '{}' utilizes out-of-core data. ...", ...));
862    }
```

But `ITKImageWriterFilter` does **not** call `ITK::DataCheck` or `ITK::Execute` — it dispatches directly through `ITK::ArraySwitchFunc<WriteImageFunctor, ArrayOptionsType>(...)` at line 258 in its custom `SaveImageData` helper. So the OOC guard added in PR #1555 to other ITK filters never runs for the writer.

`preflightImpl()` (lines 332–389) checks dimension match, fill-char size, computes a preview — but **zero** check on the input array's storage backend. Compare to `ITKMaskImageFilter` (line 197) which delegates to `ITK::DataCheck`, getting the OOC guard for free.

`HDF5ChunkedStore` is a *sibling* of `DataStore` under `AbstractDataStore`, so the cast throws `std::bad_cast`. The exception is **not caught** in the `ArraySwitchFunc → WriteImageFunctor::operator() → WriteImage` path. The only `try/catch` blocks in this file (lines 62–77 and 106–119) catch `itk::ExceptionObject`, not `std::bad_cast`.

### User-visible consequence

- **At preflight:** clean (no error).
- **At execute:** filter runs through the slicing loop until the first call to `WriteImage`, then throws an uncaught `std::bad_cast`. The filter framework catches `std::exception` and converts to a generic `Result<>` error like `"Unhandled exception during execution: std::bad_cast"` — unhelpful and not pointing to the OOC limitation.
- **Strictly worse than pre-PR-#1489 behavior:** before PR #1489 deleted the explicit in-memory check, the filter would have produced a clean error. Today's behavior is preflight-passes-then-execute-crashes.

### Failing Catch2 test (proposal)

Add to `src/Plugins/ITKImageProcessing/test/ITKImageWriterTest.cpp`. Requires the `simplnx-ooc` library to be loaded so the `"HDF5-OOC"` factory is registered (the test binary already loads all plugins via `UnitTest::LoadPlugins()`).

```cpp
#include "simplnx/DataStructure/IDataStore.hpp"

TEST_CASE("ITKImageProcessing::ITKImageWriterFilter: OOC Input Rejected At Preflight", "[ITKImageProcessing][ITKImageWriterFilter][OOC]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();

  // Force OOC storage so the cell array is backed by HDF5ChunkedStore.
  // Threshold of 8 bytes guarantees a 4x4x4 uint8 array (64 bytes) lands in OOC.
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 8, true);

  DataStructure dataStructure;
  const DataPath imageGeomPath({"ImageGeometry"});
  const DataPath cellAttrMatPath = imageGeomPath.createChildPath(ImageGeom::k_CellAttributeMatrixName);
  const DataPath imageDataPath = cellAttrMatPath.createChildPath("ImageData");

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, imageGeomPath.getTargetName());
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 4, 4});
  imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f});

  auto* cellAttrMat = AttributeMatrix::Create(dataStructure, cellAttrMatPath.getTargetName(),
                                              AttributeMatrix::ShapeType{4, 4, 4}, imageGeomPtr->getId());
  REQUIRE(cellAttrMat != nullptr);
  imageGeomPtr->setCellData(*cellAttrMat);

  auto* iocCollection = app->getIOCollection();
  auto store = iocCollection->createDataStore(IDataStore::StoreType::OutOfCore, DataType::uint8,
                                              {4, 4, 4}, {1});
  REQUIRE(store != nullptr);
  auto* imageDataArray = DataArray<uint8>::Create(dataStructure, imageDataPath.getTargetName(),
                                                  std::move(store), cellAttrMat->getId());
  REQUIRE(imageDataArray != nullptr);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(imageDataPath));
  const auto& imageArrayCheck = dataStructure.getDataRefAs<IDataArray>(imageDataPath);
  REQUIRE(imageArrayCheck.getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);

  const std::string tempDir = fmt::format("{}/itk_writer_ooc_test", unit_test::k_BinaryTestOutputDir.view());
  fs::create_directories(tempDir);
  const fs::path outputPath = fs::path(tempDir) / "slice.tif";

  ITKImageWriterFilter filter;
  Arguments args;
  args.insertOrAssign(ITKImageWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(ITKImageWriterFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(imageDataPath));
  args.insertOrAssign(ITKImageWriterFilter::k_FileName_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(ITKImageWriterFilter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriterFilter::k_Plane_Key, std::make_any<uint64>(ITKImageWriterFilter::k_XYPlane));
  args.insertOrAssign(ITKImageWriterFilter::k_TotalIndexDigits_Key, std::make_any<Int32Parameter::ValueType>(3));
  args.insertOrAssign(ITKImageWriterFilter::k_LeadingDigitCharacter_Key, std::make_any<StringParameter::ValueType>("0"));

  // Preflight must reject the OOC input cleanly.
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() >= 1);
  CHECK(preflightResult.outputActions.errors()[0].code ==
        nx::core::ITK::Constants::k_OutOfCoreDataNotSupported);  // -2002

  // Execute must NOT escape with std::bad_cast.
  REQUIRE_NOTHROW([&]() {
    auto executeResult = filter.execute(dataStructure, args);
    if(executeResult.result.invalid())
    {
      CHECK(executeResult.result.errors()[0].code ==
            nx::core::ITK::Constants::k_OutOfCoreDataNotSupported);
    }
  }());

  std::error_code ec;
  fs::remove_all(tempDir, ec);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

### Proposed minimal fix

**Recommendation: Option A (preflight gate).** Mirror the same `getDataFormat().empty()` guard PR #1555 added to `ITK::DataCheck`/`ITK::Execute`. Consistent with PR #1555's design intent, low risk, ~7 lines.

**Why not Option B (chunk-aware writing):** would re-introduce the exact OOC-copy code path PR #1555 deliberately removed. Belongs in a separate enhancement PR if a future use case justifies OOC writer support.

```diff
--- a/src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters/ITKImageWriterFilter.cpp
+++ b/src/Plugins/ITKImageProcessing/src/ITKImageProcessing/Filters/ITKImageWriterFilter.cpp
@@ -345,6 +345,13 @@ IFilter::PreflightResult ITKImageWriterFilter::preflightImpl(const DataStructure
   const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
   // Stored slowest to fastest i.e. Z Y X
   const auto& imageArray = dataStructure.getDataRefAs<IDataArray>(imageArrayPath);

+  // ITK filters require in-memory data. Reject out-of-core arrays at preflight
+  // rather than throwing std::bad_cast at execute time. (Mirrors the OOC guard
+  // added in PR #1555 to ITK::DataCheck / ITK::Execute, which this filter bypasses.)
+  if(!imageArray.getDataFormat().empty())
+  {
+    return {MakeErrorResult<OutputActions>(nx::core::ITK::Constants::k_OutOfCoreDataNotSupported,
+                                           fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageArrayPath.toString()))};
+  }
+
   const IDataStore& imageArrayStore = imageArray.getIDataStoreRef();

   if(!ITK::DoDimensionsMatch(imageArrayStore, imageGeom))
```

`ITKArrayHelper.hpp` is already `#include`d at line 3, so `nx::core::ITK::Constants::k_OutOfCoreDataNotSupported` is in scope. Optionally a matching guard at the top of `executeImpl` provides defense-in-depth, but is strictly redundant once preflight is in place.

---

## In-flight branch coordination

Both relevant remote branches need attention as part of the bug-fix landing plan:

| Branch | Latest relevant commit | What it has | What it needs |
|---|---|---|---|
| `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation` | `d6ec06d5f` "REV: Improve ReplaceElementAttributesWithNeighborValues algorithm quality" | Bug 7 fix (`T best`, `T thresholdValue`, templated comparators); test rewrite | Add Bug 6 defensive `std::fill` reset; add Bug 7 failing-test fixture; add Bug 6 invariant-pinning fixture |
| `joey/ooc-filter-optimizations` | `05bbd3277` "PERF: Out-of-core (OOC) optimized algorithms ..." | Per-Z-slice processing (incidentally fixes Bug 6 by construction) | **Forward-port Bug 7's `T best` typing fix** — currently re-introduces `float32 best` in slice-buffered loop |
| `Matthew/mtr/MultiThresholdObjects` | `9b3fe3dd3` "Fixed MultiThresholdObjects ThresholdSets algorithm" | Removes both `std::reverse` calls; unifies apply logic | Add Bug 1 failing-test fixture; review and merge |

Fork point of the two `joey/*` branches: `f7d4f6ffe` (Feb 2026). They have not been merged together.

## Next steps

1. **Bundle PRs A–D** as described in the plan above. Coordinate PR A (sibling divisor fix) and PR C (FillBadData hotfixes) for the next release window — both contain production-relevant fixes.
2. **Open companion issue for PR #1515 algorithm review.** The 11 additional suspect patterns surfaced during FillBadData triage all need independent verification by a qualified reviewer; the entire AI-generated rewrite is a V&V red flag.
3. **Coordinate with `joey/*` branch authors** to land Bugs 6 and 7 through their existing work rather than creating parallel hotfixes.
4. **After fixes land, update `docs/vv_retroactive_reports/INDEX.md`** to reflect each bug's resolution status (clear the bug flags; add commit references).
