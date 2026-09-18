#include "CropImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".cropped_image_geometry";
constexpr usize k_CropScratchBytes = 1024 * 1024;
constexpr usize k_ZSliceBatch = 32;

/**
 * @class CropImageGeomDataArray
 * @brief Copies one cropped cell array with storage-specific transfers.
 * @tparam T Cell-array value type.
 *
 * Resident pairs copy selected rows directly.
 * Other pairs use buffers with a 1 MiB
 * total cap for each task.
 * Storage backends and caches can allocate more memory.
 *
 * Parallel tasks own separate array pairs.
 * The shared task result propagates the first transfer error.
 *
 * Cancellation keeps completed transfers.
 */
template <typename T>
class CropImageGeomDataArray
{
public:
  /**
   * @brief Creates one array-cropping task.
   * @param oldCellArray Supplies source cell tuples.
   * @param newCellArray Receives cropped cell tuples.
   * @param srcImageGeom Supplies source dimensions.
   * @param bounds Specifies half-open XYZ crop bounds.
   * @param shouldCancel Stops the task between row or slab transfers.
   * @param taskResult Stores the first validation or transfer error.
   */
  CropImageGeomDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const ImageGeom& srcImageGeom, std::array<uint64, 6> bounds, const std::atomic_bool& shouldCancel,
                         CopyFromArray::ParallelTaskResult& taskResult)
  : m_OldCellStore(oldCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_NewCellStore(newCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_SourceArrayName(oldCellArray.getName())
  , m_DestinationArrayName(newCellArray.getName())
  , m_SrcImageGeom(srcImageGeom)
  , m_Bounds(bounds)
  , m_ShouldCancel(shouldCancel)
  , m_TaskResult(taskResult)
  {
  }

  ~CropImageGeomDataArray() = default;

  CropImageGeomDataArray(const CropImageGeomDataArray&) = default;
  CropImageGeomDataArray(CropImageGeomDataArray&&) noexcept = default;
  CropImageGeomDataArray& operator=(const CropImageGeomDataArray&) = delete;
  CropImageGeomDataArray& operator=(CropImageGeomDataArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    const usize numComps = m_OldCellStore.getNumberOfComponents();

    if(m_ShouldCancel || m_TaskResult.shouldAbort())
    {
      return;
    }
    m_NewCellStore.fill(static_cast<T>(-1));

    const auto srcDims = m_SrcImageGeom.getDimensions();
    const uint64 srcDimX = srcDims[0];
    const uint64 srcDimY = srcDims[1];
    const uint64 srcDimZ = srcDims[2];

    // Read the half-open copy bounds prepared by the outer executor.
    const uint64 xMin = m_Bounds[0];
    const uint64 xMax = m_Bounds[1];
    const uint64 yMin = m_Bounds[2];
    const uint64 yMax = m_Bounds[3];
    const uint64 zMin = m_Bounds[4];
    const uint64 zMax = m_Bounds[5];

    if(xMin >= xMax || xMax > srcDimX || yMin >= yMax || yMax > srcDimY || zMin >= zMax || zMax > srcDimZ)
    {
      m_TaskResult.store(MakeErrorResult(-953, fmt::format("Cannot crop source array '{}' with XYZ dimensions [{}, {}, {}]. The internal half-open XYZ bounds are [{}, {}), [{}, {}), and [{}, {}). "
                                                           "Select bounds inside the source geometry.",
                                                           m_SourceArrayName, srcDimX, srcDimY, srcDimZ, xMin, xMax, yMin, yMax, zMin, zMax)));
      return;
    }

    const uint64 cropX = xMax - xMin;
    const uint64 cropY = yMax - yMin;
    const uint64 cropZ = zMax - zMin;
    const auto checkedProduct = [](std::initializer_list<uint64> factors) -> std::optional<usize> {
      usize product = 1;
      for(const uint64 factor : factors)
      {
        if(factor == 0 || factor > (std::numeric_limits<usize>::max)() / product)
        {
          return std::nullopt;
        }
        product *= static_cast<usize>(factor);
      }
      return product;
    };
    const auto sourceValues = checkedProduct({srcDimX, srcDimY, srcDimZ, numComps});
    const auto destinationValues = checkedProduct({cropX, cropY, cropZ, numComps});
    const usize destinationNumComps = m_NewCellStore.getNumberOfComponents();
    if(!sourceValues.has_value() || !destinationValues.has_value() || *sourceValues != m_OldCellStore.getSize() || *destinationValues != m_NewCellStore.getSize() || destinationNumComps != numComps)
    {
      m_TaskResult.store(MakeErrorResult(
          -953,
          fmt::format("Cannot crop source array '{}' with XYZ dimensions [{}, {}, {}] to destination array '{}' with XYZ dimensions [{}, {}, {}]. The source store has {} components and {} "
                      "values. The destination store has {} components and {} values. Make each array shape match its image geometry before cropping.",
                      m_SourceArrayName, srcDimX, srcDimY, srcDimZ, m_DestinationArrayName, cropX, cropY, cropZ, numComps, m_OldCellStore.getSize(), destinationNumComps, m_NewCellStore.getSize())));
      return;
    }

    const usize sourceDimX = static_cast<usize>(srcDimX);
    const usize sourceDimY = static_cast<usize>(srcDimY);
    const usize xMinIndex = static_cast<usize>(xMin);
    const usize yMinIndex = static_cast<usize>(yMin);
    const usize yMaxIndex = static_cast<usize>(yMax);
    const usize zMinIndex = static_cast<usize>(zMin);
    const usize zMaxIndex = static_cast<usize>(zMax);
    const usize cropDimX = static_cast<usize>(cropX);
    const usize cropDimY = static_cast<usize>(cropY);
    const usize cropDimZ = static_cast<usize>(cropZ);
    const usize sourceSliceTuples = sourceDimX * sourceDimY;
    const usize destinationSliceTuples = cropDimX * cropDimY;
    const usize rowElements = cropDimX * numComps;

    const auto* sourceStore = dynamic_cast<const DataStore<T>*>(&m_OldCellStore);
    auto* destinationStore = dynamic_cast<DataStore<T>*>(&m_NewCellStore);
    if(m_OldCellStore.getStoreType() == IDataStore::StoreType::InMemory && m_NewCellStore.getStoreType() == IDataStore::StoreType::InMemory && sourceStore != nullptr && destinationStore != nullptr)
    {
      const T* sourceData = sourceStore->data();
      T* destinationData = destinationStore->data();
      for(usize zIndex = zMinIndex; zIndex < zMaxIndex; ++zIndex)
      {
        for(usize yIndex = yMinIndex; yIndex < yMaxIndex; ++yIndex)
        {
          if(m_ShouldCancel || m_TaskResult.shouldAbort())
          {
            return;
          }
          const usize sourceOffset = ((zIndex * sourceDimY + yIndex) * sourceDimX + xMinIndex) * numComps;
          const usize destinationOffset = ((zIndex - zMinIndex) * cropDimY + yIndex - yMinIndex) * rowElements;
          std::copy_n(sourceData + sourceOffset, rowElements, destinationData + destinationOffset);
        }
      }
      return;
    }

    constexpr usize maxScratchValues = k_CropScratchBytes / sizeof(T);
    const bool sourceSliceFits = sourceSliceTuples <= maxScratchValues / numComps;
    const bool destinationSliceFits = destinationSliceTuples <= maxScratchValues / numComps;
    usize batchLimit = 0;
    if(sourceSliceFits && destinationSliceFits)
    {
      const usize sourceSliceValues = sourceSliceTuples * numComps;
      const usize destinationSliceValues = destinationSliceTuples * numComps;
      if(sourceSliceValues <= maxScratchValues - destinationSliceValues)
      {
        const usize combinedSliceValues = sourceSliceValues + destinationSliceValues;
        batchLimit = (std::min)(k_ZSliceBatch, maxScratchValues / combinedSliceValues);
      }
    }

    if(batchLimit > 0)
    {
      const usize initialBatch = (std::min)(batchLimit, cropDimZ);
      auto sourceSlab = std::make_unique<T[]>(initialBatch * sourceSliceTuples * numComps);
      auto destinationSlab = std::make_unique<T[]>(initialBatch * destinationSliceTuples * numComps);
      const usize rowBytes = rowElements * sizeof(T);

      for(usize zStart = zMinIndex; zStart < zMaxIndex; zStart += batchLimit)
      {
        if(m_ShouldCancel || m_TaskResult.shouldAbort())
        {
          return;
        }
        const usize batch = (std::min)(batchLimit, zMaxIndex - zStart);
        const usize sourceSlabElements = batch * sourceSliceTuples * numComps;
        const usize destinationSlabElements = batch * destinationSliceTuples * numComps;

        Result<> readResult = m_OldCellStore.copyIntoBuffer(zStart * sourceSliceTuples * numComps, nonstd::span<T>(sourceSlab.get(), sourceSlabElements));
        if(readResult.invalid())
        {
          m_TaskResult.store(std::move(readResult));
          return;
        }

        // The slab route reduces HDF5 calls when both crop-owned buffers fit the cap.
        for(usize zOffset = 0; zOffset < batch; ++zOffset)
        {
          const T* sourceSlice = sourceSlab.get() + zOffset * sourceSliceTuples * numComps;
          T* destinationSlice = destinationSlab.get() + zOffset * destinationSliceTuples * numComps;
          for(usize yOffset = 0; yOffset < cropDimY; ++yOffset)
          {
            const T* sourceRow = sourceSlice + ((yMinIndex + yOffset) * sourceDimX + xMinIndex) * numComps;
            T* destinationRow = destinationSlice + yOffset * rowElements;
            std::memcpy(destinationRow, sourceRow, rowBytes);
          }
        }

        Result<> writeResult = m_NewCellStore.copyFromBuffer((zStart - zMinIndex) * destinationSliceTuples * numComps, nonstd::span<const T>(destinationSlab.get(), destinationSlabElements));
        if(writeResult.invalid())
        {
          m_TaskResult.store(std::move(writeResult));
          return;
        }
      }
      return;
    }

    // Align segments to complete tuples when one tuple fits. Wider tuples must split at flat value offsets.
    const usize segmentValues = numComps <= maxScratchValues ? (maxScratchValues / numComps) * numComps : maxScratchValues;
    const usize bufferValues = (std::min)(rowElements, segmentValues);
    auto buffer = std::make_unique<T[]>(bufferValues);
    for(usize zIndex = zMinIndex; zIndex < zMaxIndex; ++zIndex)
    {
      for(usize yIndex = yMinIndex; yIndex < yMaxIndex; ++yIndex)
      {
        const usize sourceOffset = ((zIndex * sourceDimY + yIndex) * sourceDimX + xMinIndex) * numComps;
        const usize destinationOffset = ((zIndex - zMinIndex) * cropDimY + yIndex - yMinIndex) * rowElements;
        for(usize rowValue = 0; rowValue < rowElements;)
        {
          if(m_ShouldCancel || m_TaskResult.shouldAbort())
          {
            return;
          }
          const usize count = (std::min)(bufferValues, rowElements - rowValue);
          Result<> readResult = m_OldCellStore.copyIntoBuffer(sourceOffset + rowValue, nonstd::span<T>(buffer.get(), count));
          if(readResult.invalid())
          {
            m_TaskResult.store(std::move(readResult));
            return;
          }
          Result<> writeResult = m_NewCellStore.copyFromBuffer(destinationOffset + rowValue, nonstd::span<const T>(buffer.get(), count));
          if(writeResult.invalid())
          {
            m_TaskResult.store(std::move(writeResult));
            return;
          }
          rowValue += count;
        }
      }
    }
  }

private:
  const AbstractDataStore<T>& m_OldCellStore;
  AbstractDataStore<T>& m_NewCellStore;
  std::string m_SourceArrayName;
  std::string m_DestinationArrayName;
  const ImageGeom& m_SrcImageGeom;
  std::array<uint64, 6> m_Bounds;
  const std::atomic_bool& m_ShouldCancel;
  CopyFromArray::ParallelTaskResult& m_TaskResult;
};
} // namespace

