#include "ReadImageStack.hpp"
#include "ReadImageStackCropping.hpp"

#include "NXImageProcessing/Filters/ReadImageFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{

Arguments BuildReadImageFilterArgs(const ReadImageSubFilterConfig& config)
{
  Arguments args;
  args.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<fs::path>(config.file_path));
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(config.image_geometry_path));
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, std::make_any<DataObjectNameParameter::ValueType>(config.cell_data_name));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, std::make_any<DataObjectNameParameter::ValueType>(config.image_data_array_name));
  args.insertOrAssign(ReadImageFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(IGeometry::LengthUnit::Micrometer)));
  args.insertOrAssign(ReadImageFilter::k_ChangeDataType_Key, std::make_any<BoolParameter::ValueType>(config.change_data_type));
  args.insertOrAssign(ReadImageFilter::k_ImageDataType_Key, std::make_any<ChoicesParameter::ValueType>(ImageDataTypeToChoice(config.image_data_type)));
  const bool forwardOrigin = config.change_origin;
  const bool forwardSpacing = config.change_spacing;
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, std::make_any<BoolParameter::ValueType>(forwardOrigin));
  args.insertOrAssign(ReadImageFilter::k_CenterOrigin_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ReadImageFilter::k_Origin_Key, std::make_any<VectorFloat32ParameterType::ValueType>(config.origin));
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, std::make_any<BoolParameter::ValueType>(forwardSpacing));
  args.insertOrAssign(ReadImageFilter::k_Spacing_Key, std::make_any<VectorFloat32ParameterType::ValueType>(config.spacing));
  args.insertOrAssign(ReadImageFilter::k_OriginSpacingProcessing_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(config.origin_spacing_processing)));
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(config.cropping_options));
  return args;
}

DataPath ResampledTemporaryGeometryPath(const DataPath& imageGeomPath)
{
  return imageGeomPath.replaceName(imageGeomPath.getTargetName() + "_resampled");
}

} // namespace nx::core

