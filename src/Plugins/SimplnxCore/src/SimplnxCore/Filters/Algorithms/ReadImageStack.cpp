#include "ReadImageStack.hpp"

#include "SimplnxCore/Filters/ConvertColorToGrayScaleFilter.hpp"
#include "SimplnxCore/Filters/ReadImageFilter.hpp"
#include "SimplnxCore/Filters/ResampleImageGeomFilter.hpp"

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
  args.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<fs::path>(config.filePath));
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(config.imageGeometryPath));
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, std::make_any<DataObjectNameParameter::ValueType>(config.cellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, std::make_any<DataObjectNameParameter::ValueType>(config.imageDataArrayName));
  args.insertOrAssign(ReadImageFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(IGeometry::LengthUnit::Micrometer)));
  args.insertOrAssign(ReadImageFilter::k_ChangeDataType_Key, std::make_any<BoolParameter::ValueType>(config.changeDataType));
  args.insertOrAssign(ReadImageFilter::k_ImageDataType_Key, std::make_any<ChoicesParameter::ValueType>(ImageDataTypeToChoice(config.imageDataType)));
  const bool forwardOrigin = config.changeOrigin && config.originSpacingProcessing == OriginSpacingProcessing::Preprocessed;
  const bool forwardSpacing = config.changeSpacing && config.originSpacingProcessing == OriginSpacingProcessing::Preprocessed;
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, std::make_any<BoolParameter::ValueType>(forwardOrigin));
  args.insertOrAssign(ReadImageFilter::k_CenterOrigin_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(ReadImageFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(config.origin));
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, std::make_any<BoolParameter::ValueType>(forwardSpacing));
  args.insertOrAssign(ReadImageFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(config.spacing));
  args.insertOrAssign(ReadImageFilter::k_OriginSpacingProcessing_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(config.originSpacingProcessing)));
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(config.croppingOptions));
  return args;
}

} // namespace nx::core

namespace
{
const ChoicesParameter::ValueType k_NoResampleModeIndex = 0;
const ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;

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
  const DataPath& imageGeomPath = inputValues.imageGeometryPath;
  const std::string& cellDataName = inputValues.cellDataName;
  const std::string& imageArrayName = inputValues.imageDataArrayName;
  const std::vector<std::string>& files = inputValues.fileList;
  const ImageFlipTransform transformType = inputValues.imageTransform;
  const bool convertToGrayscale = inputValues.convertToGrayScale;
  const VectorFloat32Parameter::ValueType& luminosityValues = inputValues.colorWeights;
  const ChoicesParameter::ValueType resample = inputValues.resampleImagesChoice;
  const float32 scalingFactor = inputValues.scaling;
  const VectorUInt64Parameter::ValueType& exactDims = inputValues.exactXYDimensions;
  const bool changeDataType = inputValues.changeDataType;
  const DataType destType = inputValues.imageDataType;
  const CropGeometryParameter::ValueType& croppingOptions = inputValues.croppingOptions;
  const bool shouldChangeOrigin = inputValues.changeOrigin;
  const VectorFloat32Parameter::ValueType& origin = inputValues.origin;
  const bool shouldChangeSpacing = inputValues.changeSpacing;
  const VectorFloat32Parameter::ValueType& spacing = inputValues.spacing;
  const OriginSpacingProcessing originSpacingProcessing = inputValues.originSpacingProcessing;

  if(files.empty())
  {
    return MakeErrorResult(-64511, "Input file list is empty; nothing to read.");
  }

  DataPath destImageGeomPath = imageGeomPath;
  auto& initialImageGeom = dataStructure.getDataRefAs<ImageGeom>(destImageGeomPath);

  Result<> outputResult;

  usize startSlice = 0;
  usize endSlice = files.size() - 1;
  if(croppingOptions.cropZ && croppingOptions.type == CropGeometryParameter::ValueType::TypeEnum::VoxelSubvolume)
  {
    startSlice = static_cast<usize>(croppingOptions.zBoundVoxels[0]);
    endSlice = static_cast<usize>(croppingOptions.zBoundVoxels[1]);
  }
  else if(croppingOptions.cropZ && croppingOptions.type == CropGeometryParameter::ValueType::TypeEnum::PhysicalSubvolume)
  {
    SizeVec3 destDims = initialImageGeom.getDimensions();
    FloatVec3 destOrigin = initialImageGeom.getOrigin();

    // Reject physical crop bounds outside the geometry. A full-range fallback
    // would silently create the wrong output volume.
    std::optional<usize> result = initialImageGeom.getIndex(destOrigin[0], destOrigin[1], croppingOptions.zBoundPhysical[0]);
    if(!result.has_value())
    {
      return MakeErrorResult(-64512, fmt::format("Physical Z crop minimum {} is outside the destination image-geometry Z extent", croppingOptions.zBoundPhysical[0]));
    }
    startSlice = result.value() / (destDims[0] * destDims[1]);

    result = initialImageGeom.getIndex(destOrigin[0], destOrigin[1], croppingOptions.zBoundPhysical[1]);
    if(!result.has_value())
    {
      return MakeErrorResult(-64513, fmt::format("Physical Z crop maximum {} is outside the destination image-geometry Z extent", croppingOptions.zBoundPhysical[1]));
    }
    endSlice = result.value() / (destDims[0] * destDims[1]);
  }

