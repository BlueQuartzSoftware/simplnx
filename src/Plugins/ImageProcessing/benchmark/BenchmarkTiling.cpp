#include "BenchmarkTiling.hpp"

// Reuse the golden-test harness's ITK-free input reader + data-dir path resolver (do NOT duplicate them).
#include "ItkGoldenTestUtils.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp" // ExecuteDataFunction
#include "simplnx/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace
{
using namespace nx::core;

class StorageModeSentinel
{
public:
  explicit StorageModeSentinel(DataStorageMode mode)
  : m_Preferences(Application::Instance()->getPreferences())
  , m_OriginalMode(m_Preferences->dataStorageMode())
  {
    m_Preferences->setDataStorageMode(mode);
  }

  ~StorageModeSentinel()
  {
    m_Preferences->setDataStorageMode(m_OriginalMode);
  }

  StorageModeSentinel(const StorageModeSentinel&) = delete;
  StorageModeSentinel(StorageModeSentinel&&) = delete;
  StorageModeSentinel& operator=(const StorageModeSentinel&) = delete;
  StorageModeSentinel& operator=(StorageModeSentinel&&) = delete;

private:
  Preferences* m_Preferences = nullptr;
  DataStorageMode m_OriginalMode = DataStorageMode::Adaptive;
};

Result<usize> CheckedMultiply(usize left, usize right, std::string_view context)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return MakeErrorResult<usize>(
        -9123, fmt::format("Benchmark tiling size overflow while calculating {}: {} * {} exceeds the maximum supported size {}.", context, left, right, std::numeric_limits<usize>::max()));
  }
  return {left * right};
}

Result<usize> CheckedProduct(std::initializer_list<usize> factors, std::string_view context)
{
  usize product = 1;
  for(const usize factor : factors)
  {
    Result<usize> multiplyResult = CheckedMultiply(product, factor, context);
    if(multiplyResult.invalid())
    {
      return multiplyResult;
    }
    product = multiplyResult.value();
  }
  return {product};
}

Result<usize> CheckedCeilDiv(usize dividend, usize divisor, std::string_view context)
{
  if(divisor == 0)
  {
    return MakeErrorResult<usize>(-9124, fmt::format("Benchmark tiling cannot calculate {} because the divisor is zero (dividend={}).", context, dividend));
  }
  return {dividend / divisor + static_cast<usize>(dividend % divisor != 0)};
}

// Everything the fill functor needs. Shapes are in ELEMENTS (tuple count * components).
struct TileParams
{
  usize srcX = 0;
  usize srcY = 0;
  usize srcZ = 0;
  usize dstX = 0;
  usize dstY = 0;
  usize dstZ = 0;
  usize numComp = 1;
  DataStorageMode storeMode = DataStorageMode::Adaptive;
};

bool IsInMemoryFormat(const std::string& format)
{
  return format.empty() || format == Preferences::k_InMemoryFormat.view();
}

