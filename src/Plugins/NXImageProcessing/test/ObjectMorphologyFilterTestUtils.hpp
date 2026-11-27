#pragma once

#include <catch2/catch.hpp>

#include "ItkGoldenTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
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
inline DataPath BuildObjectImage(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, T objectValue, T backgroundValue)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = data_store_utilities::CreateDataStore<T>(dataStructure, inputPath, cellShape, {1});
  auto* arrayPtr = DataArray<T>::Create(dataStructure, "Input", store, cellAmPtr->getId());
  auto& ref = arrayPtr->getDataStoreRef();

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
inline DataPath BuildObjectImageWithExtraLabel(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, T objectValue, T extraLabel, T backgroundValue)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = data_store_utilities::CreateDataStore<T>(dataStructure, inputPath, cellShape, {1});
  auto* arrayPtr = DataArray<T>::Create(dataStructure, "Input", store, cellAmPtr->getId());
  auto& ref = arrayPtr->getDataStoreRef();

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
  DataStructure dataStructure;
  auto* inArrayPtr = unit_test::CreateTestDataArray<T>(dataStructure, "in", {dimZ, dimY, dimX}, {1});
  auto* outArrayPtr = unit_test::CreateTestDataArray<T>(dataStructure, "out", {dimZ, dimY, dimX}, {1});
  for(usize i = 0; i < input.size(); ++i)
  {
    inArrayPtr->getDataStoreRef().setValue(i, input[i]);
  }

  const StructuringElement se = MakeStructuringElement(kernelType, radius);
  const std::vector<SEOffset> boxOffsets = ImageProcessing::detail::MakeFullBoxNeighborOffsets();
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = ImageProcessing::ApplyObjectMorphology<T>(inArrayPtr->getDataStoreRef(), outArrayPtr->getDataStoreRef(), SizeVec3Type{dimX, dimY, dimZ}, se, boxOffsets, op, objectValue,
                                                                    backgroundValue, *inArrayPtr, *outArrayPtr, shouldCancel, messageHandler);
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outArrayPtr->getDataStoreRef().getValue(i);
  }
  return out;
}

//------------------------------------------------------------------------------
/**
 * @brief Sets the KernelType + KernelRadius + ObjectValue [+ any op-specific value] on a shared Arguments.
 *        Injected into the parity/oracle runners so the Task-2 Erode filter (which adds a Background Value)
 *        can reuse the exact same runners with its own setter.
 */
using ObjectParamSetterType = std::function<void(Arguments&, uint64 kernelType, const std::vector<uint32>& radius)>;

/**
 * @brief Parameter setter for the Dilate Object Morphology filter (kernel type + radius + Object Value).
 */
template <class NewFilterT>
inline ObjectParamSetterType DilateParamSetter(float64 objectValue)
{
  return [objectValue](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(radius));
    args.insertOrAssign(NewFilterT::k_ObjectValue_Key, std::make_any<float64>(objectValue));
  };
}

/**
 * @brief Parameter setter for the Erode Object Morphology filter (kernel type + radius + Object Value +
 *        Background Value).
 */
template <class NewFilterT>
inline ObjectParamSetterType ErodeParamSetter(float64 objectValue, float64 backgroundValue)
{
  return [objectValue, backgroundValue](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(NewFilterT::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(NewFilterT::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(radius));
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
inline void RequirePreflightRejectsNonScalar(int32 expectedErrorCode, const ObjectParamSetterType& setParams)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dim = 4;
  const ShapeType cellShape = {k_Dim, k_Dim, k_Dim};
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({k_Dim, k_Dim, k_Dim});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = data_store_utilities::CreateDataStore<T>(dataStructure, inputPath, cellShape, {3}); // 3-component (non-scalar)
  DataArray<T>::Create(dataStructure, "Input", store, cellAmPtr->getId());

  NewFilterT filter;
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args, static_cast<uint64>(KernelType::Box), {1, 1, 1});

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().front().code == expectedErrorCode);
}

//------------------------------------------------------------------------------
// A single (radius, image-dims) parity configuration (the kernel shape is iterated separately).
struct ObjCase
{
  const char* label;
  std::vector<uint32> radius;
  usize dim_x;
  usize dim_y;
  usize dim_z;
};

