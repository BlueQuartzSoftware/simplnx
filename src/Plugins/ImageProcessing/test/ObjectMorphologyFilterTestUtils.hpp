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
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ObjectMorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace object_morph_test
{
using namespace nx::core;
using nx::core::ImageProcessing::KernelType;
using nx::core::ImageProcessing::MakeStructuringElement;
using nx::core::ImageProcessing::ObjectMorphOp;
using nx::core::ImageProcessing::SEOffset;
using nx::core::ImageProcessing::StructuringElement;

//------------------------------------------------------------------------------
// simplnx flat index for voxel (x,y,z): X fastest-moving, then Y, then Z.
inline usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

/**
 * @brief INDEPENDENT scatter oracle for object morphology. A plain triple-loop, structurally independent of
 *        the engine (no Z-slab streaming, no parallel plane bodies, no shared indexing / boundary helper): the
 *        boundary test's fixed radius-1 full box is rebuilt inline here. It reproduces ITK exactly -- output
 *        starts as a copy of the input, then every BOUNDARY object pixel (value == @p objectValue and at least
 *        one IN-BOUNDS immediate-box neighbor != @p objectValue) paints the @c MakeStructuringElement offsets
 *        around itself (Dilate -> @p objectValue, Erode -> @p backgroundValue), skipping out-of-bounds paint
 *        targets. Boundary/object state is read from the ORIGINAL input, never the partial output.
 */
template <class T>
inline std::vector<T> ObjectMorphologyOracle(const std::vector<T>& in, usize dimX, usize dimY, usize dimZ, KernelType kernelType, const std::array<int32, 3>& radius, ObjectMorphOp op, T objectValue,
                                             T backgroundValue)
{
  const StructuringElement se = MakeStructuringElement(kernelType, radius);
  const T paintValue = (op == ObjectMorphOp::Dilate) ? objectValue : backgroundValue;
  const int64 dimXi = static_cast<int64>(dimX);
  const int64 dimYi = static_cast<int64>(dimY);
  const int64 dimZi = static_cast<int64>(dimZ);

  std::vector<T> out = in; // output starts as a copy of the input

  // Independent boundary test: center == objectValue AND some in-bounds full-box (radius-1, center-excluded)
  // neighbor != objectValue. Built inline with a triple loop so it shares no code with the engine.
  auto isBoundary = [&](int64 xi, int64 yi, int64 zi) -> bool {
    if(in[FlatIndex(static_cast<usize>(xi), static_cast<usize>(yi), static_cast<usize>(zi), dimX, dimY)] != objectValue)
    {
      return false;
    }
    for(int64 dz = -1; dz <= 1; ++dz)
    {
      for(int64 dy = -1; dy <= 1; ++dy)
      {
        for(int64 dx = -1; dx <= 1; ++dx)
        {
          if(dx == 0 && dy == 0 && dz == 0)
          {
            continue;
          }
          const int64 nx = xi + dx;
          const int64 ny = yi + dy;
          const int64 nz = zi + dz;
          if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
          {
            continue; // OOB neighbor ignored
          }
          if(in[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)] != objectValue)
          {
            return true;
          }
        }
      }
    }
    return false;
  };

  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        if(!isBoundary(static_cast<int64>(x), static_cast<int64>(y), static_cast<int64>(z)))
        {
          continue;
        }
        for(const SEOffset& o : se.offsets)
        {
          const int64 nx = static_cast<int64>(x) + o.dx;
          const int64 ny = static_cast<int64>(y) + o.dy;
          const int64 nz = static_cast<int64>(z) + o.dz;
          if(nx < 0 || nx >= dimXi || ny < 0 || ny >= dimYi || nz < 0 || nz >= dimZi)
          {
            continue; // skip OOB paint target
          }
          out[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)] = paintValue;
        }
      }
    }
  }
  return out;
}

