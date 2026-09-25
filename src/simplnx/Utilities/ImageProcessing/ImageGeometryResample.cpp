#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".resampled_image_geometry";
constexpr ChoicesParameter::ValueType k_ScalingModeIndex = 1;
constexpr ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;

void CalculateResampledSpacing(const DataStructure& dataStructure, ResampleImageGeomInputValues& inputValues)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues.SelectedImageGeometryPath);
  if(inputValues.ResamplingMode == k_ScalingModeIndex)
  {
    const auto spacing = imageGeom.getSpacing();
    std::transform(spacing.begin(), spacing.end(), inputValues.Scaling.begin(), inputValues.Spacing.begin(), [](float32 value, float32 scale) { return value / (scale / 100); });
  }
  else if(inputValues.ResamplingMode == k_ExactDimensionsModeIndex)
  {
    const auto spacing = imageGeom.getSpacing();
    const auto dimensions = imageGeom.getDimensions();
    inputValues.Spacing[0] = spacing[0] * static_cast<float32>(dimensions[0]) / static_cast<float32>(inputValues.ExactDimensions[0]);
    inputValues.Spacing[1] = spacing[1] * static_cast<float32>(dimensions[1]) / static_cast<float32>(inputValues.ExactDimensions[1]);
    inputValues.Spacing[2] = spacing[2] * static_cast<float32>(dimensions[2]) / static_cast<float32>(inputValues.ExactDimensions[2]);
  }
}

// Destination rows completed between shared-seam reports.
constexpr usize k_ProgressRowBatch = 256;

// This sentinel marks a destination position outside the source bounds.
constexpr usize k_InvalidAxisIndex = std::numeric_limits<usize>::max();

/**
 * @brief Maps destination positions on one axis to source cell indices.
 *
 * Axis-aligned regular grids make this mapping independent for each axis. Precomputation removes bounds checks and division from the voxel loop.
 *
 * @param destDimSize Number of destination coordinates to resolve along this axis
 * @param destOriginComp Destination Image Geometry origin component for this axis
 * @param destSpacingComp Destination Image Geometry spacing component for this axis
 * @param srcDimSize Source Image Geometry dimension for this axis
 * @param srcOriginComp Source Image Geometry origin component for this axis
 * @param srcSpacingComp Source Image Geometry spacing component for this axis
 * @return Source indices or k_InvalidAxisIndex for positions outside the source bounds.
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
 * @brief Resamples one cell-data array by nearest-source-cell lookup.
 * @tparam T Specifies the array value type.
 *
 * The algorithm reads and writes complete X rows. This keeps store access proportional to the row count.
 * The algorithm reuses a source row when consecutive destination rows have the same mapping. Buffers are bounded by one row.
 */
template <typename T>
class ResampleImageGeomArrayImpl
{
public:
  ResampleImageGeomArrayImpl(ResampleImageGeom* algorithm, const IDataArray& srcArray, IDataArray& destArray, const ImageGeom& srcImageGeom, const ImageGeom& destImageGeom,
                             const std::atomic_bool& shouldCancel, CopyFromArray::ParallelTaskResult& taskResult)
  : m_AlgorithmPtr(algorithm)
  , m_SrcArray(srcArray)
  , m_DestArray(destArray)
  , m_SrcImageGeom(srcImageGeom)
  , m_DestImageGeom(destImageGeom)
  , m_ShouldCancel(shouldCancel)
  , m_TaskResult(taskResult)
  {
  }

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

    // Precompute the independent source-cell mapping for each axis.
    const std::vector<usize> xIndices = ComputeAxisSrcIndices(destDims[0], destOrigin[0], destSpacing[0], srcDims[0], srcOrigin[0], srcSpacing[0]);
    const std::vector<usize> yIndices = ComputeAxisSrcIndices(destDims[1], destOrigin[1], destSpacing[1], srcDims[1], srcOrigin[1], srcSpacing[1]);
    const std::vector<usize> zIndices = ComputeAxisSrcIndices(destDims[2], destOrigin[2], destSpacing[2], srcDims[2], srcOrigin[2], srcSpacing[2]);