namespace
{
const ChoicesParameter::ValueType k_NoResampleModeIndex = 0;
const ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;

Result<> ExecuteResample(DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  IFilter::PreflightResult preflightResult = PreflightImageGeometryResample(dataStructure, inputValues);
  if(preflightResult.output_actions.invalid())
  {
    return ConvertResult(std::move(preflightResult.output_actions));
  }

  OutputActions outputActions = std::move(preflightResult.output_actions.value());
  Result<> regularResult = outputActions.applyRegular(dataStructure, IDataAction::Mode::Execute);
  if(regularResult.invalid())
  {
    return regularResult;
  }

  Result<> executeResult = ResampleImageGeometry(dataStructure, inputValues, messageHandler, shouldCancel);
  if(executeResult.invalid())
  {
    return MergeResults(std::move(regularResult), std::move(executeResult));
  }

  Result<> deferredResult = outputActions.applyDeferred(dataStructure, IDataAction::Mode::Execute);
  return MergeResults(MergeResults(std::move(regularResult), std::move(executeResult)), std::move(deferredResult));
}

Result<> ExecuteGrayscale(DataStructure& dataStructure, const ConvertColorToGrayScaleInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  IFilter::PreflightResult preflightResult = PreflightColorToGrayScale(dataStructure, inputValues);
  if(preflightResult.output_actions.invalid())
  {
    return ConvertResult(std::move(preflightResult.output_actions));
  }

  OutputActions outputActions = std::move(preflightResult.output_actions.value());
  Result<> regularResult = outputActions.applyRegular(dataStructure, IDataAction::Mode::Execute);
  if(regularResult.invalid())
  {
    return regularResult;
  }

  Result<> executeResult = ConvertColorToGrayScaleArrays(dataStructure, inputValues, messageHandler, shouldCancel);
  if(executeResult.invalid())
  {
    return MergeResults(std::move(regularResult), std::move(executeResult));
  }

  Result<> deferredResult = outputActions.applyDeferred(dataStructure, IDataAction::Mode::Execute);
  return MergeResults(MergeResults(std::move(regularResult), std::move(executeResult)), std::move(deferredResult));
}

/**
 * @brief Reverses tuple order within each image row.
 * @tparam T Specifies the image scalar type.
 * @param dataArray Provides and receives one image slice.
 * @param dims Specifies slice dimensions.
 * @return First row read or write error, or success.
 *
 * One typed row buffer bounds scratch memory.
 */
template <class T>
Result<> FlipAboutYAxis(DataArray<T>& dataArray, const Vec3<usize>& dims)
{
  AbstractDataStore<T>& dataStoreRef = dataArray.getDataStoreRef();

  const usize numComp = dataStoreRef.getNumberOfComponents();
  const usize rowElements = dims[0] * numComp;
  auto currentRowBuffer = std::make_unique<T[]>(rowElements);

  for(usize row = 0; row < dims[1]; row++)
  {
    const usize rowOffset = row * rowElements;
    Result<> readResult = dataStoreRef.copyIntoBuffer(rowOffset, nonstd::span<T>(currentRowBuffer.get(), rowElements));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(usize leftTuple = 0, rightTuple = dims[0] - 1; leftTuple < rightTuple; leftTuple++, rightTuple--)
    {
      for(usize component = 0; component < numComp; component++)
      {
        std::swap(currentRowBuffer[leftTuple * numComp + component], currentRowBuffer[rightTuple * numComp + component]);
      }
    }

    Result<> writeResult = dataStoreRef.copyFromBuffer(rowOffset, nonstd::span<const T>(currentRowBuffer.get(), rowElements));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

/**
 * @brief Swaps mirrored image rows.
 * @tparam T Specifies the image scalar type.
 * @param dataArray Provides and receives one image slice.
 * @param dims Specifies slice dimensions.
 * @return First row read or write error, or success.
 *
 * Two row buffers bound scratch memory. An odd middle row remains unchanged.
 */
template <class T>
Result<> FlipAboutXAxis(DataArray<T>& dataArray, const Vec3<usize>& dims)
{
  AbstractDataStore<T>& dataStoreRef = dataArray.getDataStoreRef();
  const usize numComp = dataStoreRef.getNumberOfComponents();
  const usize rowElements = dims[0] * numComp;
  auto topRowBuffer = std::make_unique<T[]>(rowElements);
  auto bottomRowBuffer = std::make_unique<T[]>(rowElements);
  const usize rowSwapCount = dims[1] / 2;

  for(usize topRow = 0; topRow < rowSwapCount; topRow++)
  {
    const usize bottomRow = dims[1] - 1 - topRow;
    const usize topOffset = topRow * rowElements;
    const usize bottomOffset = bottomRow * rowElements;

    Result<> topReadResult = dataStoreRef.copyIntoBuffer(topOffset, nonstd::span<T>(topRowBuffer.get(), rowElements));
    if(topReadResult.invalid())
    {
      return topReadResult;
    }
    Result<> bottomReadResult = dataStoreRef.copyIntoBuffer(bottomOffset, nonstd::span<T>(bottomRowBuffer.get(), rowElements));
    if(bottomReadResult.invalid())
    {
      return bottomReadResult;
    }
    Result<> topWriteResult = dataStoreRef.copyFromBuffer(topOffset, nonstd::span<const T>(bottomRowBuffer.get(), rowElements));
    if(topWriteResult.invalid())
    {
      return topWriteResult;
    }
    Result<> bottomWriteResult = dataStoreRef.copyFromBuffer(bottomOffset, nonstd::span<const T>(topRowBuffer.get(), rowElements));
    if(bottomWriteResult.invalid())
    {
      return bottomWriteResult;
    }
  }
  return {};
}

/**
 * @brief Reads, optionally converts/resamples/flips, and appends stack slices for one destination value type.
 * @tparam T Specifies the destination image scalar type.
 * @param dataStructure Provides the destination stack.
 * @param inputValues Specifies files, transforms, crop, and output settings.
 * @param messageHandler Receives per-file and phase messages.
 * @param shouldCancel Stops after the current slice copy when true.
 * @return First validation, delegated-filter, or bulk-I/O error, or accumulated warnings.
 *
 * Each decoded slice is consumed before the next file is opened. Final flips
 * and slice copies use row/slice bulk transfers, so no complete stack copy is
 * created even when the destination store is disk-backed.
 */
template <class T>
Result<> ReadImageStackImpl(DataStructure& dataStructure, const ReadImageStackInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const DataPath& imageGeomPath = inputValues.image_geometry_path;
  const std::string& cellDataName = inputValues.cell_data_name;
  const std::string& imageArrayName = inputValues.image_data_array_name;
  const std::vector<std::string>& files = inputValues.file_list;
  const ImageFlipTransform transformType = inputValues.image_transform;
  const bool convertToGrayscale = inputValues.convert_to_gray_scale;
  const VectorFloat32ParameterType::ValueType& luminosityValues = inputValues.color_weights;
  const ChoicesParameter::ValueType resample = inputValues.resample_images_choice;
  const float32 scalingFactor = inputValues.scaling;
  const VectorUInt64ParameterType::ValueType& exactDims = inputValues.exact_xy_dimensions;
  const bool changeDataType = inputValues.change_data_type;
  const DataType destType = inputValues.image_data_type;
  const CropGeometryParameter::ValueType& croppingOptions = inputValues.cropping_options;
  const bool shouldChangeOrigin = inputValues.change_origin;
  const VectorFloat32ParameterType::ValueType& origin = inputValues.origin;
  const bool shouldChangeSpacing = inputValues.change_spacing;
  const VectorFloat32ParameterType::ValueType& spacing = inputValues.spacing;
  const OriginSpacingProcessing originSpacingProcessing = inputValues.origin_spacing_processing;

  if(files.empty())
  {
    return MakeErrorResult(-64511, "Input file list is empty; nothing to read.");
  }

  DataPath destImageGeomPath = imageGeomPath;

  Result<> outputResult;

  auto zRangeResult = ComputeCroppedZRange(croppingOptions, files.size(), origin, spacing, shouldChangeOrigin, shouldChangeSpacing, originSpacingProcessing);
  if(zRangeResult.invalid())
  {
    return ConvertResult(std::move(zRangeResult));
  }
  const auto& zRange = zRangeResult.value();

  usize slice = 0;
  for(usize i = zRange.z_min; i <= zRange.z_max; i++)
  {
    const std::string& filePath = files[i];
    messageHandler.sendInfoMessage(fmt::format("Importing: {}", filePath));

    DataStructure importedDataStructure;
    {
      ReadImageFilter imageReader;
      // Each file contributes page 0 as one stack slice.
      // This limit prevents unused TIFF pages from increasing I/O and memory use.
      // The delegated reader also reports a multi-page diagnostic warning.
      imageReader.setReadSinglePageOnly(true);

      ReadImageSubFilterConfig subConfig{};
      subConfig.file_path = filePath;
      subConfig.image_geometry_path = imageGeomPath;
      subConfig.cell_data_name = cellDataName;
      subConfig.image_data_array_name = imageArrayName;
      subConfig.change_origin = shouldChangeOrigin;
      subConfig.change_spacing = shouldChangeSpacing;
      subConfig.origin = origin;
      subConfig.spacing = spacing;
      subConfig.origin_spacing_processing = originSpacingProcessing;
      subConfig.change_data_type = changeDataType;
      subConfig.image_data_type = destType;
      subConfig.cropping_options = croppingOptions;

      IFilter::ExecuteResult executeResult = imageReader.execute(importedDataStructure, BuildReadImageFilterArgs(subConfig));
      if(executeResult.result.invalid())
      {
        return executeResult.result;
      }
      // Carry any per-file read warnings (e.g. the multi-page-file diagnostic) up into the stack result.
      for(auto& warning : executeResult.result.warnings())
      {
        outputResult.warnings().push_back(std::move(warning));
      }
    }

    DataPath srcImageDataPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);
    const DataType sliceType = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath).getDataType();
    if(sliceType != GetDataType<T>())
    {
      return MakeErrorResult(-23534, fmt::format("Slice {} ('{}') has data type {}, but the stack requires {}.", i, filePath, DataTypeToString(sliceType), DataTypeToString(GetDataType<T>())));
    }

    if(resample != k_NoResampleModeIndex)
    {
      if(resample == k_ScalingModeIndex)
      {
        // A 100 percent scale preserves the decoded slice and needs no resampling.
        if(scalingFactor != 100.0f)
        {
          ResampleImageGeomInputValues resampleInputValues{};
          resampleInputValues.selected_image_geometry_path = imageGeomPath;
          resampleInputValues.remove_original_image_geom = true;
          resampleInputValues.resampling_mode = 1;
          resampleInputValues.spacing = {1.0F, 1.0F, 1.0F};
          resampleInputValues.scaling = {scalingFactor, scalingFactor, 100.0F};
          Result<> result = ExecuteResample(importedDataStructure, resampleInputValues, messageHandler, shouldCancel);
          if(result.invalid())
          {
            return result;
          }
        }
      }
      else
      {
        ResampleImageGeomInputValues resampleInputValues{};
        resampleInputValues.selected_image_geometry_path = imageGeomPath;
        resampleInputValues.remove_original_image_geom = true;
        resampleInputValues.resampling_mode = 2;
        resampleInputValues.spacing = {1.0F, 1.0F, 1.0F};
        resampleInputValues.exact_dimensions = {exactDims[0], exactDims[1], 1};
        Result<> result = ExecuteResample(importedDataStructure, resampleInputValues, messageHandler, shouldCancel);
        if(result.invalid())
        {
          return result;
        }
      }

      // ReadImageStackFilter renames the resampled geometry in a deferred action.
      // Until that action runs, the output geometry is at the temporary path beside the selected geometry.
      destImageGeomPath = ResampledTemporaryGeometryPath(imageGeomPath);
    }

    bool validInputForGrayScaleConversion = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath).getDataType() == DataType::uint8;
    if(convertToGrayscale && validInputForGrayScaleConversion)
    {
      ConvertColorToGrayScaleInputValues grayscaleInputValues;
      grayscaleInputValues.conversion_algorithm = 0;
      grayscaleInputValues.color_weights = luminosityValues;
      grayscaleInputValues.color_channel = 0;
      grayscaleInputValues.input_data_array_paths = {srcImageDataPath};
      grayscaleInputValues.output_array_prefix = "gray";
      Result<> result = ExecuteGrayscale(importedDataStructure, grayscaleInputValues, messageHandler, shouldCancel);
      if(result.invalid())
      {
        return result;
      }

      DataObject::IdType id = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath).getId();
      importedDataStructure.removeData(id);

      {
        auto& gray = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath.replaceName("gray" + srcImageDataPath.getTargetName()));
        if(!gray.canRename(srcImageDataPath.getTargetName()))
        {
          return MakeErrorResult(-64543, fmt::format("Unable to rename the internal grayscale array to {}", srcImageDataPath.getTargetName()));
        }
        gray.rename(srcImageDataPath.getTargetName());
      }
    }
    else if(convertToGrayscale && !validInputForGrayScaleConversion)
    {
      outputResult.warnings().emplace_back(
          Warning{.code = -74320,
                  .message = fmt::format("The array ({}) resulting from reading the input image file is not a UInt8Array. The input image will not be converted to grayscale.",
                                         srcImageDataPath.getTargetName())});
    }

    auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(destImageGeomPath);
    SizeVec3Type destDims = destImageGeom.getDimensions();
    const usize destTuplesPerSlice = destDims[0] * destDims[1];

    const auto& importedImageGeom = importedDataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    SizeVec3Type importedDims = importedImageGeom.getDimensions();
    if(destDims[0] != importedDims[0] || destDims[1] != importedDims[1])
    {
      return MakeErrorResult(-64510, fmt::format("Slice {} image dimensions are different than expected dimensions.\n  Expected Slice Dims are:  {} x {}\n  Received Slice Dims are: {} x {}\n", slice,
                                                 destDims[0], destDims[1], importedDims[0], importedDims[1]));
    }

    const usize destTupleIndex = (slice * destDims[0] * destDims[1]);

    auto& srcData = importedDataStructure.getDataRefAs<DataArray<T>>(srcImageDataPath);
    AbstractDataStore<T>& srcDataStore = srcData.getDataStoreRef();

    if(transformType == ImageFlipTransform::FlipAboutYAxis)
    {
      Result<> flipResult = FlipAboutYAxis<T>(srcData, destDims);
      if(flipResult.invalid())
      {
        return flipResult;
      }
    }
    else if(transformType == ImageFlipTransform::FlipAboutXAxis)
    {
      Result<> flipResult = FlipAboutXAxis<T>(srcData, destDims);
      if(flipResult.invalid())
      {
        return flipResult;
      }
    }

    // Grayscale preflight creates a prefixed array and defers its final rename.
    // Write the stack to that temporary path during execution.
    DataPath destImageDataPath = convertToGrayscale ? destImageGeomPath.createChildPath(cellDataName).createChildPath("grayscale_" + imageArrayName) :
                                                      destImageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);
    auto& outputData = dataStructure.getDataRefAs<DataArray<T>>(destImageDataPath);
    AbstractDataStore<T>& outputDataStore = outputData.getDataStoreRef();
    Result<> result = outputDataStore.copyFrom(destTupleIndex, srcDataStore, 0, destTuplesPerSlice);
    if(result.invalid())
    {
      return result;
    }

    slice++;

    if(shouldCancel)
    {
      return {};
    }
  }

  return outputResult;
}