/**
 * @brief Deterministic MULTI-OBJECT pattern with EDGE-TOUCHING objects and solid interiors. Every voxel is
 *        either @p objectValue or @p backgroundValue. A coarse 4-voxel-block 3D checkerboard makes many
 *        disjoint object blocks (multi-object), the block at the origin touches the (0,0,0) corner and blocks
 *        along every max face touch the image edge (edge-touching -> exercises the OOB boundary/ paint rules),
 *        and a 4-wide block has solid interior voxels whose full-box neighbors are all object (so the boundary
 *        test's negative case -- no paint -- is exercised too). Works for any dims incl. a 2D (Z=1) image.
 */
template <class T>
inline std::vector<T> MakeObjectPattern(usize dimX, usize dimY, usize dimZ, T objectValue, T backgroundValue)
{
  std::vector<T> v(dimX * dimY * dimZ, backgroundValue);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const bool on = (((x / 4) + (y / 4) + (z / 4)) % 2 == 0);
        v[FlatIndex(x, y, z, dimX, dimY)] = on ? objectValue : backgroundValue;
      }
    }
  }
  return v;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" (dimX x dimY x dimZ) with cell AttributeMatrix "CellData" and a
 *        scalar cell array "Input" of element type T, filled with @ref MakeObjectPattern (multi-object,
 *        edge-touching, solid interiors) using @p objectValue / @p backgroundValue. Returns the input path.
 */
template <class T>
inline DataPath BuildObjectImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ, T objectValue, T backgroundValue)
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

  const std::vector<T> pattern = MakeObjectPattern<T>(dimX, dimY, dimZ, objectValue, backgroundValue);
  for(usize i = 0; i < pattern.size(); ++i)
  {
    ref.setValue(i, pattern[i]);
  }
  return inputPath;
}

/**
 * @brief Builds an ImageGeom "Image Geometry" (dimX x dimY x dimZ) with cell AttributeMatrix "CellData" and a
 *        scalar cell array "Input" of element type T carrying THREE distinct labels: @p objectValue in a coarse
 *        4-voxel-block checkerboard (multi-object, edge-touching, solid interiors), and among the remaining
 *        (non-object) voxels a deterministic mix of @p extraLabel and @p backgroundValue. Used to pin that the
 *        boundary test treats EVERY value != objectValue as non-object, and that Erode paints its Background
 *        Value regardless of whether that value already exists in the image. Returns the input path.
 */
template <class T>
inline DataPath BuildObjectImageWithExtraLabel(DataStructure& ds, usize dimX, usize dimY, usize dimZ, T objectValue, T extraLabel, T backgroundValue)
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
        T value;
        if((((x / 4) + (y / 4) + (z / 4)) % 2) == 0)
        {
          value = objectValue;
        }
        else
        {
          value = ((x * 131 + y * 57 + z * 29) % 3 == 0) ? extraLabel : backgroundValue;
        }
        ref.setValue(FlatIndex(x, y, z, dimX, dimY), value);
      }
    }
  }
  return inputPath;
}

/**
 * @brief Runs the public @ref nx::core::ImageProcessing::ApplyObjectMorphology entry on real DataArrays (so
 *        DispatchAlgorithm can inspect storage) and returns the output as a flat vector. The caller sets the
 *        Force{InCore,Ooc}AlgorithmGuard to select the scatter vs gather path deterministically. Uses the
 *        engine's own fixed-box builder (exactly as the façade does) for the boundary neighborhood.
 */
template <class T>
inline std::vector<T> RunApplyObjectMorphology(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, KernelType kernelType, const std::array<int32, 3>& radius, ObjectMorphOp op,
                                               T objectValue, T backgroundValue)
{
  DataStructure ds;
  auto* inArray = UnitTest::CreateTestDataArray<T>(ds, "in", {dimZ, dimY, dimX}, {1});
  auto* outArray = UnitTest::CreateTestDataArray<T>(ds, "out", {dimZ, dimY, dimX}, {1});
  for(usize i = 0; i < input.size(); ++i)
  {
    inArray->getDataStoreRef().setValue(i, input[i]);
  }

  const StructuringElement se = MakeStructuringElement(kernelType, radius);
  const std::vector<SEOffset> boxOffsets = ImageProcessing::detail::MakeFullBoxNeighborOffsets();
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = ImageProcessing::ApplyObjectMorphology<T>(inArray->getDataStoreRef(), outArray->getDataStoreRef(), SizeVec3{dimX, dimY, dimZ}, se, boxOffsets, op, objectValue,
                                                                    backgroundValue, *inArray, *outArray, shouldCancel, messageHandler);
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outArray->getDataStoreRef().getValue(i);
  }
  return out;
}