// Creates the destination DataArray<T> using the canonical storage resolver and streams the tiled fill. Only one
// source slice plus one destination slice are materialized at once. All maximum row/slice/volume products are checked
// before allocation; the smaller loop offsets below are therefore bounded by those checked maxima.
struct FillTiledFunctor
{
  template <typename T>
  Result<> operator()(DataStructure& ds, DataObject::IdType cellAmId, const IDataArray& srcArrayBase, const DataPath& arrayPath, const TileParams& p)
  {
    const auto& srcStore = srcArrayBase.getIDataStoreRefAs<AbstractDataStore<T>>();
    if(p.srcX == 0 || p.srcY == 0 || p.srcZ == 0 || p.dstX == 0 || p.dstY == 0 || p.dstZ == 0 || p.numComp == 0)
    {
      return MakeErrorResult(-9124, fmt::format("Benchmark tiling cannot fill destination array '{}' because source dimensions are X={}, Y={}, Z={}, destination dimensions are X={}, Y={}, Z={}, and "
                                                "component count is {}. All values must be positive.",
                                                arrayPath.toString(), p.srcX, p.srcY, p.srcZ, p.dstX, p.dstY, p.dstZ, p.numComp));
    }

    Result<usize> srcRowResult = CheckedProduct({p.srcX, p.numComp}, fmt::format("source row elements for '{}'", srcArrayBase.getName()));
    Result<usize> dstRowResult = CheckedProduct({p.dstX, p.numComp}, fmt::format("destination row elements for '{}'", arrayPath.toString()));
    if(srcRowResult.invalid())
    {
      return ConvertResult(std::move(srcRowResult));
    }
    if(dstRowResult.invalid())
    {
      return ConvertResult(std::move(dstRowResult));
    }
    const usize srcRowElems = srcRowResult.value();
    const usize dstRowElems = dstRowResult.value();

    Result<usize> srcSliceResult = CheckedMultiply(p.srcY, srcRowElems, fmt::format("source slice elements for '{}'", srcArrayBase.getName()));
    Result<usize> dstSliceResult = CheckedMultiply(p.dstY, dstRowElems, fmt::format("destination slice elements for '{}'", arrayPath.toString()));
    if(srcSliceResult.invalid())
    {
      return ConvertResult(std::move(srcSliceResult));
    }
    if(dstSliceResult.invalid())
    {
      return ConvertResult(std::move(dstSliceResult));
    }
    const usize srcSliceElems = srcSliceResult.value();
    const usize dstSliceElems = dstSliceResult.value();

    Result<usize> srcTotalResult = CheckedMultiply(p.srcZ, srcSliceElems, fmt::format("source volume elements for '{}'", srcArrayBase.getName()));
    Result<usize> dstTotalResult = CheckedMultiply(p.dstZ, dstSliceElems, fmt::format("destination volume elements for '{}'", arrayPath.toString()));
    Result<usize> srcSliceBytesResult = CheckedMultiply(srcSliceElems, sizeof(T), fmt::format("source slice bytes for '{}'", srcArrayBase.getName()));
    Result<usize> dstSliceBytesResult = CheckedMultiply(dstSliceElems, sizeof(T), fmt::format("destination slice bytes for '{}'", arrayPath.toString()));
    if(srcTotalResult.invalid())
    {
      return ConvertResult(std::move(srcTotalResult));
    }
    if(dstTotalResult.invalid())
    {
      return ConvertResult(std::move(dstTotalResult));
    }
    if(srcSliceBytesResult.invalid())
    {
      return ConvertResult(std::move(srcSliceBytesResult));
    }
    if(dstSliceBytesResult.invalid())
    {
      return ConvertResult(std::move(dstSliceBytesResult));
    }
    Result<usize> srcTotalBytesResult = CheckedMultiply(srcTotalResult.value(), sizeof(T), fmt::format("source volume bytes for '{}'", srcArrayBase.getName()));
    Result<usize> dstTotalBytesResult = CheckedMultiply(dstTotalResult.value(), sizeof(T), fmt::format("destination volume bytes for '{}'", arrayPath.toString()));
    if(srcTotalBytesResult.invalid())
    {
      return ConvertResult(std::move(srcTotalBytesResult));
    }
    if(dstTotalBytesResult.invalid())
    {
      return ConvertResult(std::move(dstTotalBytesResult));
    }
    if(srcStore.getSize() != srcTotalResult.value())
    {
      return MakeErrorResult(-9124, fmt::format("Benchmark tiling source array '{}' has {} element(s), but its checked dimensions Z={}, Y={}, X={} with {} component(s) require {} element(s).",
                                                srcArrayBase.getName(), srcStore.getSize(), p.srcZ, p.srcY, p.srcX, p.numComp, srcTotalResult.value()));
    }
    if(srcSliceElems > std::vector<T>{}.max_size() || dstSliceElems > std::vector<T>{}.max_size())
    {
      return MakeErrorResult(-9124, fmt::format("Benchmark tiling slice allocation exceeds std::vector capacity for type '{}': source={} element(s) ({} bytes), destination={} element(s) ({} bytes).",
                                                DataTypeToString(GetDataType<T>()), srcSliceElems, srcSliceBytesResult.value(), dstSliceElems, dstSliceBytesResult.value()));
    }

    const ShapeType tupleShape = {p.dstZ, p.dstY, p.dstX};
    const ShapeType componentShape = {p.numComp};
    std::shared_ptr<AbstractDataStore<T>> store;
    {
      const StorageModeSentinel storageModeSentinel(p.storeMode);
      store = DataStoreUtilities::CreateDataStore<T>(ds, arrayPath, tupleShape, componentShape);
    }
    if(store == nullptr)
    {
      return MakeErrorResult(
          -9110,
          fmt::format("Benchmark tiling could not create a resolver-selected DataStore for destination array '{}' with type '{}', dimensions Z={}, Y={}, X={}, {} component(s), and {} checked bytes.",
                      arrayPath.toString(), DataTypeToString(GetDataType<T>()), p.dstZ, p.dstY, p.dstX, p.numComp, dstTotalBytesResult.value()));
    }
    const bool isInMemory = IsInMemoryFormat(store->getDataFormat());
    if((p.storeMode == DataStorageMode::ForceOutOfCore && isInMemory) || (p.storeMode == DataStorageMode::ForceInCore && !isInMemory))
    {
      return MakeErrorResult(-9125, fmt::format("Benchmark tiling requested storage mode '{}' for destination array '{}', but the canonical resolver selected format '{}'. Ensure the expected format "
                                                "resolver and IO backend are registered.",
                                                p.storeMode == DataStorageMode::ForceOutOfCore ? "ForceOutOfCore" : "ForceInCore", arrayPath.toString(),
                                                store->getDataFormat().empty() ? "in-memory" : store->getDataFormat()));
    }
    auto* dstArray = DataArray<T>::Create(ds, arrayPath.getTargetName(), store, cellAmId);
    if(dstArray == nullptr)
    {
      return MakeErrorResult(-9111, fmt::format("Benchmark tiling could not create destination DataArray '{}' with type '{}', dimensions Z={}, Y={}, X={}, and {} component(s).", arrayPath.toString(),
                                                DataTypeToString(GetDataType<T>()), p.dstZ, p.dstY, p.dstX, p.numComp));
    }
    auto& dstStore = dstArray->getDataStoreRef();
    if(dstStore.getSize() != dstTotalResult.value())
    {
      return MakeErrorResult(-9124, fmt::format("Benchmark tiling destination array '{}' has {} element(s), but its checked dimensions Z={}, Y={}, X={} with {} component(s) require {} element(s).",
                                                arrayPath.toString(), dstStore.getSize(), p.dstZ, p.dstY, p.dstX, p.numComp, dstTotalResult.value()));
    }

    std::vector<T> srcSlice(srcSliceElems);
    std::vector<T> dstSlice(dstSliceElems);

    for(usize dz = 0; dz < p.dstZ; ++dz)
    {
      const usize sz = dz % p.srcZ;
      Result<usize> srcOffsetResult = CheckedMultiply(sz, srcSliceElems, fmt::format("source slice offset {} for '{}'", sz, srcArrayBase.getName()));
      if(srcOffsetResult.invalid())
      {
        return ConvertResult(std::move(srcOffsetResult));
      }
      if(Result<> r = srcStore.copyIntoBuffer(srcOffsetResult.value(), nonstd::span<T>(srcSlice.data(), srcSliceElems)); r.invalid())
      {
        if(r.errors().empty())
        {
          return MakeErrorResult(
              -9121, fmt::format("Benchmark tiling could not read source array '{}' slice {} of {} while tiling type '{}' data with {} component(s); the data store did not provide an error message.",
                                 srcArrayBase.getName(), sz, p.srcZ, DataTypeToString(GetDataType<T>()), p.numComp));
        }
        for(Error& error : r.errors())
        {
          error.message = fmt::format("Benchmark tiling could not read source array '{}' slice {} of {} while tiling type '{}' data with {} component(s): {}", srcArrayBase.getName(), sz, p.srcZ,
                                      DataTypeToString(GetDataType<T>()), p.numComp, error.message);
        }
        return r;
      }

      // Tile the source slice across the destination slice (X fastest, then Y), preserving component interleave.
      for(usize y = 0; y < p.dstY; ++y)
      {
        const usize sy = y % p.srcY;
        // These products and sums are bounded by the checked row/slice maxima above.
        const usize srcRowOffset = sy * srcRowElems;
        const usize dstRowOffset = y * dstRowElems;
        for(usize x = 0; x < p.dstX; ++x)
        {
          const usize sx = x % p.srcX;
          const usize s = srcRowOffset + sx * p.numComp;
          const usize d = dstRowOffset + x * p.numComp;
          for(usize c = 0; c < p.numComp; ++c)
          {
            dstSlice[d + c] = srcSlice[s + c];
          }
        }
      }

      Result<usize> dstOffsetResult = CheckedMultiply(dz, dstSliceElems, fmt::format("destination slice offset {} for '{}'", dz, arrayPath.toString()));
      if(dstOffsetResult.invalid())
      {
        return ConvertResult(std::move(dstOffsetResult));
      }
      if(Result<> w = dstStore.copyFromBuffer(dstOffsetResult.value(), nonstd::span<const T>(dstSlice.data(), dstSliceElems)); w.invalid())
      {
        if(w.errors().empty())
        {
          return MakeErrorResult(
              -9122,
              fmt::format("Benchmark tiling could not write destination array '{}' slice {} of {} while tiling type '{}' data with {} component(s); the data store did not provide an error message.",
                          arrayPath.toString(), dz, p.dstZ, DataTypeToString(GetDataType<T>()), p.numComp));
        }
        for(Error& error : w.errors())
        {
          error.message = fmt::format("Benchmark tiling could not write destination array '{}' slice {} of {} while tiling type '{}' data with {} component(s): {}", arrayPath.toString(), dz, p.dstZ,
                                      DataTypeToString(GetDataType<T>()), p.numComp, error.message);
        }
        return w;
      }
    }
    return {};
  }
};

} // namespace

