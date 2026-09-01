#include "ResampleImageGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
// Marks a destination axis position outside the source geometry.
constexpr usize k_InvalidAxisIndex = std::numeric_limits<usize>::max();

/**
 * @brief Maps destination minimum corners to containing source cells on one axis.
 * @param destDimSize Number of destination positions.
 * @param destOriginComp Destination origin coordinate.
 * @param destSpacingComp Destination cell spacing.
 * @param srcDimSize Number of source cells.
 * @param srcOriginComp Source origin coordinate.
 * @param srcSpacingComp Source cell spacing.
 * @return Source indexes or k_InvalidAxisIndex for positions outside source bounds.
 * @pre destSpacingComp and srcSpacingComp are positive.
 *
 * Regular-grid lookup is separable by axis. Precomputation replaces a bounds
 * check and floor division in every destination-cell iteration. float64
 * arithmetic preserves the ImageGeom lookup promotion of float32 geometry data.
 */
std::vector<usize> ComputeAxisSrcIndices(usize destDimSize, float64 destOriginComp, float64 destSpacingComp, usize srcDimSize, float64 srcOriginComp, float64 srcSpacingComp)
{
  std::vector<usize> srcIndices(destDimSize, k_InvalidAxisIndex);
  const float64 srcMaxCoord = static_cast<float64>(srcDimSize) * srcSpacingComp + srcOriginComp;
  for(usize i = 0; i < destDimSize; i++)
  {
    const float64 destCoord = static_cast<float64>(i) * destSpacingComp + destOriginComp;
    if(destCoord < srcOriginComp || destCoord > srcMaxCoord)
    {
      continue;
    }
    const auto srcIdx = static_cast<usize>(std::floor((destCoord - srcOriginComp) / srcSpacingComp));
    if(srcIdx < srcDimSize)
    {
      srcIndices[i] = srcIdx;
    }
  }
  return srcIndices;
}

/**
 * @class ResampleImageGeomArrayImpl
 * @brief Resamples one typed cell array through reusable row buffers.
 * @tparam T Specifies the cell-array value type.
 *
 * X-contiguous rows make destination writes sequential. Consecutive destination
 * rows that map to the same source row reuse one bulk read. Working memory is
 * proportional to source and destination X dimensions, not total cell count.
 * Bulk-I/O results are discarded.
 */
