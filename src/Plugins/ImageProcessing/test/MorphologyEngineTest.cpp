
#include "ImageProcessing/Filters/BinaryDilateImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryErodeImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/BlackTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleDilateImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleErodeImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalGradientImageFilter.hpp"
#include "ImageProcessing/Filters/WhiteTopHatImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;
using BinaryMorphWord = ImageProcessing::detail::BinaryMorphWord;

namespace
{
// Deliberately NON-CUBIC so a transposed/swapped-axis or bad-stride bug cannot pass: X even, Y odd, Z even.
constexpr usize k_X = 4;
constexpr usize k_Y = 5;
constexpr usize k_Z = 6;

// simplnx flat index for voxel (x,y,z): X fastest-moving, then Y, then Z.
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return z * (dimY * dimX) + y * dimX + x;
}

// Build an input DataStore laid out {Z, Y, X} (slowest -> fastest) and copy in the flat values.
template <class T>
DataStore<T> MakeStore(const std::vector<T>& values, usize dimX, usize dimY, usize dimZ)
{
  DataStore<T> store(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < values.size(); ++i)
  {
    store.setValue(i, values[i]);
  }
  return store;
}

template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    m_ReadValues += buffer.size();
    m_ReadBatchSizes.push_back(buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WrittenValues += buffer.size();
    m_WriteBatchSizes.push_back(buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize maxReadValues() const noexcept
  {
    return m_MaxReadValues;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  const std::vector<usize>& readBatchSizes() const noexcept
  {
    return m_ReadBatchSizes;
  }

  const std::vector<usize>& writeBatchSizes() const noexcept
  {
    return m_WriteBatchSizes;
  }

private:
  mutable usize m_MaxReadValues = 0;
  mutable usize m_ReadValues = 0;
  mutable std::vector<usize> m_ReadBatchSizes;
  usize m_MaxWriteValues = 0;
  usize m_WrittenValues = 0;
  std::vector<usize> m_WriteBatchSizes;
};

class CancelOnReadDataStore : public TransferCountingDataStore<uint8>
{
public:
  CancelOnReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, uint8 initialValue, std::atomic_bool& shouldCancel)
  : TransferCountingDataStore<uint8>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<uint8> buffer) const override
  {
    Result<> result = TransferCountingDataStore<uint8>::copyIntoBuffer(startIndex, buffer);
    m_ShouldCancel.store(true);
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
};

DataPath BuildMorphologyFilterPreflightInput(DataStructure& dataStructure)
{
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image Geometry");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({4, 4, 3});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{3, 4, 4}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  auto store = std::make_shared<DataStore<uint8>>(ShapeType{3, 4, 4}, ShapeType{1}, uint8{0});
  auto* input = DataArray<uint8>::Create(dataStructure, "Input", store, cellData->getId());
  REQUIRE(input != nullptr);
  return DataPath({"Image Geometry", "CellData", "Input"});
}

template <class FilterT>
void RequireDeferredMorphologyOutputAction(DataStructure& dataStructure, const DataPath& inputPath)
{
  FilterT filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(FilterT::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(FilterT::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterT::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* action = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(action != nullptr);
  REQUIRE(action->initializationMode() == DataStoreInitializationMode::DeferredZeroFill);
}

template <class T>
void RequireCropSubtractCase(const SizeVec3& dims, const std::array<usize, 3>& radius, ImageProcessing::detail::CropSubtractOrder order, bool aliasOutput)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize pdimX = dimX + 2 * radius[0];
  const usize pdimY = dimY + 2 * radius[1];
  const usize pdimZ = dimZ + 2 * radius[2];
  const usize volume = dimX * dimY * dimZ;
  const usize paddedVolume = pdimX * pdimY * pdimZ;

  std::vector<T> originalValues(volume);
  std::vector<T> paddedValues(paddedVolume, static_cast<T>(211));
  std::vector<T> expected(volume);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize index = FlatIndex(x, y, z, dimX, dimY);
        const usize paddedIndex = FlatIndex(x + radius[0], y + radius[1], z + radius[2], pdimX, pdimY);
        const T originalValue = static_cast<T>(100 + (index * 13) % 47);
        const T delta = static_cast<T>((index * 5 + 3) % 11);
        const T croppedValue = order == ImageProcessing::detail::CropSubtractOrder::OriginalMinusCropped ? static_cast<T>(originalValue - delta) : static_cast<T>(originalValue + delta);
        originalValues[index] = originalValue;
        paddedValues[paddedIndex] = croppedValue;
        expected[index] = order == ImageProcessing::detail::CropSubtractOrder::OriginalMinusCropped ? static_cast<T>(originalValue - paddedValues[paddedIndex]) :
                                                                                                      static_cast<T>(paddedValues[paddedIndex] - originalValue);
      }
    }
  }

  DataStore<T> paddedStore(ShapeType{pdimZ, pdimY, pdimX}, ShapeType{1}, static_cast<T>(0));
  DataStore<T> originalStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  DataStore<T> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(77));
  auto copyFromBufferResult = paddedStore.copyFromBuffer(0, nonstd::span<const T>(paddedValues.data(), paddedValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  auto copyFromBufferResult2 = originalStore.copyFromBuffer(0, nonstd::span<const T>(originalValues.data(), originalValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult2);

  std::atomic_bool shouldCancel{false};
  AbstractDataStore<T>& destination = aliasOutput ? static_cast<AbstractDataStore<T>&>(originalStore) : static_cast<AbstractDataStore<T>&>(outputStore);
  auto cropSubtractFromStoreResult = ImageProcessing::detail::CropSubtractFromStore<T>(paddedStore, originalStore, destination, dims, radius, order, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(cropSubtractFromStoreResult);

  std::vector<T> actual(volume);
  auto copyIntoBufferResult = destination.copyIntoBuffer(0, nonstd::span<T>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
}

// Run the Scanline morphology path directly (this task does not depend on the Task-4 dispatch seam).
template <class T>
std::vector<T> RunScanline(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = MorphScanline<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, se, op, shouldCancel, messageHandler}();
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Run the adaptive Direct path directly (head-to-head cross-validation against Scanline).
template <class T>
std::vector<T> RunDirect(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = MorphDirect<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, se, op, shouldCancel, messageHandler}();
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Independent oracle: for each output voxel, gather ONLY the in-bounds SE neighbors (skip-OOB boundary,
// per the pinned Task-1 rule) via a plain triple-loop and reduce by max (Dilate) / min (Erode). Uses the
// same MakeStructuringElement offsets, but is otherwise structurally independent of the engine (no
// Z-slab streaming, no parallelism, no shared indexing helper) so a slab/stride/transpose bug cannot hide.
template <class T>
std::vector<T> Oracle(const std::vector<T>& in, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op)
{
  const bool dilate = (op == MorphOp::Dilate);
  std::vector<T> out(in.size());
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        // Extremum-fill identity: for a max-fold the identity is lowest(); for a min-fold it is max().
        // This is bit-identical to ITK substituting the type extremum for out-of-bounds neighbors.
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
          const T v = in[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)];
          acc = dilate ? std::max(acc, v) : std::min(acc, v);
        }
        out[FlatIndex(x, y, z, dimX, dimY)] = acc;
      }
    }
  }
  return out;
}

// Run the FUSED gradient Scanline path directly (single-pass max - min).
template <class T>
std::vector<T> RunGradientScanline(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = MorphGradientScanline<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, se, shouldCancel, messageHandler}();
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Run the FUSED gradient adaptive Direct path directly (head-to-head vs Scanline + oracle).
template <class T>
std::vector<T> RunGradientDirect(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = MorphGradientDirect<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, se, shouldCancel, messageHandler}();
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Independent fused-gradient oracle: dilate(in) - erode(in), each fold computed by the standalone triple-loop
// Oracle above (skip-OOB seeds lowest()/max()), then subtracted exactly as the former SubtractStores did
// (static_cast<T>(dilate - erode)). For a voxel with NO in-bounds neighbor this yields
// static_cast<T>(lowest() - max()), the empty-window corner the fused engine must reproduce bit-for-bit.
template <class T>
std::vector<T> GradientOracle(const std::vector<T>& in, usize dimX, usize dimY, usize dimZ, const StructuringElement& se)
{
  const std::vector<T> dil = Oracle(in, dimX, dimY, dimZ, se, MorphOp::Dilate);
  const std::vector<T> ero = Oracle(in, dimX, dimY, dimZ, se, MorphOp::Erode);
  std::vector<T> out(in.size());
  for(usize i = 0; i < in.size(); ++i)
  {
    out[i] = static_cast<T>(dil[i] - ero[i]);
  }
  return out;
}

const char* KernelName(KernelType kt)
{
  switch(kt)
  {
  case KernelType::Annulus:
    return "Annulus";
  case KernelType::Ball:
    return "Ball";
  case KernelType::Box:
    return "Box";
  case KernelType::Cross:
    return "Cross";
  }
  return "?";
}

// Deterministic non-trivial volume: an axis gradient + a coarse checkerboard of blocks + a pseudo-noise
// term, so the min/max winner varies per voxel and is not a fixed corner of the neighborhood.
template <class T>
std::vector<T> MakePattern(usize dimX, usize dimY, usize dimZ)
{
  constexpr usize k_ValueModulus = std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) == 1 ? 120 : 200;
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize gradient = x + 2 * y + 3 * z;
        const usize block = (((x / 4) + (y / 4) + (z / 4)) % 2 == 0) ? 0 : 41;
        const usize noise = (x * 131 + y * 57 + z * 29) % 37;
        const usize value = (gradient + block + noise) % k_ValueModulus;
        v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(value);
      }
    }
  }
  return v;
}

// CORRECTNESS GATE for the adaptive Direct path: it must reproduce the (already-verified)
// Scanline path bit-for-bit for every kernel x radius x op. Because grayscale morphology only *selects* an
// existing input value (never averages), exact equality is well-defined for float32 too.
template <class T>
void CrossValidateDirectVsScanline(usize dimX, usize dimY, usize dimZ)
{
  const std::vector<T> input = MakePattern<T>(dimX, dimY, dimZ);
  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<std::array<int32, 3>> radii = {{2, 2, 2}, {3, 1, 2}, {2, 2, 0}};
  const std::vector<MorphOp> ops = {MorphOp::Dilate, MorphOp::Erode};

  for(KernelType kt : kernels)
  {
    for(const std::array<int32, 3>& radius : radii)
    {
      const StructuringElement se = MakeStructuringElement(kt, radius);
      for(MorphOp op : ops)
      {
        const std::vector<T> scan = RunScanline(input, dimX, dimY, dimZ, se, op);
        const std::vector<T> direct = RunDirect(input, dimX, dimY, dimZ, se, op);
        for(usize i = 0; i < input.size(); ++i)
        {
          INFO("kernel=" << KernelName(kt) << " radius={" << radius[0] << "," << radius[1] << "," << radius[2] << "} op=" << (op == MorphOp::Dilate ? "Dilate" : "Erode") << " index=" << i);
          REQUIRE(direct[i] == scan[i]);
        }
      }
    }
  }
}

// CORRECTNESS GATE for the FUSED gradient: the in-core Direct and OOC Scanline single-pass paths must agree
// with EACH OTHER and with the independent dilate-minus-erode oracle bit-for-bit, for every kernel x radius.
// The 3x3x3 case drives heavy clipping so the all-OOB Annulus corners exercise the empty-window seed
// (static_cast<T>(lowest() - max())); all radii here keep the SE non-empty (the empty-SE Annulus is a distinct
// engine branch that emits 0). Gradient (max - min) selects existing values then subtracts once, so exact
// equality is well-defined for float32 too.
template <class T>
void CrossValidateGradient(usize dimX, usize dimY, usize dimZ)
{
  const std::vector<T> input = MakePattern<T>(dimX, dimY, dimZ);
  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<std::array<int32, 3>> radii = {{2, 2, 2}, {3, 1, 2}, {2, 2, 0}};

  for(KernelType kt : kernels)
  {
    for(const std::array<int32, 3>& radius : radii)
    {
      const StructuringElement se = MakeStructuringElement(kt, radius);
      const std::vector<T> scan = RunGradientScanline(input, dimX, dimY, dimZ, se);
      const std::vector<T> direct = RunGradientDirect(input, dimX, dimY, dimZ, se);
      const std::vector<T> expected = GradientOracle(input, dimX, dimY, dimZ, se);
      for(usize i = 0; i < input.size(); ++i)
      {
        INFO("kernel=" << KernelName(kt) << " radius={" << radius[0] << "," << radius[1] << "," << radius[2] << "} index=" << i);
        REQUIRE(direct[i] == scan[i]);
        REQUIRE(direct[i] == expected[i]);
      }
    }
  }
}

// Independent binary-morphology oracle. For each output voxel it iterates the SE offsets and computes
// fgCount = (# in-bounds neighbors == fg) + (# out-of-bounds neighbors iff boundaryToForeground). It then
// emits Dilate = (fgCount > 0 ? fg : bg), Erode = (fgCount == |SE| ? fg : bg). Deliberately structurally
// independent of the engine (no slab streaming / parallelism / shared indexing) so a stride/transpose/slab
// bug cannot hide, and equality-based so a neighbor that is neither fg nor bg correctly counts as non-fg.
template <class T>
std::vector<T> MakeBinaryPattern(usize dimX, usize dimY, usize dimZ);

