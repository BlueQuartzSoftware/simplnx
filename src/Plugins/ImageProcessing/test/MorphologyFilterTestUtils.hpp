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
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace morph_test
{
using namespace nx::core;
using nx::core::ImageProcessing::KernelType;
using nx::core::ImageProcessing::MakeStructuringElement;
using nx::core::ImageProcessing::MorphOp;
using nx::core::ImageProcessing::SEOffset;
using nx::core::ImageProcessing::StructuringElement;

//------------------------------------------------------------------------------
// simplnx flat index for voxel (x,y,z): X fastest-moving, then Y, then Z.
inline usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" (dims dimX x dimY x dimZ) with cell AttributeMatrix "CellData"
 *        and a scalar cell array "Input" of element type T, filled with a deterministic non-monotonic pattern
 *        (a gradient + coarse checkerboard + pseudo-noise, all in [0,199] so it is representable in every
 *        scalar type). Non-monotonic so the morphology winner varies per voxel and is not a fixed neighbor.
 *        Returns the input array DataPath; the geometry and cell-data-matrix paths derive from it.
 */
template <class T>
inline DataPath BuildMorphologyImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ)
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

  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize gradient = x + 2 * y + 3 * z;
        const usize block = (((x / 4) + (y / 4) + (z / 4)) % 2 == 0) ? 0 : 41;
        const usize noise = (x * 131 + y * 57 + z * 29) % 37;
        const usize value = (gradient + block + noise) % 200;
        ref.setValue(FlatIndex(x, y, z, dimX, dimY), static_cast<T>(value));
      }
    }
  }
  return inputPath;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" (dim^3) with cell AttributeMatrix "CellData" and a scalar cell
 *        array "Input" of integer element type T, filled via BULK copyFromBuffer (never a per-element setValue
 *        loop, so it is fast for large out-of-core volumes) with a STRICTLY binary pattern: a coarse
 *        8-voxel-block 3D checkerboard alternates @p foreground / @p background (only those two values, as the
 *        binary morphology filters require). The blocks are far larger than any test radius, so solid
 *        all-foreground and all-background windows exist (exercising both the Dilate any() and Erode all()
 *        branches). Returns the input array DataPath.
 */
template <class T>
inline DataPath BuildBinaryPatternImage(DataStructure& ds, usize dim, T foreground, T background)
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
 * @brief Builds an ImageGeom "Image Geometry" (dimX x dimY x dimZ) with cell AttributeMatrix "CellData" and a
 *        scalar cell array "Input" of integer element type T holding a STRICTLY binary label pattern -- only
 *        @p foreground and @p background values, arranged as a coarse 4-voxel-block 3D checkerboard so the
 *        image has solid foreground regions (surviving windows for erosion), solid background regions, and
 *        many fg/bg boundaries (so dilation/erosion meaningfully change values). Returns the input array path.
 *
 * A strictly binary input is REQUIRED for live-legacy parity: the legacy ITK Binary Dilate/Erode filters
 * PRESERVE the original value of any non-foreground input voxel (they only rewrite foreground voxels and fill
 * newly-dilated / newly-eroded voxels), whereas this plugin's binary morphology façade requires a strictly
 * {foreground, background} image and emits one. The two therefore agree exactly only when every non-foreground
 * input voxel already equals @p background -- i.e. a truly binary image. A non-binary (multi-label) input is
 * instead REJECTED at execution by the façade's binary-input safeguard, exercised by each filter's
 * "Rejects non-binary input at execute" test.
 */
template <class T>
inline DataPath BuildBinaryLabelImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ, T foreground, T background)
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

  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const bool on = (((x / 4) + (y / 4) + (z / 4)) % 2 == 0);
        ref.setValue(FlatIndex(x, y, z, dimX, dimY), on ? foreground : background);
      }
    }
  }
  return inputPath;
}

/**
 * @brief Sets the KernelType (ChoicesParameter, uint64) + KernelRadius (VectorUInt32) on a shared Arguments.
 *        Injected into the parity/oracle helpers so Task 6's composite morphology filters (Open/Close/...),
 *        which share the same kernel-parameter shape, can reuse the exact same driver with their own keys.
 */
using KernelParamSetter = std::function<void(Arguments&, uint64 kernelType, const std::vector<uint32>& radius)>;