  if(startSlice > endSlice || endSlice >= files.size())
  {
    return MakeErrorResult(-64514, fmt::format("Computed slice range [{}, {}] is invalid for {} input files", startSlice, endSlice, files.size()));
  }

  usize slice = 0;
  for(usize i = startSlice; i <= endSlice; i++)
  {
    const std::string& filePath = files[i];
    messageHandler(IFilter::Message::Type::Info, fmt::format("Importing: {}", filePath));

    DataStructure importedDataStructure;
    {
      ReadImageFilter imageReader;

      ReadImageSubFilterConfig subConfig{};
      subConfig.filePath = filePath;
      subConfig.imageGeometryPath = imageGeomPath;
      subConfig.cellDataName = cellDataName;
      subConfig.imageDataArrayName = imageArrayName;
      subConfig.changeOrigin = shouldChangeOrigin;
      subConfig.changeSpacing = shouldChangeSpacing;
      subConfig.origin = origin;
      subConfig.spacing = spacing;
      subConfig.originSpacingProcessing = originSpacingProcessing;
      subConfig.changeDataType = changeDataType;
      subConfig.imageDataType = destType;
      subConfig.croppingOptions = croppingOptions;

      IFilter::ExecuteResult executeResult = imageReader.execute(importedDataStructure, BuildReadImageFilterArgs(subConfig));
      if(executeResult.result.invalid())
      {
        return executeResult.result;
      }
    }

    if(resample != k_NoResampleModeIndex)
    {
      ResampleImageGeomFilter resampleImageGeomFilter;
      if(resample == k_ScalingModeIndex)
      {
        // A 100 percent scale preserves the decoded slice and needs no resampling.
        if(scalingFactor != 100.0f)
        {
          Arguments resampleImageGeomArgs;
          resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
          resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<BoolParameter::ValueType>(true));

          resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(1));
          resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{scalingFactor, scalingFactor, 100.0f}));

          Result<> result = resampleImageGeomFilter.execute(importedDataStructure, resampleImageGeomArgs).result;
          if(result.invalid())
          {
            return result;
          }
        }
      }
      else
      {
        Arguments resampleImageGeomArgs;
        resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
        resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<BoolParameter::ValueType>(true));

        resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(2));
        resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{exactDims[0], exactDims[1], 1}));

        Result<> result = resampleImageGeomFilter.execute(importedDataStructure, resampleImageGeomArgs).result;
        if(result.invalid())
        {
          return result;
        }
      }

      // The resample filter defers its final rename. Use the temporary path until
      // deferred actions complete.
      destImageGeomPath = DataPath({imageGeomPath.getTargetName() + "_resampled"});
    }

    DataPath srcImageDataPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);

    bool validInputForGrayScaleConversion = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath).getDataType() == DataType::uint8;
    if(convertToGrayscale && validInputForGrayScaleConversion)
    {
      ConvertColorToGrayScaleFilter grayScaleFilter;
      Arguments colorToGrayscaleArgs;
      colorToGrayscaleArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
      colorToGrayscaleArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(luminosityValues));
      colorToGrayscaleArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_InputDataArrayPath_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{srcImageDataPath}));
      colorToGrayscaleArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key, std::make_any<std::string>("gray"));

      Result<> result = grayScaleFilter.execute(importedDataStructure, colorToGrayscaleArgs).result;
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
          Warning{-74320, fmt::format("The array ({}) resulting from reading the input image file is not a UInt8Array. The input image will not be converted to grayscale.",
                                      srcImageDataPath.getTargetName())});
    }

    auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(destImageGeomPath);
    SizeVec3 destDims = destImageGeom.getDimensions();
    const usize destTuplesPerSlice = destDims[0] * destDims[1];

    const auto& importedImageGeom = importedDataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    SizeVec3 importedDims = importedImageGeom.getDimensions();
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

ReadImageStack::~ReadImageStack() noexcept = default;

Result<> ReadImageStack::operator()()
{
  const std::string& imageArrayName = m_InputValues.imageDataArrayName;
  const std::string& cellDataName = m_InputValues.cellDataName;
  DataPath destImageGeomPath = m_InputValues.imageGeometryPath;
  DataPath destImageDataPath = destImageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);

  auto& destArray = m_DataStructure.getDataRefAs<IDataArray>(destImageDataPath);
  DataType destDataType = destArray.getDataType();

  return ExecuteDataFunction(ReadImageStackDispatchFunctor{}, destDataType, m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel);
}