template <class T>
std::vector<T> BinaryOracle(const std::vector<T>& in, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op, T fg, T bg, bool boundaryToForeground)
{
  const bool dilate = (op == MorphOp::Dilate);
  const usize numOffsets = se.offsets.size();
  std::vector<T> out(in.size());
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
          const T v = in[FlatIndex(static_cast<usize>(nx), static_cast<usize>(ny), static_cast<usize>(nz), dimX, dimY)];
          if(v == fg)
          {
            ++fgCount;
          }
        }
        const T result = dilate ? ((fgCount > 0) ? fg : bg) : ((fgCount == numOffsets) ? fg : bg);
        out[FlatIndex(x, y, z, dimX, dimY)] = result;
      }
    }
  }
  return out;
}

// Extract a local [Z][Y][X] block from a complete-volume oracle result. This intentionally does
// not share the packed helper's staging or word logic.
template <class T>
std::vector<T> ExtractBinaryOracleBlock(const std::vector<T>& completeValues, const SizeVec3& boundDims, const SizeVec3& outputOrigin, const SizeVec3& outputDims)
{
  std::vector<T> block(outputDims[0] * outputDims[1] * outputDims[2]);
  for(usize localZ = 0; localZ < outputDims[2]; ++localZ)
  {
    for(usize localY = 0; localY < outputDims[1]; ++localY)
    {
      for(usize localX = 0; localX < outputDims[0]; ++localX)
      {
        block[FlatIndex(localX, localY, localZ, outputDims[0], outputDims[1])] =
            completeValues[FlatIndex(outputOrigin[0] + localX, outputOrigin[1] + localY, outputOrigin[2] + localZ, boundDims[0], boundDims[1])];
      }
    }
  }
  return block;
}

template <bool Dilate>
void CheckPackedBinaryMorphologyBlock(const SizeVec3& boundDims, usize stagedZLo, usize stagedDepth, const SizeVec3& outputDims, const SizeVec3& outputOrigin, const StructuringElement& se,
                                      bool boundaryToForeground, const char* caseName)
{
  constexpr uint8 k_Foreground = 1;
  constexpr uint8 k_Background = 0;
  const usize planeValues = boundDims[0] * boundDims[1];
  const std::vector<uint8> input = MakeBinaryPattern<uint8>(boundDims[0], boundDims[1], boundDims[2]);
  const std::vector<uint8> stagedInput(input.cbegin() + stagedZLo * planeValues, input.cbegin() + (stagedZLo + stagedDepth) * planeValues);
  std::vector<uint8> actual(outputDims[0] * outputDims[1] * outputDims[2], 255);
  std::atomic_bool shouldCancel{false};

  ImageProcessing::detail::RunPackedBinaryMorphologyBlock<uint8, Dilate>(stagedInput.data(), actual.data(), boundDims, stagedZLo, stagedDepth, outputDims, outputOrigin, se, k_Foreground, k_Background,
                                                                         boundaryToForeground, shouldCancel);

  const MorphOp op = Dilate ? MorphOp::Dilate : MorphOp::Erode;
  const std::vector<uint8> expected =
      ExtractBinaryOracleBlock(BinaryOracle(input, boundDims[0], boundDims[1], boundDims[2], se, op, k_Foreground, k_Background, boundaryToForeground), boundDims, outputOrigin, outputDims);
  INFO(caseName << " op=" << (Dilate ? "Dilate" : "Erode") << " boundaryToForeground=" << (boundaryToForeground ? "true" : "false"));
  REQUIRE(actual == expected);
}

// Run the binary-morphology Scanline path directly (this task does not depend on the Task-3 dispatch seam).
template <class T>
std::vector<T> RunBinaryScanline(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op, T fg, T bg, bool boundaryToForeground)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = BinaryMorphScanline<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, se, op, fg, bg, boundaryToForeground, shouldCancel, messageHandler}();
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Run the binary-morphology adaptive Direct path directly (head-to-head vs BinaryMorphScanline).
template <class T>
std::vector<T> RunBinaryDirect(const std::vector<T>& input, usize dimX, usize dimY, usize dimZ, const StructuringElement& se, MorphOp op, T fg, T bg, bool boundaryToForeground)
{
  DataStore<T> inStore = MakeStore(input, dimX, dimY, dimZ);
  DataStore<T> outStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = BinaryMorphDirect<T>{inStore, outStore, SizeVec3{dimX, dimY, dimZ}, se, op, fg, bg, boundaryToForeground, shouldCancel, messageHandler}();
  REQUIRE(result.valid());

  std::vector<T> out(input.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}

// Deterministic label volume mixing fg (1), bg (0), AND labels that are neither fg nor bg (2, 3, 5), so the
// equality-based binary semantics are exercised: a "2"/"3"/"5" neighbor must count as non-fg (never as fg or
// as boundary). fg is biased to appear often enough that dilate spreads it and erode has all-fg windows.
template <class T>
std::vector<T> MakeBinaryPattern(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize h = (x * 131 + y * 57 + z * 29) % 7;
        usize label = 0;
        switch(h)
        {
        case 0:
        case 4:
          label = 0; // bg
          break;
        case 1:
        case 2:
        case 6:
          label = 1; // fg (biased common)
          break;
        case 3:
          label = 2; // non-{fg,bg} label
          break;
        default:
          label = 3; // non-{fg,bg} label
          break;
        }
        // Sprinkle a fourth distinct label so more than one non-{fg,bg} value is present.
        if((x + y + z) % 11 == 0)
        {
          label = 5;
        }
        v[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(label);
      }
    }
  }
  return v;
}

// Compare the binary Scanline engine output to the independent oracle over every kernel x op x boundary-mode
// combination at radius {1,1,1}. The oracle and engine are shape-agnostic (they only iterate the SE's
// offsets), so all four kernels are exercised, exactly like the grayscale CheckMorphology; Annulus at
// r{1,1,1} is non-degenerate (18 offsets), so no empty-SE passthrough path is hit here.
template <class T>
void CheckBinaryMorphology(usize dimX, usize dimY, usize dimZ)
{
  const std::vector<T> input = MakeBinaryPattern<T>(dimX, dimY, dimZ);
  const T fg = static_cast<T>(1);
  const T bg = static_cast<T>(0);
  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<MorphOp> ops = {MorphOp::Dilate, MorphOp::Erode};
  const std::vector<bool> boundaryModes = {true, false};

  for(KernelType kt : kernels)
  {
    const StructuringElement se = MakeStructuringElement(kt, {1, 1, 1});
    for(MorphOp op : ops)
    {
      for(bool b2f : boundaryModes)
      {
        const std::vector<T> expected = BinaryOracle(input, dimX, dimY, dimZ, se, op, fg, bg, b2f);
        const std::vector<T> actual = RunBinaryScanline(input, dimX, dimY, dimZ, se, op, fg, bg, b2f);
        for(usize i = 0; i < input.size(); ++i)
        {
          INFO("kernel=" << KernelName(kt) << " op=" << (op == MorphOp::Dilate ? "Dilate" : "Erode") << " boundaryToForeground=" << (b2f ? "true" : "false") << " index=" << i);
          REQUIRE(actual[i] == expected[i]);
        }
      }
    }
  }
}

// CORRECTNESS GATE for the adaptive binary Direct path: BinaryMorphDirect must
// reproduce the (already-oracle-verified) BinaryMorphScanline path bit-for-bit for every kernel x radius x
// op x boundary-mode. Uses the larger 20x20x20 label volume mixing fg/bg AND non-{fg,bg} labels (0,1,2,3,5),
// with a symmetric {2,2,2} and an asymmetric {3,1,2} radius so a per-axis stride/offset bug in the added/
// removed slide cannot hide. Because binary morphology only emits {fg, bg}, exact equality is well-defined.
template <class T>
void CrossValidateBinaryDirectVsScanline(usize dimX, usize dimY, usize dimZ)
{
  const std::vector<T> input = MakeBinaryPattern<T>(dimX, dimY, dimZ);
  const T fg = static_cast<T>(1);
  const T bg = static_cast<T>(0);
  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<std::array<int32, 3>> radii = {{2, 2, 2}, {3, 1, 2}, {2, 2, 0}};
  const std::vector<MorphOp> ops = {MorphOp::Dilate, MorphOp::Erode};
  const std::vector<bool> boundaryModes = {true, false};

  for(KernelType kt : kernels)
  {
    for(const std::array<int32, 3>& radius : radii)
    {
      const StructuringElement se = MakeStructuringElement(kt, radius);
      for(MorphOp op : ops)
      {
        for(bool b2f : boundaryModes)
        {
          const std::vector<T> scan = RunBinaryScanline(input, dimX, dimY, dimZ, se, op, fg, bg, b2f);
          const std::vector<T> direct = RunBinaryDirect(input, dimX, dimY, dimZ, se, op, fg, bg, b2f);
          for(usize i = 0; i < input.size(); ++i)
          {
            INFO("kernel=" << KernelName(kt) << " radius={" << radius[0] << "," << radius[1] << "," << radius[2] << "} op=" << (op == MorphOp::Dilate ? "Dilate" : "Erode")
                           << " boundaryToForeground=" << (b2f ? "true" : "false") << " index=" << i);
            REQUIRE(direct[i] == scan[i]);
          }
        }
      }
    }
  }
}

// Compare the engine output to the independent oracle over every kernel x op x pattern combination.
template <class T>
void CheckMorphology(usize dimX, usize dimY, usize dimZ)
{
  const usize vol = dimX * dimY * dimZ;
  std::vector<T> ramp(vol);
  std::vector<T> nonMono(vol);
  for(usize i = 0; i < vol; ++i)
  {
    ramp[i] = static_cast<T>(i);               // strictly increasing so min/max pick opposite corners
    nonMono[i] = static_cast<T>((i * 7) % 53); // non-monotonic so the winner is not a fixed neighbor
  }

  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<MorphOp> ops = {MorphOp::Dilate, MorphOp::Erode};

  for(KernelType kt : kernels)
  {
    const StructuringElement se = MakeStructuringElement(kt, {1, 1, 1});
    for(MorphOp op : ops)
    {
      const bool dilate = (op == MorphOp::Dilate);
      const std::vector<T> expectedRamp = Oracle(ramp, dimX, dimY, dimZ, se, op);
      const std::vector<T> actualRamp = RunScanline(ramp, dimX, dimY, dimZ, se, op);
      const std::vector<T> expectedNonMono = Oracle(nonMono, dimX, dimY, dimZ, se, op);
      const std::vector<T> actualNonMono = RunScanline(nonMono, dimX, dimY, dimZ, se, op);

      for(usize i = 0; i < vol; ++i)
      {
        INFO("kernel=" << KernelName(kt) << " op=" << (dilate ? "Dilate" : "Erode") << " index=" << i);
        REQUIRE(actualRamp[i] == expectedRamp[i]);
        REQUIRE(actualNonMono[i] == expectedNonMono[i]);
      }
    }
  }
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: Scanline dilate/erode matches SE min/max (3D + 2D)", "[ImageProcessing][MorphologyEngine]", int32, uint8, float32)
{
  using T = TestType;
  SECTION("3D non-cubic 4x5x6")
  {
    CheckMorphology<T>(k_X, k_Y, k_Z);
  }
  SECTION("2D single-plane 4x5x1 (rz=1 kernel clipped to plane)")
  {
    CheckMorphology<T>(k_X, k_Y, 1);
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: rolling top-hat pipeline pre-cancel preserves poison output", "[ImageProcessing][MorphologyEngine]")
{
  const SizeVec3 dims{5, 4, 3};
  const usize count = dims[0] * dims[1] * dims[2];
  constexpr int32 poison = 901;
  const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 1});
  DataStore<int32> input(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 7);
  DataStore<int32> output(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, poison);
  auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<int32>(dims, se, true);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredBytesResult);
  std::atomic_bool shouldCancel{true};
  auto applyMorphologyCompositePipelineResult = ApplyMorphologyCompositePipeline<int32>(
      input, output, dims, se, MorphOp::Erode, MorphOp::Dilate, true, ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology, shouldCancel, requiredBytesResult.value());
  SIMPLNX_RESULT_REQUIRE_VALID(applyMorphologyCompositePipelineResult);
  std::vector<int32> actual(count);
  auto copyIntoBufferResult = output.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(std::all_of(actual.cbegin(), actual.cend(), [poison](int32 value) { return value == poison; }));
}

