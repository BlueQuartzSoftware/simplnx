#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"

#include "simplnx/Common/ScratchBuffer.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
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
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
constexpr nx::core::int32 k_ExactDimensionsDomain = -86153;
constexpr nx::core::int32 k_AxisConversionRepresentation = -86137;
constexpr nx::core::int32 k_PreparedGeometryRepresentation = -86136;
constexpr nx::core::int32 k_ProvisionalProductWarning = -86131;
constexpr nx::core::int32 k_ProvisionalDimensionWarning = -86130;
constexpr nx::core::int32 k_ProvisionalSpacingWarning = -86129;
constexpr nx::core::int32 k_TupleProductRepresentation = -86128;
constexpr nx::core::int32 k_DimensionRepresentation = -86127;
constexpr nx::core::int32 k_DerivedSpacingRepresentation = -86126;
constexpr nx::core::int32 k_ScalingFinite = -86114;
constexpr nx::core::int32 k_SpacingPositiveFinite = -86113;

const std::string k_TempGeometryName = ".resampled_image_geometry";
constexpr ChoicesParameter::ValueType k_SpacingModeIndex = 0;
constexpr ChoicesParameter::ValueType k_ScalingModeIndex = 1;
constexpr ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;

// Destination rows completed between shared-seam reports.
constexpr usize k_ProgressRowBatch = 256;

// This sentinel marks a destination position outside the source bounds.
constexpr usize k_InvalidAxisIndex = std::numeric_limits<usize>::max();

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
    auto xResult = ResampleImageGeom::ComputeAxisSourceIndices(destDims[0], destOrigin[0], destSpacing[0], srcDims[0], srcOrigin[0], srcSpacing[0]);
    if(xResult.invalid())
    {
      m_TaskResult.store(ConvertResult(std::move(xResult)));
      return;
    }
    auto yResult = ResampleImageGeom::ComputeAxisSourceIndices(destDims[1], destOrigin[1], destSpacing[1], srcDims[1], srcOrigin[1], srcSpacing[1]);
    if(yResult.invalid())
    {
      m_TaskResult.store(ConvertResult(std::move(yResult)));
      return;
    }
    auto zResult = ResampleImageGeom::ComputeAxisSourceIndices(destDims[2], destOrigin[2], destSpacing[2], srcDims[2], srcOrigin[2], srcSpacing[2]);
    if(zResult.invalid())
    {
      m_TaskResult.store(ConvertResult(std::move(zResult)));
      return;
    }
    const auto& xIndices = xResult.value();
    const auto& yIndices = yResult.value();
    const auto& zIndices = zResult.value();

    const usize numComponents = m_DestArray.getNumberOfComponents();
    const usize destRowLength = destDims[0] * numComponents;
    const usize srcRowLength = srcDims[0] * numComponents;

    // Reuse row buffers so working memory is independent of the total cell count.
    ScratchBuffer<T> destRowBuffer(destRowLength);
    ScratchBuffer<T> srcRowBuffer(srcRowLength);

    // When every destination column maps to a source column the gather below overwrites the whole row,
    // so the row only needs to be zeroed when some column (or the whole row) has no source.
    const bool anyUnmappedX = std::find(xIndices.begin(), xIndices.end(), k_InvalidAxisIndex) != xIndices.end();

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

        if(!rowHasSource || anyUnmappedX)
        {
          // Matches the original per-voxel fillTuple(0) fallback for an out-of-bounds source lookup.
          std::fill_n(destRowBuffer.data(), destRowLength, static_cast<T>(0));
        }

        if(rowHasSource)
        {
          // Reuse the cached source row when upsampling maps consecutive destination rows to it.
          if(!haveCachedSrcRow || yIndex != cachedYIndex || zIndex != cachedZIndex)
          {
            const usize srcRowStart = ((srcDims[0] * srcDims[1] * zIndex) + (srcDims[0] * yIndex)) * numComponents;
            Result<> readResult = srcDataStore.copyIntoBuffer(srcRowStart, nonstd::span<T>(srcRowBuffer.data(), srcRowLength));
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
            T* destTuple = destRowBuffer.data() + (x * numComponents);
            if(xIndex != k_InvalidAxisIndex)
            {
              const T* srcTuple = srcRowBuffer.data() + (xIndex * numComponents);
              std::copy_n(srcTuple, numComponents, destTuple);
            }
          }
        }

        // Bulk-write the fully assembled destination row in a single store access.
        const usize destRowStart = ((z * destDims[1] * destDims[0]) + (y * destDims[0])) * numComponents;
        Result<> writeResult = destDataStore.copyFromBuffer(destRowStart, nonstd::span<const T>(destRowBuffer.data(), destRowLength));
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