/**
 * @brief Default kernel-parameter setter for any filter exposing k_KernelType_Key / k_KernelRadius_Key with
 *        the grayscale-morphology semantics (a ChoicesParameter index + an X/Y/Z uint32 radius).
 */
template <class NewFilterT>
inline KernelParamSetter DefaultKernelParamSetter()
{
  return [](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
  };
}

/**
 * @brief Kernel-parameter setter for the COMPOSITE morphology filters, which additionally expose a
 *        Safe Border bool (@c k_SafeBorder_Key). Sets kernel type + radius like @ref DefaultKernelParamSetter
 *        and pins Safe Border to @p safeBorder. The composite legacy filters share the same key string, so
 *        the same setter drives both the new and the legacy filter in the parity grid: pass @c true there
 *        (the ITK default) for exact parity, and @c false to validate the skip-OOB composition oracle.
 */
template <class NewFilterT>
inline KernelParamSetter CompositeKernelParamSetter(bool safeBorder)
{
  return [safeBorder](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(NewFilterT::k_SafeBorder_Key, std::make_any<bool>(safeBorder));
  };
}

/**
 * @brief Kernel-parameter setter for the BINARY morphology filters (Binary Dilate/Erode), which additionally
 *        expose Foreground/Background (Float64) and Boundary To Foreground (Bool). Sets kernel type + radius
 *        like @ref DefaultKernelParamSetter and pins the foreground/background values and the boundary mode.
 *        The legacy ITK binary filters share the same key strings, so this drives both the new and the legacy
 *        filter in the parity grid. Reusable by Task 5's binary composites (same kernel + fg/bg/boundary shape).
 */
template <class NewFilterT>
inline KernelParamSetter BinaryKernelParamSetter(float64 foreground, float64 background, bool boundaryToForeground)
{
  return [foreground, background, boundaryToForeground](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(NewFilterT::k_ForegroundValue_Key, std::make_any<float64>(foreground));
    args.insertOrAssign(NewFilterT::k_BackgroundValue_Key, std::make_any<float64>(background));
    args.insertOrAssign(NewFilterT::k_BoundaryToForeground_Key, std::make_any<bool>(boundaryToForeground));
  };
}

//------------------------------------------------------------------------------
/**
 * @brief Preflight-rejection check for a MULTI-COMPONENT (non-scalar) input. Builds a valid ImageGeom
 *        "Image Geometry" + cell AttributeMatrix "CellData" holding a 3-component cell array "Input" of element
 *        type T -- its tuple count still equals the geometry's cell count, so the ONLY preflight violation is
 *        the non-scalar component count. Points @p NewFilterT at that array, sets otherwise-valid geom/input/
 *        output + kernel arguments via @p setKernelParams (Box r{1,1,1}), and asserts preflight is INVALID with
 *        @p expectedErrorCode -- each morphology filter's own `getNumberOfComponents() != 1` guard, which fires
 *        before the shared PreflightImageFilter type/geometry checks. Isolates that non-scalar rejection guard.
 */
template <class NewFilterT, class T>
inline void RequirePreflightRejectsNonScalar(int32 expectedErrorCode, const KernelParamSetter& setKernelParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize dim = 4;
  const ShapeType cellShape = {dim, dim, dim};
  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, inputPath, cellShape, {3}); // 3-component (non-scalar)
  DataArray<T>::Create(ds, "Input", store, cellAM->getId());

  NewFilterT filter;
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setKernelParams(args, static_cast<uint64>(KernelType::Box), {1, 1, 1});

  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == expectedErrorCode);
}

//------------------------------------------------------------------------------
// A single (radius, image-dims) parity configuration (the kernel shape is iterated separately).
struct MorphCase
{
  const char* label;
  std::vector<uint32> radius;
  usize dimX;
  usize dimY;
  usize dimZ;
};