TEST_CASE("ImageProcessing::MorphologyEngine: rolling composite pipeline planner has the exact Ball-r2 benchmark payload", "[ImageProcessing][MorphologyEngine][WorkingMemory]")
{
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {2, 2, 2});
  const auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<uint8>(SizeVec3{1200, 900, 256}, se, true);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredBytesResult);
  REQUIRE(requiredBytesResult.value() == 11'964'160);
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: rolling composite pipeline emits morphology and top hats with one read and write", "[ImageProcessing][MorphologyEngine]", int16, float32)
{
  using T = TestType;
  const SizeVec3 dims{5, 4, 5};
  const usize count = dims[0] * dims[1] * dims[2];
  const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 1});
  const std::vector<T> values = MakePattern<T>(dims[0], dims[1], dims[2]);
  struct Case
  {
    MorphOp firstOp;
    MorphOp secondOp;
    ImageProcessing::detail::MorphologyCompositeOutputMode outputMode;
  };
  const std::array<Case, 4> cases = {{{MorphOp::Erode, MorphOp::Dilate, ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology},
                                      {MorphOp::Dilate, MorphOp::Erode, ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology},
                                      {MorphOp::Erode, MorphOp::Dilate, ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology},
                                      {MorphOp::Dilate, MorphOp::Erode, ImageProcessing::detail::MorphologyCompositeOutputMode::MorphologyMinusOriginal}}};
  for(const bool safeBorder : {false, true})
  {
    for(const Case testCase : cases)
    {
      TransferCountingDataStore<T> input(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{});
      TransferCountingDataStore<T> output(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::numeric_limits<T>::lowest());
      auto copyFromBufferResult = input.copyFromBuffer(0, values);
      SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
      auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<T>(dims, se, safeBorder);
      SIMPLNX_RESULT_REQUIRE_VALID(requiredBytesResult);
      std::atomic_bool shouldCancel{false};
      auto applyMorphologyCompositePipelineResult =
          ApplyMorphologyCompositePipeline<T>(input, output, dims, se, testCase.firstOp, testCase.secondOp, safeBorder, testCase.outputMode, shouldCancel, requiredBytesResult.value());
      SIMPLNX_RESULT_REQUIRE_VALID(applyMorphologyCompositePipelineResult);

      std::vector<T> expectedInput = values;
      if(safeBorder)
      {
        const std::array<usize, 3> radius{1, 1, 1};
        const SizeVec3 paddedDims{dims[0] + 2, dims[1] + 2, dims[2] + 2};
        const T padValue = testCase.firstOp == MorphOp::Erode ? std::numeric_limits<T>::max() : std::numeric_limits<T>::lowest();
        std::vector<T> padded(paddedDims[0] * paddedDims[1] * paddedDims[2], padValue);
        for(usize z = 0; z < dims[2]; ++z)
        {
          for(usize y = 0; y < dims[1]; ++y)
          {
            for(usize x = 0; x < dims[0]; ++x)
            {
              padded[FlatIndex(x + radius[0], y + radius[1], z + radius[2], paddedDims[0], paddedDims[1])] = values[FlatIndex(x, y, z, dims[0], dims[1])];
            }
          }
        }
        const std::vector<T> first = Oracle(padded, paddedDims[0], paddedDims[1], paddedDims[2], se, testCase.firstOp);
        const std::vector<T> second = Oracle(first, paddedDims[0], paddedDims[1], paddedDims[2], se, testCase.secondOp);
        expectedInput.resize(count);
        for(usize z = 0; z < dims[2]; ++z)
        {
          for(usize y = 0; y < dims[1]; ++y)
          {
            for(usize x = 0; x < dims[0]; ++x)
            {
              expectedInput[FlatIndex(x, y, z, dims[0], dims[1])] = second[FlatIndex(x + 1, y + 1, z + 1, paddedDims[0], paddedDims[1])];
            }
          }
        }
      }
      else
      {
        expectedInput = Oracle(Oracle(values, dims[0], dims[1], dims[2], se, testCase.firstOp), dims[0], dims[1], dims[2], se, testCase.secondOp);
      }
      std::vector<T> expected(count);
      for(usize index = 0; index < count; ++index)
      {
        if(testCase.outputMode == ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology)
        {
          expected[index] = expectedInput[index];
        }
        else
        {
          expected[index] = testCase.outputMode == ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology ? static_cast<T>(values[index] - expectedInput[index]) :
                                                                                                                                     static_cast<T>(expectedInput[index] - values[index]);
        }
      }
      std::vector<T> actual(count);
      auto copyIntoBufferResult = output.copyIntoBuffer(0, nonstd::span<T>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      REQUIRE(actual == expected);
      REQUIRE(input.readValues() == count);
      REQUIRE(output.writtenValues() == count);
    }
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: rolling top-hat pipeline handles a Z radius larger than the image", "[ImageProcessing][MorphologyEngine]")
{
  const SizeVec3 dims{5, 4, 2};
  const usize count = dims[0] * dims[1] * dims[2];
  const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 3});
  const std::vector<int32> values = MakePattern<int32>(dims[0], dims[1], dims[2]);
  DataStore<int32> input(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0);
  DataStore<int32> output(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 777);
  auto copyFromBufferResult = input.copyFromBuffer(0, values);
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<int32>(dims, se, false);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredBytesResult);
  std::atomic_bool shouldCancel{false};
  auto applyMorphologyCompositePipelineResult = ApplyMorphologyCompositePipeline<int32>(
      input, output, dims, se, MorphOp::Erode, MorphOp::Dilate, false, ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology, shouldCancel, requiredBytesResult.value());
  SIMPLNX_RESULT_REQUIRE_VALID(applyMorphologyCompositePipelineResult);
  const std::vector<int32> morphology = Oracle(Oracle(values, dims[0], dims[1], dims[2], se, MorphOp::Erode), dims[0], dims[1], dims[2], se, MorphOp::Dilate);
  std::vector<int32> actual(count);
  auto copyIntoBufferResult = output.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  for(usize index = 0; index < count; ++index)
  {
    REQUIRE(actual[index] == static_cast<int32>(values[index] - morphology[index]));
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: rolling composite pipeline uses true 2D Z collapse", "[ImageProcessing][MorphologyEngine]")
{
  const SizeVec3 dims{5, 4, 1};
  const usize count = dims[0] * dims[1];
  const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 2});
  const std::vector<int32> values = MakePattern<int32>(dims[0], dims[1], dims[2]);
  struct Case
  {
    MorphOp firstOp;
    MorphOp secondOp;
    ImageProcessing::detail::MorphologyCompositeOutputMode outputMode;
  };
  const std::array<Case, 4> cases = {{{MorphOp::Erode, MorphOp::Dilate, ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology},
                                      {MorphOp::Dilate, MorphOp::Erode, ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology},
                                      {MorphOp::Erode, MorphOp::Dilate, ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology},
                                      {MorphOp::Dilate, MorphOp::Erode, ImageProcessing::detail::MorphologyCompositeOutputMode::MorphologyMinusOriginal}}};
  for(const bool safeBorder : {false, true})
  {
    for(const Case testCase : cases)
    {
      TransferCountingDataStore<int32> input(ShapeType{1, dims[1], dims[0]}, ShapeType{1}, 0);
      TransferCountingDataStore<int32> output(ShapeType{1, dims[1], dims[0]}, ShapeType{1}, 777);
      auto copyFromBufferResult = input.copyFromBuffer(0, values);
      SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
      const auto requiredBytesResult = CalculateMorphologyCompositePipelineWorkingMemoryBytes<int32>(dims, se, safeBorder);
      SIMPLNX_RESULT_REQUIRE_VALID(requiredBytesResult);
      std::atomic_bool shouldCancel{false};
      auto applyMorphologyCompositePipelineResult =
          ApplyMorphologyCompositePipeline<int32>(input, output, dims, se, testCase.firstOp, testCase.secondOp, safeBorder, testCase.outputMode, shouldCancel, requiredBytesResult.value());
      SIMPLNX_RESULT_REQUIRE_VALID(applyMorphologyCompositePipelineResult);
      std::vector<int32> actual(count);
      auto copyIntoBufferResult = output.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);

      std::vector<int32> morphology;
      if(safeBorder)
      {
        const SizeVec3 paddedDims{dims[0] + 2, dims[1] + 2, 1};
        const int32 padValue = testCase.firstOp == MorphOp::Erode ? std::numeric_limits<int32>::max() : std::numeric_limits<int32>::lowest();
        std::vector<int32> padded(paddedDims[0] * paddedDims[1], padValue);
        for(usize y = 0; y < dims[1]; ++y)
        {
          std::copy_n(values.data() + y * dims[0], dims[0], padded.data() + (y + 1) * paddedDims[0] + 1);
        }
        const std::vector<int32> first = Oracle(padded, paddedDims[0], paddedDims[1], 1, se, testCase.firstOp);
        const std::vector<int32> second = Oracle(first, paddedDims[0], paddedDims[1], 1, se, testCase.secondOp);
        morphology.resize(count);
        for(usize y = 0; y < dims[1]; ++y)
        {
          std::copy_n(second.data() + (y + 1) * paddedDims[0] + 1, dims[0], morphology.data() + y * dims[0]);
        }
      }
      else
      {
        morphology = Oracle(Oracle(values, dims[0], dims[1], dims[2], se, testCase.firstOp), dims[0], dims[1], dims[2], se, testCase.secondOp);
      }
      for(usize index = 0; index < count; ++index)
      {
        int32 expected = morphology[index];
        if(testCase.outputMode != ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology)
        {
          expected = testCase.outputMode == ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology ? static_cast<int32>(values[index] - morphology[index]) :
                                                                                                                              static_cast<int32>(morphology[index] - values[index]);
        }
        REQUIRE(actual[index] == expected);
      }
      REQUIRE(input.readValues() == count);
      REQUIRE(output.writtenValues() == count);
    }
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: BinaryMorphScanline dilate/erode matches fg-count oracle (3D + 2D)", "[ImageProcessing][MorphologyEngine]", int32, uint8, float32)
{
  using T = TestType;
  SECTION("3D non-cubic 4x5x6")
  {
    CheckBinaryMorphology<T>(k_X, k_Y, k_Z);
  }
  SECTION("2D single-plane 4x5x1 (rz=1 kernel clipped to plane)")
  {
    CheckBinaryMorphology<T>(k_X, k_Y, 1);
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: explicit hand-computed Box radius-1 values", "[ImageProcessing][MorphologyEngine]")
{
  // Ramp value at (x,y,z) == flat index == z*20 + y*4 + x on the 4x5x6 image.
  const usize vol = k_X * k_Y * k_Z;
  std::vector<int32> ramp(vol);
  for(usize i = 0; i < vol; ++i)
  {
    ramp[i] = static_cast<int32>(i);
  }
  const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 1});
  const std::vector<int32> dil = RunScanline<int32>(ramp, k_X, k_Y, k_Z, se, MorphOp::Dilate);
  const std::vector<int32> ero = RunScanline<int32>(ramp, k_X, k_Y, k_Z, se, MorphOp::Erode);

  auto at = [&](usize x, usize y, usize z) { return FlatIndex(x, y, z, k_X, k_Y); };

  // Corner (0,0,0): in-bounds box neighbors span x,y,z in {0,1}. Max at (1,1,1)=25, min at (0,0,0)=0.
  REQUIRE(dil[at(0, 0, 0)] == 25);
  REQUIRE(ero[at(0, 0, 0)] == 0);

  // Interior (1,1,1): neighbors span {0,1,2} per axis. Max at (2,2,2)=50, min at (0,0,0)=0.
  REQUIRE(dil[at(1, 1, 1)] == 50);
  REQUIRE(ero[at(1, 1, 1)] == 0);

  // Far corner (3,4,5)=119: neighbors span x{2,3} y{3,4} z{4,5}. Max at (3,4,5)=119, min at (2,3,4)=94.
  REQUIRE(dil[at(3, 4, 5)] == 119);
  REQUIRE(ero[at(3, 4, 5)] == 94);
}

TEST_CASE("ImageProcessing::MorphologyEngine: gradient subtraction defines full-range integer wrap", "[ImageProcessing][MorphologyEngine]")
{
  REQUIRE(ImageProcessing::detail::MorphologyGradientDifference<int32>(std::numeric_limits<int32>::lowest(), std::numeric_limits<int32>::max()) == int32{1});
  REQUIRE(ImageProcessing::detail::MorphologyGradientDifference<int32>(std::numeric_limits<int32>::max(), std::numeric_limits<int32>::lowest()) == int32{-1});
  REQUIRE(ImageProcessing::detail::MorphologyGradientDifference<int64>(std::numeric_limits<int64>::lowest(), std::numeric_limits<int64>::max()) == int64{1});
  REQUIRE(ImageProcessing::detail::MorphologyGradientDifference<int64>(std::numeric_limits<int64>::max(), std::numeric_limits<int64>::lowest()) == int64{-1});
  REQUIRE(ImageProcessing::detail::MorphologyGradientDifference<uint32>(std::numeric_limits<uint32>::lowest(), std::numeric_limits<uint32>::max()) == uint32{1});
  REQUIRE(ImageProcessing::detail::MorphologyGradientDifference<uint32>(std::numeric_limits<uint32>::max(), std::numeric_limits<uint32>::lowest()) == std::numeric_limits<uint32>::max());
}