// -----------------------------------------------------------------------------
CropImageGeometry::CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CropImageGeometry::~CropImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CropImageGeometry::operator()()
{
  auto srcImagePath = m_InputValues->InputImageGeometryPath;
  auto destImagePath = m_InputValues->OutputImageGeometryPath;
  const auto featureIdsArrayPath = m_InputValues->FeatureIdsPath;
  auto shouldRenumberFeatures = m_InputValues->RenumberFeatures;
  auto cellFeatureAMPath = m_InputValues->CellFeatureAttributeMatrixPath;
  auto removeOriginalGeometry = m_InputValues->RemoveOriginalGeometry;

  uint64 xMin = m_InputValues->XMin;
  uint64 xMax = m_InputValues->XMax;
  uint64 yMax = m_InputValues->YMax;
  uint64 yMin = m_InputValues->YMin;
  uint64 zMax = m_InputValues->ZMax;
  uint64 zMin = m_InputValues->ZMin;

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(srcImagePath);

  // Source and destination paths are resolved before array tasks start.
  SizeVec3 udims = srcImageGeom.getDimensions();

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Check to see if the dims have actually changed.
  if(dims[0] == (xMax - xMin) && dims[1] == (yMax - yMin) && dims[2] == (zMax - zMin))
  {
    return {};
  }

  if(removeOriginalGeometry)
  {
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);
  FloatVec3 oldOrigin = destImageGeom.getOrigin();

  // Check to make sure the new dimensions are not "out of bounds" and warn the user if they are
  if(dims[0] <= xMax)
  {
    std::string errMsg = fmt::format("The Max X value ({}) is greater than the Image Geometry X extent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     xMax, dims[0]);
    return MakeErrorResult(-950, errMsg);
  }
  if(dims[1] <= yMax)
  {
    std::string errMsg = fmt::format("The Max Y value ({}) is greater than the Image Geometry Y extent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     yMax, dims[1]);
    return MakeErrorResult(-951, errMsg);
  }
  if(dims[2] <= zMax)
  {
    std::string errMsg = fmt::format("The Max Z value ({}) is greater than the Image Geometry Z extent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     zMax, dims[2]);
    return MakeErrorResult(-952, errMsg);
  }

  std::array<uint64, 6> bounds = {xMin, xMax + 1, yMin, yMax + 1, zMin, zMax + 1};

  // Each task owns one source and destination array. Arrays can copy in parallel.
  // Declared before the task runner so the runner's destructor joins every worker while this holder is still alive.
  CopyFromArray::ParallelTaskResult taskResult;
  ParallelTaskAlgorithm taskRunner;
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();
  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));

    m_MessageHandler(fmt::format("Cropping Volume || Copying Data Array {}", srcName));
    ExecuteParallelFunction<CropImageGeomDataArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, srcImageGeom, bounds, m_ShouldCancel, taskResult);
  }
  taskRunner.wait();
  Result<> cropResult = taskResult.takeResult();
  if(cropResult.invalid())
  {
    return cropResult;
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Copy feature arrays before renumbering so each tuple supplies initial data.
  if(shouldRenumberFeatures)
  {
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(featureIdsArrayPath);
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, cellFeatureAMPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
    std::vector<DataPath> sourceFeatureDataPaths;
    auto childPathsResult = GetAllChildArrayDataPaths(m_DataStructure, cellFeatureAMPath);
    if(childPathsResult.has_value())
    {
      sourceFeatureDataPaths = childPathsResult.value();
    }

    std::vector<DataPath> destFeatureDataPaths = sourceFeatureDataPaths;
    DataPath destCellFeatureAMPath = destImagePath.createChildPath(cellFeatureAMPath.getTargetName());

    for(auto& dataPath : destFeatureDataPaths)
    {
      dataPath = destCellFeatureAMPath.createChildPath(dataPath.getTargetName());
    }

    // DeepCopy replaces preflight outputs before renumbering resizes feature data.
    for(usize index = 0; index < sourceFeatureDataPaths.size(); index++)
    {
      DataObject* dataObject = m_DataStructure.getData(sourceFeatureDataPaths[index]);
      if(dataObject->getDataObjectType() == DataObject::Type::DataArray)
      {
        auto result = DeepCopy<IDataArray>(m_DataStructure, sourceFeatureDataPaths[index], destFeatureDataPaths[index]);
        if(result.invalid())
        {
          return result;
        }
      }
      else if(dataObject->getDataObjectType() == DataObject::Type::StringArray)
      {
        auto result = DeepCopy<StringArray>(m_DataStructure, sourceFeatureDataPaths[index], destFeatureDataPaths[index]);
        if(result.invalid())
        {
          return result;
        }
      }
    }

    // Renumber copied feature data and cropped cell Feature IDs together.
    DataPath destFeatureIdsPath = destImagePath.createChildPath(srcCellDataAM.getName()).createChildPath(featureIdsArrayPath.getTargetName());
    return Sampling::RenumberFeatures(m_DataStructure, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, m_MessageHandler, m_ShouldCancel);
  }

  // The deferred actions will take care of removing the original and renaming the output if
  // the user decided to do the crop "in place"
  return {};
}