// -----------------------------------------------------------------------------
Result<> nx::core::ValidateResamplingParameters(const ResampleImageGeomInputValues& inputValues)
{
  if(inputValues.ResamplingMode == k_ExactDimensionsModeIndex)
  {
    return ConvertResult(GeometryUtilities::ValidateImageDimensions(inputValues.ExactDimensions, "Exact Dimensions", k_ExactDimensionsDomain));
  }
  const bool scaling = inputValues.ResamplingMode == k_ScalingModeIndex;
  const auto& values = scaling ? inputValues.Scaling : inputValues.Spacing;
  const std::string name = scaling ? "Scaling Factor" : "Spacing";
  for(usize axis = 0; axis < 3; ++axis)
  {
    const float32 value = values[axis];
    if(!std::isfinite(value))
    {
      return MakeErrorResult(scaling ? k_ScalingFinite : k_SpacingPositiveFinite, fmt::format("{} {} value {} must be finite and greater than 0.", name, "XYZ"[axis], value));
    }
    if(value < 0 || (scaling && value == 0))
    {
      return MakeErrorResult(scaling ? -11500 : -11502, fmt::format("{} {} value {} must be greater than 0.", name, "XYZ"[axis], value));
    }
    if(value == 0)
    {
      return MakeErrorResult(k_SpacingPositiveFinite, fmt::format("Spacing {} value {} must be greater than 0.", "XYZ"[axis], value));
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<ResampledImageLayout> nx::core::ComputeResampledImageLayout(const ImageGeom& source, const ResampleImageGeomInputValues& inputValues)
{
  ResampledImageLayout layout;
  const auto sourceDimensions = source.getDimensions();
  const auto sourceSpacing = source.getSpacing();
  const float64 exclusiveBound = std::ldexp(1.0, std::numeric_limits<usize>::digits);
  usize product = 1;
  for(usize axis = 0; axis < 3; ++axis)
  {
    float32 spacing = 0;
    if(inputValues.ResamplingMode == k_ScalingModeIndex)
    {
      spacing = sourceSpacing[axis] / (inputValues.Scaling[axis] / 100);
    }
    else if(inputValues.ResamplingMode == k_ExactDimensionsModeIndex)
    {
      spacing = sourceSpacing[axis] * static_cast<float32>(sourceDimensions[axis]) / static_cast<float32>(inputValues.ExactDimensions[axis]);
    }
    else
    {
      spacing = inputValues.Spacing[axis];
    }
    if(inputValues.ResamplingMode != k_SpacingModeIndex && (!std::isfinite(spacing) || spacing <= 0))
    {
      return MakeErrorResult<ResampledImageLayout>(
          k_DerivedSpacingRepresentation,
          fmt::format("Image Geometry '{}' derived Spacing {} result {} must be finite and positive. Source spacing={}, source dimension={}, Scaling Factor={}, Exact Dimensions={}.",
                      inputValues.SelectedImageGeometryPath.toString(), "XYZ"[axis], spacing, sourceSpacing[axis], sourceDimensions[axis],
                      inputValues.ResamplingMode == k_ScalingModeIndex ? inputValues.Scaling[axis] : 100.0F,
                      inputValues.ResamplingMode == k_ExactDimensionsModeIndex ? inputValues.ExactDimensions[axis] : uint64{0}));
    }
    layout.Spacing[axis] = spacing;
    const float32 quotient = (sourceSpacing[axis] * static_cast<float32>(sourceDimensions[axis])) / spacing;
    if(!std::isfinite(quotient) || quotient < 0 || static_cast<float64>(quotient) >= exclusiveBound)
    {
      return MakeErrorResult<ResampledImageLayout>(
          k_DimensionRepresentation,
          fmt::format(
              "Image Geometry '{}' dimension conversion {} quotient {} must be finite, nonnegative and below the exclusive bound {}. Source dimension={}, source spacing={}, derived Spacing={}.",
              inputValues.SelectedImageGeometryPath.toString(), "XYZ"[axis], quotient, exclusiveBound, sourceDimensions[axis], sourceSpacing[axis], spacing));
    }
    layout.Dimensions[axis] = std::max<usize>(1, static_cast<usize>(quotient));
  }
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(layout.Dimensions[axis] > std::numeric_limits<usize>::max() / product)
    {
      return MakeErrorResult<ResampledImageLayout>(k_TupleProductRepresentation, fmt::format("Image Geometry '{}' resampled tuple product cannot fit usize: {}.",
                                                                                             inputValues.SelectedImageGeometryPath.toString(), StringUtilities::formatDimensions3D(layout.Dimensions)));
    }
    product *= layout.Dimensions[axis];
  }
  return {layout};
}

// -----------------------------------------------------------------------------
Result<std::vector<usize>> ResampleImageGeom::ComputeAxisSourceIndices(usize destDim, float64 destOrigin, float64 destSpacing, usize srcDim, float64 srcOrigin, float64 srcSpacing)
{
  std::vector<usize> indices(destDim, k_InvalidAxisIndex);
  const float64 sourceMaximum = static_cast<float64>(srcDim) * srcSpacing + srcOrigin;
  const float64 exclusiveBound = std::ldexp(1.0, std::numeric_limits<usize>::digits);
  for(usize index = 0; index < destDim; ++index)
  {
    const float64 coordinate = static_cast<float64>(index) * destSpacing + destOrigin;
    if(coordinate < srcOrigin || coordinate > sourceMaximum)
    {
      continue;
    }
    const float64 quotient = std::floor((coordinate - srcOrigin) / srcSpacing);
    if(!std::isfinite(quotient) || quotient < 0 || quotient >= exclusiveBound)
    {
      return MakeErrorResult<std::vector<usize>>(k_AxisConversionRepresentation,
                                                 fmt::format("Resampling axis conversion at destination index {} has coordinate {} and source quotient {}. The quotient must be finite, nonnegative "
                                                             "and below exclusive bound {} (source origin={}, spacing={}, dimension={}).",
                                                             index, coordinate, quotient, exclusiveBound, srcOrigin, srcSpacing, srcDim));
    }
    const usize sourceIndex = static_cast<usize>(quotient);
    if(sourceIndex < srcDim)
    {
      indices[index] = sourceIndex;
    }
  }
  return {std::move(indices)};
}

// -----------------------------------------------------------------------------
Result<> ResampleImageGeom::ValidatePreparedGeometry(const DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues)
{
  for(const auto& path : {inputValues.SelectedImageGeometryPath, inputValues.CreatedImageGeometryPath})
  {
    const auto& geometry = dataStructure.getDataRefAs<ImageGeom>(path);
    const auto dimensions = geometry.getDimensions();
    const auto spacing = geometry.getSpacing();
    const auto origin = geometry.getOrigin();
    usize tuples = 1;
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(!std::isfinite(spacing[axis]) || spacing[axis] <= 0 || !std::isfinite(origin[axis]))
      {
        return MakeErrorResult(k_PreparedGeometryRepresentation, fmt::format("Prepared Image Geometry '{}' {} metadata requires finite origin and positive finite spacing. Origin={}, Spacing={}.",
                                                                             path.toString(), "XYZ"[axis], origin[axis], spacing[axis]));
      }
    }
    if(dimensions[0] == 0 || dimensions[1] == 0 || dimensions[2] == 0)
    {
      tuples = 0;
    }
    else
    {
      for(usize axis = 0; axis < 3; ++axis)
      {
        if(dimensions[axis] > std::numeric_limits<usize>::max() / tuples)
        {
          return MakeErrorResult(k_PreparedGeometryRepresentation,
                                 fmt::format("Prepared Image Geometry '{}' tuple product overflows: {}.", path.toString(), StringUtilities::formatDimensions3D(dimensions)));
        }
        tuples *= dimensions[axis];
      }
    }
    if(dimensions[2] != 0 && dimensions[1] > std::numeric_limits<usize>::max() / dimensions[2])
    {
      return MakeErrorResult(k_PreparedGeometryRepresentation, fmt::format("Prepared Image Geometry '{}' row count overflows: Y={}, Z={}.", path.toString(), dimensions[1], dimensions[2]));
    }
    const usize rows = dimensions[1] * dimensions[2];
    if(path == inputValues.CreatedImageGeometryPath && rows != 0 && geometry.getCellDataRef().getSize() > std::numeric_limits<usize>::max() / rows)
    {
      return MakeErrorResult(k_PreparedGeometryRepresentation,
                             fmt::format("Prepared Image Geometry '{}' progress row count overflows: arrays={}, rows={}.", path.toString(), geometry.getCellDataRef().getSize(), rows));
    }
    for(const auto& [identifier, object] : geometry.getCellDataRef())
    {
      const auto& array = dynamic_cast<const IDataArray&>(*object);
      const usize components = array.getNumberOfComponents();
      const usize elementBytes = GetDataTypeSize(array.getDataType());
      const usize maximum = std::numeric_limits<usize>::max();
      if((components != 0 && (tuples > maximum / components || dimensions[0] > maximum / components)) ||
         (components != 0 && (tuples * components > maximum / elementBytes || dimensions[0] * components > maximum / elementBytes)))
      {
        return MakeErrorResult(k_PreparedGeometryRepresentation,
                               fmt::format("Prepared Image Geometry '{}' array '{}' row/value/byte size overflows: tuples={}, row={}, components={}, element bytes={}.", path.toString(),
                                           array.getName(), tuples, dimensions[0], components, elementBytes));
      }
    }
    if(path == inputValues.CreatedImageGeometryPath &&
       (dimensions[0] > std::vector<usize>{}.max_size() || dimensions[1] > std::vector<usize>{}.max_size() || dimensions[2] > std::vector<usize>{}.max_size()))
    {
      return MakeErrorResult(k_PreparedGeometryRepresentation, fmt::format("Prepared Image Geometry '{}' axis-map container size exceeds {}: {}.", path.toString(), std::vector<usize>{}.max_size(),
                                                                           StringUtilities::formatDimensions3D(dimensions)));
    }
  }
  return {};
}

IFilter::PreflightResult nx::core::PreflightImageGeometryResample(const DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues)
{
  ResampleImageGeomInputValues resolvedValues = inputValues;
  DataPath destImagePath = inputValues.CreatedImageGeometryPath;
  const DataPath& srcImagePath = inputValues.SelectedImageGeometryPath;

  Result<OutputActions> resultOutputActions;
  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  const auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  const auto srcOrigin = srcImageGeom.getOrigin().toContainer<std::vector<float32>>();
  auto layoutResult = ComputeResampledImageLayout(srcImageGeom, inputValues);
  ResampledImageLayout layout;
  if(layoutResult.invalid())
  {
    const auto& error = layoutResult.errors().front();
    const int32 warningCode = error.code == k_DerivedSpacingRepresentation ? k_ProvisionalSpacingWarning :
                              error.code == k_DimensionRepresentation      ? k_ProvisionalDimensionWarning :
                                                                             k_ProvisionalProductWarning;
    resultOutputActions.warnings().push_back(
        {warningCode, fmt::format("{} Preflight uses stand-in dimensions X=1, Y=1, Z=1 and unit spacing. Execution recomputes the layout from final geometry metadata.", error.message)});
  }
  else
  {
    layout = layoutResult.value();
  }
  resolvedValues.Spacing = layout.Spacing.toContainer<std::vector<float32>>();
  std::vector<usize> geometryDimensions = layout.Dimensions.toContainer<std::vector<usize>>();
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
      destImagePath, geometryDimensions, srcOrigin, CreateImageGeometryAction::OriginType{resolvedValues.Spacing[0], resolvedValues.Spacing[1], resolvedValues.Spacing[2]}, cellDataName,
      srcImageGeom.getUnits()));

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
    }

    AppendRenumberedFeatureAMWarnings(dataStructure, cellFeatureAttributeMatrixPath, inputValues.FeatureIdsArrayPath, preflightUpdatedValues);
  }

  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      const DataPath copiedChildPath = childPath.rebase(srcImagePath, destImagePath).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            allCreatedPaths.push_back(sourcePath.rebase(srcImagePath, destImagePath).value());
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

  AppendCopiedAMStaleWarning(dataStructure, childPaths.value_or(std::vector<DataPath>{}), resultOutputActions);

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
  auto preparedResult = ValidatePreparedGeometry(m_DataStructure, *m_InputValues);
  if(preparedResult.invalid())
  {
    return preparedResult;
  }

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->CreatedImageGeometryPath);
  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

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

    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();
    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    {
      const std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
      m_MessageHandler.sendInfoMessage(fmt::format("Resampling Data Array: '{}'", srcName));
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
  auto layout = ComputeResampledImageLayout(dataStructure.getDataRefAs<ImageGeom>(inputValues.SelectedImageGeometryPath), inputValues);
  if(layout.invalid())
  {
    return ConvertResult(std::move(layout));
  }
  resolvedValues.Spacing = layout.value().Spacing.toContainer<std::vector<float32>>();

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