TEST_CASE("ImageProcessing::MorphologyEngine: interior X range excludes incomplete neighborhoods", "[ImageProcessing][MorphologyEngine]")
{
  struct Case
  {
    usize boundDimX;
    usize outputOriginX;
    usize outputWidth;
    usize radiusX;
    usize expectedBegin;
    usize expectedEnd;
  };
  const std::array<Case, 6> cases = {{{9, 0, 9, 2, 2, 7}, {9, 1, 4, 2, 1, 4}, {9, 3, 2, 2, 0, 2}, {4, 0, 4, 2, 0, 0}, {9, 7, 2, 2, 0, 0}, {9, 2, 4, 0, 0, 4}}};
  for(const Case testCase : cases)
  {
    const auto [begin, end] = ImageProcessing::detail::MorphologyInteriorXRange(testCase.boundDimX, testCase.outputOriginX, testCase.outputWidth, testCase.radiusX);
    REQUIRE(begin == testCase.expectedBegin);
    REQUIRE(end == testCase.expectedEnd);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: first non-binary value is deterministic across parallel ranges", "[ImageProcessing][MorphologyEngine]", uint8, int32)
{
  using T = TestType;
  static constexpr usize k_Count = (usize{1} << 20) + 37;
  const T fg = T{1};
  const T bg = T{0};
  const auto makeBuffer = [fg](T background) {
    std::vector<T> buffer(k_Count);
    for(usize i = 0; i < k_Count; ++i)
    {
      buffer[i] = (i % usize{2} == usize{0}) ? fg : background;
    }
    return buffer;
  };

  SECTION("All values are binary")
  {
    const std::vector<T> buffer = makeBuffer(bg);
    const auto result = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>(buffer.data(), buffer.size()), fg, bg);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("One invalid value")
  {
    std::vector<T> buffer = makeBuffer(bg);
    buffer[usize{777777}] = T{7};
    const auto result = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>(buffer.data(), buffer.size()), fg, bg);
    REQUIRE(result == usize{777777});
  }

  SECTION("The smallest invalid index wins")
  {
    std::vector<T> buffer = makeBuffer(bg);
    buffer[usize{900000}] = T{7};
    buffer[usize{123}] = T{7};
    buffer[usize{500000}] = T{7};
    const auto result = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>(buffer.data(), buffer.size()), fg, bg);
    REQUIRE(result == usize{123});
  }

  SECTION("Boundary invalid indexes")
  {
    std::vector<T> buffer = makeBuffer(bg);
    buffer[usize{0}] = T{7};
    buffer[k_Count - usize{1}] = T{7};
    const auto firstResult = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>(buffer.data(), buffer.size()), fg, bg);
    REQUIRE(firstResult == usize{0});

    std::vector<T> lastBuffer = makeBuffer(bg);
    lastBuffer[k_Count - usize{1}] = T{7};
    const auto lastResult = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>(lastBuffer.data(), lastBuffer.size()), fg, bg);
    REQUIRE(lastResult == k_Count - usize{1});
  }

  SECTION("Empty span")
  {
    const auto result = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>{}, fg, bg);
    REQUIRE_FALSE(result.has_value());
  }

  if constexpr(std::is_same_v<T, int32>)
  {
    SECTION("Negative background and invalid values")
    {
      const T negativeBg = T{-5};
      std::vector<T> buffer = makeBuffer(negativeBg);
      buffer[usize{2468}] = T{-6};
      const auto result = ImageProcessing::detail::FindFirstNonBinaryValue(nonstd::span<const T>(buffer.data(), buffer.size()), fg, negativeBg);
      REQUIRE(result == usize{2468});
    }
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: binary input scan reports global indexes across chunks", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_ChunkValues = 8;
  constexpr uint8 k_Foreground = 1;
  constexpr uint8 k_Background = 0;
  const DataPath inputPath({"Image", "Cell", "Input"});
  const auto makeValues = [](usize size) {
    std::vector<uint8> values(size);
    for(usize i = 0; i < size; ++i)
    {
      values[i] = (i % usize{2} == usize{0}) ? k_Background : k_Foreground;
    }
    return values;
  };
  const auto expectedMessage = [&inputPath](uint8 value, usize index) {
    return fmt::format("Binary morphology requires a binary image containing only the foreground ({}) or background ({}) value, but input array '{}' contains the value {} at index {}. Threshold or "
                       "relabel the input first.",
                       static_cast<int64>(k_Foreground), static_cast<int64>(k_Background), inputPath.toString(), static_cast<int64>(value), index);
  };

  SECTION("All chunks contain only binary values")
  {
    DataStore<uint8> store = MakeStore(makeValues(21), 21, 1, 1);
    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
  }

  SECTION("A violation in the second chunk reports its global index")
  {
    std::vector<uint8> values = makeValues(21);
    values[11] = uint8{7};
    DataStore<uint8> store = MakeStore(values, 21, 1, 1);
    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
    REQUIRE(result.errors()[0].message == expectedMessage(uint8{7}, usize{11}));
  }

  SECTION("Two full chunks report the last index")
  {
    std::vector<uint8> values = makeValues(16);
    values[15] = uint8{7};
    DataStore<uint8> store = MakeStore(values, 16, 1, 1);
    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
    REQUIRE(result.errors()[0].message == expectedMessage(uint8{7}, usize{15}));
  }

  SECTION("A final partial chunk reports its last index")
  {
    std::vector<uint8> values = makeValues(21);
    values[20] = uint8{7};
    DataStore<uint8> store = MakeStore(values, 21, 1, 1);
    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
    REQUIRE(result.errors()[0].message == expectedMessage(uint8{7}, usize{20}));
  }

  SECTION("The earliest invalid chunk stops later reads")
  {
    std::vector<uint8> values = makeValues(21);
    values[11] = uint8{7};
    values[20] = uint8{9};
    TransferCountingDataStore<uint8> store(ShapeType{1, 1, 21}, ShapeType{1}, uint8{0});
    auto copyFromBufferResult = store.copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
    std::atomic_bool shouldCancel{false};
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
    REQUIRE(result.errors()[0].message == expectedMessage(uint8{7}, usize{11}));
    REQUIRE(store.readValues() == usize{16});
  }

  SECTION("A pre-cancelled scan reads nothing and returns a valid result")
  {
    std::vector<uint8> values = makeValues(21);
    values[11] = uint8{7};
    TransferCountingDataStore<uint8> store(ShapeType{1, 1, 21}, ShapeType{1}, uint8{0});
    auto copyFromBufferResult2 = store.copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult2);
    std::atomic_bool shouldCancel{true};
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
    REQUIRE(store.readValues() == usize{0});
  }

  SECTION("Cancellation during a violating chunk read still reports the violation")
  {
    std::vector<uint8> values = makeValues(21);
    values[3] = uint8{7};
    std::atomic_bool shouldCancel{false};
    CancelOnReadDataStore store(ShapeType{1, 1, 21}, ShapeType{1}, uint8{0}, shouldCancel);
    auto copyFromBufferResult3 = store.copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult3);
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.invalid());
    REQUIRE_FALSE(result.errors().empty());
    REQUIRE(result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
    REQUIRE(result.errors()[0].message == expectedMessage(uint8{7}, usize{3}));
    REQUIRE(shouldCancel.load() == true);
  }

  SECTION("Cancellation after a binary chunk read stops before the next chunk")
  {
    std::vector<uint8> values = makeValues(21);
    values[11] = uint8{7};
    std::atomic_bool shouldCancel{false};
    CancelOnReadDataStore store(ShapeType{1, 1, 21}, ShapeType{1}, uint8{0}, shouldCancel);
    auto copyFromBufferResult4 = store.copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult4);
    const Result<> result = ImageProcessing::detail::ScanBinaryInput<uint8>(store, k_Foreground, k_Background, inputPath, shouldCancel, k_ChunkValues);
    REQUIRE(result.valid());
    REQUIRE(store.readValues() == usize{8});
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: smaller invalid index wins regardless of publish order", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_Size = 1'000'000;
  std::atomic<usize> largerFirst{k_Size};
  ImageProcessing::detail::PublishSmallerIndex(largerFirst, usize{900000});
  ImageProcessing::detail::PublishSmallerIndex(largerFirst, usize{123});
  REQUIRE(largerFirst.load(std::memory_order_relaxed) == usize{123});

  std::atomic<usize> smallerFirst{k_Size};
  ImageProcessing::detail::PublishSmallerIndex(smallerFirst, usize{123});
  ImageProcessing::detail::PublishSmallerIndex(smallerFirst, usize{900000});
  REQUIRE(smallerFirst.load(std::memory_order_relaxed) == usize{123});

  std::atomic<usize> equalValue{usize{123}};
  ImageProcessing::detail::PublishSmallerIndex(equalValue, usize{123});
  REQUIRE(equalValue.load(std::memory_order_relaxed) == usize{123});

  std::atomic<usize> largerValue{usize{123}};
  ImageProcessing::detail::PublishSmallerIndex(largerValue, usize{900000});
  REQUIRE(largerValue.load(std::memory_order_relaxed) == usize{123});
}