template <typename T>
class ResampleImageGeomArrayImpl
{
public:
  /**
   * @brief Initializes one array resampling task.
   * @param algorithm Receives thread-safe progress messages.
   * @param srcArray Supplies source cell tuples.
   * @param destArray Receives destination cell tuples.
   * @param srcImageGeom Supplies source grid coordinates.
   * @param destImageGeom Supplies destination grid coordinates.
   * @param shouldCancel Signals cancellation between destination Z slices.
   * @pre algorithm is not null.
   * @pre All arguments outlive this task.
   */
  ResampleImageGeomArrayImpl(ResampleImageGeom* algorithm, const IDataArray& srcArray, IDataArray& destArray, const ImageGeom& srcImageGeom, const ImageGeom& destImageGeom,
                             const std::atomic_bool& shouldCancel)
  : m_AlgorithmPtr(algorithm)
  , m_SrcArray(srcArray)
  , m_DestArray(destArray)
  , m_SrcImageGeom(srcImageGeom)
  , m_DestImageGeom(destImageGeom)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Copies mapped source tuples to destination rows.
   *
   * An out-of-bounds axis position writes zeros. Cancellation stops before a
   * later Z slice and keeps completed destination rows.
   */
  void operator()() const
  {
    const auto& srcDataStore = m_SrcArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destDataStore = m_DestArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    const SizeVec3 srcDims = m_SrcImageGeom.getDimensions();
    const SizeVec3 destDims = m_DestImageGeom.getDimensions();
    const FloatVec3 srcOrigin = m_SrcImageGeom.getOrigin();
    const FloatVec3 srcSpacing = m_SrcImageGeom.getSpacing();
    const FloatVec3 destOrigin = m_DestImageGeom.getOrigin();
    const FloatVec3 destSpacing = m_DestImageGeom.getSpacing();

    // Precompute separable source-cell lookup for each destination axis.
    const std::vector<usize> xIndices = ComputeAxisSrcIndices(destDims[0], destOrigin[0], destSpacing[0], srcDims[0], srcOrigin[0], srcSpacing[0]);
    const std::vector<usize> yIndices = ComputeAxisSrcIndices(destDims[1], destOrigin[1], destSpacing[1], srcDims[1], srcOrigin[1], srcSpacing[1]);
    const std::vector<usize> zIndices = ComputeAxisSrcIndices(destDims[2], destOrigin[2], destSpacing[2], srcDims[2], srcOrigin[2], srcSpacing[2]);

    const usize numComponents = m_DestArray.getNumberOfComponents();
    const usize destRowLength = destDims[0] * numComponents;
    const usize srcRowLength = srcDims[0] * numComponents;

    // Reuse one source and one destination row for the complete array.
    auto destRowBuffer = std::make_unique<T[]>(destRowLength);
    auto srcRowBuffer = std::make_unique<T[]>(srcRowLength);

    bool haveCachedSrcRow = false;
    usize cachedYIndex = k_InvalidAxisIndex;
    usize cachedZIndex = k_InvalidAxisIndex;

    const usize numVoxels = m_DestImageGeom.getNumberOfCells();
    const usize counterIncrement = numVoxels / 100 == 0 ? 100 : numVoxels / 100;
    usize processedVoxels = 0;
    usize counter = 0;

    for(usize z = 0; z < destDims[2]; z++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize zIndex = zIndices[z];
      for(usize y = 0; y < destDims[1]; y++)
      {
        const usize yIndex = yIndices[y];
        const bool rowHasSource = (zIndex != k_InvalidAxisIndex) && (yIndex != k_InvalidAxisIndex);

        if(rowHasSource)
        {
          // Reuse a source row when consecutive destination rows map to it.
          if(!haveCachedSrcRow || yIndex != cachedYIndex || zIndex != cachedZIndex)
          {
            const usize srcRowStart = ((srcDims[0] * srcDims[1] * zIndex) + (srcDims[0] * yIndex)) * numComponents;
            srcDataStore.copyIntoBuffer(srcRowStart, nonstd::span<T>(srcRowBuffer.get(), srcRowLength));
            cachedYIndex = yIndex;
            cachedZIndex = zIndex;
            haveCachedSrcRow = true;
          }

          // Gather X positions locally without per-cell store access.
          for(usize x = 0; x < destDims[0]; x++)
          {
            const usize xIndex = xIndices[x];
            T* destTuple = destRowBuffer.get() + (x * numComponents);
            if(xIndex != k_InvalidAxisIndex)
            {
              const T* srcTuple = srcRowBuffer.get() + (xIndex * numComponents);
              std::copy_n(srcTuple, numComponents, destTuple);
            }
            else
            {
              std::fill_n(destTuple, numComponents, static_cast<T>(0));
            }
          }
        }
        else
        {
          // A row outside source Y or Z bounds receives zero tuples.
          std::fill_n(destRowBuffer.get(), destRowLength, static_cast<T>(0));
        }

        // Publish the complete destination row with one store operation.
        const usize destRowStart = ((z * destDims[1] * destDims[0]) + (y * destDims[0])) * numComponents;
        destDataStore.copyFromBuffer(destRowStart, nonstd::span<const T>(destRowBuffer.get(), destRowLength));

        processedVoxels += destDims[0];
        counter += destDims[0];
        if(counter >= counterIncrement)
        {
          const float progress = static_cast<float>(processedVoxels) / static_cast<float>(numVoxels) * 100.0f;
          m_AlgorithmPtr->sendThreadSafeProgressMessage(fmt::format("Resampling Data Array '{}' {:.0f}% Complete", m_DestArray.getName(), progress));
          counter = 0;
        }
      }
    }
    m_AlgorithmPtr->sendThreadSafeProgressMessage(fmt::format("Resampling Data Array '{}' Complete", m_DestArray.getName()));
  }

private:
  ResampleImageGeom* m_AlgorithmPtr = nullptr;
  const IDataArray& m_SrcArray;
  IDataArray& m_DestArray;
  const ImageGeom& m_SrcImageGeom;
  const ImageGeom& m_DestImageGeom;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

ResampleImageGeom::ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

ResampleImageGeom::~ResampleImageGeom() noexcept = default;

const std::atomic_bool& ResampleImageGeom::getCancel()
{
  return m_ShouldCancel;
}

Result<> ResampleImageGeom::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  m_ThrottledMessengerPtr = &throttledMessenger;

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->CreatedImageGeometryPath);
  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  usize arrayIndex = 0;
  usize totalArrays = srcCellDataAM.getSize();

  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    arrayIndex++;
    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();
    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler(fmt::format("Resampling Data Array: '{}' ({}/{})", srcName, arrayIndex, totalArrays));

    ExecuteParallelFunction<ResampleImageGeomArrayImpl>(oldDataArray.getDataType(), taskRunner, this, oldDataArray, newDataArray, selectedImageGeom, destImageGeom, m_ShouldCancel);
  }

  taskRunner.wait();

  if(m_ShouldCancel)
  {
    return {};
  }

  // Feature renumbering needs independent arrays because compaction resizes them.
  DataPath cellFeatureAMPath = m_InputValues->CellFeatureAttributeMatrix;
  auto destImagePath = m_InputValues->CreatedImageGeometryPath;
  DataPath featureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;

  if(m_InputValues->RenumberFeatures)
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

    // Replace preflight placeholders with deep copies before feature compaction.
    for(size_t index = 0; index < sourceFeatureDataPaths.size(); index++)
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

    DataPath destFeatureIdsPath = destImagePath.createChildPath(srcCellDataAM.getName()).createChildPath(featureIdsArrayPath.getTargetName());
    return Sampling::RenumberFeatures(m_DataStructure, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}

void ResampleImageGeom::sendThreadSafeProgressMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  if(nullptr != m_ThrottledMessengerPtr)
  {
    m_ThrottledMessengerPtr->sendThrottledMessage([&]() { return message; });
  }
}