namespace detail
{
// Drives BOTH the new NewFilterT and the legacy filter (passed as @p filter) with a shared Arguments over the
// standard geom/input/output keys plus the injected kernel parameters, then requires preflight + execute
// succeed. Load-bearing back-compat contract: this reuses NewFilterT::k_*_Key for the LEGACY filter too,
// which is only correct because the legacy and new filters share identical parameter-key strings; a future
// new/legacy pair with mismatched keys would silently drive the legacy filter with defaults.
template <class NewFilterT>
inline void RunFilter(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint64 kernelType, const std::vector<uint32>& radius, const KernelParamSetter& setKernelParams)
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setKernelParams(args, kernelType, radius);
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace detail

/**
 * @brief Legacy-vs-new parity grid shared by every grayscale-morphology filter.
 *
 * For each structuring-element shape in {Ball, Box, Cross} crossed with four configurations -- 3D uniform
 * radius {1,1,1}, 3D asymmetric radius {2,1,1} (all non-zero), 3D asymmetric radius {2,1,0} (zero z), and a
 * 2D (Z=1) image with radius {2,2,0} -- this builds byte-identical input into two DataStructures via
 * @p buildImage, runs the legacy ITK morphology filter (created at runtime by @p legacyUuid, so the
 * ImageProcessing test target does not link ITKImageProcessing) and the new @p NewFilterT with identical
 * Arguments, and asserts the output arrays match exactly via @p compare (default: exact CompareDataArrays<T>).
 *
 * Storage is forced in-core so the legacy ITK filter (which rejects out-of-core arrays) can run in any build
 * configuration, and so the new filter exercises its in-core moving-histogram (Direct) path here.
 *
 * ANNULUS IS DELIBERATELY EXCLUDED (never in @p kernels): the legacy SimpleITK wrapper builds an EMPTY Annulus
 * structuring element (a wrapper bug), so live legacy parity is not the correctness target for Annulus.
 * Annulus is validated against an independent computed oracle instead -- see @ref RunMorphologyComputedExpected.
 *
 * PER-CELL EXCLUSION (@p excludeCell): a specific (kernelName, config) cell can be skipped for LIVE-legacy
 * comparison via @p excludeCell (default excludes nothing). This exists so the (Box, "3D r{2,1,0}") cell can
 * be skipped -- on a 3D image the legacy ITK decomposable-Box anchor path produces garbage for a zero-radius
 * axis, reaching boundary values our correctly-centered radius-0 kernel does not (a legacy bug of the same
 * family as the empty Annulus). Ball/Cross at {2,1,0} on a 3D image DO match live legacy, so only the Box
 * cell is excluded; the excluded Box cell's correctness is instead gated by a Box {2,1,0} computed-expected
 * case (see @ref RunMorphologyComputedExpected) so the legacy bug is test-documented, not silently absent.
 *
 * Reusability (the point of this helper) comes from five injection points, mirroring ProjectionFilterTestUtils:
 *   - @p NewFilterT       supplies the shared geom/input/output parameter-key constants;
 *   - @p T                is the element/output type the parity is compared at;
 *   - @p buildImage       injects the test image (takes the per-case dims), returning the input array DataPath;
 *   - @p setKernelParams  sets the kernel type + radius (defaults available via DefaultKernelParamSetter);
 *   - @p excludeCell      skips live-legacy comparison for a specific (kernelName, config) cell;
 *   - @p compare          the output-array comparator (default exact CompareDataArrays<T>).
 */
template <class NewFilterT, class T>
inline void RunMorphologyParityGrid(
    const Uuid& legacyUuid, const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, const KernelParamSetter& setKernelParams,
    const std::function<bool(std::string_view kernelName, const MorphCase&)>& excludeCell = [](std::string_view, const MorphCase&) { return false; },
    const std::function<void(const IDataArray&, const IDataArray&)>& compare = [](const IDataArray& newOut, const IDataArray& legacyOut) { UnitTest::CompareDataArrays<T>(newOut, legacyOut); })
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // Fail fast (once) with a clear message if the legacy plugin is not loaded, rather than deep in a section.
  REQUIRE(Application::Instance()->getFilterList()->createFilter(legacyUuid) != nullptr);

  const std::vector<std::pair<const char*, uint64>> kernels = {
      {"Ball", static_cast<uint64>(KernelType::Ball)}, {"Box", static_cast<uint64>(KernelType::Box)}, {"Cross", static_cast<uint64>(KernelType::Cross)}};
  const std::vector<MorphCase> configs = {
      {"3D r{1,1,1}", {1, 1, 1}, 12, 12, 12}, {"3D r{2,1,1}", {2, 1, 1}, 12, 12, 12}, {"3D r{2,1,0}", {2, 1, 0}, 12, 12, 12}, {"2D Z=1 r{2,2,0}", {2, 2, 0}, 20, 16, 1}};