TEST_CASE("ImageProcessing::MorphologyEngine: composite plane subtraction supports independent row ranges", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_OutputWidth = 4;
  constexpr usize k_OutputHeight = 3;
  constexpr usize k_OriginalStride = 7;
  constexpr usize k_CropX = 2;
  constexpr usize k_CropY = 1;
  std::array<int32, k_OriginalStride*(k_OutputHeight + 2)> original{};
  std::array<int32, k_OutputWidth * k_OutputHeight> morphology{};
  for(usize y = 0; y < k_OutputHeight; ++y)
  {
    for(usize x = 0; x < k_OutputWidth; ++x)
    {
      const usize outputIndex = y * k_OutputWidth + x;
      original[(k_CropY + y) * k_OriginalStride + k_CropX + x] = static_cast<int32>(40 + outputIndex * 3);
      morphology[outputIndex] = static_cast<int32>(7 + outputIndex);
    }
  }

  auto originalMinusMorphology = morphology;
  ImageProcessing::detail::SubtractMorphologyPlaneRows(original.data(), originalMinusMorphology.data(), k_OriginalStride, k_OutputWidth, k_CropX, k_CropY,
                                                       ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology, 0, 1);
  ImageProcessing::detail::SubtractMorphologyPlaneRows(original.data(), originalMinusMorphology.data(), k_OriginalStride, k_OutputWidth, k_CropX, k_CropY,
                                                       ImageProcessing::detail::MorphologyCompositeOutputMode::OriginalMinusMorphology, 1, k_OutputHeight);

  auto morphologyMinusOriginal = morphology;
  ImageProcessing::detail::SubtractMorphologyPlaneRows(original.data(), morphologyMinusOriginal.data(), k_OriginalStride, k_OutputWidth, k_CropX, k_CropY,
                                                       ImageProcessing::detail::MorphologyCompositeOutputMode::MorphologyMinusOriginal, 0, 2);
  ImageProcessing::detail::SubtractMorphologyPlaneRows(original.data(), morphologyMinusOriginal.data(), k_OriginalStride, k_OutputWidth, k_CropX, k_CropY,
                                                       ImageProcessing::detail::MorphologyCompositeOutputMode::MorphologyMinusOriginal, 2, k_OutputHeight);

  for(usize index = 0; index < morphology.size(); ++index)
  {
    const int32 originalValue = static_cast<int32>(40 + index * 3);
    REQUIRE(originalMinusMorphology[index] == originalValue - morphology[index]);
    REQUIRE(morphologyMinusOriginal[index] == morphology[index] - originalValue);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: Direct matches Scanline exactly", "[ImageProcessing][MorphologyEngine]", int32, int8, uint8, uint16, float32)
{
  using T = TestType;
  // Larger, non-cubic-friendly cube with a varied pattern; radii include a symmetric {2,2,2}, an asymmetric
  // {3,1,2}, and a zero-Z-axis {2,2,0} so a per-axis stride/offset bug cannot hide. int8/uint8 exercise both
  // the moderate flat fold and the larger-neighborhood dense histogram; wider types exercise the map histogram.
  SECTION("3D 20x20x20")
  {
    CrossValidateDirectVsScanline<T>(20, 20, 20);
  }
  SECTION("2D single-plane 20x20x1 (rz kernel clipped to the plane)")
  {
    CrossValidateDirectVsScanline<T>(20, 20, 1);
  }
  SECTION("small 3x3x3 (heavy clipping; all-OOB Annulus corners hit the boundary-only seed)")
  {
    CrossValidateDirectVsScanline<T>(3, 3, 3);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: fused Gradient Direct == Scanline == dilate-minus-erode oracle", "[ImageProcessing][MorphologyEngine]", int32, int8, uint8, uint16, float32)
{
  using T = TestType;
  // Fused single-pass max - min (in-core Direct + OOC Scanline) vs the independent dilate-minus-erode oracle,
  // over all four SE shapes and a symmetric {2,2,2}, asymmetric {3,1,2}, and zero-Z {2,2,0} radius. int8/uint8
  // exercise the flat fold and dense both-extremes histogram; wider types exercise the map. The 3x3x3
  // section hits the all-OOB Annulus corners, gating the empty-window lowest()-max() seed bit-for-bit.
  SECTION("3D 20x20x20")
  {
    CrossValidateGradient<T>(20, 20, 20);
  }
  SECTION("2D single-plane 20x20x1 (rz kernel clipped to the plane)")
  {
    CrossValidateGradient<T>(20, 20, 1);
  }
  SECTION("small 3x3x3 (heavy clipping; all-OOB Annulus corners hit the empty-window seed)")
  {
    CrossValidateGradient<T>(3, 3, 3);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: BinaryMorphDirect matches BinaryMorphScanline exactly", "[ImageProcessing][MorphologyEngine]", int32, int8, uint8)
{
  using T = TestType;
  // Adaptive binary Direct path vs the oracle-verified Scanline reference, over a label volume carrying
  // non-{fg,bg} labels (0,1,2,3,5), all four SE shapes, symmetric {2,2,2} + asymmetric {3,1,2} + zero-Z {2,2,0}
  // radii, both ops, and both boundary-fill modes. Direct MUST equal Scanline bit-for-bit.
  SECTION("3D 20x20x20")
  {
    CrossValidateBinaryDirectVsScanline<T>(20, 20, 20);
  }
  SECTION("2D single-plane 20x20x1 (rz kernel clipped to the plane)")
  {
    CrossValidateBinaryDirectVsScanline<T>(20, 20, 1);
  }
  SECTION("small 3x3x3 (heavy clipping; all-OOB Annulus corners)")
  {
    CrossValidateBinaryDirectVsScanline<T>(3, 3, 3);
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: Direct matches Scanline on float data with NaNs", "[ImageProcessing][MorphologyEngine]")
{
  // NaN would violate the std::map comparator's strict-weak-ordering on the Direct path; the engine skips
  // it, exactly as MorphScanline's v>acc / v<acc comparisons never let a NaN win. Both paths must still
  // agree bit-for-bit. Deterministic NaN injection over a moderate volume.
  constexpr usize dimX = 12;
  constexpr usize dimY = 12;
  constexpr usize dimZ = 12;
  std::vector<float32> input = MakePattern<float32>(dimX, dimY, dimZ);
  for(usize i = 0; i < input.size(); ++i)
  {
    if(i % 13 == 0)
    {
      input[i] = std::numeric_limits<float32>::quiet_NaN();
    }
  }

  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<MorphOp> ops = {MorphOp::Dilate, MorphOp::Erode};
  for(KernelType kt : kernels)
  {
    const StructuringElement se = MakeStructuringElement(kt, {2, 2, 2});
    for(MorphOp op : ops)
    {
      const std::vector<float32> scan = RunScanline(input, dimX, dimY, dimZ, se, op);
      const std::vector<float32> direct = RunDirect(input, dimX, dimY, dimZ, se, op);
      for(usize i = 0; i < input.size(); ++i)
      {
        INFO("kernel=" << KernelName(kt) << " op=" << (op == MorphOp::Dilate ? "Dilate" : "Erode") << " index=" << i);
        REQUIRE(direct[i] == scan[i]);
      }
    }
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: ApplyMorphology entry point dispatches Direct/Scanline", "[ImageProcessing][MorphologyEngine]")
{
  // Exercises the public ApplyMorphology entry, which now routes through DispatchAlgorithm. Both the
  // in-core Direct path and the OOC Scanline path must agree with the independent oracle. The force
  // guards select the path deterministically regardless of the build's storage type.
  const usize vol = k_X * k_Y * k_Z;
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {1, 1, 1});

  auto runAndCheck = [&](MorphOp op) {
    DataStructure dataStructure;
    auto* inArray = UnitTest::CreateTestDataArray<int32>(dataStructure, "in", {k_Z, k_Y, k_X}, {1});
    auto* outArray = UnitTest::CreateTestDataArray<int32>(dataStructure, "out", {k_Z, k_Y, k_X}, {1});

    std::vector<int32> ramp(vol);
    for(usize i = 0; i < vol; ++i)
    {
      ramp[i] = static_cast<int32>((i * 7) % 53); // non-monotonic so the winner is not a fixed neighbor
      inArray->getDataStoreRef().setValue(i, ramp[i]);
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};

    const Result<> result = ApplyMorphology<int32>(inArray->getDataStoreRef(), outArray->getDataStoreRef(), SizeVec3{k_X, k_Y, k_Z}, se, op, *inArray, *outArray, shouldCancel, messageHandler);
    REQUIRE(result.valid());

    const std::vector<int32> expected = Oracle(ramp, k_X, k_Y, k_Z, se, op);
    for(usize i = 0; i < vol; ++i)
    {
      INFO("index=" << i);
      REQUIRE(outArray->getDataStoreRef().getValue(i) == expected[i]);
    }
  };

  SECTION("in-core Direct path")
  {
    const ForceInCoreAlgorithmGuard guard;
    runAndCheck(MorphOp::Dilate);
    runAndCheck(MorphOp::Erode);
  }
  SECTION("OOC Scanline path")
  {
    const ForceOocAlgorithmGuard guard(true);
    runAndCheck(MorphOp::Dilate);
    runAndCheck(MorphOp::Erode);
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: OOC scanline slab batches preserve a short tail exactly", "[ImageProcessing][MorphologyEngine]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceOutOfCore, 0);

  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 8;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr usize k_VolumeValues = k_SliceValues * k_DimZ;
  constexpr usize k_TargetBytes = 8 * k_SliceValues * sizeof(int32); // radius-1 halo + 2*3 output planes => batches 3,3,2
  constexpr int32 k_Foreground = 1;
  constexpr int32 k_Background = 0;

  std::vector<int32> input(k_VolumeValues);
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = static_cast<int32>((i * 11 + i / k_SliceValues * 7) % 19);
  }
  for(usize i = 0; i < input.size(); i += 5)
  {
    input[i] = k_Foreground;
  }

  DataStructure dataStructure;
  const ShapeType tupleShape{k_DimZ, k_DimY, k_DimX};
  auto inputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Input"}), tupleShape, {1});
  auto morphStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Morph"}), tupleShape, {1});
  auto gradientStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Gradient"}), tupleShape, {1});
  auto binaryStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Binary"}), tupleShape, {1});
  REQUIRE(inputStore != nullptr);
  REQUIRE(morphStore != nullptr);
  REQUIRE(gradientStore != nullptr);
  REQUIRE(binaryStore != nullptr);
  auto copyFromBufferResult = inputStore->copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  const auto application = Application::Instance();
  REQUIRE(application != nullptr);
  if(application->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(inputStore->getStoreType() == IDataStore::StoreType::OutOfCore);
    REQUIRE(morphStore->getStoreType() == IDataStore::StoreType::OutOfCore);
    REQUIRE(gradientStore->getStoreType() == IDataStore::StoreType::OutOfCore);
    REQUIRE(binaryStore->getStoreType() == IDataStore::StoreType::OutOfCore);
  }

  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {1, 1, 1});
  ImageProcessing::detail::MorphologySlabPlan plan;
  auto makeMorphologySlabPlanResult = ImageProcessing::detail::MakeMorphologySlabPlan(*inputStore, *morphStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, 1, k_TargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult);
  REQUIRE(plan.outputBatchDepth == 3);
  REQUIRE(k_DimZ % plan.outputBatchDepth == 2);

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MorphScanline<int32> morphAlgorithm{*inputStore, *morphStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, MorphOp::Erode, shouldCancel, messageHandler, k_TargetBytes};
  MorphGradientScanline<int32> gradientAlgorithm{*inputStore, *gradientStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, shouldCancel, messageHandler, k_TargetBytes};
  BinaryMorphScanline<int32> binaryAlgorithm{*inputStore,    *binaryStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, MorphOp::Dilate, k_Foreground, k_Background, false, shouldCancel,
                                             messageHandler, k_TargetBytes};
  auto morphAlgorithmResult = morphAlgorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(morphAlgorithmResult);
  auto gradientAlgorithmResult = gradientAlgorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(gradientAlgorithmResult);
  auto binaryAlgorithmResult = binaryAlgorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(binaryAlgorithmResult);

  const std::vector<int32> expectedMorph = Oracle(input, k_DimX, k_DimY, k_DimZ, se, MorphOp::Erode);
  const std::vector<int32> expectedGradient = GradientOracle(input, k_DimX, k_DimY, k_DimZ, se);
  const std::vector<int32> expectedBinary = BinaryOracle(input, k_DimX, k_DimY, k_DimZ, se, MorphOp::Dilate, k_Foreground, k_Background, false);
  std::vector<int32> actual(k_VolumeValues);
  auto copyIntoBufferResult = morphStore->copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expectedMorph);
  auto copyIntoBufferResult2 = gradientStore->copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
  REQUIRE(actual == expectedGradient);
  auto copyIntoBufferResult3 = binaryStore->copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult3);
  REQUIRE(actual == expectedBinary);
}

TEST_CASE("ImageProcessing::MorphologyEngine: BinaryMorphScanline reads each 256-plane input once", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 256;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr usize k_TotalValues = k_SliceValues * k_DimZ;
  constexpr usize k_TargetBytes = 128 * k_SliceValues * sizeof(int32);
  constexpr int32 k_Foreground = 1;
  constexpr int32 k_Background = 0;
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {2, 2, 2});
  const std::vector<int32> input = MakeBinaryPattern<int32>(k_DimX, k_DimY, k_DimZ);
  TransferCountingDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  TransferCountingDataStore<int32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  ImageProcessing::detail::MorphologySlabPlan plan;
  auto makeMorphologySlabPlanResult = ImageProcessing::detail::MakeMorphologySlabPlan(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, 2, k_TargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult);
  REQUIRE(plan.outputBatchDepth == 52);
  REQUIRE(k_DimZ % plan.outputBatchDepth == 48);

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  BinaryMorphScanline<int32> algorithm{inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, MorphOp::Dilate, k_Foreground, k_Background, false, shouldCancel, messageHandler, k_TargetBytes};
  auto algorithmResult = algorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(algorithmResult);

  std::vector<int32> actual(k_TotalValues);
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == BinaryOracle(input, k_DimX, k_DimY, k_DimZ, se, MorphOp::Dilate, k_Foreground, k_Background, false));
  REQUIRE(inputStore.readValues() == k_TotalValues);
  REQUIRE(outputStore.writtenValues() == k_TotalValues);
  REQUIRE(inputStore.readBatchSizes() == std::vector<usize>{54 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 46 * k_SliceValues});
  REQUIRE(outputStore.writeBatchSizes() == std::vector<usize>{52 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 48 * k_SliceValues});
}

TEST_CASE("ImageProcessing::MorphologyEngine: MorphGradientScanline reads each 256-plane input once", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 256;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr usize k_TotalValues = k_SliceValues * k_DimZ;
  constexpr usize k_TargetBytes = 128 * k_SliceValues * sizeof(int32);
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {2, 2, 2});
  const std::vector<int32> input = MakePattern<int32>(k_DimX, k_DimY, k_DimZ);
  TransferCountingDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  TransferCountingDataStore<int32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int32{0});
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

  ImageProcessing::detail::MorphologySlabPlan plan;
  auto makeMorphologySlabPlanResult = ImageProcessing::detail::MakeMorphologySlabPlan(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, 2, k_TargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult);
  REQUIRE(plan.outputBatchDepth == 52);
  REQUIRE(k_DimZ % plan.outputBatchDepth == 48);

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MorphGradientScanline<int32> algorithm{inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, shouldCancel, messageHandler, k_TargetBytes};
  auto algorithmResult = algorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(algorithmResult);

  std::vector<int32> actual(k_TotalValues);
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == GradientOracle(input, k_DimX, k_DimY, k_DimZ, se));
  REQUIRE(inputStore.readValues() == k_TotalValues);
  REQUIRE(outputStore.writtenValues() == k_TotalValues);
  REQUIRE(inputStore.readBatchSizes() == std::vector<usize>{54 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 46 * k_SliceValues});
  REQUIRE(outputStore.writeBatchSizes() == std::vector<usize>{52 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 52 * k_SliceValues, 48 * k_SliceValues});
}

TEST_CASE("ImageProcessing::MorphologyEngine: full-overwrite morphology filters defer output initialization", "[ImageProcessing][MorphologyEngine]")
{
  DataStructure dataStructure;
  const DataPath inputPath = BuildMorphologyFilterPreflightInput(dataStructure);
  RequireDeferredMorphologyOutputAction<BinaryDilateImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<BinaryErodeImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<BinaryMorphologicalOpeningImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<BinaryMorphologicalClosingImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<GrayscaleDilateImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<GrayscaleErodeImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<GrayscaleMorphologicalOpeningImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<GrayscaleMorphologicalClosingImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<WhiteTopHatImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<BlackTopHatImageFilter>(dataStructure, inputPath);
  RequireDeferredMorphologyOutputAction<MorphologicalGradientImageFilter>(dataStructure, inputPath);
}