    const usize numComponents = m_DestArray.getNumberOfComponents();
    const usize destRowLength = destDims[0] * numComponents;
    const usize srcRowLength = srcDims[0] * numComponents;

    // Reuse row buffers so working memory is independent of the total cell count.
    auto destRowBuffer = std::make_unique<T[]>(destRowLength);
    auto srcRowBuffer = std::make_unique<T[]>(srcRowLength);

    bool haveCachedSrcRow = false;
    usize cachedYIndex = k_InvalidAxisIndex;
    usize cachedZIndex = k_InvalidAxisIndex;

    // Rows are too small a unit to enter the shared seam on. Batch them so the lock is taken once
    // per k_ProgressRowBatch rows, and flush whatever remains when this worker finishes.
    usize pendingRows = 0;

    for(usize z = 0; z < destDims[2]; z++)
    {
      if(m_ShouldCancel || m_TaskResult.shouldAbort())
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
          // Reuse the cached source row when upsampling maps consecutive destination rows to it.
          if(!haveCachedSrcRow || yIndex != cachedYIndex || zIndex != cachedZIndex)
          {
            const usize srcRowStart = ((srcDims[0] * srcDims[1] * zIndex) + (srcDims[0] * yIndex)) * numComponents;
            Result<> readResult = srcDataStore.copyIntoBuffer(srcRowStart, nonstd::span<T>(srcRowBuffer.get(), srcRowLength));
            if(readResult.invalid())
            {
              m_TaskResult.store(std::move(readResult));
              return;
            }
            cachedYIndex = yIndex;
            cachedZIndex = zIndex;
            haveCachedSrcRow = true;
          }

          // Gather mapped values in memory without per-voxel store access.
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
          // The whole row falls outside the source geometry along y or z - matches the original
          // per-voxel fillTuple(0) fallback for an out-of-bounds source lookup.
          std::fill_n(destRowBuffer.get(), destRowLength, static_cast<T>(0));
        }

        // Bulk-write the fully assembled destination row in a single store access.
        const usize destRowStart = ((z * destDims[1] * destDims[0]) + (y * destDims[0])) * numComponents;
        Result<> writeResult = destDataStore.copyFromBuffer(destRowStart, nonstd::span<const T>(destRowBuffer.get(), destRowLength));
        if(writeResult.invalid())
        {
          m_TaskResult.store(std::move(writeResult));
          return;
        }

        pendingRows++;
        if(pendingRows >= k_ProgressRowBatch)
        {
          m_AlgorithmPtr->sendThreadSafeProgressMessage(pendingRows);
          pendingRows = 0;
        }
      }
    }

    if(pendingRows > 0)
    {
      m_AlgorithmPtr->sendThreadSafeProgressMessage(pendingRows);
    }
  }

private:
  ResampleImageGeom* m_AlgorithmPtr = nullptr;
  const IDataArray& m_SrcArray;
  IDataArray& m_DestArray;
  const ImageGeom& m_SrcImageGeom;
  const ImageGeom& m_DestImageGeom;
  const std::atomic_bool& m_ShouldCancel;
  CopyFromArray::ParallelTaskResult& m_TaskResult;
};
} // namespace