//------------------------------------------------------------------------------
/**
 * @brief Sets the KernelType + KernelRadius + ObjectValue [+ any op-specific value] on a shared Arguments.
 *        Injected into the parity/oracle runners so the Task-2 Erode filter (which adds a Background Value)
 *        can reuse the exact same runners with its own setter.
 */
using ObjectParamSetter = std::function<void(Arguments&, uint64 kernelType, const std::vector<uint32>& radius)>;

/**
 * @brief Parameter setter for the Dilate Object Morphology filter (kernel type + radius + Object Value). The
 *        legacy ITK Dilate Object Morphology filter shares these key strings, so this same setter drives both
 *        the new and the legacy filter in the parity grid.
 */
template <class NewFilterT>
inline ObjectParamSetter DilateParamSetter(float64 objectValue)
{
  return [objectValue](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(NewFilterT::k_ObjectValue_Key, std::make_any<float64>(objectValue));
  };
}

/**
 * @brief Parameter setter for the Erode Object Morphology filter (kernel type + radius + Object Value +
 *        Background Value). The legacy ITK Erode Object Morphology filter shares these key strings, so this
 *        same setter drives both the new and the legacy filter in the parity grid.
 */
template <class NewFilterT>
inline ObjectParamSetter ErodeParamSetter(float64 objectValue, float64 backgroundValue)
{
  return [objectValue, backgroundValue](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(NewFilterT::k_ObjectValue_Key, std::make_any<float64>(objectValue));
    args.insertOrAssign(NewFilterT::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
  };
}

//------------------------------------------------------------------------------
/**
 * @brief Preflight-rejection check for a MULTI-COMPONENT (non-scalar) input. Builds a valid ImageGeom
 *        "Image Geometry" + cell AttributeMatrix "CellData" holding a 3-component cell array "Input" of element
 *        type T -- its tuple count still equals the geometry's cell count, so the ONLY preflight violation is
 *        the non-scalar component count. Points @p NewFilterT at that array, sets otherwise-valid geom/input/
 *        output + params via @p setParams (Box r{1,1,1}), and asserts preflight is INVALID with
 *        @p expectedErrorCode -- each object-morphology filter's own `getNumberOfComponents() != 1` guard, which
 *        fires before the shared PreflightImageFilter type/geometry checks. Isolates that non-scalar guard.
 */
template <class NewFilterT, class T>
inline void RequirePreflightRejectsNonScalar(int32 expectedErrorCode, const ObjectParamSetter& setParams)
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
  setParams(args, static_cast<uint64>(KernelType::Box), {1, 1, 1});

  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == expectedErrorCode);
}

//------------------------------------------------------------------------------
// A single (radius, image-dims) parity configuration (the kernel shape is iterated separately).
struct ObjCase
{
  const char* label;
  std::vector<uint32> radius;
  usize dimX;
  usize dimY;
  usize dimZ;
};

namespace detail
{
// Drives BOTH the new NewFilterT and a legacy filter (passed as @p filter) with a shared Arguments over the
// standard geom/input/output keys plus the injected params, then requires preflight + execute succeed. Reuses
// NewFilterT::k_*_Key for the legacy filter too (correct only because the legacy and new filters share the
// identical parameter-key strings).
template <class NewFilterT>
inline void RunFilter(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint64 kernelType, const std::vector<uint32>& radius, const ObjectParamSetter& setParams)
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args, kernelType, radius);
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace detail