TEST_CASE("ImageProcessing::MorphologyEngine: bounded 2D row blocks and overwide tiles match independent oracles", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  constexpr usize fullWidthTargetBytes = 216; // six int32 rows => two output rows plus two Y halos across input/output
  constexpr usize overwideTargetBytes = 64;   // three halo rows plus one output row force two-column X tiles
  constexpr int32 foreground = 1;
  constexpr int32 background = 0;
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {1, 1, 0});
  const std::vector<int32> grayscaleInput = MakePattern<int32>(dimX, dimY, dimZ);
  const std::vector<int32> binaryInput = MakeBinaryPattern<int32>(dimX, dimY, dimZ);

  for(const usize targetBytes : {fullWidthTargetBytes, overwideTargetBytes})
  {
    for(const MorphOp op : {MorphOp::Dilate, MorphOp::Erode})
    {
      DataStore<int32> grayscaleStore = MakeStore(grayscaleInput, dimX, dimY, dimZ);
      DataStore<int32> grayscaleOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
      std::atomic_bool shouldCancel{false};
      IFilter::MessageHandler messageHandler{};
      MorphScanline<int32> grayscaleAlgorithm{grayscaleStore, grayscaleOutput, SizeVec3{dimX, dimY, dimZ}, se, op, shouldCancel, messageHandler, targetBytes};
      auto grayscaleAlgorithmResult = grayscaleAlgorithm();
      SIMPLNX_RESULT_REQUIRE_VALID(grayscaleAlgorithmResult);
      std::vector<int32> actual(grayscaleInput.size());
      auto copyIntoBufferResult = grayscaleOutput.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      REQUIRE(actual == Oracle(grayscaleInput, dimX, dimY, dimZ, se, op));

      for(const bool boundaryToForeground : {false, true})
      {
        DataStore<int32> binaryStore = MakeStore(binaryInput, dimX, dimY, dimZ);
        DataStore<int32> binaryOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
        BinaryMorphScanline<int32> binaryAlgorithm{binaryStore,    binaryOutput, SizeVec3{dimX, dimY, dimZ}, se, op, foreground, background, boundaryToForeground, shouldCancel,
                                                   messageHandler, targetBytes};
        auto binaryAlgorithmResult = binaryAlgorithm();
        SIMPLNX_RESULT_REQUIRE_VALID(binaryAlgorithmResult);
        auto copyIntoBufferResult2 = binaryOutput.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
        SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
        REQUIRE(actual == BinaryOracle(binaryInput, dimX, dimY, dimZ, se, op, foreground, background, boundaryToForeground));
      }
    }
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: bounded gradient 2D transfers never span the complete plane", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  constexpr usize totalValues = dimX * dimY;
  const std::vector<int32> input = MakePattern<int32>(dimX, dimY, dimZ);
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {1, 1, 0});

  struct TransferCase
  {
    const char* label;
    usize targetBytes;
    usize maximumReadValues;
    usize maximumWriteValues;
  };
  const std::array<TransferCase, 2> transferCases = {{{"full-width blocks", 216, 9, 18}, {"overwide X tiles", 64, 4, 2}}};

  for(const TransferCase& transferCase : transferCases)
  {
    DYNAMIC_SECTION(transferCase.label)
    {
      TransferCountingDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
      TransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
      auto copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);

      std::atomic_bool shouldCancel{false};
      IFilter::MessageHandler messageHandler{};
      MorphGradientScanline<int32> gradient{inputStore, outputStore, SizeVec3{dimX, dimY, dimZ}, se, shouldCancel, messageHandler, transferCase.targetBytes};
      auto gradientResult = gradient();
      SIMPLNX_RESULT_REQUIRE_VALID(gradientResult);

      std::vector<int32> actual(totalValues);
      auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      REQUIRE(actual == GradientOracle(input, dimX, dimY, dimZ, se));
      CAPTURE(transferCase.label, inputStore.maxReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues());
      REQUIRE(inputStore.maxReadValues() == transferCase.maximumReadValues);
      REQUIRE(outputStore.maxWriteValues() == transferCase.maximumWriteValues);
      REQUIRE(outputStore.writtenValues() == totalValues);
    }
  }

  SECTION("empty structuring element uses bounded zero fills")
  {
    constexpr usize targetBytes = 32;
    constexpr usize maximumWriteValues = targetBytes / sizeof(int32);
    const StructuringElement emptySe = MakeStructuringElement(KernelType::Annulus, {0, 0, 0});
    REQUIRE(emptySe.offsets.empty());

    TransferCountingDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
    TransferCountingDataStore<int32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{7});
    auto copyFromBufferResult2 = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult2);

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    MorphGradientScanline<int32> gradient{inputStore, outputStore, SizeVec3{dimX, dimY, dimZ}, emptySe, shouldCancel, messageHandler, targetBytes};
    auto gradientResult2 = gradient();
    SIMPLNX_RESULT_REQUIRE_VALID(gradientResult2);

    std::vector<int32> actual(totalValues);
    auto copyIntoBufferResult2 = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
    REQUIRE(actual == std::vector<int32>(totalValues, 0));
    CAPTURE(inputStore.maxReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues());
    REQUIRE(inputStore.maxReadValues() == 0);
    REQUIRE(outputStore.maxWriteValues() == maximumWriteValues);
    REQUIRE(outputStore.writtenValues() == totalValues);
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: fixed-input 2D route requires one output row to fit its allowance", "[ImageProcessing][MorphologyEngine]")
{
  using ImageProcessing::detail::CanUseFixedMorphologyInput2D;

  REQUIRE(CanUseFixedMorphologyInput2D(/*dimX=*/16, /*totalValues=*/48, /*inputCapacityValues=*/48, /*outputCapacityValues=*/16));
  REQUIRE_FALSE(CanUseFixedMorphologyInput2D(/*dimX=*/17, /*totalValues=*/17, /*inputCapacityValues=*/48, /*outputCapacityValues=*/16));
  REQUIRE_FALSE(CanUseFixedMorphologyInput2D(/*dimX=*/16, /*totalValues=*/49, /*inputCapacityValues=*/48, /*outputCapacityValues=*/16));
}

/**
 * @brief Reports an oversized tuple count without allocating the values.
 *
 * MakeMorphologySlabPlan reads only the store sizes. getSize() multiplies the two
 * virtual counts, so this stub makes the plane-overflow branch reachable.
 */
class OversizedVolumeDataStore : public DataStore<int32>
{
public:
  OversizedVolumeDataStore(usize reportedTupleCount)
  : DataStore<int32>({1}, {1}, 0)
  , m_ReportedTupleCount(reportedTupleCount)
  {
  }

  usize getNumberOfTuples() const override
  {
    return m_ReportedTupleCount;
  }

private:
  usize m_ReportedTupleCount = 0;
};

TEST_CASE("ImageProcessing::MorphologyEngine: slab planner validates dimensions and store sizes", "[ImageProcessing][MorphologyEngine]")
{
  auto requireErrorCode = [](const Result<>& result, int32 code) {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == code);
  };

  DataStore<int32> oneValueStore({1}, {1}, 0);
  ImageProcessing::detail::MorphologySlabPlan plan;

  if constexpr(std::numeric_limits<usize>::max() > static_cast<usize>(std::numeric_limits<int64>::max()))
  {
    constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
    const Result<> signedRangeResult =
        ImageProcessing::detail::MakeMorphologySlabPlan(oneValueStore, oneValueStore, SizeVec3{k_Int64Max + 1, 1, 1}, 0, ImageProcessing::detail::k_MorphologySlabTargetBytes, plan);
    requireErrorCode(signedRangeResult, -8650);

    const Result<> volumeOverflowResult =
        ImageProcessing::detail::MakeMorphologySlabPlan(oneValueStore, oneValueStore, SizeVec3{k_Int64Max, 3, 1}, 0, ImageProcessing::detail::k_MorphologySlabTargetBytes, plan);
    requireErrorCode(volumeOverflowResult, -8651);

    OversizedVolumeDataStore hugeStore(k_Int64Max);
    const Result<> planeOverflowResult =
        ImageProcessing::detail::MakeMorphologySlabPlan(hugeStore, hugeStore, SizeVec3{k_Int64Max, 1, 1}, 0, ImageProcessing::detail::k_MorphologySlabTargetBytes, plan);
    requireErrorCode(planeOverflowResult, -8654);
  }

  DataStore<int32> sevenValueStore({7}, {1}, 0);
  DataStore<int32> eightValueStore({8}, {1}, 0);
  const Result<> inputMismatchResult =
      ImageProcessing::detail::MakeMorphologySlabPlan(sevenValueStore, eightValueStore, SizeVec3{2, 2, 2}, 1, ImageProcessing::detail::k_MorphologySlabTargetBytes, plan);
  requireErrorCode(inputMismatchResult, -8652);

  const Result<> outputMismatchResult =
      ImageProcessing::detail::MakeMorphologySlabPlan(eightValueStore, sevenValueStore, SizeVec3{2, 2, 2}, 1, ImageProcessing::detail::k_MorphologySlabTargetBytes, plan);
  requireErrorCode(outputMismatchResult, -8653);
}

TEST_CASE("ImageProcessing::MorphologyEngine: slab planner bounds normal batches and retains required large-radius halo", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 8;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr usize k_PlaneBytes = k_SliceValues * sizeof(int32);
  DataStore<int32> inputStore({k_DimZ, k_DimY, k_DimX}, {1}, 0);
  DataStore<int32> outputStore({k_DimZ, k_DimY, k_DimX}, {1}, 0);
  ImageProcessing::detail::MorphologySlabPlan plan;

  constexpr usize k_HappyTargetBytes = 8 * k_PlaneBytes;
  auto makeMorphologySlabPlanResult = ImageProcessing::detail::MakeMorphologySlabPlan(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, 1, k_HappyTargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult);
  REQUIRE(plan.outputBatchDepth == 3);
  REQUIRE(plan.maxInputDepth == 5);
  REQUIRE((plan.outputBatchDepth + plan.maxInputDepth) * k_PlaneBytes <= k_HappyTargetBytes);

  constexpr usize k_NormalizedTargetBytes = 12 * k_PlaneBytes;
  auto makeMorphologySlabPlanResult2 = ImageProcessing::detail::MakeMorphologySlabPlan(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, 1, k_NormalizedTargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult2);
  REQUIRE(plan.outputBatchDepth == 4);
  REQUIRE(plan.maxInputDepth == 6);

  constexpr usize k_OnePlaneTargetBytes = 2 * k_PlaneBytes;
  auto makeMorphologySlabPlanResult3 = ImageProcessing::detail::MakeMorphologySlabPlan(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, 1, k_OnePlaneTargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult3);
  REQUIRE(plan.outputBatchDepth == 1);

  constexpr usize k_LargeRadiusDimZ = 4;
  DataStore<int32> shallowInput({k_LargeRadiusDimZ, k_DimY, k_DimX}, {1}, 0);
  DataStore<int32> shallowOutput({k_LargeRadiusDimZ, k_DimY, k_DimX}, {1}, 0);
  constexpr usize k_LargeRadiusTargetBytes = 3 * k_PlaneBytes;
  auto makeMorphologySlabPlanResult4 = ImageProcessing::detail::MakeMorphologySlabPlan(shallowInput, shallowOutput, SizeVec3{k_DimX, k_DimY, k_LargeRadiusDimZ}, 10, k_LargeRadiusTargetBytes, plan);
  SIMPLNX_RESULT_REQUIRE_VALID(makeMorphologySlabPlanResult4);
  REQUIRE(plan.outputBatchDepth == 1);
  REQUIRE(plan.maxInputDepth == k_LargeRadiusDimZ);
  REQUIRE((plan.outputBatchDepth + plan.maxInputDepth) * k_PlaneBytes > k_LargeRadiusTargetBytes);

  std::vector<int32> input(k_DimX * k_DimY * k_LargeRadiusDimZ);
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = static_cast<int32>((i * 13) % 37);
  }
  auto copyFromBufferResult = shallowInput.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 10});
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  MorphScanline<int32> algorithm{shallowInput, shallowOutput, SizeVec3{k_DimX, k_DimY, k_LargeRadiusDimZ}, se, MorphOp::Dilate, shouldCancel, messageHandler, k_LargeRadiusTargetBytes};
  auto algorithmResult = algorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(algorithmResult);
  std::vector<int32> actual(input.size());
  auto copyIntoBufferResult = shallowOutput.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == Oracle(input, k_DimX, k_DimY, k_LargeRadiusDimZ, se, MorphOp::Dilate));
}

TEST_CASE("ImageProcessing::MorphologyEngine: resident 8-bit plan requires complete dataset-scaled input and output memory", "[ImageProcessing][MorphologyEngine][WorkingMemory]")
{
  constexpr usize dimX = 600;
  constexpr usize dimY = 450;
  constexpr usize dimZ = 128;
  constexpr usize valueCount = dimX * dimY * dimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto requiredResult = ImageProcessing::detail::CalculateMorphologyResidentWorkingMemoryBytes<uint8>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == 2 * valueCount * sizeof(uint8));
  auto halfResult = ImageProcessing::detail::CalculateMorphologyResidentWorkingMemoryBytes<uint8>(SizeVec3{dimX, dimY, dimZ / 2});
  SIMPLNX_RESULT_REQUIRE_VALID(halfResult);
  REQUIRE(halfResult.value() * 2 == requiredResult.value());
  auto calculateMorphologyResidentWorkingMemoryBytesResult = ImageProcessing::detail::CalculateMorphologyResidentWorkingMemoryBytes<uint8>(SizeVec3{std::numeric_limits<usize>::max(), 2, 1});
  SIMPLNX_RESULT_REQUIRE_INVALID(calculateMorphologyResidentWorkingMemoryBytesResult);

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(128 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveMorphologyResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 32 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(512 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveMorphologyResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::MorphologyEngine: resident flat fold is limited to moderate multi-plane neighborhoods", "[ImageProcessing][MorphologyEngine][WorkingMemory]")
{
  const StructuringElement moderate = MakeStructuringElement(KernelType::Ball, {2, 2, 2});
  const StructuringElement large = MakeStructuringElement(KernelType::Ball, {3, 3, 3});

  REQUIRE(ImageProcessing::detail::ShouldUseMorphologyResidentFlatFold(SizeVec3{600, 450, 128}, moderate));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseMorphologyResidentFlatFold(SizeVec3{600, 450, 1}, moderate));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseMorphologyResidentFlatFold(SizeVec3{600, 450, 128}, large));
}