  for(const auto& [kernelName, kernelType] : kernels)
  {
    for(const MorphCase& cfg : configs)
    {
      if(excludeCell(kernelName, cfg))
      {
        continue; // this cell hits a known legacy bug; its correctness is gated by a computed-expected case
      }
      DYNAMIC_SECTION("kernel=" << kernelName << " " << cfg.label)
      {
        DataStructure newDs;
        const DataPath newInput = buildImage(newDs, cfg.dimX, cfg.dimY, cfg.dimZ);
        NewFilterT newFilter;
        detail::RunFilter<NewFilterT>(newFilter, newDs, newInput, kernelType, cfg.radius, setKernelParams);

        IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(legacyUuid);
        REQUIRE(legacyFilter != nullptr);
        DataStructure legacyDs;
        const DataPath legacyInput = buildImage(legacyDs, cfg.dimX, cfg.dimY, cfg.dimZ);
        detail::RunFilter<NewFilterT>(*legacyFilter, legacyDs, legacyInput, kernelType, cfg.radius, setKernelParams);

        const DataPath outputPath({"Image Geometry", "CellData", "Output"});
        const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
        const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
        REQUIRE(newOut.getDataType() == legacyOut.getDataType());
        compare(newOut, legacyOut);
      }
    }
  }
}

/**
 * @brief Computed-expected check (NOT live legacy parity) for ANY kernel shape. Used where live legacy parity
 *        is not the correctness target -- Annulus (the legacy SimpleITK Annulus kernel is empty, a wrapper
 *        bug, so this filter's proper thickness-1 shell has no legacy oracle) and the (Box, 3D r{2,1,0}) cell
 *        (the legacy decomposable-Box anchor path produces garbage for a zero-radius axis). Runs @p NewFilterT
 *        with @p kernelType at @p radius and asserts the output equals an INDEPENDENT oracle: for each voxel,
 *        gather the in-bounds neighbors of MakeStructuringElement(kernelType, radius) (skip-OOB boundary) and
 *        fold by max (Dilate) / min (Erode). Grayscale morphology only selects an existing input value, so
 *        exact equality holds for float types too. Storage is forced in-core (small correctness image).
 *
 * @pre @p kernelType + @p radius must yield a NON-empty structuring element. A degenerate empty SE (e.g.
 *      Annulus {0,0,0}) triggers the engine's passthrough (output == input), which this max/min oracle does
 *      NOT model -- use @ref RunMorphologyEmptySEPassthrough for that path instead.
 */
template <class NewFilterT, class T>
inline void RunMorphologyComputedExpected(const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, usize dimX, usize dimY, usize dimZ, KernelType kernelType,
                                          const std::array<int32, 3>& radius, MorphOp op, const KernelParamSetter& setKernelParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = buildImage(ds, dimX, dimY, dimZ);

  // Snapshot the input BEFORE running the filter (the filter writes a separate Output array, but read the
  // input independently so the oracle never depends on filter state).
  const auto& inStore = ds.getDataRefAs<DataArray<T>>(inputPath).getDataStoreRef();
  std::vector<T> input(inStore.getSize());
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = inStore.getValue(i);
  }

  NewFilterT filter;
  detail::RunFilter<NewFilterT>(filter, ds, inputPath, static_cast<uint64>(kernelType), {static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])},
                                setKernelParams);

  // Independent oracle over the SAME offsets, but a plain triple-loop with explicit OOB skipping --
  // structurally independent of the engine's slab streaming / moving histogram.
  const StructuringElement se = MakeStructuringElement(kernelType, radius);
  REQUIRE_FALSE(se.offsets.empty()); // guard: this oracle does not model the empty-SE passthrough
  const bool dilate = (op == MorphOp::Dilate);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();

  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        T acc = dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
        for(const SEOffset& off : se.offsets)
        {
          const int64 nx = static_cast<int64>(x) + off.dx;
          const int64 ny = static_cast<int64>(y) + off.dy;
          const int64 nz = static_cast<int64>(z) + off.dz;
          if(nx < 0 || nx >= static_cast<int64>(dimX) || ny < 0 || ny >= static_cast<int64>(dimY) || nz < 0 || nz >= static_cast<int64>(dimZ))
          {
            continue;
          }
          const T v = input[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)];
          acc = dilate ? std::max(acc, v) : std::min(acc, v);
        }
        const usize idx = FlatIndex(x, y, z, dimX, dimY);
        INFO("radius={" << radius[0] << "," << radius[1] << "," << radius[2] << "} op=" << (dilate ? "Dilate" : "Erode") << " voxel=(" << x << "," << y << "," << z << ")");
        REQUIRE(outStore.getValue(idx) == acc);
      }
    }
  }
}