/**
 * @struct ReadImageStackDispatchFunctor
 * @brief Dispatches the destination type to the bounded stack reader.
 */
struct ReadImageStackDispatchFunctor
{
  /**
   * @brief Invokes the complete typed stack reader.
   * @tparam T Specifies the destination image scalar type.
   * @param dataStructure Provides the destination stack.
   * @param inputValues Specifies files, transforms, crop, and output settings.
   * @param messageHandler Receives per-file and phase messages.
   * @param shouldCancel Stops after the current slice copy when true.
   * @return First error or accumulated warning result.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const ReadImageStackInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    return ReadImageStackImpl<T>(dataStructure, inputValues, messageHandler, shouldCancel);
  }
};
} // namespace

ReadImageStack::ReadImageStack(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageStackInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> ReadImageStack::operator()()
{
  const std::string& imageArrayName = m_InputValues.image_data_array_name;
  const std::string& cellDataName = m_InputValues.cell_data_name;
  DataPath destImageGeomPath = m_InputValues.image_geometry_path;
  DataPath destImageDataPath = destImageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);

  auto& destArray = m_DataStructure.getDataRefAs<IDataArray>(destImageDataPath);
  DataType destDataType = destArray.getDataType();

  return ExecuteDataFunction(ReadImageStackDispatchFunctor{}, destDataType, m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel);
}