TEST_CASE("ImageProcessing::MorphologyEngine: flat-fold blocks map staged slabs and cropped rings to independent oracles", "[ImageProcessing][MorphologyEngine]")
{
  constexpr uint8 foreground = 1;
  constexpr uint8 background = 0;
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {2, 2, 2});
  std::atomic_bool shouldCancel{false};

  const auto extractBlock = [](const std::vector<uint8>& values, const SizeVec3& boundDims, const SizeVec3& outputOrigin, const SizeVec3& outputDims) {
    std::vector<uint8> block(outputDims[0] * outputDims[1] * outputDims[2]);
    for(usize localZ = 0; localZ < outputDims[2]; ++localZ)
    {
      for(usize localY = 0; localY < outputDims[1]; ++localY)
      {
        for(usize localX = 0; localX < outputDims[0]; ++localX)
        {
          block[FlatIndex(localX, localY, localZ, outputDims[0], outputDims[1])] =
              values[FlatIndex(outputOrigin[0] + localX, outputOrigin[1] + localY, outputOrigin[2] + localZ, boundDims[0], boundDims[1])];
        }
      }
    }
    return block;
  };

  SECTION("multi-plane slab uses the full X/Y output at a nonzero Z origin")
  {
    const SizeVec3 boundDims{7, 6, 8};
    const SizeVec3 outputOrigin{0, 0, 3};
    const SizeVec3 outputDims{boundDims[0], boundDims[1], 2};
    constexpr usize stagedZLo = 1;
    constexpr usize stagedDepth = 6;
    const usize planeValues = boundDims[0] * boundDims[1];
    const std::vector<uint8> grayscaleVolume = MakePattern<uint8>(boundDims[0], boundDims[1], boundDims[2]);
    const std::vector<uint8> binaryVolume = MakeBinaryPattern<uint8>(boundDims[0], boundDims[1], boundDims[2]);
    const std::vector<uint8> grayscaleSlab(grayscaleVolume.cbegin() + stagedZLo * planeValues, grayscaleVolume.cbegin() + (stagedZLo + stagedDepth) * planeValues);
    const std::vector<uint8> binarySlab(binaryVolume.cbegin() + stagedZLo * planeValues, binaryVolume.cbegin() + (stagedZLo + stagedDepth) * planeValues);
    const usize outputValues = outputDims[0] * outputDims[1] * outputDims[2];

    std::vector<uint8> grayscaleDilate(outputValues, 255);
    std::vector<uint8> grayscaleErode(outputValues, 255);
    std::vector<uint8> gradient(outputValues, 255);
    std::vector<uint8> binaryDilate(outputValues, 255);
    std::vector<uint8> binaryErode(outputValues, 255);
    ImageProcessing::detail::RunMorphologyFlatFoldBlock<uint8, true>(grayscaleSlab.data(), grayscaleDilate.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, shouldCancel);
    ImageProcessing::detail::RunMorphologyFlatFoldBlock<uint8, false>(grayscaleSlab.data(), grayscaleErode.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, shouldCancel);
    ImageProcessing::detail::RunMorphologyGradientFlatFoldBlock<uint8>(grayscaleSlab.data(), gradient.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, shouldCancel);
    ImageProcessing::detail::RunBinaryMorphologyFlatFoldBlock<uint8, true>(binarySlab.data(), binaryDilate.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, foreground, background, false,
                                                                           shouldCancel);
    ImageProcessing::detail::RunBinaryMorphologyFlatFoldBlock<uint8, false>(binarySlab.data(), binaryErode.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, foreground, background, false,
                                                                            shouldCancel);

    REQUIRE(grayscaleDilate == extractBlock(Oracle(grayscaleVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Dilate), boundDims, outputOrigin, outputDims));
    REQUIRE(grayscaleErode == extractBlock(Oracle(grayscaleVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Erode), boundDims, outputOrigin, outputDims));
    REQUIRE(gradient == extractBlock(GradientOracle(grayscaleVolume, boundDims[0], boundDims[1], boundDims[2], se), boundDims, outputOrigin, outputDims));
    REQUIRE(binaryDilate ==
            extractBlock(BinaryOracle(binaryVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Dilate, foreground, background, false), boundDims, outputOrigin, outputDims));
    REQUIRE(binaryErode == extractBlock(BinaryOracle(binaryVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Erode, foreground, background, false), boundDims, outputOrigin, outputDims));
  }

  SECTION("one-plane cropped ring uses nonzero padded-space origins")
  {
    const SizeVec3 boundDims{9, 8, 7};
    const SizeVec3 outputOrigin{1, 1, 3};
    const SizeVec3 outputDims{4, 3, 1};
    constexpr usize stagedZLo = 1;
    constexpr usize stagedDepth = 5;
    const usize planeValues = boundDims[0] * boundDims[1];
    const std::vector<uint8> grayscaleVolume = MakePattern<uint8>(boundDims[0], boundDims[1], boundDims[2]);
    const std::vector<uint8> binaryVolume = MakeBinaryPattern<uint8>(boundDims[0], boundDims[1], boundDims[2]);
    const std::vector<uint8> grayscaleRing(grayscaleVolume.cbegin() + stagedZLo * planeValues, grayscaleVolume.cbegin() + (stagedZLo + stagedDepth) * planeValues);
    const std::vector<uint8> binaryRing(binaryVolume.cbegin() + stagedZLo * planeValues, binaryVolume.cbegin() + (stagedZLo + stagedDepth) * planeValues);
    const usize outputValues = outputDims[0] * outputDims[1] * outputDims[2];

    std::vector<uint8> grayscaleDilate(outputValues, 255);
    std::vector<uint8> grayscaleErode(outputValues, 255);
    std::vector<uint8> gradient(outputValues, 255);
    std::vector<uint8> binaryDilate(outputValues, 255);
    std::vector<uint8> binaryErode(outputValues, 255);
    ImageProcessing::detail::RunMorphologyFlatFoldBlock<uint8, true>(grayscaleRing.data(), grayscaleDilate.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, shouldCancel);
    ImageProcessing::detail::RunMorphologyFlatFoldBlock<uint8, false>(grayscaleRing.data(), grayscaleErode.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, shouldCancel);
    ImageProcessing::detail::RunMorphologyGradientFlatFoldBlock<uint8>(grayscaleRing.data(), gradient.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, shouldCancel);
    ImageProcessing::detail::RunBinaryMorphologyFlatFoldBlock<uint8, true>(binaryRing.data(), binaryDilate.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, foreground, background, true,
                                                                           shouldCancel);
    ImageProcessing::detail::RunBinaryMorphologyFlatFoldBlock<uint8, false>(binaryRing.data(), binaryErode.data(), boundDims, stagedZLo, outputDims, outputOrigin, se, foreground, background, true,
                                                                            shouldCancel);

    REQUIRE(grayscaleDilate == extractBlock(Oracle(grayscaleVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Dilate), boundDims, outputOrigin, outputDims));
    REQUIRE(grayscaleErode == extractBlock(Oracle(grayscaleVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Erode), boundDims, outputOrigin, outputDims));
    REQUIRE(gradient == extractBlock(GradientOracle(grayscaleVolume, boundDims[0], boundDims[1], boundDims[2], se), boundDims, outputOrigin, outputDims));
    REQUIRE(binaryDilate == extractBlock(BinaryOracle(binaryVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Dilate, foreground, background, true), boundDims, outputOrigin, outputDims));
    REQUIRE(binaryErode == extractBlock(BinaryOracle(binaryVolume, boundDims[0], boundDims[1], boundDims[2], se, MorphOp::Erode, foreground, background, true), boundDims, outputOrigin, outputDims));
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: packed binary blocks match complete-volume independent oracles", "[ImageProcessing][MorphologyEngine]")
{
  const std::array<bool, 2> boundaryModes = {false, true};
  const auto checkBothOperations = [&](const SizeVec3& boundDims, usize stagedZLo, usize stagedDepth, const SizeVec3& outputDims, const SizeVec3& outputOrigin, const StructuringElement& se,
                                       bool boundaryToForeground, const char* caseName) {
    CheckPackedBinaryMorphologyBlock<true>(boundDims, stagedZLo, stagedDepth, outputDims, outputOrigin, se, boundaryToForeground, caseName);
    CheckPackedBinaryMorphologyBlock<false>(boundDims, stagedZLo, stagedDepth, outputDims, outputOrigin, se, boundaryToForeground, caseName);
  };

  SECTION("full Box radius-1 volumes cross 64-bit word boundaries")
  {
    for(const usize dimX : {usize{63}, usize{64}, usize{65}})
    {
      const SizeVec3 boundDims{dimX, 4, 3};
      const StructuringElement se = MakeStructuringElement(KernelType::Box, {1, 1, 1});
      for(const bool boundaryToForeground : boundaryModes)
      {
        checkBothOperations(boundDims, 0, boundDims[2], boundDims, SizeVec3{0, 0, 0}, se, boundaryToForeground, "full Box r1 word boundary");
      }
    }
  }

  SECTION("all kernel shapes use radius 2")
  {
    const SizeVec3 boundDims{65, 6, 5};
    for(const KernelType kernel : {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus})
    {
      const StructuringElement se = MakeStructuringElement(kernel, {2, 2, 2});
      for(const bool boundaryToForeground : boundaryModes)
      {
        checkBothOperations(boundDims, 0, boundDims[2], boundDims, SizeVec3{0, 0, 0}, se, boundaryToForeground, "full all-shape r2");
      }
    }
  }

  SECTION("asymmetric and zero-Z neighborhoods preserve multi-plane addressing")
  {
    const SizeVec3 boundDims{67, 6, 5};
    for(const std::array<int32, 3>& radius : {std::array<int32, 3>{3, 1, 2}, std::array<int32, 3>{2, 2, 0}})
    {
      const StructuringElement se = MakeStructuringElement(KernelType::Box, radius);
      for(const bool boundaryToForeground : boundaryModes)
      {
        checkBothOperations(boundDims, 0, boundDims[2], boundDims, SizeVec3{0, 0, 0}, se, boundaryToForeground, "full asymmetric or zero-Z");
      }
    }
  }

  SECTION("cropped staged block reaches X Y and Z borders")
  {
    const SizeVec3 boundDims{64, 6, 8};
    const SizeVec3 outputOrigin{1, 0, 3};
    const SizeVec3 outputDims{63, 6, 5};
    const StructuringElement se = MakeStructuringElement(KernelType::Box, {3, 1, 2});
    for(const bool boundaryToForeground : boundaryModes)
    {
      // The staged planes are global Z [1, 8). They contain every in-bounds neighbor needed
      // by output Z [3, 8), while the nonzero stage origin exposes local/global Z mapping.
      checkBothOperations(boundDims, 1, 7, outputDims, outputOrigin, se, boundaryToForeground, "cropped unaligned 63-wide staged block");
    }
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: packed binary rings retain logical planes across wrap", "[ImageProcessing][MorphologyEngine]")
{
  const SizeVec3 k_BoundDims{65, 2, 8};
  constexpr BinaryMorphWord k_TailMask = (BinaryMorphWord{1} << 1) - BinaryMorphWord{1};
  const ImageProcessing::detail::PackedBinaryPlaneLayout layout = ImageProcessing::detail::MakePackedBinaryPlaneLayout(k_BoundDims);
  ImageProcessing::detail::PackedBinaryRing ring(layout, 3);

  BinaryMorphWord* plane4 = ring.appendPlane(4);
  plane4[0] = BinaryMorphWord{0x0123456789ABCDEF};
  plane4[1] = BinaryMorphWord{0x13579BDF2468ACE0} & k_TailMask;
  plane4[2] = BinaryMorphWord{0x0F0E0D0C0B0A0908};
  plane4[3] = BinaryMorphWord{0x2468ACE013579BDF} & k_TailMask;

  BinaryMorphWord* plane5 = ring.appendPlane(5);
  plane5[0] = BinaryMorphWord{0x89ABCDEF01234567};
  plane5[1] = BinaryMorphWord{0xFEDCBA9876543211} & k_TailMask;
  plane5[2] = BinaryMorphWord{0x76543210FEDCBA98};
  plane5[3] = BinaryMorphWord{0xABCDEF0123456781} & k_TailMask;

  BinaryMorphWord* plane6 = ring.appendPlane(6);
  plane6[0] = BinaryMorphWord{0x55AA55AA55AA55AA};
  plane6[1] = BinaryMorphWord{0xAAAAAAAAAAAAAAAA} & k_TailMask;
  plane6[2] = BinaryMorphWord{0xAA55AA55AA55AA55};
  plane6[3] = BinaryMorphWord{0x5555555555555555} & k_TailMask;

  const ImageProcessing::detail::PackedBinaryPlanesView initialView = ring.view();
  REQUIRE(initialView.zLo == 4);
  REQUIRE(initialView.depth == 3);
  REQUIRE(initialView.capacity == 3);
  REQUIRE(initialView.head == 0);
  REQUIRE(initialView.words != nullptr);
  REQUIRE(initialView.layout.dimX == 65);
  REQUIRE(initialView.layout.dimY == 2);
  REQUIRE(initialView.layout.wordsPerRow == 2);
  REQUIRE(initialView.layout.planeWords == 4);
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(initialView, k_BoundDims, 0, 0, 4, false) == BinaryMorphWord{0x0123456789ABCDEF});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(initialView, k_BoundDims, 0, 1, 4, false) == BinaryMorphWord{0x0F0E0D0C0B0A0908});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(initialView, k_BoundDims, 0, 0, 5, false) == BinaryMorphWord{0x89ABCDEF01234567});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(initialView, k_BoundDims, 0, 1, 5, false) == BinaryMorphWord{0x76543210FEDCBA98});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(initialView, k_BoundDims, 0, 0, 6, false) == BinaryMorphWord{0x55AA55AA55AA55AA});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(initialView, k_BoundDims, 0, 1, 6, false) == BinaryMorphWord{0xAA55AA55AA55AA55});

  BinaryMorphWord* plane7 = ring.appendPlane(7);
  plane7[0] = BinaryMorphWord{0x1111222233334444};
  plane7[1] = BinaryMorphWord{0xFFFFFFFFFFFFFFFF} & k_TailMask;
  plane7[2] = BinaryMorphWord{0x4444333322221111};
  plane7[3] = BinaryMorphWord{0x0000000000000001} & k_TailMask;

  const ImageProcessing::detail::PackedBinaryPlanesView wrappedView = ring.view();
  REQUIRE(wrappedView.zLo == 5);
  REQUIRE(wrappedView.depth == 3);
  REQUIRE(wrappedView.capacity == 3);
  REQUIRE(wrappedView.head == 1);
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 0, 5, false) == BinaryMorphWord{0x89ABCDEF01234567});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 1, 5, false) == BinaryMorphWord{0x76543210FEDCBA98});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 0, 6, false) == BinaryMorphWord{0x55AA55AA55AA55AA});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 1, 6, false) == BinaryMorphWord{0xAA55AA55AA55AA55});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 0, 7, false) == BinaryMorphWord{0x1111222233334444});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 1, 7, false) == BinaryMorphWord{0x4444333322221111});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 1, 0, 5, false) ==
          ((BinaryMorphWord{0x89ABCDEF01234567} >> 1) | ((BinaryMorphWord{0xFEDCBA9876543211} & k_TailMask) << 63)));
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 0, -1, false) == BinaryMorphWord{0});
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(wrappedView, k_BoundDims, 0, 0, 8, true) == std::numeric_limits<BinaryMorphWord>::max());

  const StructuringElement boxRadiusOne = MakeStructuringElement(KernelType::Box, {1, 1, 1});
  std::vector<uint8> rawOutput(65 * 2, uint8{0xA5});
  std::vector<BinaryMorphWord> packedOutput(layout.planeWords, BinaryMorphWord{0xDEADBEEFDEADBEEF});
  std::atomic_bool shouldCancel{true};
  const bool rawCompleted = ImageProcessing::detail::RunPrepackedBinaryMorphologyBlock<uint8, true>(wrappedView, rawOutput.data(), k_BoundDims, SizeVec3{65, 2, 1}, SizeVec3{0, 0, 6}, boxRadiusOne,
                                                                                                    uint8{1}, uint8{0}, false, shouldCancel);
  const bool packedCompleted = ImageProcessing::detail::RunPrepackedBinaryMorphologyPlane<true>(wrappedView, packedOutput.data(), k_BoundDims, SizeVec3{0, 0, 6}, boxRadiusOne, false, shouldCancel);
  REQUIRE_FALSE(rawCompleted);
  REQUIRE_FALSE(packedCompleted);
  REQUIRE(std::all_of(rawOutput.cbegin(), rawOutput.cend(), [](uint8 value) { return value == uint8{0xA5}; }));
  REQUIRE(std::all_of(packedOutput.cbegin(), packedOutput.cend(), [](BinaryMorphWord value) { return value == BinaryMorphWord{0xDEADBEEFDEADBEEF}; }));

  ImageProcessing::detail::PackedBinaryRing onePlaneRing(layout, 1);
  BinaryMorphWord* plane2 = onePlaneRing.appendPlane(2);
  plane2[0] = BinaryMorphWord{0x0000000000000001};
  plane2[1] = BinaryMorphWord{0x0000000000000001} & k_TailMask;
  BinaryMorphWord* plane3 = onePlaneRing.appendPlane(3);
  plane3[0] = BinaryMorphWord{0x8000000000000000};
  plane3[1] = BinaryMorphWord{0x0000000000000000} & k_TailMask;

  const ImageProcessing::detail::PackedBinaryPlanesView onePlaneView = onePlaneRing.view();
  REQUIRE(onePlaneView.zLo == 3);
  REQUIRE(onePlaneView.depth == 1);
  REQUIRE(onePlaneView.capacity == 1);
  REQUIRE(onePlaneView.head == 0);
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(onePlaneView, k_BoundDims, 0, 0, 3, false) == BinaryMorphWord{0x8000000000000000});
}

