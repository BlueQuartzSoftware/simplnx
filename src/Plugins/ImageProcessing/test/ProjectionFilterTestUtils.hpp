#pragma once

#include <catch2/catch.hpp>

#include "ItkGoldenTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/core.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>

namespace projection_test
{
using namespace nx::core;

/**
 * @brief Legacy-vs-new parity grid shared by every axis-projection filter (Maximum, Minimum, Mean, ...).
 *
 * For every combination of ProjectionDimension in {0,1,2} and RemoveOriginalGeometry ("Perform In-Place")
 * in {true,false}, this builds byte-identical input into two DataStructures via @p buildImage, runs the
 * legacy ITK projection filter (created at runtime by @p legacyUuid, so the ImageProcessing test target
 * does not link ITKImageProcessing) and the new ImageProcessing filter @p NewFilterT with identical
 * Arguments, and asserts the projected output arrays match exactly via CompareDataArrays<T>.
 *
 * Storage is forced in-core so the legacy ITK filter (which rejects out-of-core arrays) can run in any
 * build configuration. Reusability (the point of this helper) comes from three injection points so Task 7's
 * other projection filters reuse the same body instead of re-copying the parity boilerplate:
 *   - @p NewFilterT   supplies the parameter-key constants (identical across all projection filters);
 *   - @p T            is the element/output type the parity is compared at;
 *   - @p buildImage   injects the test image, returning the input array DataPath (the geometry and
 *                     cell-data-matrix paths are derived from it);
 *   - @p setExtraParams sets any filter-specific parameters (empty for the type-preserving reducers).
 *
 * The new and legacy projection filters share the exact same parameter-key strings, so a single Arguments
 * drives both.
 *
 * @p compare is the output-array comparator, defaulting to an EXACT CompareDataArrays<T> (correct for the
 * type-preserving reducers like Max). Reducers that produce rounded/derived values (Mean/StdDev/Sum) inject
 * a tolerance comparator here instead of forking this helper.
 */
template <class NewFilterT, class T>
inline void RunProjectionParityGrid(
    const Uuid& legacyUuid, const std::function<DataPath(DataStructure&)>& buildImage, const std::function<void(Arguments&)>& setExtraParams = [](Arguments&) {},
    const std::function<void(const IDataArray&, const IDataArray&)>& compare = [](const IDataArray& newOut, const IDataArray& legacyOut) { UnitTest::CompareDataArrays<T>(newOut, legacyOut); })
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const std::string k_OutputArrayName = "Output";
  const std::string k_OutputGeomName = "Projected Image";

  // Fail fast (once) with a clear message if the legacy plugin is not loaded, rather than deep in a section.
  REQUIRE(Application::Instance()->getFilterList()->createFilter(legacyUuid) != nullptr);

  for(uint32 projDim = 0; projDim <= 2; ++projDim)
  {
    for(const bool performInPlace : {true, false})
    {
      DYNAMIC_SECTION("projectionDimension=" << projDim << " performInPlace=" << (performInPlace ? "true" : "false"))
      {
        auto run = [&](IFilter& filter, DataStructure& ds, const DataPath& geomPath, const DataPath& inputPath) {
          Arguments args;
          args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
          args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
          args.insertOrAssign(NewFilterT::k_ProjectionDimension_Key, std::make_any<uint32>(projDim));
          args.insertOrAssign(NewFilterT::k_RemoveOriginalGeometry_Key, std::make_any<bool>(performInPlace));
          args.insertOrAssign(NewFilterT::k_OutputImageGeomName_Key, std::make_any<std::string>(k_OutputGeomName));
          args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>(k_OutputArrayName));
          setExtraParams(args);
          auto preflightResult = filter.preflight(ds, args);
          SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
          auto executeResult = filter.execute(ds, args);
          SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
        };

        DataStructure newDs;
        const DataPath newInput = buildImage(newDs);
        const DataPath cellDataPath = newInput.getParent();
        const DataPath geomPath = cellDataPath.getParent();
        NewFilterT newFilter;
        run(newFilter, newDs, geomPath, newInput);

        IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(legacyUuid);
        REQUIRE(legacyFilter != nullptr);
        DataStructure legacyDs;
        const DataPath legacyInput = buildImage(legacyDs);
        run(*legacyFilter, legacyDs, legacyInput.getParent().getParent(), legacyInput);

        // In-place: the projected output ends in the ORIGINAL geometry (the deferred swap renamed the
        // freshly-built collapsed geometry into its place). New-geometry: it is under the created geometry.
        const std::string cellDataName = cellDataPath.getTargetName();
        const DataPath outputGeomPath = performInPlace ? geomPath : DataPath({k_OutputGeomName});
        const DataPath outputArrayPath = outputGeomPath.createChildPath(cellDataName).createChildPath(k_OutputArrayName);

        const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputArrayPath);
        const auto& newOut = newDs.getDataRefAs<IDataArray>(outputArrayPath);
        REQUIRE(legacyOut.getDataType() == newOut.getDataType());
        // Default: exact voxel-for-voxel parity; a tolerance comparator can be injected for rounding reducers.
        compare(newOut, legacyOut);
      }
    }
  }
}