IFilter::PreflightResult nx::core::PreflightImageGeometryResample(const DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues)
{
  ResampleImageGeomInputValues resolvedValues = inputValues;
  DataPath destImagePath = inputValues.CreatedImageGeometryPath;
  const DataPath& srcImagePath = inputValues.SelectedImageGeometryPath;

  Result<OutputActions> resultOutputActions;
  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  if(inputValues.ResamplingMode == k_ScalingModeIndex)
  {
    if(std::any_of(inputValues.Scaling.begin(), inputValues.Scaling.end(), [](float value) { return value <= 0.0F; }))
    {
      const std::string errorMessage = fmt::format("Scaling Factor has a non-positive value. {}, {}, {}", inputValues.Scaling[0], inputValues.Scaling[1], inputValues.Scaling[2]);
      return IFilter::MakePreflightErrorResult(-11500, errorMessage);
    }
    CalculateResampledSpacing(dataStructure, resolvedValues);
  }
  else if(inputValues.ResamplingMode == k_ExactDimensionsModeIndex)
  {
    CalculateResampledSpacing(dataStructure, resolvedValues);
  }
  else if(inputValues.Spacing[0] < 0.0F || inputValues.Spacing[1] < 0.0F || inputValues.Spacing[2] < 0.0F)
  {
    const std::string errorMessage = fmt::format("Input Spacing has a negative value. {}, {}, {}", inputValues.Spacing[0], inputValues.Spacing[1], inputValues.Spacing[2]);
    return IFilter::MakePreflightErrorResult(-11502, errorMessage);
  }

  const auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  const SizeVec3 srcDimensions = srcImageGeom.getDimensions();
  const FloatVec3 srcSpacing = srcImageGeom.getSpacing();
  const auto srcOrigin = srcImageGeom.getOrigin().toContainer<std::vector<float>>();

  auto xDimension = static_cast<size_t>((srcSpacing[0] * static_cast<float>(srcDimensions[0])) / resolvedValues.Spacing[0]);
  auto yDimension = static_cast<size_t>((srcSpacing[1] * static_cast<float>(srcDimensions[1])) / resolvedValues.Spacing[1]);
  auto zDimension = static_cast<size_t>((srcSpacing[2] * static_cast<float>(srcDimensions[2])) / resolvedValues.Spacing[2]);
  if(xDimension == 0)
  {
    xDimension = 1;
  }
  if(yDimension == 0)
  {
    yDimension = 1;
  }
  if(zDimension == 0)
  {
    zDimension = 1;
  }

  std::vector<usize> geometryDimensions = {xDimension, yDimension, zDimension};
  std::vector<usize> dataArrayShape = {geometryDimensions[2], geometryDimensions[1], geometryDimensions[0]};
  std::vector<DataPath> ignorePaths;

  if(inputValues.RemoveOriginalImageGeom)
  {
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    const DataPath tempPath(tempPathVector);
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(srcImagePath, tempName));
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));

    tempPathVector = srcImagePath.getPathVector();
    tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  const AttributeMatrix* selectedCellData = srcImageGeom.getCellData();
  if(selectedCellData == nullptr)
  {
    return IFilter::MakePreflightErrorResult(-5851, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()));
  }
  const std::string cellDataName = selectedCellData->getName();
  ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

  resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(
      destImagePath, geometryDimensions, srcOrigin, CreateImageGeometryAction::OriginType{resolvedValues.Spacing[0], resolvedValues.Spacing[1], resolvedValues.Spacing[2]}, cellDataName));

  const DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);
  for(const auto& [identifier, object] : *selectedCellData)
  {
    const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
    const DataType dataType = srcArray.getDataType();
    ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
    const DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
  }

  preflightUpdatedValues.push_back(
      {"Input Geometry Info", GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom.getDimensions(), srcImageGeom.getSpacing(), srcImageGeom.getOrigin(), srcImageGeom.getUnits())});
  preflightUpdatedValues.push_back(
      {"Resampled Image Geometry Info", GeometryHelpers::Description::GenerateGeometryInfo(geometryDimensions, resolvedValues.Spacing, srcOrigin, srcImageGeom.getUnits())});

  if(inputValues.RenumberFeatures)
  {
    const DataPath& cellFeatureAttributeMatrixPath = inputValues.CellFeatureAttributeMatrix;
    ignorePaths.push_back(cellFeatureAttributeMatrixPath);
    const auto* srcCellFeatureData = dataStructure.getDataAs<AttributeMatrix>(cellFeatureAttributeMatrixPath);
    if(srcCellFeatureData == nullptr)
    {
      return IFilter::MakePreflightErrorResult(-55502, fmt::format("Could not find the selected Attribute Matrix '{}'", cellFeatureAttributeMatrixPath.toString()));
    }

    std::string warningMessage;
    const DataPath destCellFeatureAttributeMatrixPath = destImagePath.createChildPath(cellFeatureAttributeMatrixPath.getTargetName());
    const auto tupleDimensions = srcCellFeatureData->getShape();
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(destCellFeatureAttributeMatrixPath, tupleDimensions));
    for(const auto& [identifier, object] : *srcCellFeatureData)
    {
      if(const auto* srcArray = dynamic_cast<const IDataArray*>(object.get()); srcArray != nullptr)
      {
        const DataType dataType = srcArray->getDataType();
        ShapeType componentShape = srcArray->getIDataStoreRef().getComponentShape();
        const DataPath dataArrayPath = destCellFeatureAttributeMatrixPath.createChildPath(srcArray->getName());
        resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, tupleDimensions, std::move(componentShape), dataArrayPath));
      }
      else if(const auto* srcNeighborListArray = dynamic_cast<const INeighborList*>(object.get()); srcNeighborListArray != nullptr)
      {
        warningMessage += "\n" + cellFeatureAttributeMatrixPath.toString() + "/" + srcNeighborListArray->getName();
      }
    }
    if(!warningMessage.empty())
    {
      preflightUpdatedValues.push_back(
          {"Invalidated NeighborLists",
           fmt::format(
               "This filter will modify the Cell Level Array '{}' which causes all Feature level NeighborLists to become invalid. These NeighborLists will not be copied to the new geometry:{}",
               inputValues.FeatureIdsArrayPath.toString(), warningMessage)});
    }
  }

  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      const std::string copiedChildName = StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
      const DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            const std::string createdPathName = StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
            allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
          }
        }
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, allCreatedPaths));
      }
      else
      {
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, std::vector<DataPath>{copiedChildPath}));
      }
    }
  }

  if(inputValues.RemoveOriginalImageGeom)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
    AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, srcImageGeom.getCellDataPath(), {});
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
ResampleImageGeom::ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
, m_Throttle(msgHandler)
{
}