//------------------------------------------------------------------------------
/**
 * @brief Independent BINARY-morphology oracle. For each output voxel it iterates the SE offsets and computes
 *        fgCount = (# in-bounds neighbors == @p fg) + (# out-of-bounds neighbors iff @p boundaryToForeground),
 *        then emits Dilate = (fgCount > 0 ? fg : bg) and Erode = (fgCount == |SE| ? fg : bg). This is the same
 *        semantics the engine's validated BinaryOracle uses, factored into the shared test util so the filter
 *        tests (and Task 5's binary composites) can assert against it. Deliberately a plain triple-loop with
 *        explicit OOB handling, structurally independent of the engine's slab streaming / moving accumulator,
 *        and equality-based so a neighbor that is neither fg nor bg correctly counts as non-foreground.
 *
 * @pre @p se has a non-empty offset list (the empty-SE passthrough is a distinct engine path).
 */
template <class T>
inline std::vector<T> BinaryMorphologyFgOracle(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op, T fg, T bg, bool boundaryToForeground)
{
  const bool dilate = (op == MorphOp::Dilate);
  const usize numOffsets = se.offsets.size();
  std::vector<T> output(input.size());
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        usize fgCount = 0;
        for(const SEOffset& off : se.offsets)
        {
          const int64 nx = static_cast<int64>(x) + off.dx;
          const int64 ny = static_cast<int64>(y) + off.dy;
          const int64 nz = static_cast<int64>(z) + off.dz;
          if(nx < 0 || nx >= static_cast<int64>(dimX) || ny < 0 || ny >= static_cast<int64>(dimY) || nz < 0 || nz >= static_cast<int64>(dimZ))
          {
            if(boundaryToForeground)
            {
              ++fgCount;
            }
            continue;
          }
          if(input[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)] == fg)
          {
            ++fgCount;
          }
        }
        output[FlatIndex(x, y, z, dimX, dimY)] = dilate ? ((fgCount > 0) ? fg : bg) : ((fgCount == numOffsets) ? fg : bg);
      }
    }
  }
  return output;
}

/**
 * @brief The composite binary-morphology operation a binary composite filter computes; mirrors the façade's
 *        BinaryCompositeOp but kept local so the test util does not depend on the façade header.
 */
enum class BinaryCompositeKind
{
  Opening,
  Closing
};

//------------------------------------------------------------------------------
/**
 * @brief Independent computed-expected oracle for a COMPOSITE binary-morphology filter, composed from two
 *        @ref BinaryMorphologyFgOracle passes (the same fg-count SE-gather the primitive oracle uses):
 *          - Opening = binaryErode(boundaryToForeground=true) then binaryDilate(boundaryToForeground=false)
 *          - Closing = binaryDilate(boundaryToForeground=false) then binaryErode(boundaryToForeground=true)
 *        Both passes write @p bg for non-foreground voxels; for a valid Closing input @p bg is the filter's
 *        internal background (0, or the type max when @p fg is 0), which the caller passes explicitly. This
 *        models the NON-padded composition (ITK SafeBorder == false for Closing; Opening never pads), so a
 *        Closing caller MUST drive the filter with Safe Border == false. The padded SafeBorder == true
 *        convention is validated separately (live legacy parity + the SafeBorder cross-storage OOC check).
 *
 * @pre @p se has a non-empty offset list (the empty-SE passthrough is a distinct engine path).
 */
template <class T>
inline std::vector<T> BinaryMorphologyCompositeFgOracle(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, BinaryCompositeKind kind, T fg, T bg)
{
  if(kind == BinaryCompositeKind::Opening)
  {
    const std::vector<T> eroded = BinaryMorphologyFgOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Erode, fg, bg, /*boundaryToForeground=*/true);
    return BinaryMorphologyFgOracle<T>(eroded, dimX, dimY, dimZ, se, MorphOp::Dilate, fg, bg, /*boundaryToForeground=*/false);
  }
  const std::vector<T> dilated = BinaryMorphologyFgOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Dilate, fg, bg, /*boundaryToForeground=*/false);
  return BinaryMorphologyFgOracle<T>(dilated, dimX, dimY, dimZ, se, MorphOp::Erode, fg, bg, /*boundaryToForeground=*/true);
}

