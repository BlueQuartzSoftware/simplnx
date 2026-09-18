#pragma once

#include <catch2/catch.hpp>

#include "ItkGoldenTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ContourEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Reusable test utilities for the radius-1 CONTOUR engine + its filters. Task 2 (Binary Contour) uses these;
// Task 3 (Label Contour) reuses the SAME oracle/builders/parity driver with its own predicate + param setter,
// so everything here is generic over the per-voxel predicate and the filter's parameter-key constants.
namespace contour_test
{
using namespace nx::core;
using nx::core::ImageProcessing::BinaryContourPredicate;
using nx::core::ImageProcessing::MakeContourNeighborOffsets;
using nx::core::ImageProcessing::SEOffset;

//------------------------------------------------------------------------------
// simplnx flat index for voxel (x,y,z): X fastest-moving, then Y, then Z.
inline usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

//------------------------------------------------------------------------------
/**
 * @brief Deterministic BINARY volume: a coarse 3-voxel-block 3D checkerboard alternating @p fg / @p bg. The
 *        3-voxel blocks are large enough that a fg block has genuine interior voxels (their face-neighbors are
 *        all foreground, so they must become background), while every block boundary is a fg/bg contour, and
 *        the block at the origin TOUCHES the image edges (so the "out-of-bounds neighbors are ignored" border
 *        rule is exercised). Only the two values @p fg / @p bg appear.
 */
template <class T>
inline std::vector<T> MakeContourPattern(usize dimX, usize dimY, usize dimZ, T fg, T bg)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const bool on = (((x / 3) + (y / 3) + (z / 3)) % 2 == 0);
        v[FlatIndex(x, y, z, dimX, dimY)] = on ? fg : bg;
      }
    }
  }
  return v;
}

/**
 * @brief Deterministic MULTI-LABEL volume cycling {bg=0, fg=1, 2, 3} by coarse 3-voxel block, so non-foreground
 *        labels (2, 3) are present and must be PASSED THROUGH unchanged (never treated as background) -- the key
 *        distinguishing behavior of the contour predicate versus binary morphology. Foreground is label 1.
 */
template <class T>
inline std::vector<T> MakeMultiLabelContourPattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize label = ((x / 3) + (y / 3) + (z / 3)) % 4; // 0(bg),1(fg),2,3
        v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(label);
      }
    }
  }
  return v;
}

//------------------------------------------------------------------------------
/**
 * @brief Independent CONTOUR oracle. For each voxel it gathers ONLY the in-bounds neighbors of
 *        @p neighborOffsets (out-of-bounds offsets dropped, exactly like the engine and ITK) via a plain
 *        triple-nested loop, then applies @p predicate. Deliberately a different traversal from the engine's
 *        Z-slab streaming / parallel plane body, so a slab/stride/transpose/OOB bug in the engine cannot hide.
 *        Generic over the predicate, so Task 3's Label contour reuses it by passing its own predicate.
 */
template <class T, class PredicateT>
inline std::vector<T> ContourGatherOracle(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const std::vector<SEOffset>& neighborOffsets, const PredicateT& predicate)
{
  std::vector<T> output(input.size());
  std::vector<T> neighbors;
  neighbors.reserve(neighborOffsets.size());
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const T center = input[FlatIndex(x, y, z, dimX, dimY)];
        neighbors.clear();
        for(const SEOffset& off : neighborOffsets)
        {
          const int64 nx = static_cast<int64>(x) + off.dx;
          const int64 ny = static_cast<int64>(y) + off.dy;
          const int64 nz = static_cast<int64>(z) + off.dz;
          if(nx < 0 || nx >= static_cast<int64>(dimX) || ny < 0 || ny >= static_cast<int64>(dimY) || nz < 0 || nz >= static_cast<int64>(dimZ))
          {
            continue; // out-of-bounds neighbor ignored
          }
          neighbors.push_back(input[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)]);
        }
        output[FlatIndex(x, y, z, dimX, dimY)] = predicate(center, nonstd::span<const T>(neighbors.data(), neighbors.size()));
      }
    }
  }
  return output;
}

//------------------------------------------------------------------------------
/**
 * @brief Builds an ImageGeom "Image Geometry" (dimX x dimY x dimZ) with cell AttributeMatrix "CellData" and a
 *        scalar cell array "Input" of integer type T holding @ref MakeContourPattern (binary fg/bg). Returns the
 *        input array DataPath; the geometry/cell-data paths derive from it. Uses per-element setValue (small
 *        correctness images).
 */