/**
 * @brief Streams two Float64 arrays and asserts every element agrees within a mixed absolute/relative
 *        tolerance: |computed - expected| <= absTol + relTol * |expected|. Suited to the derived
 *        projection reducers (Mean/StdDev/Sum) whose Float64 output is only ALGEBRAICALLY (not
 *        bit-identically) equal to the legacy ITK two-pass computation, and whose magnitudes can be large
 *        (a plain absolute EPSILON would be far too tight for e.g. a sum over a long column). Both arrays
 *        are read in bounded chunks via copyIntoBuffer so it is fast for in-core AND out-of-core stores
 *        (it never touches an element at a time). The relative term makes the tolerance scale with the
 *        expected magnitude; the absolute term keeps values near zero from demanding exact equality.
 */
inline void CompareFloat64ArraysWithinTolerance(const IDataArray& computed, const IDataArray& expected, float64 absTol = 1.0e-6, float64 relTol = 1.0e-9)
{
  const auto& computedStore = computed.getIDataStoreRefAs<AbstractDataStore<float64>>();
  const auto& expectedStore = expected.getIDataStoreRefAs<AbstractDataStore<float64>>();
  const usize totalSize = computedStore.getSize();
  INFO(fmt::format("Tolerance comparison of '{}' (computed) vs '{}' (expected)", computed.getName(), expected.getName()));
  REQUIRE(totalSize == expectedStore.getSize());

  constexpr usize k_ChunkSize = 40000;
  auto computedBuf = std::make_unique<float64[]>(k_ChunkSize);
  auto expectedBuf = std::make_unique<float64[]>(k_ChunkSize);

  bool failed = false;
  usize failIndex = 0;
  float64 failComputed = 0.0;
  float64 failExpected = 0.0;
  float64 failTol = 0.0;

  for(usize offset = 0; offset < totalSize && !failed; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, totalSize - offset);
    Result<> computedReadResult = computedStore.copyIntoBuffer(offset, nonstd::span<float64>(computedBuf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(computedReadResult);
    Result<> expectedReadResult = expectedStore.copyIntoBuffer(offset, nonstd::span<float64>(expectedBuf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(expectedReadResult);
    for(usize i = 0; i < count; ++i)
    {
      const float64 a = computedBuf[i];
      const float64 b = expectedBuf[i];
      if(std::isnan(a) && std::isnan(b))
      {
        continue;
      }
      const float64 tol = absTol + relTol * std::abs(b);
      if(std::abs(a - b) > tol)
      {
        failed = true;
        failIndex = offset + i;
        failComputed = a;
        failExpected = b;
        failTol = tol;
        break;
      }
    }
  }

  if(failed)
  {
    UNSCOPED_INFO(fmt::format("index={}: |{} - {}| = {} exceeds tolerance {}", failIndex, failComputed, failExpected, std::abs(failComputed - failExpected), failTol));
  }
  REQUIRE(!failed);
}

/**
 * @brief Computed-expected parity grid for projection filters whose LEGACY ITK counterpart is unavailable
 *        at runtime (the legacy ITK Sum and Standard Deviation projection filters are commented out of the
 *        ITKImageProcessing FilterList, so createFilter() returns null for them), meaning correctness must
 *        be gated on an INDEPENDENT closed-form oracle rather than legacy parity. Over ProjectionDimension
 *        in {0,1,2} x Perform-In-Place in {true,false}, it builds the input via @p buildImage, runs
 *        @p NewFilterT, resolves the collapsed Float64 output array (in the original geometry for an
 *        in-place run, or under @p outputGeomName otherwise), and asserts every output slot equals
 *        @p expectedForSlot(projDim, slot) within a mixed absolute/relative tolerance. This exercises BOTH
 *        output modes of the shared façade with the new filter's own reduce functor. Storage is forced
 *        in-core (small correctness grid); the large out-of-core behavior is covered by each filter's
 *        dedicated 200^3 test.
 */
template <class NewFilterT>
inline void RunProjectionComputedExpectedGrid(const std::function<DataPath(DataStructure&)>& buildImage, const std::function<float64(uint32 projDim, usize slot)>& expectedForSlot,
                                              float64 absTol = 1.0e-6, float64 relTol = 1.0e-9)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const std::string k_OutputArrayName = "Output";
  const std::string k_OutputGeomName = "Projected Image";

  for(uint32 projDim = 0; projDim <= 2; ++projDim)
  {
    for(const bool performInPlace : {true, false})
    {
      DYNAMIC_SECTION("projectionDimension=" << projDim << " performInPlace=" << (performInPlace ? "true" : "false"))
      {
        DataStructure ds;
        const DataPath inputPath = buildImage(ds);
        const DataPath cellDataPath = inputPath.getParent();
        const DataPath geomPath = cellDataPath.getParent();
        const std::string cellDataName = cellDataPath.getTargetName();

        NewFilterT filter;
        Arguments args;
        args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
        args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
        args.insertOrAssign(NewFilterT::k_ProjectionDimension_Key, std::make_any<uint32>(projDim));
        args.insertOrAssign(NewFilterT::k_RemoveOriginalGeometry_Key, std::make_any<bool>(performInPlace));
        args.insertOrAssign(NewFilterT::k_OutputImageGeomName_Key, std::make_any<std::string>(k_OutputGeomName));
        args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>(k_OutputArrayName));

        auto preflightResult = filter.preflight(ds, args);
        SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
        auto executeResult = filter.execute(ds, args);
        SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

        const DataPath outputGeomPath = performInPlace ? geomPath : DataPath({k_OutputGeomName});
        const DataPath outputArrayPath = outputGeomPath.createChildPath(cellDataName).createChildPath(k_OutputArrayName);
        const auto& outStore = ds.getDataRefAs<DataArray<float64>>(outputArrayPath).getDataStoreRef();
        const usize numSlots = outStore.getSize();
        REQUIRE(numSlots > 0);

        constexpr usize k_ChunkValues = 65536;
        auto buffer = std::make_unique<float64[]>(std::min(k_ChunkValues, numSlots));
        bool allMatch = true;
        usize badSlot = 0;
        float64 badGot = 0.0;
        float64 badExp = 0.0;
        for(usize start = 0; start < numSlots && allMatch; start += k_ChunkValues)
        {
          const usize count = std::min(k_ChunkValues, numSlots - start);
          Result<> copyResult = outStore.copyIntoBuffer(start, nonstd::span<float64>(buffer.get(), count));
          SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
          for(usize i = 0; i < count; ++i)
          {
            const float64 expected = expectedForSlot(projDim, start + i);
            const float64 tol = absTol + relTol * std::abs(expected);
            if(std::abs(buffer[i] - expected) > tol)
            {
              allMatch = false;
              badSlot = start + i;
              badGot = buffer[i];
              badExp = expected;
              break;
            }
          }
        }
        if(!allMatch)
        {
          UNSCOPED_INFO(fmt::format("projDim={} slot={} got={} expected={}", projDim, badSlot, badGot, badExp));
        }
        REQUIRE(allMatch);
        UnitTest::CheckArraysInheritTupleDims(ds);
      }
    }
  }
}

//------------------------------------------------------------------------------
// ITK-sourced real-image golden helpers (plan Task 5). Each duplicates one legacy ITK*ProjectionImageTest case
// on OUR ITK-free projection filter, carrying TWO oracles: (A) a durable golden -- a committed md5 for the
// type-preserving reducers, or a stored baseline for the Float64 reducers -- and (B) live-ITK parity
// (coexistence only, dropped at ITK removal). The whole case is pinned ForceInCore: reading via OUR readers is
// OOC-safe, but (B) runs the legacy ITK filter, which dynamic_casts buffers to the in-core DataStore and
// bad_casts an out-of-core store.
enum class LiveItkParity
{
  Required,               // the legacy ITK filter MUST be creatable and match (Max/Min/Median/Mean/Binary).
  OptionalRecordIfMissing // legacy ITK NX filter may be unregistered (Sum/StandardDeviation) -> (A)-only, recorded.
};

// md5-golden projection case (type-preserving reducers Max/Min/Median/Binary). Reads @p inputFile through OUR
// reader, runs @p FilterT with the ITK case's parameters, asserts the output md5 equals @p committedMd5 (A),
// then requires BIT-EXACT parity with the live legacy ITK filter on the same input (B). @p setExtraParams sets
// filter-specific parameters (Binary's foreground/background); it is a no-op for the plain reducers. The output
// location follows the geometry mode: in-place -> the (now-collapsed) original geometry; new-geometry -> the
// created geometry (the ITK "New Geometry" test rung).
template <class FilterT>
inline void RunProjectionMd5ItkGolden(
    const std::string& inputFile, uint32 projectionDimension, bool removeOriginalGeometry, const std::string& committedMd5, const std::function<void(Arguments&)>& setExtraParams = [](Arguments&) {})
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const std::string k_NewGeomName = "New Image Geometry";

  const auto makeArgs = [&] {
    Arguments args;
    args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
    args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
    args.insertOrAssign(FilterT::k_ProjectionDimension_Key, std::make_any<uint32>(projectionDimension));
    args.insertOrAssign(FilterT::k_RemoveOriginalGeometry_Key, std::make_any<bool>(removeOriginalGeometry));
    args.insertOrAssign(FilterT::k_OutputImageGeomName_Key, std::make_any<std::string>(k_NewGeomName));
    args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    setExtraParams(args);
    return args;
  };

  const DataPath output = removeOriginalGeometry ? cellData.createChildPath("Output") : DataPath({k_NewGeomName, "CellData", "Output"});

  // --- run OUR filter (bind every Result to a local: SIMPLNX_RESULT_REQUIRE_VALID evaluates its argument
  //     multiple times, so a read/execute call passed inline would run twice and the second run would fail) ---
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  const Arguments args = makeArgs();
  FilterT filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE golden: md5 (plan Sec.4 md5-validity-first) ---
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("projection md5: ours='{}' committed='{}' (input={} projDim={} inPlace={})", ourMd5, committedMd5, inputFile, projectionDimension, removeOriginalGeometry));
  REQUIRE(ourMd5 == committedMd5);

  // --- (B) LIVE-ITK bit-exact parity (coexistence only): identical bytes -> identical md5 ---
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<FilterT>::uuid);
  REQUIRE(legacyUuid.has_value());
  DataStructure itkDs;
  const Result<> readItkInput = ip_golden::ReadInputImage(itkDs, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readItkInput);
  const Arguments itkArgs = makeArgs();
  const Result<> itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, itkDs, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const auto& ourOut = ds.getDataRefAs<IDataArray>(output);
  const auto& itkOut = itkDs.getDataRefAs<IDataArray>(output);
  REQUIRE(ourOut.getDataType() == itkOut.getDataType());
  REQUIRE(ip_golden::ComputeMd5Hash(itkDs, output) == ourMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// baseline-golden projection case (Float64 reducers Mean/Sum/StandardDeviation). Reads @p inputFile, projects
// @p projectionDimension in-place, then CompareImages the collapsed output against the stored @p baselineFile at
// @p tolerance (A). Because a projection collapses one axis, the OUTPUT geometry (in-place: the original
// geometry, now reduced) owns the output array and must match the baseline's reduced dims -- so the input
// geometry path IS the output geometry after the in-place collapse. (B) is a Float64 tolerant parity when the
// legacy filter is registered; the legacy NX Sum / Standard Deviation projection filters are commented out of
// the ITKImageProcessing FilterList, so those cases are (A)-only and recorded via @p liveMode.
template <class FilterT>
inline void RunProjectionBaselineItkGolden(const std::string& inputFile, const std::string& baselineFile, uint32 projectionDimension, float64 tolerance, LiveItkParity liveMode)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  const auto makeArgs = [&] {
    Arguments args;
    args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
    args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
    args.insertOrAssign(FilterT::k_ProjectionDimension_Key, std::make_any<uint32>(projectionDimension));
    args.insertOrAssign(FilterT::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true)); // in-place: output owns the collapsed geometry
    args.insertOrAssign(FilterT::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
    args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    return args;
  };

  // --- run OUR filter (bind every Result to a local: SIMPLNX_RESULT_REQUIRE_VALID evaluates its argument
  //     multiple times, so a read/execute call passed inline would run twice and the second run would fail) ---
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  const Arguments args = makeArgs();
  FilterT filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE golden: baseline compare (output geometry == collapsed input geometry) ---
  const DataPath bGeom({"Baseline Geometry"});
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, tolerance);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // --- (B) LIVE-ITK Float64 tolerant parity (coexistence only); (A)-only when the legacy NX filter is unregistered ---
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<FilterT>::uuid);
  bool ranLiveItk = false;
  if(legacyUuid.has_value())
  {
    DataStructure itkDs;
    const Result<> readItkInput = ip_golden::ReadInputImage(itkDs, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
    SIMPLNX_RESULT_REQUIRE_VALID(readItkInput);
    const Arguments itkArgs = makeArgs();
    const Result<> itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, itkDs, itkArgs);
    if(itkRun.valid())
    {
      CompareFloat64ArraysWithinTolerance(ds.getDataRefAs<IDataArray>(output), itkDs.getDataRefAs<IDataArray>(output));
      ranLiveItk = true;
    }
  }
  if(liveMode == LiveItkParity::Required)
  {
    REQUIRE(ranLiveItk);
  }
  else if(!ranLiveItk)
  {
    WARN(fmt::format("(B) live-ITK parity SKIPPED (legacy NX projection filter unregistered): case is (A)-only. input={} baseline={}", inputFile, baselineFile));
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace projection_test