/**
 * @brief Computed-expected check for a COMPOSITE binary-morphology filter (Opening/Closing), the binary
 *        analogue of @ref RunMorphologyCompositeComputedExpected. Builds a binary image via @p buildImage,
 *        runs @p NewFilterT with @p kernelType at @p radius (its kernel/fg[/bg][/safeBorder] parameters set by
 *        @p setKernelParams), and asserts the output equals the INDEPENDENT @ref BinaryMorphologyCompositeFgOracle.
 *        The oracle models the NON-padded composition, so a Closing caller MUST drive Safe Border == false
 *        (Opening never pads). Used where live legacy parity is not the target -- Annulus (empty legacy kernel)
 *        and the (Box, 3D r{2,1,0}) cell (broken legacy anchor path). Storage forced in-core (small image).
 *
 * @p bg is the background the oracle writes for non-foreground voxels: the user Background Value for Opening,
 * or the filter's internal background (0, or the type max when @p fg is 0) for Closing; it MUST equal the
 * background value present in the @p buildImage input.
 *
 * @pre @p kernelType + @p radius yield a NON-empty structuring element.
 */
template <class NewFilterT, class T>
inline void RunBinaryMorphologyCompositeComputedExpected(const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, usize dimX, usize dimY, usize dimZ, KernelType kernelType,
                                                         const std::array<int32, 3>& radius, BinaryCompositeKind kind, T fg, T bg, const KernelParamSetter& setKernelParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = buildImage(ds, dimX, dimY, dimZ);

  // Snapshot the input independently of the filter (which writes a separate "Output" array).
  const auto& inStore = ds.getDataRefAs<DataArray<T>>(inputPath).getDataStoreRef();
  std::vector<T> input(inStore.getSize());
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = inStore.getValue(i);
  }

  NewFilterT filter;
  detail::RunFilter<NewFilterT>(filter, ds, inputPath, static_cast<uint64>(kernelType), {static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])},
                                setKernelParams);

  const StructuringElement se = MakeStructuringElement(kernelType, radius);
  REQUIRE_FALSE(se.offsets.empty()); // guard: this oracle does not model the empty-SE passthrough

  const std::vector<T> expected = BinaryMorphologyCompositeFgOracle<T>(input, dimX, dimY, dimZ, se, kind, fg, bg);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    INFO("binary composite computed-expected index=" << i);
    REQUIRE(outStore.getValue(i) == expected[i]);
  }
}

//------------------------------------------------------------------------------
/**
 * @brief Single skip-OOB structuring-element fold over a flat input buffer: for each voxel, gather the
 *        in-bounds SE neighbors (dropping out-of-image ones) and fold by max (Dilate) or min (Erode).
 *        This is the SAME independent oracle primitive @ref RunMorphologyComputedExpected asserts against,
 *        factored out so the composite oracle below can COMPOSE it (open/close/tophat/gradient are just two
 *        folds plus a subtract). Returns a full dimX*dimY*dimZ buffer. Grayscale morphology only ever selects
 *        an existing input value, so this is exact for float types too.
 *
 * @pre @p se has a non-empty offset list (the empty-SE passthrough is a distinct engine path).
 */
template <class T>
inline std::vector<T> MorphologyFoldOracle(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op)
{
  const bool dilate = (op == MorphOp::Dilate);
  std::vector<T> output(input.size());
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        T acc = dilate ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
        for(const SEOffset& off : se.offsets)
        {
          const int64 nx = static_cast<int64>(x) + off.dx;
          const int64 ny = static_cast<int64>(y) + off.dy;
          const int64 nz = static_cast<int64>(z) + off.dz;
          if(nx < 0 || nx >= static_cast<int64>(dimX) || ny < 0 || ny >= static_cast<int64>(dimY) || nz < 0 || nz >= static_cast<int64>(dimZ))
          {
            continue;
          }
          const T v = input[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)];
          acc = dilate ? std::max(acc, v) : std::min(acc, v);
        }
        output[FlatIndex(x, y, z, dimX, dimY)] = acc;
      }
    }
  }
  return output;
}

/**
 * @brief The composite grayscale-morphology operation a composite filter computes; mirrors the façade's
 *        MorphCompositeOp but kept local so the test util does not depend on the façade header.
 */