template <class T>
inline DataPath BuildBinaryContourImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ, T foreground, T background)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<T>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const std::vector<T> pattern = MakeContourPattern<T>(dimX, dimY, dimZ, foreground, background);
  for(usize i = 0; i < pattern.size(); ++i)
  {
    ref.setValue(i, pattern[i]);
  }
  return inputPath;
}

/**
 * @brief Like @ref BuildBinaryContourImage but with @ref MakeMultiLabelContourPattern (labels {0,1,2,3}), so a
 *        parity/oracle test can confirm non-foreground labels pass through unchanged. Foreground is label 1.
 */
template <class T>
inline DataPath BuildMultiLabelContourImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<T>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const std::vector<T> pattern = MakeMultiLabelContourPattern<T>(dimX, dimY, dimZ);
  for(usize i = 0; i < pattern.size(); ++i)
  {
    ref.setValue(i, pattern[i]);
  }
  return inputPath;
}

/**
 * @brief Builds a large cubic (dim^3) binary contour image filled via BULK copyFromBuffer (no per-element
 *        setValue loop, so it is fast for large out-of-core volumes): a coarse 8-voxel-block 3D checkerboard of
 *        @p foreground / @p background. Used by the large-OOC filter test. Returns the input array DataPath.
 */
template <class T>
inline DataPath BuildBinaryContourImageBulk(DataStructure& ds, usize dim, T foreground, T background)
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
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(k_ChunkValues);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, total - start);
    for(usize i = 0; i < count; ++i)
    {
      const usize flatIndex = start + i;
      const usize x = flatIndex % dim;
      const usize y = (flatIndex / dim) % dim;
      const usize z = flatIndex / (dim * dim);
      buffer[i] = (((x / 8) + (y / 8) + (z / 8)) % 2 == 0) ? foreground : background;
    }
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return inputPath;
}

/**
 * @brief Builds a large cubic (dim^3) MULTI-LABEL contour image filled via BULK copyFromBuffer: a coarse
 *        8-voxel-block 3D pattern cycling labels {0(bg), 1, 2, 3} so several distinct regions share faces (and
 *        the background region is present). Used by the large-OOC label-contour filter test. Returns the input
 *        array DataPath.
 */
template <class T>
inline DataPath BuildMultiLabelContourImageBulk(DataStructure& ds, usize dim)
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
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<T[]>(k_ChunkValues);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, total - start);
    for(usize i = 0; i < count; ++i)
    {
      const usize flatIndex = start + i;
      const usize x = flatIndex % dim;
      const usize y = (flatIndex / dim) % dim;
      const usize z = flatIndex / (dim * dim);
      buffer[i] = static_cast<T>(((x / 8) + (y / 8) + (z / 8)) % 4); // 0(bg),1,2,3
    }
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return inputPath;
}

//------------------------------------------------------------------------------
/**
 * @brief Sets a contour filter's parameters on a shared Arguments. Injected into the parity driver so Task 3's
 *        Label contour (which has Fully Connected + Background but no Foreground) can reuse the same driver with
 *        its own setter.
 */
using ContourParamSetter = std::function<void(Arguments&)>;

/**
 * @brief Parameter setter for the BINARY contour filter (Fully Connected + Foreground + Background). The legacy
 *        ITK Binary Contour filter shares the same key strings, so the SAME setter drives both the new and the
 *        legacy filter in the parity grid.
 */