TEST_CASE("ImageProcessing::MorphologyEngine: pre-cancel preserves poisoned scanline outputs", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 5;
  constexpr usize k_DimZ = 4;
  constexpr usize k_VolumeValues = k_DimX * k_DimY * k_DimZ;
  constexpr int32 k_Poison = 42;
  DataStore<int32> inputStore({k_DimZ, k_DimY, k_DimX}, {1}, 0);
  DataStore<int32> morphStore({k_DimZ, k_DimY, k_DimX}, {1}, k_Poison);
  DataStore<int32> gradientStore({k_DimZ, k_DimY, k_DimX}, {1}, k_Poison);
  DataStore<int32> binaryStore({k_DimZ, k_DimY, k_DimX}, {1}, k_Poison);
  const StructuringElement se = MakeStructuringElement(KernelType::Ball, {1, 1, 1});
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};

  MorphScanline<int32> morphAlgorithm{inputStore, morphStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, MorphOp::Dilate, shouldCancel, messageHandler};
  MorphGradientScanline<int32> gradientAlgorithm{inputStore, gradientStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, shouldCancel, messageHandler};
  BinaryMorphScanline<int32> binaryAlgorithm{inputStore, binaryStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, se, MorphOp::Erode, 1, 0, true, shouldCancel, messageHandler};
  auto morphAlgorithmResult = morphAlgorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(morphAlgorithmResult);
  auto gradientAlgorithmResult = gradientAlgorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(gradientAlgorithmResult);
  auto binaryAlgorithmResult = binaryAlgorithm();
  SIMPLNX_RESULT_REQUIRE_VALID(binaryAlgorithmResult);

  std::vector<int32> actual(k_VolumeValues);
  for(AbstractDataStore<int32>* store : std::array<AbstractDataStore<int32>*, 3>{&morphStore, &gradientStore, &binaryStore})
  {
    auto copyIntoBufferResult = store->copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    REQUIRE(std::all_of(actual.cbegin(), actual.cend(), [](int32 value) { return value == k_Poison; }));
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologyEngine: fused SafeBorder crop-subtract matches manual oracle", "[ImageProcessing][MorphologyEngine]", uint8, int32, float32)
{
  using Order = ImageProcessing::detail::CropSubtractOrder;

  SECTION("asymmetric 3D, original minus cropped")
  {
    RequireCropSubtractCase<TestType>(SizeVec3{5, 3, 2}, {1, 2, 1}, Order::OriginalMinusCropped, false);
  }
  SECTION("asymmetric 3D, cropped minus original")
  {
    RequireCropSubtractCase<TestType>(SizeVec3{5, 3, 2}, {1, 2, 1}, Order::CroppedMinusOriginal, false);
  }
  SECTION("2D with zero Z pad")
  {
    RequireCropSubtractCase<TestType>(SizeVec3{4, 3, 1}, {2, 1, 0}, Order::OriginalMinusCropped, false);
  }
  SECTION("zero radius")
  {
    RequireCropSubtractCase<TestType>(SizeVec3{3, 2, 2}, {0, 0, 0}, Order::CroppedMinusOriginal, false);
  }
  SECTION("original/output alias, original minus cropped")
  {
    RequireCropSubtractCase<TestType>(SizeVec3{5, 3, 2}, {1, 2, 1}, Order::OriginalMinusCropped, true);
  }
  SECTION("original/output alias, cropped minus original")
  {
    RequireCropSubtractCase<TestType>(SizeVec3{5, 3, 2}, {1, 2, 1}, Order::CroppedMinusOriginal, true);
  }
}

TEST_CASE("ImageProcessing::MorphologyEngine: fused SafeBorder crop-subtract honors pre-cancel", "[ImageProcessing][MorphologyEngine]")
{
  constexpr usize k_DimX = 3;
  constexpr usize k_DimY = 2;
  constexpr usize k_DimZ = 2;
  const SizeVec3 k_Dims{k_DimX, k_DimY, k_DimZ};
  constexpr std::array<usize, 3> k_Radius{1, 2, 1};
  constexpr usize k_PaddedX = k_DimX + 2 * k_Radius[0];
  constexpr usize k_PaddedY = k_DimY + 2 * k_Radius[1];
  constexpr usize k_PaddedZ = k_DimZ + 2 * k_Radius[2];
  constexpr int32 k_Poison = 73;
  DataStore<int32> paddedStore({k_PaddedZ, k_PaddedY, k_PaddedX}, {1}, 11);
  DataStore<int32> originalStore({k_DimZ, k_DimY, k_DimX}, {1}, 29);
  DataStore<int32> outputStore({k_DimZ, k_DimY, k_DimX}, {1}, k_Poison);
  std::atomic_bool shouldCancel{true};

  auto cropSubtractFromStoreResult =
      ImageProcessing::detail::CropSubtractFromStore<int32>(paddedStore, originalStore, outputStore, k_Dims, k_Radius, ImageProcessing::detail::CropSubtractOrder::CroppedMinusOriginal, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(cropSubtractFromStoreResult);

  std::vector<int32> actual(k_DimX * k_DimY * k_DimZ);
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(std::all_of(actual.cbegin(), actual.cend(), [](int32 value) { return value == k_Poison; }));
}

TEST_CASE("ImageProcessing::MorphologyEngine: fused SafeBorder crop-subtract uses registered OOC stores", "[ImageProcessing][MorphologyEngine]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceOutOfCore, 0);
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 3;
  constexpr usize k_DimZ = 2;
  const SizeVec3 k_Dims{k_DimX, k_DimY, k_DimZ};
  constexpr std::array<usize, 3> k_Radius{1, 2, 1};
  constexpr usize k_PaddedX = k_DimX + 2 * k_Radius[0];
  constexpr usize k_PaddedY = k_DimY + 2 * k_Radius[1];
  constexpr usize k_PaddedZ = k_DimZ + 2 * k_Radius[2];
  constexpr usize k_Volume = k_DimX * k_DimY * k_DimZ;
  constexpr usize k_PaddedVolume = k_PaddedX * k_PaddedY * k_PaddedZ;

  DataStructure dataStructure;
  auto paddedStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Padded"}), {k_PaddedZ, k_PaddedY, k_PaddedX}, {1});
  auto originalStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Original"}), {k_DimZ, k_DimY, k_DimX}, {1});
  auto outputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Output"}), {k_DimZ, k_DimY, k_DimX}, {1});
  REQUIRE(paddedStore != nullptr);
  REQUIRE(originalStore != nullptr);
  REQUIRE(outputStore != nullptr);

  const auto application = Application::Instance();
  REQUIRE(application != nullptr);
  if(application->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(paddedStore->getStoreType() == IDataStore::StoreType::OutOfCore);
    REQUIRE(originalStore->getStoreType() == IDataStore::StoreType::OutOfCore);
    REQUIRE(outputStore->getStoreType() == IDataStore::StoreType::OutOfCore);
  }

  std::vector<int32> originalValues(k_Volume);
  std::vector<int32> paddedValues(k_PaddedVolume, -901);
  std::vector<int32> expected(k_Volume);
  for(usize z = 0; z < k_DimZ; ++z)
  {
    for(usize y = 0; y < k_DimY; ++y)
    {
      for(usize x = 0; x < k_DimX; ++x)
      {
        const usize index = FlatIndex(x, y, z, k_DimX, k_DimY);
        const usize paddedIndex = FlatIndex(x + k_Radius[0], y + k_Radius[1], z + k_Radius[2], k_PaddedX, k_PaddedY);
        originalValues[index] = static_cast<int32>(31 + index * 3);
        paddedValues[paddedIndex] = static_cast<int32>(originalValues[index] + (index * 7) % 13);
        expected[index] = paddedValues[paddedIndex] - originalValues[index];
      }
    }
  }
  auto copyFromBufferResult = originalStore->copyFromBuffer(0, nonstd::span<const int32>(originalValues.data(), originalValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  auto copyFromBufferResult2 = paddedStore->copyFromBuffer(0, nonstd::span<const int32>(paddedValues.data(), paddedValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult2);

  std::atomic_bool shouldCancel{false};
  auto cropSubtractFromStoreResult = ImageProcessing::detail::CropSubtractFromStore<int32>(*paddedStore, *originalStore, *outputStore, k_Dims, k_Radius,
                                                                                           ImageProcessing::detail::CropSubtractOrder::CroppedMinusOriginal, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(cropSubtractFromStoreResult);
  std::vector<int32> actual(k_Volume);
  auto copyIntoBufferResult = outputStore->copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
}