enum class CompositeKind
{
  Opening,
  Closing,
  WhiteTopHat,
  BlackTopHat,
  Gradient
};

/**
 * @brief Computed-expected check for a COMPOSITE morphology filter (Opening/Closing/WhiteTopHat/BlackTopHat/
 *        Gradient). Runs @p NewFilterT with @p kernelType at @p radius and asserts the output equals an
 *        INDEPENDENT oracle composed from @ref MorphologyFoldOracle (the same skip-OOB SE-gather the
 *        primitive oracle uses) plus, for the difference forms, an elementwise subtract:
 *          - Opening     = Dilate(Erode(in));  Closing = Erode(Dilate(in))
 *          - WhiteTopHat = in - Opening(in);    BlackTopHat = Closing(in) - in
 *          - Gradient    = Dilate(in) - Erode(in)
 *        Equality is EXACT: each fold selects an existing value and the subtract is a single exact op
 *        (integer differences are exact; float differences here are a single exact operation). Storage is
 *        forced in-core (small correctness image). Used where live legacy parity is not the target -- Annulus
 *        (the legacy SimpleITK Annulus kernel is empty, a wrapper bug) and the (Box, 3D r{2,1,0}) cell (the
 *        legacy decomposable-Box anchor path is broken for a zero-radius axis).
 *
 * The oracle models the skip-OOB (extremum-fill at the true image edge) composition, i.e. ITK with
 * SafeBorder == false. The caller MUST therefore drive the filter with Safe Border set false (pass
 * @ref CompositeKernelParamSetter with @c safeBorder=false as @p setKernelParams); the padded SafeBorder ==
 * true convention is validated separately against live legacy in @ref RunMorphologyParityGrid.
 *
 * @pre @p kernelType + @p radius must yield a NON-empty structuring element.
 */
template <class NewFilterT, class T>
inline void RunMorphologyCompositeComputedExpected(const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, usize dimX, usize dimY, usize dimZ, KernelType kernelType,
                                                   const std::array<int32, 3>& radius, CompositeKind kind, const KernelParamSetter& setKernelParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = buildImage(ds, dimX, dimY, dimZ);

  // Snapshot the input independently of the filter (which writes a separate "Output" array).
  const auto& inStore = ds.getDataRefAs<DataArray<T>>(inputPath).getDataStoreRef();
  std::vector<T> input(inStore.getSize());
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = inStore.getValue(i);
  }

  NewFilterT filter;
  detail::RunFilter<NewFilterT>(filter, ds, inputPath, static_cast<uint64>(kernelType), {static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])},
                                setKernelParams);

  const StructuringElement se = MakeStructuringElement(kernelType, radius);
  REQUIRE_FALSE(se.offsets.empty()); // guard: this oracle does not model the empty-SE passthrough

  std::vector<T> expected(input.size());
  switch(kind)
  {
  case CompositeKind::Opening: {
    expected = MorphologyFoldOracle<T>(MorphologyFoldOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Erode), dimX, dimY, dimZ, se, MorphOp::Dilate);
    break;
  }
  case CompositeKind::Closing: {
    expected = MorphologyFoldOracle<T>(MorphologyFoldOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Dilate), dimX, dimY, dimZ, se, MorphOp::Erode);
    break;
  }
  case CompositeKind::WhiteTopHat: {
    const std::vector<T> opening = MorphologyFoldOracle<T>(MorphologyFoldOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Erode), dimX, dimY, dimZ, se, MorphOp::Dilate);
    for(usize i = 0; i < input.size(); ++i)
    {
      expected[i] = static_cast<T>(input[i] - opening[i]);
    }
    break;
  }
  case CompositeKind::BlackTopHat: {
    const std::vector<T> closing = MorphologyFoldOracle<T>(MorphologyFoldOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Dilate), dimX, dimY, dimZ, se, MorphOp::Erode);
    for(usize i = 0; i < input.size(); ++i)
    {
      expected[i] = static_cast<T>(closing[i] - input[i]);
    }
    break;
  }
  case CompositeKind::Gradient: {
    const std::vector<T> dilated = MorphologyFoldOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Dilate);
    const std::vector<T> eroded = MorphologyFoldOracle<T>(input, dimX, dimY, dimZ, se, MorphOp::Erode);
    for(usize i = 0; i < input.size(); ++i)
    {
      expected[i] = static_cast<T>(dilated[i] - eroded[i]);
    }
    break;
  }
  }

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    INFO("composite computed-expected index=" << i);
    REQUIRE(outStore.getValue(i) == expected[i]);
  }
}