template <class NewFilterT>
inline ContourParamSetter BinaryContourParamSetter(bool fullyConnected, float64 foreground, float64 background)
{
  return [fullyConnected, foreground, background](Arguments& args) {
    args.insertOrAssign(NewFilterT::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
    args.insertOrAssign(NewFilterT::k_ForegroundValue_Key, std::make_any<float64>(foreground));
    args.insertOrAssign(NewFilterT::k_BackgroundValue_Key, std::make_any<float64>(background));
  };
}

/**
 * @brief Parameter setter for the LABEL contour filter (Fully Connected + Background; no Foreground). The legacy
 *        ITK Label Contour filter shares the same key strings, so the SAME setter drives both the new and the
 *        legacy filter in the parity grid.
 */
template <class NewFilterT>
inline ContourParamSetter LabelContourParamSetter(bool fullyConnected, float64 background)
{
  return [fullyConnected, background](Arguments& args) {
    args.insertOrAssign(NewFilterT::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
    args.insertOrAssign(NewFilterT::k_BackgroundValue_Key, std::make_any<float64>(background));
  };
}

//------------------------------------------------------------------------------
// A single image-shape parity configuration.
struct ContourCase
{
  const char* label;
  usize dimX;
  usize dimY;
  usize dimZ;
};

namespace detail
{
// Drives BOTH the new NewFilterT and the legacy filter (passed as @p filter) with a shared Arguments over the
// standard geom/input/output keys plus the injected contour parameters, then requires preflight + execute
// succeed. Reuses NewFilterT::k_*_Key for the LEGACY filter too, which is correct only because the legacy and
// new Binary Contour filters share identical parameter-key strings (verified against ITKBinaryContourImageFilter).
template <class NewFilterT>
inline void RunContourFilter(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const ContourParamSetter& setParams)
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args);
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace detail

/**
 * @brief Legacy-vs-new parity grid shared by the contour filters. For each image shape (3D cubic, 3D non-cubic,
 *        and a 2D Z=1 slice) it builds byte-identical input into two DataStructures via @p buildImage, runs the
 *        legacy ITK contour filter (created at runtime by @p legacyUuid, so the ImageProcessing test target does
 *        not link ITKImageProcessing) and the new @p NewFilterT with identical Arguments (set by @p setParams),
 *        and asserts the output arrays match exactly (integer predicate -> exact match).
 *
 * Storage is forced in-core so the legacy ITK filter (which rejects out-of-core arrays) can run in any build
 * configuration; this also exercises the new filter's engine on resident stores.
 */
template <class NewFilterT, class T>
inline void RunContourParityGrid(
    const Uuid& legacyUuid, const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, const ContourParamSetter& setParams,
    const std::function<void(const IDataArray&, const IDataArray&)>& compare = [](const IDataArray& newOut, const IDataArray& legacyOut) { UnitTest::CompareDataArrays<T>(newOut, legacyOut); })
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // Fail fast (once) with a clear message if the legacy plugin is not loaded, rather than deep in a section.
  REQUIRE(Application::Instance()->getFilterList()->createFilter(legacyUuid) != nullptr);

  const std::vector<ContourCase> configs = {{"3D 12x12x12", 12, 12, 12}, {"3D non-cubic 10x8x6", 10, 8, 6}, {"2D Z=1 20x16x1", 20, 16, 1}};

  for(const ContourCase& cfg : configs)
  {
    DYNAMIC_SECTION(cfg.label)
    {
      DataStructure newDs;
      const DataPath newInput = buildImage(newDs, cfg.dimX, cfg.dimY, cfg.dimZ);
      NewFilterT newFilter;
      detail::RunContourFilter<NewFilterT>(newFilter, newDs, newInput, setParams);

      IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(legacyUuid);
      REQUIRE(legacyFilter != nullptr);
      DataStructure legacyDs;
      const DataPath legacyInput = buildImage(legacyDs, cfg.dimX, cfg.dimY, cfg.dimZ);
      detail::RunContourFilter<NewFilterT>(*legacyFilter, legacyDs, legacyInput, setParams);

      const DataPath outputPath({"Image Geometry", "CellData", "Output"});
      const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
      const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
      REQUIRE(newOut.getDataType() == legacyOut.getDataType());
      compare(newOut, legacyOut);
    }
  }
}

//------------------------------------------------------------------------------
// ITK-sourced real-image golden helper (plan Task 6). Duplicates a legacy ITK*ContourImageTest case on OUR
// ITK-free contour filter, carrying TWO oracles: (A) the output md5 == ITK's committed hash, and (B) live-ITK
// BIT-EXACT parity (coexistence only). The contour filters are SameAsInput (output shares the input geometry, no
// axis collapse), so the output lives at "Image Geometry"/"CellData"/"Output". @p setParams injects the case's
// filter parameters (BinaryContourParamSetter / LabelContourParamSetter). The whole case is pinned ForceInCore:
// reading via OUR reader is OOC-safe, but (B) runs the legacy ITK filter, which bad_casts an out-of-core store.
template <class FilterT>
inline void RunContourMd5ItkGolden(const std::string& inputFile, const std::string& committedMd5, const ContourParamSetter& setParams)
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
    args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    setParams(args);
    return args;
  };

  // --- run OUR filter (every Result bound to a local: SIMPLNX_RESULT_REQUIRE_VALID evaluates its argument
  //     multiple times, so an inline read/execute call would run twice and the second run would fail) ---
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
  INFO(fmt::format("contour md5: ours='{}' committed='{}' (input={})", ourMd5, committedMd5, inputFile));
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
} // namespace contour_test