namespace detail
{
// Drives @p filter with a shared Arguments over the standard geom/input/output keys plus the injected params.
// The function then requires that preflight and execute succeed. It reads the key constants from NewFilterT.
template <class NewFilterT>
inline void RunFilter(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, uint64 kernelType, const std::vector<uint32>& radius, const ObjectParamSetterType& setParams)
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(NewFilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(NewFilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NewFilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args, kernelType, radius);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace detail

/**
 * @brief Computed-expected check for ANY kernel shape / op. Runs @p NewFilterT with @p kernelType at @p radius
 *        (its params set by @p setParams) and asserts the output equals the INDEPENDENT @ref ObjectMorphologyOracle
 *        over the same input. The oracle covers every kernel, and it covers the float32 case. Storage is forced
 *        in-core (small image).
 */
template <class NewFilterT, class T>
inline void RunObjectMorphologyComputedExpected(const std::function<DataPath(DataStructure&, usize, usize, usize)>& buildImage, usize dimX, usize dimY, usize dimZ, KernelType kernelType,
                                                const std::array<int32, 3>& radius, ObjectMorphOp op, T objectValue, T backgroundValue, const ObjectParamSetterType& setParams)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath inputPath = buildImage(dataStructure, dimX, dimY, dimZ);

  // Snapshot the input BEFORE running the filter (the filter writes a separate Output array).
  const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(inputPath).getDataStoreRef();
  std::vector<T> input(inStore.getSize());
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = inStore.getValue(i);
  }

  NewFilterT filter;
  detail::RunFilter<NewFilterT>(filter, dataStructure, inputPath, static_cast<uint64>(kernelType), {static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])},
                                setParams);

  const std::vector<T> expected = ObjectMorphologyOracle<T>(input, dimX, dimY, dimZ, kernelType, radius, op, objectValue, backgroundValue);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<T>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    INFO("object-morphology computed-expected index=" << i);
    REQUIRE(outStore.getValue(i) == expected[i]);
  }
}

//------------------------------------------------------------------------------
/**
 * @brief Real-image golden for a SameAsInput object-morphology filter (plan Sec.3/Sec.6, Task 8).
 *
 * The function reads @p inputFile (an RA-Slice-* 64x64x1 image) through our reader, runs @p FilterT with Ball
 * radius {1,1,1} and Object Value 1 (@p setParams; Erode also pins Background Value 0), then applies two oracles
 * on ONE in-core DataStructure:
 *   (B) DETERMINISTIC computed-expected -- our output must EXACTLY equal @ref ObjectMorphologyOracle over the real
 *       input. Our single-threaded scatter obeys the single-threaded object-morphology semantics.
 *   (A) DURABLE baseline golden -- the stored Baseline/@p baselineFile compared via CompareImages at
 *       @p tolerance (0.01). Storage is pinned ForceInCore for consistency with the rest of the golden suite.
 */
template <class FilterT, class T>
inline void RunObjectMorphologyItkGoldenBaseline(const std::string& inputFile, const std::string& baselineFile, ObjectMorphOp op, T objectValue, T backgroundValue,
                                                 const ObjectParamSetterType& setParams, float64 tolerance)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR reader ---
  DataStructure dataStructure;
  const Result<> readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  // --- geometry dims + input snapshot (for the deterministic oracle; the filter writes a separate Output array) ---
  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(geom);
  REQUIRE(imageGeomPtr != nullptr);
  const SizeVec3Type dims = imageGeomPtr->getDimensions();
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const auto& inStore = dataStructure.getDataRefAs<DataArray<T>>(input).getDataStoreRef();
  std::vector<T> inputBuf(inStore.getSize());
  for(usize i = 0; i < inputBuf.size(); ++i)
  {
    inputBuf[i] = inStore.getValue(i);
  }

  // --- run OUR filter (Ball r{1,1,1}, Object Value 1) ---
  Arguments args;
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args, static_cast<uint64>(KernelType::Ball), {1, 1, 1});
  FilterT filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (B) DETERMINISTIC oracle over the real input ---
  const std::vector<T> expected = ObjectMorphologyOracle<T>(inputBuf, dimX, dimY, dimZ, KernelType::Ball, {1, 1, 1}, op, objectValue, backgroundValue);
  const auto& outStore = dataStructure.getDataRefAs<DataArray<T>>(output).getDataStoreRef();
  REQUIRE(outStore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); ++i)
  {
    INFO("object-morphology golden oracle index=" << i);
    REQUIRE(outStore.getValue(i) == expected[i]);
  }

  // --- (A) DURABLE baseline golden: stored baseline compared at tolerance ---
  const DataPath baselineGeom({"Baseline Geometry"});
  const DataPath baselineData = baselineGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath(baselineFile), baselineGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const Result<> compareResult = ip_golden::CompareImages(dataStructure, baselineGeom, baselineData, geom, output, tolerance);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace object_morph_test