namespace ip_bench
{
Result<> BuildTiledInput(DataStructure& ds, const fs::path& realInput, const DataPath& geomPath, const std::string& cellAmName, const std::string& arrayName, usize targetVoxels, usize minZSlices,
                         DataStorageMode storeMode, DataPath& outArrayPath, bool force2D, const SourceArrayValidator& sourceValidator)
{
  // --- 1) Read the real input ONCE into a scratch DataStructure to recover dims/type/components. Force the scratch
  //        in-core (fast slice reads) regardless of the caller's active preference; restore the preference after. ---
  DataStructure scratch;
  const DataPath scratchGeom({"__benchmark_scratch_input"});
  Result<> readResult;
  {
    const StorageModeSentinel storageModeSentinel(DataStorageMode::ForceInCore);
    readResult = ip_golden::ReadInputImage(scratch, realInput, scratchGeom, cellAmName, arrayName);
  }
  if(readResult.invalid())
  {
    return readResult;
  }

  const auto* srcGeom = scratch.getDataAs<ImageGeom>(scratchGeom);
  const DataPath scratchArray = scratchGeom.createChildPath(cellAmName).createChildPath(arrayName);
  const auto* srcArray = scratch.getDataAs<IDataArray>(scratchArray);
  if(srcGeom == nullptr || srcArray == nullptr)
  {
    return MakeErrorResult(
        -9113, fmt::format("BuildTiledInput: reading '{}' did not produce the expected ImageGeom '{}' and array '{}'.", realInput.string(), scratchGeom.toString(), scratchArray.toString()));
  }
  if(sourceValidator)
  {
    if(Result<> validationResult = sourceValidator(*srcArray, realInput); validationResult.invalid())
    {
      return validationResult;
    }
  }

  const SizeVec3 srcDims = srcGeom->getDimensions(); // {X, Y, Z}
  const usize srcX = srcDims[0];
  const usize srcY = srcDims[1];
  const usize srcZ = srcDims[2];
  if(srcX == 0 || srcY == 0 || srcZ == 0)
  {
    return MakeErrorResult(-9114, fmt::format("BuildTiledInput: source image '{}' has invalid dimensions ({}).", realInput.string(), StringUtilities::formatDimensions3D(srcDims)));
  }
  const usize numComp = srcArray->getNumberOfComponents();
  const DataType dataType = srcArray->getDataType();
  if(numComp == 0)
  {
    return MakeErrorResult(-9124, fmt::format("BuildTiledInput: source array '{}' from '{}' has zero components.", scratchArray.toString(), realInput.string()));
  }

  Result<usize> srcVoxelResult = CheckedProduct({srcX, srcY, srcZ}, fmt::format("source voxel count for '{}'", realInput.string()));
  if(srcVoxelResult.invalid())
  {
    return ConvertResult(std::move(srcVoxelResult));
  }
  if(srcArray->getNumberOfTuples() != srcVoxelResult.value())
  {
    return MakeErrorResult(-9124, fmt::format("BuildTiledInput: source array '{}' from '{}' has {} tuple(s), but image dimensions ({}) require {} voxel tuple(s).", scratchArray.toString(),
                                              realInput.string(), srcArray->getNumberOfTuples(), StringUtilities::formatDimensions3D(srcDims), srcVoxelResult.value()));
  }

  // --- 2) Tile factors: either select one source slice for an explicit 2-D benchmark, or grow Z first to guarantee
  //        slab depth. Distribute the remaining XY multiplier ~evenly. ---
  usize dstZ = 1;
  if(!force2D)
  {
    Result<usize> tzResult = CheckedCeilDiv(minZSlices, srcZ, fmt::format("Z tile factor for '{}'", realInput.string()));
    if(tzResult.invalid())
    {
      return ConvertResult(std::move(tzResult));
    }
    const usize tz = std::max<usize>(1, tzResult.value());
    Result<usize> dstZResult = CheckedMultiply(tz, srcZ, fmt::format("destination Z dimension for '{}'", realInput.string()));
    if(dstZResult.invalid())
    {
      return ConvertResult(std::move(dstZResult));
    }
    dstZ = dstZResult.value();
  }
  Result<usize> srcXYResult = CheckedMultiply(srcX, srcY, fmt::format("source XY plane voxels for '{}'", realInput.string()));
  if(srcXYResult.invalid())
  {
    return ConvertResult(std::move(srcXYResult));
  }
  Result<usize> baseVoxelsResult = CheckedMultiply(srcXYResult.value(), dstZ, fmt::format("Z-tiled base voxel count for '{}'", realInput.string()));
  if(baseVoxelsResult.invalid())
  {
    return ConvertResult(std::move(baseVoxelsResult));
  }
  Result<usize> xyMultResult = CheckedCeilDiv(targetVoxels, baseVoxelsResult.value(), fmt::format("XY tile multiplier for '{}'", realInput.string()));
  if(xyMultResult.invalid())
  {
    return ConvertResult(std::move(xyMultResult));
  }
  const usize xyMult = std::max<usize>(1, xyMultResult.value());
  const double txEstimate = std::ceil(std::sqrt(static_cast<double>(xyMult)));
  if(!std::isfinite(txEstimate) || txEstimate < 1.0 || txEstimate > static_cast<double>(std::numeric_limits<usize>::max()))
  {
    return MakeErrorResult(-9124, fmt::format("BuildTiledInput: cannot represent balanced X tile factor {} for source '{}' and XY multiplier {}.", txEstimate, realInput.string(), xyMult));
  }
  const usize tx = static_cast<usize>(txEstimate);
  Result<usize> tyResult = CheckedCeilDiv(xyMult, tx, fmt::format("Y tile factor for '{}'", realInput.string()));
  if(tyResult.invalid())
  {
    return ConvertResult(std::move(tyResult));
  }
  const usize ty = std::max<usize>(1, tyResult.value());

  Result<usize> dstXResult = CheckedMultiply(tx, srcX, fmt::format("destination X dimension for '{}'", realInput.string()));
  Result<usize> dstYResult = CheckedMultiply(ty, srcY, fmt::format("destination Y dimension for '{}'", realInput.string()));
  if(dstXResult.invalid())
  {
    return ConvertResult(std::move(dstXResult));
  }
  if(dstYResult.invalid())
  {
    return ConvertResult(std::move(dstYResult));
  }
  const usize dstX = dstXResult.value();
  const usize dstY = dstYResult.value();
  Result<usize> dstVoxelResult = CheckedProduct({dstX, dstY, dstZ}, fmt::format("destination voxel count for '{}'", realInput.string()));
  if(dstVoxelResult.invalid())
  {
    return ConvertResult(std::move(dstVoxelResult));
  }
  if(dstVoxelResult.value() < targetVoxels)
  {
    return MakeErrorResult(-9124, fmt::format("BuildTiledInput: checked destination dimensions X={}, Y={}, Z={} produce {} voxels, below requested target {} for source '{}'.", dstX, dstY, dstZ,
                                              dstVoxelResult.value(), targetVoxels, realInput.string()));
  }

  // --- 3) Build the destination ImageGeom + cell AttributeMatrix in the caller's DataStructure. ---
  std::optional<DataObject::IdType> parentId;
  if(geomPath.getLength() > 1)
  {
    parentId = ds.getId(geomPath.getParent());
    if(!parentId.has_value())
    {
      return MakeErrorResult(-9115, fmt::format("BuildTiledInput: destination geometry parent '{}' does not exist", geomPath.getParent().toString()));
    }
  }
  auto* dstGeom = ImageGeom::Create(ds, geomPath.getTargetName(), parentId);
  if(dstGeom == nullptr)
  {
    return MakeErrorResult(-9116, fmt::format("BuildTiledInput: could not create destination ImageGeom '{}'", geomPath.toString()));
  }
  dstGeom->setDimensions({dstX, dstY, dstZ});
  dstGeom->setSpacing(srcGeom->getSpacing());
  dstGeom->setOrigin(srcGeom->getOrigin());

  const ShapeType tupleShape = {dstZ, dstY, dstX}; // AttributeMatrix tuple shape is slowest-to-fastest (Z, Y, X)
  auto* cellAM = AttributeMatrix::Create(ds, cellAmName, tupleShape, dstGeom->getId());
  if(cellAM == nullptr)
  {
    return MakeErrorResult(-9117, fmt::format("BuildTiledInput: could not create destination cell AttributeMatrix '{}'", cellAmName));
  }
  dstGeom->setCellData(*cellAM);

  // --- 4) Resolver-create + stream-fill the typed destination array (bounded memory). ---
  TileParams params;
  params.srcX = srcX;
  params.srcY = srcY;
  params.srcZ = srcZ;
  params.dstX = dstX;
  params.dstY = dstY;
  params.dstZ = dstZ;
  params.numComp = numComp;
  params.storeMode = storeMode;

  // NoBool: image voxel data is never boolean, and std::vector<bool> has no contiguous buffer to stream.
  const DataPath destinationArrayPath = geomPath.createChildPath(cellAmName).createChildPath(arrayName);
  const Result<> fillResult = ExecuteDataFunctionNoBool(FillTiledFunctor{}, dataType, ds, cellAM->getId(), *srcArray, destinationArrayPath, params);
  if(fillResult.invalid())
  {
    return fillResult;
  }

  outArrayPath = destinationArrayPath;
  return {};
}

Result<> AddTiledArray(DataStructure& ds, const fs::path& realInput, const DataPath& geomPath, const std::string& cellAmName, const std::string& arrayName, DataStorageMode storeMode,
                       DataPath& outArrayPath, const SourceArrayValidator& sourceValidator)
{
  DataStructure scratch;
  const DataPath scratchGeom({"__benchmark_scratch_companion"});
  Result<> readResult;
  {
    const StorageModeSentinel storageModeSentinel(DataStorageMode::ForceInCore);
    readResult = ip_golden::ReadInputImage(scratch, realInput, scratchGeom, cellAmName, arrayName);
  }
  if(readResult.invalid())
  {
    return readResult;
  }

  const auto* srcGeom = scratch.getDataAs<ImageGeom>(scratchGeom);
  const DataPath scratchArray = scratchGeom.createChildPath(cellAmName).createChildPath(arrayName);
  const auto* srcArray = scratch.getDataAs<IDataArray>(scratchArray);
  if(srcGeom == nullptr || srcArray == nullptr)
  {
    return MakeErrorResult(-9118,
                           fmt::format("AddTiledArray: reading '{}' did not produce the expected ImageGeom '{}' and array '{}'.", realInput.string(), scratchGeom.toString(), scratchArray.toString()));
  }
  if(sourceValidator)
  {
    if(Result<> validationResult = sourceValidator(*srcArray, realInput); validationResult.invalid())
    {
      return validationResult;
    }
  }

  const auto* dstGeom = ds.getDataAs<ImageGeom>(geomPath);
  const DataPath cellAmPath = geomPath.createChildPath(cellAmName);
  const auto* cellAM = ds.getDataAs<AttributeMatrix>(cellAmPath);
  if(dstGeom == nullptr || cellAM == nullptr)
  {
    return MakeErrorResult(-9119, fmt::format("AddTiledArray: destination ImageGeom '{}' or cell AttributeMatrix '{}' does not exist while adding source '{}'.", geomPath.toString(),
                                              cellAmPath.toString(), realInput.string()));
  }

  const SizeVec3 srcDims = srcGeom->getDimensions();
  const SizeVec3 dstDims = dstGeom->getDimensions();
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(srcDims[axis] == 0 || dstDims[axis] == 0 || dstDims[axis] % srcDims[axis] != 0)
    {
      return MakeErrorResult(-9120, fmt::format("AddTiledArray: source '{}' dimensions ({}) do not tile exactly into destination ImageGeom '{}' dimensions ({}). Use companion images with dimensions "
                                                "matching the primary benchmark source.",
                                                realInput.string(), StringUtilities::formatDimensions3D(srcDims), geomPath.toString(), StringUtilities::formatDimensions3D(dstDims)));
    }
  }

  TileParams params;
  params.srcX = srcDims[0];
  params.srcY = srcDims[1];
  params.srcZ = srcDims[2];
  params.dstX = dstDims[0];
  params.dstY = dstDims[1];
  params.dstZ = dstDims[2];
  params.numComp = srcArray->getNumberOfComponents();
  params.storeMode = storeMode;

  const DataPath destinationArrayPath = cellAmPath.createChildPath(arrayName);
  const Result<> fillResult = ExecuteDataFunctionNoBool(FillTiledFunctor{}, srcArray->getDataType(), ds, cellAM->getId(), *srcArray, destinationArrayPath, params);
  if(fillResult.invalid())
  {
    return fillResult;
  }

  outArrayPath = destinationArrayPath;
  return {};
}
} // namespace ip_bench
