#pragma once

#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ip_test
{
using namespace nx::core;

/**
 * @brief Builds an ImageGeom "Image Geometry" with cell AttributeMatrix "CellData" and a cell array
 *        "Input" of element type T, filled with a ramp: value(i) = startValue + step * i.
 *        Filled via bulk copyFromBuffer in bounded chunks so it is fast in-core AND out-of-core
 *        (never a per-element setValue loop) and allocates only a fixed chunk buffer.
 *        The element type is a template parameter so integer-only filters (e.g. Not) can use int32,
 *        and `step` lets domain-restricted ops (Acos/Asin need [-1,1], Log/Sqrt need >=0) get in-range data.
 */
template <class T>
inline DataPath BuildRampImage(DataStructure& ds, usize dim, double startValue = 0.0, double step = 1.0)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<T>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const usize total = ref.getSize();
  std::cout << "  [build] filling " << dim << "^3 ramp (" << total << " values) via bulk I/O..." << std::endl;
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(k_ChunkValues);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, total - start);
    for(usize i = 0; i < count; ++i)
    {
      buffer[i] = static_cast<T>(startValue + step * static_cast<double>(start + i));
    }
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return inputPath;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" with cell AttributeMatrix "CellData" and a SCALAR (1-component)
 *        cell array "Input" of element type T holding exactly @p values (one cell per value). Where
 *        @ref BuildRampImage lays down a monotonic ramp, this pins ARBITRARY values so a test can place an input
 *        exactly on a math domain edge (|x|>1 for acos/asin, a negative for sqrt/log, ~pi/2 for tan, a huge
 *        magnitude for exp/square). The geometry's cell count equals values.size(), so the shared
 *        PreflightImageFilter geometry check passes. Values are written with per-element setValue (the count is
 *        tiny, so the bulk-I/O ramp path is unnecessary here).
 */
template <class T>
inline DataPath BuildScalarValuesImage(DataStructure& ds, const std::vector<T>& values)
{
  const usize n = values.size();
  const ShapeType cellShape = {1, 1, n};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({n, 1, 1});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<T>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < n; ++i)
  {
    ref.setValue(i, values[i]);
  }
  return inputPath;
}

/**
 * @brief Runs a pointwise unary ImageProcessing filter (preflight + execute) on @p inputPath, writing the result
 *        to sibling array "Output", and returns that output DataPath. Requires the three boilerplate key constants
 *        every pointwise filter exposes. The caller owns LoadPlugins()/storage-mode setup (a domain-edge test pins
 *        ForceInCore); this is the no-legacy analogue of @ref RunLegacyParity used to check a single crafted input.
 */
template <class FilterT>
inline DataPath RunPointwiseFilter(DataStructure& ds, const DataPath& inputPath)
{
  FilterT filter;
  Arguments args;
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return inputPath.replaceName("Output");
}

/**
 * @brief Runs the new ImageProcessing filter and the legacy ITK filter (created at runtime by UUID)
 *        on identical in-memory ramp data and asserts their outputs match within CompareDataArrays
 *        tolerance (NaN == NaN is treated equal, so out-of-domain tails still compare as parity).
 *        Storage is forced in-core so the legacy ITK filter (which rejects OOC data) can run in any
 *        build configuration.
 *
 * @tparam NewFilterT  the new ImageProcessing filter class (must expose the three boilerplate key constants)
 * @tparam InputT      element type of the input ramp (float32 for most; int32 for integer-only Not)
 * @param legacyUuid   the legacy ITK filter's simplnx UUID (created via the FilterList at runtime)
 * @param setParams    sets the filter-specific parameters on the Arguments (empty for no-param filters)
 */
template <class NewFilterT, class InputT = float32>
inline void RunLegacyParity(const Uuid& legacyUuid, const std::function<void(Arguments&)>& setParams, usize dim = 12, double startValue = 0.0, double step = 1.0)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto run = [&](IFilter& filter, DataStructure& ds, const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    setParams(args);
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  DataStructure newDs;
  const DataPath newInput = BuildRampImage<InputT>(newDs, dim, startValue, step);
  NewFilterT newFilter;
  run(newFilter, newDs, newInput);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(legacyUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = BuildRampImage<InputT>(legacyDs, dim, startValue, step);
  run(*legacyFilter, legacyDs, legacyInput);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(legacyOut.getDataType() == newOut.getDataType());
  // Compares every array under CellData (Input is identical; Output is the real check), dispatching on
  // the actual DataType so it works for float64 / uint8 / same-as-input outputs alike.
  UnitTest::CompareExemplarToGeneratedData(newDs, legacyDs, DataPath({"Image Geometry", "CellData"}), "Image Geometry");
}
} // namespace ip_test