// -----------------------------------------------------------------------------
ResampleImageGeom::~ResampleImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ResampleImageGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ResampleImageGeom::operator()()
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->CreatedImageGeometryPath);
  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  usize arrayIndex = 0;
  usize totalArrays = srcCellDataAM.getSize();
  const SizeVec3 destDims = destImageGeom.getDimensions();
  m_Throttle.reset(totalArrays * destDims[1] * destDims[2], "Resampling cell arrays");

  // Declared before the task runner so the runner's destructor joins every worker while this holder is still alive.
  CopyFromArray::ParallelTaskResult taskResult;
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
    {
      const std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
      m_MessageHandler.sendInfoMessage(fmt::format("Resampling Data Array: '{}' ({}/{})", srcName, arrayIndex, totalArrays));
    }

    ExecuteParallelFunction<ResampleImageGeomArrayImpl>(oldDataArray.getDataType(), taskRunner, this, oldDataArray, newDataArray, selectedImageGeom, destImageGeom, m_ShouldCancel, taskResult);
  }

  taskRunner.wait();
  Result<> arrayResult = taskResult.takeResult();
  if(arrayResult.invalid())
  {
    return arrayResult;
  }

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

void ResampleImageGeom::sendThreadSafeProgressMessage(usize completedRows)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementPercent(completedRows);
}

Result<> nx::core::ResampleImageGeometry(DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel)
{
  ResampleImageGeomInputValues resolvedValues = inputValues;
  CalculateResampledSpacing(dataStructure, resolvedValues);

  const auto* cellDataGroup = dataStructure.getDataRefAs<ImageGeom>(resolvedValues.SelectedImageGeometryPath).getCellData();
  resolvedValues.CellDataGroupPath = resolvedValues.SelectedImageGeometryPath.createChildPath(cellDataGroup->getName());

  if(resolvedValues.RemoveOriginalImageGeom)
  {
    auto tempPathVector = resolvedValues.SelectedImageGeometryPath.getPathVector();
    tempPathVector.back() = k_TempGeometryName;
    resolvedValues.CreatedImageGeometryPath = DataPath({tempPathVector});
  }

  return ResampleImageGeom(dataStructure, messageHandler, shouldCancel, &resolvedValues)();
}