/**
 * @brief Legacy-vs-new parity grid for an object-morphology filter. For each SE shape in {Ball, Box, Cross}
 *        crossed with the given @p configs, builds byte-identical input via @p buildImage into two
 *        DataStructures, runs the legacy ITK filter (created at runtime by @p legacyUuid so the target need
 *        not link ITKImageProcessing) and the new @p NewFilterT with identical Arguments, and asserts the
 *        output arrays match EXACTLY (object morphology only ever writes a fixed paint value or copies the
 *        input, so exact equality holds for float types too). Storage is forced in-core so the legacy ITK
 *        filter (which rejects OOC arrays) runs in any build.
 *
 * ANNULUS IS DELIBERATELY EXCLUDED: the legacy SimpleITK Annulus kernel is EMPTY (a wrapper bug), so live
 * parity is not the correctness target for Annulus -- it is validated against the computed oracle instead
 * (see @ref RunObjectMorphologyComputedExpected).
 */
template <class NewFilterT, class T>
inline void RunObjectMorphologyParityGrid(const Uuid& legacyUuid, const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, const std::vector<ObjCase>& configs,
                                          const ObjectParamSetter& setParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // Fail fast (once) with a clear message if the legacy plugin is not loaded, rather than deep in a section.
  REQUIRE(Application::Instance()->getFilterList()->createFilter(legacyUuid) != nullptr);

  const std::vector<std::pair<const char*, uint64>> kernels = {
      {"Ball", static_cast<uint64>(KernelType::Ball)}, {"Box", static_cast<uint64>(KernelType::Box)}, {"Cross", static_cast<uint64>(KernelType::Cross)}};

  for(const auto& [kernelName, kernelType] : kernels)
  {
    for(const ObjCase& cfg : configs)
    {
      DYNAMIC_SECTION("kernel=" << kernelName << " " << cfg.label)
      {
        DataStructure newDs;
        const DataPath newInput = buildImage(newDs, cfg.dimX, cfg.dimY, cfg.dimZ);
        NewFilterT newFilter;
        detail::RunFilter<NewFilterT>(newFilter, newDs, newInput, kernelType, cfg.radius, setParams);

        IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(legacyUuid);
        REQUIRE(legacyFilter != nullptr);
        DataStructure legacyDs;
        const DataPath legacyInput = buildImage(legacyDs, cfg.dimX, cfg.dimY, cfg.dimZ);
        detail::RunFilter<NewFilterT>(*legacyFilter, legacyDs, legacyInput, kernelType, cfg.radius, setParams);

        const DataPath outputPath({"Image Geometry", "CellData", "Output"});
        const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
        const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
        REQUIRE(newOut.getDataType() == legacyOut.getDataType());
        UnitTest::CompareDataArrays<T>(newOut, legacyOut);
      }
    }
  }
}

/**
 * @brief Computed-expected check (NOT live legacy parity) for ANY kernel shape / op. Runs @p NewFilterT with
 *        @p kernelType at @p radius (its params set by @p setParams) and asserts the output equals the
 *        INDEPENDENT @ref ObjectMorphologyOracle over the same input. Used where live parity is not the target
 *        (Annulus -- empty legacy kernel) and for the float32 case. Storage forced in-core (small image).
 */
template <class NewFilterT, class T>
inline void RunObjectMorphologyComputedExpected(const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, usize dimX, usize dimY, usize dimZ, KernelType kernelType,
                                                const std::array<int32, 3>& radius, ObjectMorphOp op, T objectValue, T backgroundValue, const ObjectParamSetter& setParams)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = buildImage(ds, dimX, dimY, dimZ);

  // Snapshot the input BEFORE running the filter (the filter writes a separate Output array).
  const auto& inStore = ds.getDataRefAs<DataArray<T>>(inputPath).getDataStoreRef();
  std::vector<T> input(inStore.getSize());
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = inStore.getValue(i);
  }

  NewFilterT filter;
  detail::RunFilter<NewFilterT>(filter, ds, inputPath, static_cast<uint64>(kernelType), {static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])}, setParams);

  const std::vector<T> expected = ObjectMorphologyOracle<T>(input, dimX, dimY, dimZ, kernelType, radius, op, objectValue, backgroundValue);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    INFO("object-morphology computed-expected index=" << i);
    REQUIRE(outStore.getValue(i) == expected[i]);
  }
}