/**
 * @brief Degenerate empty-structuring-element passthrough (first user-reachable path for the engine's
 *        offsets.empty() branch). An Annulus of radius {0,0,0} rasterizes to an EMPTY SE (outer == inner
 *        shell, center excluded), so both Dilate and Erode must pass the input through unchanged rather than
 *        emitting the fold identity. Runs @p NewFilterT with Annulus {0,0,0} and asserts output == input
 *        exactly. Op-independent (passthrough is identical for Dilate and Erode). Storage forced in-core.
 */
template <class NewFilterT, class T>
inline void RunMorphologyEmptySEPassthrough(const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, usize dimX, usize dimY, usize dimZ,
                                            const KernelParamSetter& setKernelParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // Sanity: confirm Annulus {0,0,0} really is the empty-SE case this test targets.
  REQUIRE(MakeStructuringElement(KernelType::Annulus, {0, 0, 0}).offsets.empty());

  DataStructure ds;
  const DataPath inputPath = buildImage(ds, dimX, dimY, dimZ);

  // Snapshot the input independently, then run the filter (which writes a separate "Output" array).
  const auto& inStore = ds.getDataRefAs<DataArray<T>>(inputPath).getDataStoreRef();
  std::vector<T> input(inStore.getSize());
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = inStore.getValue(i);
  }

  NewFilterT filter;
  detail::RunFilter<NewFilterT>(filter, ds, inputPath, static_cast<uint64>(KernelType::Annulus), {0, 0, 0}, setKernelParams);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == input.size());
  for(usize i = 0; i < input.size(); ++i)
  {
    INFO("empty-SE passthrough index=" << i);
    REQUIRE(outStore.getValue(i) == input[i]);
  }
}

//------------------------------------------------------------------------------
/**
 * @brief ITK-sourced real-image md5 golden for a SameAsInput grayscale/binary morphology filter (plan Sec.3/4/6).
 *
 * Mirrors the ITK<Filter>ImageTest.cpp real-image case: reads @p inputFile (uint8) through OUR ITK-free reader, runs
 * @p FilterT with the ITK case's kernel parameters (set by @p setKernelParams -- @ref DefaultKernelParamSetter for
 * the grayscale primitives, @ref CompositeKernelParamSetter(true) for the grayscale composites, or a binary setter
 * that additionally pins Foreground/Safe Border), then applies two oracles on ONE in-core DataStructure. The whole
 * case is pinned ForceInCore: the legacy ITK filter dynamic_casts buffers to the in-core DataStore<T> and bad_casts
 * an OOC store (reading through OUR reader is OOC-safe, but the (B) run is not).
 *   (B) LIVE-ITK parity FIRST -- the legacy ITK filter (resolved from the ImageProcessing plugin's replacement map)
 *       is run on the SAME input into the SAME DataStructure under a distinct output name and compared BIT-EXACT
 *       (tolerance 0.0; integer morphology on a fixed input is deterministic), then
 *   (A) DURABLE md5 golden -- our ITK-free output's md5 must equal @p committedMd5 (md5-validity-first, plan Sec.4).
 * Ordering ((B) before (A)) matches the shipped batches so a benign ITK-version md5 skew is diagnosed against the
 * live-ITK pass rather than reported as a bare hash mismatch.
 */
template <class FilterT>
inline void RunMorphologyMd5ItkGolden(const std::string& inputFile, const std::string& committedMd5, uint64 kernelType, const std::vector<uint32>& radius, const KernelParamSetter& setKernelParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR ITK-free reader ---
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  // --- run OUR filter with the ITK case's kernel parameters ---
  Arguments args;
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setKernelParams(args, kernelType, radius);
  FilterT filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (B) LIVE-ITK parity FIRST: legacy ITK filter on the SAME input into the SAME ds (distinct output name). ---
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<FilterT>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args; // identical key strings + kernel params drive both filters
  itkArgs.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const Result<> itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = cellData.createChildPath("ITK Output");
  const Result<> parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 0.0);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  // --- (A) DURABLE golden: md5-validity-first (plan Sec.4) -- our output's md5 must equal ITK's committed hash. ---
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("{} md5: ours='{}' committed='{}'", inputFile, ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace morph_test