//------------------------------------------------------------------------------
/**
 * @brief ITK-sourced real-image golden for a SameAsInput object-morphology filter (plan Sec.3/Sec.6, Task 8).
 *
 * Mirrors ITK{Dilate,Erode}ObjectMorphologyImageTest.cpp: reads @p inputFile (an RA-Slice-* 64x64x1 image) through
 * OUR ITK-free reader, runs @p FilterT with Ball radius {1,1,1} and the ITK default Object Value 1 (@p setParams;
 * Erode additionally pins Background Value 0), then applies two oracles on ONE in-core DataStructure:
 *   (B) DETERMINISTIC computed-expected -- our output must EXACTLY equal @ref ObjectMorphologyOracle over the real
 *       input. This deliberately REPLACES a live-ITK bit-exact assertion: itk::ObjectMorphologyImageFilter is
 *       multithreaded and NONDETERMINISTIC (a non-atomic guard-copy races an unsynchronized cross-region boundary
 *       paint -- see DilateObjectMorphologyImageFilterTest.cpp(1)), so a fresh legacy run is not a stable oracle.
 *       Our single-threaded scatter matches ITK's intended single-threaded semantics.
 *   (A) DURABLE baseline golden -- the ITK-captured Baseline/@p baselineFile compared via CompareImages at
 *       @p tolerance (0.01). The stored baseline was captured once; when captured deterministically it equals our
 *       output. Pinned ForceInCore for consistency with the rest of the ItkGolden suite.
 */
template <class FilterT, class T>
inline void RunObjectMorphologyItkGoldenBaseline(const std::string& inputFile, const std::string& baselineFile, ObjectMorphOp op, T objectValue, T backgroundValue, const ObjectParamSetter& setParams,
                                                 float64 tolerance)
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

  // --- geometry dims + input snapshot (for the deterministic oracle; the filter writes a separate Output array) ---
  const auto* imageGeom = ds.getDataAs<ImageGeom>(geom);
  REQUIRE(imageGeom != nullptr);
  const SizeVec3 dims = imageGeom->getDimensions();
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const auto& inStore = ds.getDataRefAs<DataArray<T>>(input).getDataStoreRef();
  std::vector<T> inputBuf(inStore.getSize());
  for(usize i = 0; i < inputBuf.size(); ++i)
  {
    inputBuf[i] = inStore.getValue(i);
  }

  // --- run OUR filter (Ball r{1,1,1}, ITK default Object Value 1) ---
  Arguments args;
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args, static_cast<uint64>(KernelType::Ball), {1, 1, 1});
  FilterT filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (B) DETERMINISTIC oracle (ITK object morphology is a nondeterministic data race -- NOT live-ITK bit-exact) ---
  const std::vector<T> expected = ObjectMorphologyOracle<T>(inputBuf, dimX, dimY, dimZ, KernelType::Ball, {1, 1, 1}, op, objectValue, backgroundValue);
  const auto& outStore = ds.getDataRefAs<DataArray<T>>(output).getDataStoreRef();
  REQUIRE(outStore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    INFO("object-morphology ITK-golden oracle index=" << i);
    REQUIRE(outStore.getValue(i) == expected[i]);
  }

  // --- (A) DURABLE baseline golden: ITK-captured baseline compared at tolerance ---
  const DataPath baselineGeom({"Baseline Geometry"});
  const DataPath baselineData = baselineGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), baselineGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const Result<> compareResult = ip_golden::CompareImages(ds, baselineGeom, baselineData, geom, output, tolerance);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace object_morph_test
