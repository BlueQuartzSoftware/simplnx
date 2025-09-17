#include "ITKImportImageStackFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Filters/ITKImageReaderFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <itkImageFileReader.h>
#include <itkImageIOBase.h>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const ChoicesParameter::Choices k_SliceOperationChoices = {"None", "Flip about X axis", "Flip about Y axis"};
const ChoicesParameter::ValueType k_NoImageTransform = 0;
const ChoicesParameter::ValueType k_FlipAboutXAxis = 1;
const ChoicesParameter::ValueType k_FlipAboutYAxis = 2;

constexpr nx::core::StringLiteral k_NoResamplingMode = "Do Not Resample (0)";
constexpr nx::core::StringLiteral k_ScalingMode = "Scaling (1)";
constexpr nx::core::StringLiteral k_ExactDimensions = "Exact X/Y Dimensions (2)";
const nx::core::ChoicesParameter::Choices k_ResamplingChoices = {k_NoResamplingMode, k_ScalingMode, k_ExactDimensions};
const nx::core::ChoicesParameter::ValueType k_NoResampleModeIndex = 0;
const nx::core::ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const nx::core::ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;

const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_RotateSampleRefFrameFilterId = *Uuid::FromString("d2451dc1-a5a1-4ac2-a64d-7991669dcffc");
const FilterHandle k_RotateSampleRefFrameFilterHandle(k_RotateSampleRefFrameFilterId, k_SimplnxCorePluginId);
const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_SimplnxCorePluginId);
const Uuid k_ResampleImageGeomFilterId = *Uuid::FromString("9783ea2c-4cf7-46de-ab21-b40d91a48c5b");
const FilterHandle k_ResampleImageGeomFilterHandle(k_ResampleImageGeomFilterId, k_SimplnxCorePluginId);
const Uuid k_CropImageGeomFilterId = *Uuid::FromString("e6476737-4aa7-48ba-a702-3dfab82c96e2");
const FilterHandle k_CropImageGeomFilterHandle(k_CropImageGeomFilterId, k_SimplnxCorePluginId);

// Parameter Keys
constexpr StringLiteral k_RotationRepresentation_Key = "rotation_representation";
constexpr StringLiteral k_RotationAxisAngle_Key = "rotation_axis";
constexpr StringLiteral k_RotationMatrix_Key = "rotation_matrix";
constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
constexpr StringLiteral k_CreatedImageGeometry_Key = "output_image_geometry_path";
constexpr StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";
constexpr StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";
// constexpr StringLiteral k_RotatedGeometryName = ".RotatedGeometry";

enum class RotationRepresentation : uint64_t
{
  AxisAngle = 0,
  RotationMatrix = 1
};

// Make sure we can instantiate the RotateSampleRefFrame Filter
std::unique_ptr<IFilter> CreateRotateSampleRefFrameFilter()
{
  auto* filterListPtr = Application::Instance()->getFilterList();
  auto filter = filterListPtr->createFilter(k_RotateSampleRefFrameFilterHandle);
  return filter;
}

template <class T>
void FlipAboutYAxis(DataArray<T>& dataArray, Vec3<usize>& dims)
{
  auto& tempDataStore = dataArray.getDataStoreRef();

  usize numComp = tempDataStore.getNumberOfComponents();
  std::vector<T> currentRowBuffer(dims[0] * dataArray.getNumberOfComponents());

  // We could _in theory_ parallelize over the rows, not sure how the out-of-core
  // would handle that though.
  for(usize row = 0; row < dims[1]; row++)
  {
    // Copy the current row into a temp buffer
    auto startIter = tempDataStore.begin() + (dims[0] * numComp * row);
    auto endIter = startIter + dims[0] * numComp;
    std::copy(startIter, endIter, currentRowBuffer.begin());

    // Starting at the last tuple in the buffer
    usize bufferIndex = (dims[0] - 1) * numComp;
    usize dataStoreIndex = row * dims[0] * numComp;

    for(usize tupleIdx = 0; tupleIdx < dims[0]; tupleIdx++)
    {
      for(usize cIdx = 0; cIdx < numComp; cIdx++)
      {
        tempDataStore.setValue(dataStoreIndex, currentRowBuffer[bufferIndex + cIdx]);
        dataStoreIndex++;
      }
      bufferIndex = bufferIndex - numComp;
    }
  }
}

template <class T>
void FlipAboutXAxis(DataArray<T>& dataArray, Vec3<usize>& dims)
{
  auto& tempDataStore = dataArray.getDataStoreRef();
  usize numComp = tempDataStore.getNumberOfComponents();
  size_t rowLCV = (dims[1] % 2 == 1) ? ((dims[1] - 1) / 2) : dims[1] / 2;
  usize bottomRow = dims[1] - 1;

  for(usize row = 0; row < rowLCV; row++)
  {
    // Copy the "top" row into a temp buffer
    auto topStartIter = 0 + (dims[0] * numComp * row);
    auto topEndIter = topStartIter + dims[0] * numComp;
    auto bottomStartIter = 0 + (dims[0] * numComp * bottomRow);

    // Copy from bottom to top and then temp to bottom
    for(usize eleIndex = topStartIter; eleIndex < topEndIter; eleIndex++)
    {
      T value = tempDataStore.getValue(eleIndex);
      tempDataStore[eleIndex] = tempDataStore[bottomStartIter];
      tempDataStore[bottomStartIter] = value;
      bottomStartIter++;
    }
    bottomRow--;
  }
}

} // namespace

namespace cxITKImportImageStackFilter
{
template <class T>
Result<> ReadImageStack(DataStructure& dataStructure, const DataPath& imageGeomPath, const std::string& cellDataName, const std::string& imageArrayName, const std::vector<std::string>& files,
                        ChoicesParameter::ValueType transformType, bool convertToGrayscale, const VectorFloat32Parameter::ValueType& luminosityValues, ChoicesParameter::ValueType resample,
                        float32 scalingFactor, const VectorUInt64Parameter::ValueType& exactDims, bool changeDataType, ChoicesParameter::ValueType destType,
                        CropGeometryParameter::ValueType& croppingOptions, const VectorFloat64Parameter::ValueType& origin, const VectorFloat64Parameter::ValueType& spacing,
                        const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  DataPath destImageGeomPath = imageGeomPath;
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(destImageGeomPath);

  auto* filterListPtr = Application::Instance()->getFilterList();

  if((convertToGrayscale || resample != k_NoResampleModeIndex) && !filterListPtr->containsPlugin(k_SimplnxCorePluginId))
  {
    return MakeErrorResult(-18542, "SimplnxCore was not instantiated in this instance, so color to grayscale is not a valid option.");
  }
  auto grayScaleFilter = filterListPtr->createFilter(k_ColorToGrayScaleFilterHandle);
  Result<> outputResult = {};

  usize startSlice = 0;
  usize endSlice = files.size() - 1;
  if(croppingOptions.cropZ && croppingOptions.type == CropGeometryParameter::ValueType::TypeEnum::VoxelSubvolume)
  {
    startSlice = croppingOptions.zBoundVoxels[0];
    endSlice = croppingOptions.zBoundVoxels[1];
  }
  else if(croppingOptions.cropZ && croppingOptions.type == CropGeometryParameter::ValueType::TypeEnum::PhysicalSubvolume)
  {
    auto destDims = imageGeom.getDimensions();
    auto result = imageGeom.getIndex(croppingOptions.xBoundPhysical[0], croppingOptions.yBoundPhysical[0], croppingOptions.zBoundPhysical[0]);
    if(result.has_value())
    {
      startSlice = result.value() / (destDims[0] * destDims[1]);
    }
    result = imageGeom.getIndex(croppingOptions.xBoundPhysical[1], croppingOptions.yBoundPhysical[1], croppingOptions.zBoundPhysical[1]);
    if(result.has_value())
    {
      endSlice = result.value() / (destDims[0] * destDims[1]);
    }
  }

  // Loop over all the files importing them one by one and copying the data into the data array
  usize slice = 0;
  for(usize i = startSlice; i <= endSlice; i++)
  {
    const auto& filePath = files[i];
    messageHandler(IFilter::Message::Type::Info, fmt::format("Importing: {}", filePath));

    DataStructure importedDataStructure;
    {
      // Create a sub-filter to read each image, although for preflight we are going to read the first image in the
      // list and hope the rest are correct.
      const ITKImageReaderFilter imageReader;

      Arguments args;
      args.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
      args.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, std::make_any<std::string>(cellDataName));
      args.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, std::make_any<std::string>(imageArrayName));
      args.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, std::make_any<fs::path>(filePath));
      args.insertOrAssign(ITKImageReaderFilter::k_ChangeDataType_Key, std::make_any<bool>(changeDataType));
      args.insertOrAssign(ITKImageReaderFilter::k_ImageDataType_Key, std::make_any<ChoicesParameter::ValueType>(destType));
      args.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, std::make_any<BoolParameter::ValueType>(true));
      args.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, std::make_any<VectorFloat64Parameter::ValueType>(origin));
      args.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, std::make_any<BoolParameter::ValueType>(true));
      args.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, std::make_any<VectorFloat64Parameter::ValueType>(spacing));

      auto executeResult = imageReader.execute(importedDataStructure, args);
      if(executeResult.result.invalid())
      {
        return executeResult.result;
      }
    }

    // ======================= Crop Image Geometry Section ===================
    if(croppingOptions.type != CropGeometryParameter::ValueType::TypeEnum::NoCropping)
    {
      auto cropImageGeomFilter = filterListPtr->createFilter(k_CropImageGeomFilterHandle);
      Arguments cropImageGeomArgs;
      cropImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(imageGeomPath));
      cropImageGeomArgs.insertOrAssign("use_physical_bounds", std::make_any<bool>(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume));
      cropImageGeomArgs.insertOrAssign("crop_x_dim", std::make_any<bool>(croppingOptions.cropX));
      cropImageGeomArgs.insertOrAssign("crop_y_dim", std::make_any<bool>(croppingOptions.cropY));
      cropImageGeomArgs.insertOrAssign("crop_z_dim", std::make_any<bool>(false)); // Do this because we're only cropping one image at a time
      if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
      {
        cropImageGeomArgs.insertOrAssign("min_voxel",
                                         std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(croppingOptions.xBoundVoxels[0]), static_cast<uint64>(croppingOptions.yBoundVoxels[0]),
                                                                                          static_cast<uint64>(croppingOptions.zBoundVoxels[0])}));
        cropImageGeomArgs.insertOrAssign("max_voxel",
                                         std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(croppingOptions.xBoundVoxels[1]), static_cast<uint64>(croppingOptions.yBoundVoxels[1]),
                                                                                          static_cast<uint64>(croppingOptions.zBoundVoxels[1])}));
      }
      else
      {
        cropImageGeomArgs.insertOrAssign(
            "min_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(croppingOptions.xBoundPhysical[0]), static_cast<float64>(croppingOptions.yBoundPhysical[0]),
                                                                           static_cast<float64>(croppingOptions.zBoundPhysical[0])}));
        cropImageGeomArgs.insertOrAssign(
            "max_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(croppingOptions.xBoundPhysical[1]), static_cast<float64>(croppingOptions.yBoundPhysical[1]),
                                                                           static_cast<float64>(croppingOptions.zBoundPhysical[1])}));
      }
      cropImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(true));

      // Run crop image geometry filter and process results and messages
      auto result = cropImageGeomFilter->execute(importedDataStructure, cropImageGeomArgs).result;
      if(result.invalid())
      {
        return result;
      }

      destImageGeomPath = DataPath({imageGeomPath.getTargetName() + "_cropped"});
    }

    // ======================= Resample Image Geometry Section ===================
    if(resample != k_NoResampleModeIndex)
    {
      auto resampleImageGeomFilter = filterListPtr->createFilter(k_ResampleImageGeomFilterHandle);
      if(resample == k_ScalingModeIndex)
      {
        if(scalingFactor == 100.0f)
        {
          break;
        }

        Arguments resampleImageGeomArgs;
        resampleImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(imageGeomPath));
        resampleImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(true));

        resampleImageGeomArgs.insertOrAssign("resampling_mode_index", std::make_any<ChoicesParameter::ValueType>(1));
        resampleImageGeomArgs.insertOrAssign("scaling", std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{scalingFactor, scalingFactor, 100.0f}));

        // Run resample image geometry filter and process results and messages
        auto result = resampleImageGeomFilter->execute(importedDataStructure, resampleImageGeomArgs).result;
        if(result.invalid())
        {
          return result;
        }
      }
      else
      {
        Arguments resampleImageGeomArgs;
        resampleImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(imageGeomPath));
        resampleImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(true));

        resampleImageGeomArgs.insertOrAssign("resampling_mode_index", std::make_any<ChoicesParameter::ValueType>(2));
        resampleImageGeomArgs.insertOrAssign("exact_dimensions", std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{exactDims[0], exactDims[1], 1}));

        // Run resample image geometry filter and process results and messages
        auto result = resampleImageGeomFilter->execute(importedDataStructure, resampleImageGeomArgs).result;
        if(result.invalid())
        {
          return result;
        }
      }

      destImageGeomPath = DataPath({imageGeomPath.getTargetName() + "_resampled"});
    }

    // ======================= Convert to GrayScale Section ===================
    DataPath srcImageDataPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);

    bool validInputForGrayScaleConversion = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath).getDataType() == DataType::uint8;
    if(convertToGrayscale && validInputForGrayScaleConversion && nullptr != grayScaleFilter.get())
    {
      // This same filter was used to preflight so as long as nothing changes on disk this really should work....
      Arguments colorToGrayscaleArgs;
      colorToGrayscaleArgs.insertOrAssign("conversion_algorithm_index", std::make_any<ChoicesParameter::ValueType>(0));
      colorToGrayscaleArgs.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(luminosityValues));
      colorToGrayscaleArgs.insertOrAssign("input_data_array_paths", std::make_any<std::vector<DataPath>>(std::vector<DataPath>{srcImageDataPath}));
      colorToGrayscaleArgs.insertOrAssign("output_array_prefix", std::make_any<std::string>("gray"));

      // Run grayscale filter and process results and messages
      auto result = grayScaleFilter->execute(importedDataStructure, colorToGrayscaleArgs).result;
      if(result.invalid())
      {
        return result;
      }

      // deletion of non-grayscale array
      DataObject::IdType id;
      { // scoped for safety since this reference will be nonexistent in a moment
        auto& oldArray = importedDataStructure.getDataRefAs<IDataArray>(srcImageDataPath);
        id = oldArray.getId();
      }
      importedDataStructure.removeData(id);

      // rename grayscale array to reflect original
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

    // Check the ImageGeometry of the imported Image matches the destination
    const auto& importedImageGeom = importedDataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    SizeVec3 importedDims = importedImageGeom.getDimensions();
    if(destDims[0] != importedDims[0] || destDims[1] != importedDims[1])
    {
      return MakeErrorResult(-64510, fmt::format("Slice {} image dimensions are different than expected dimensions.\n  Expected Slice Dims are:  {} x {}\n  Received Slice Dims are: {} x {}\n", slice,
                                                 destDims[0], destDims[1], importedDims[0], importedDims[1]));
    }

    // Compute the Tuple Index we are at:
    const usize destTupleIndex = (slice * destDims[0] * destDims[1]);

    // get the current Slice data...
    auto& srcData = importedDataStructure.getDataRefAs<DataArray<T>>(srcImageDataPath);
    auto& srcDataStore = srcData.getDataStoreRef();

    if(transformType == k_FlipAboutYAxis)
    {
      FlipAboutYAxis<T>(srcData, destDims);
    }
    else if(transformType == k_FlipAboutXAxis)
    {
      FlipAboutXAxis<T>(srcData, destDims);
    }

    // Copy that into the output array...
    DataPath destImageDataPath = destImageGeomPath.createChildPath(cellDataName).createChildPath(imageArrayName);
    auto& outputData = dataStructure.getDataRefAs<DataArray<T>>(destImageDataPath);
    auto& outputDataStore = outputData.getDataStoreRef();
    auto result = outputDataStore.copyFrom(destTupleIndex, srcDataStore, 0, destTuplesPerSlice);
    if(result.invalid())
    {
      return result;
    }

    slice++;

    // Check to see if the filter got canceled.
    if(shouldCancel)
    {
      return outputResult;
    }
  }

  return outputResult;
}
} // namespace cxITKImportImageStackFilter

namespace nx::core
{

//------------------------------------------------------------------------------
std::string ITKImportImageStackFilter::name() const
{
  return FilterTraits<ITKImportImageStackFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKImportImageStackFilter::className() const
{
  return FilterTraits<ITKImportImageStackFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImportImageStackFilter::uuid() const
{
  return FilterTraits<ITKImportImageStackFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImportImageStackFilter::humanName() const
{
  return "Read Images [3D Stack] (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportImageStackFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportImageStackFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(
      std::make_unique<GeneratedFileListParameter>(k_InputFileListInfo_Key, "Input File List", "The list of 2D image files to be read in to a 3D volume", GeneratedFileListParameter::ValueType{}));

  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin", "The origin of the 3D volume", std::vector<float64>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "y", "Z"}));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing", "The spacing of the 3D volume", std::vector<float64>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  params.insert(std::make_unique<CropGeometryParameter>(
      k_CroppingOptions_Key, "Cropping Options",
      "The cropping options used to crop images.  These include picking the cropping type, the cropping dimensions, and the cropping ranges for each chosen dimension.",
      CropGeometryParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Resampling Options"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ResampleImagesChoice_Key, "Resample Images",
                                                                    "Mode can be [0] Do Not Rescale, [1] Scaling as Percent, [2] Exact X/Y Dimensions For Resampling Along Z Axis",
                                                                    ::k_NoResampleModeIndex, ::k_ResamplingChoices));
  params.insert(std::make_unique<Float32Parameter>(
      k_Scaling_Key, "Scaling",
      "The scaling of the 3D volume, in percentages. Percentage must be greater than or equal to 1.0f. Larger percentages will cause more voxels, smaller percentages "
      "will cause less voxels. For example, 10.0 is one-tenth the original number of pixels.  200.0 is double the number of pixels.",
      100.0f));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_ExactXYDimensions_Key, "Exact 2D Dimensions",
                                                        "The supplied dimensions will be used to determine the resampled output geometry size. See associated Filter documentation for further detail.",
                                                        std::vector<uint64>{100, 100}, std::vector<std::string>({"X", "Y"})));

  params.insertSeparator(Parameters::Separator{"Other Slice Options"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "The filter will show an error if the images are already in grayscale format", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "RGB weights for the grayscale conversion using the luminosity algorithm.",
                                                         std::vector<float32>{0.2125f, 0.7154f, 0.0721f}, std::vector<std::string>({"Red", "Green", "Blue"})));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ImageTransformChoice_Key, "Flip Slice", "Operation that is performed on each slice. 0=None, 1=Flip about X, 2=Flip about Y", 0, k_SliceOperationChoices));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeDataType_Key, "Set Image Data Type", "Set the final created image data type.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ImageDataType_Key, "Output Data Type", "Numeric Type of data to create", 0ULL,
                                                   ChoicesParameter::Choices{"uint8", "uint16", "uint32"})); // Sequence Dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Cell Data Name", "The name of the created cell attribute matrix", ImageGeom::k_CellAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageDataArrayPath_Key, "Created Image Data", "The path to the created image data array", "ImageData"));

  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);
  params.linkParameters(k_ResampleImagesChoice_Key, k_Scaling_Key, ::k_ScalingModeIndex);
  params.linkParameters(k_ResampleImagesChoice_Key, k_ExactXYDimensions_Key, ::k_ExactDimensionsModeIndex);
  params.linkParameters(k_ChangeDataType_Key, k_ImageDataType_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKImportImageStackFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Change 1:
  // Replaced - k_ScaleImages_Key = "scale_images" -> k_ResampleImagesChoice_Key = "resample_images_index";
  // Solution - `k_ResampleImagesChoice_Key Value` = static_cast<ChoicesParameter::ValueType>(`k_ScaleImages_Key Value`);
  //
  // Change 2:
  // Modified Existing - Scaling value to be in feature parity with ResampleImageGeomFilter's Scaling option (k_Scaling_Key = "scaling")
  // Solution - `New k_Scaling_Key Value` = `Old k_Scaling_Key Value` * 100.0f;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImportImageStackFilter::clone() const
{
  return std::make_unique<ITKImportImageStackFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImportImageStackFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  auto origin = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Spacing_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageDataArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageTransformValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageTransformChoice_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pResampleImagesChoiceValue = filterArgs.value<ChoicesParameter::ValueType>(k_ResampleImagesChoice_Key);
  auto pScalingValue = filterArgs.value<Float32Parameter::ValueType>(k_Scaling_Key);
  auto pExactXYDimsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_ExactXYDimensions_Key);

  auto pChangeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto numericType = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);
  auto croppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(imageTransformValue != k_NoImageTransform)
  {
    const auto rotateSampleRefFrameFilter = CreateRotateSampleRefFrameFilter();
    if(nullptr == rotateSampleRefFrameFilter)
    {
      return MakePreflightErrorResult(-23500, "ITKImageImageStack requires the use of the RotateSampleRefFrame filter to perform any image manipulation.");
    }
  }

  std::vector<std::string> files = inputFileListInfo.generate();

  if(files.empty())
  {
    return {MakeErrorResult<OutputActions>(-1, "GeneratedFileList must not be empty")};
  }

  DataStructure tmpDs;

  // Create a sub-filter to read each image, although for preflight we are going to read the first image in the
  // list and hope the rest are correct.
  Arguments imageReaderArgs;
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_CellDataName_Key, std::make_any<DataObjectNameParameter::ValueType>(cellDataName));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_ImageDataArrayPath_Key, std::make_any<DataObjectNameParameter::ValueType>(pImageDataArrayNameValue));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_FileName_Key, std::make_any<fs::path>(files.at(0)));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_ChangeDataType_Key, std::make_any<bool>(pChangeDataType));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_ImageDataType_Key, std::make_any<ChoicesParameter::ValueType>(numericType));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(croppingOptions));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_ChangeOrigin_Key, std::make_any<BoolParameter::ValueType>(true));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_Origin_Key, std::make_any<VectorFloat64Parameter::ValueType>(origin));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_ChangeSpacing_Key, std::make_any<BoolParameter::ValueType>(true));
  imageReaderArgs.insertOrAssign(ITKImageReaderFilter::k_Spacing_Key, std::make_any<VectorFloat64Parameter::ValueType>(spacing));

  const ITKImageReaderFilter imageReader;
  PreflightResult imageReaderResult = imageReader.preflight(tmpDs, imageReaderArgs, messageHandler, shouldCancel);
  if(imageReaderResult.outputActions.invalid())
  {
    return imageReaderResult;
  }

  // The first output actions should be the geometry creation
  // A better solution might be to extract the preflight code into a common function for both filters
  const IDataAction* action0Ptr = imageReaderResult.outputActions.value().actions.at(0).get();
  const auto* createImageGeomActionPtr = dynamic_cast<const CreateImageGeometryAction*>(action0Ptr);
  if(createImageGeomActionPtr != nullptr)
  {
    auto outputDims = createImageGeomActionPtr->dims();
    outputDims.back() = files.size();

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(createImageGeomActionPtr->path(), outputDims, createImageGeomActionPtr->origin(),
                                                                                         createImageGeomActionPtr->spacing(), createImageGeomActionPtr->cellAttributeMatrixName(),
                                                                                         createImageGeomActionPtr->units()));
    // The second action should be the array creation
    const IDataAction* action1Ptr = imageReaderResult.outputActions.value().actions.at(1).get();
    const auto* createArrayActionPtr = dynamic_cast<const CreateArrayAction*>(action1Ptr);
    if(createArrayActionPtr != nullptr)
    {
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(createArrayActionPtr->type(), std::vector<usize>(outputDims.rbegin(), outputDims.rend()),
                                                                                   createArrayActionPtr->componentDims(), createArrayActionPtr->path(), createArrayActionPtr->dataFormat(),
                                                                                   createArrayActionPtr->fillValue()));
    }

    Result<> actionsResult = resultOutputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }
  }

  auto currentImageGeomPath = imageGeomPath;
  std::vector<DataPath> pathsToDelete;

  auto* filterListPtr = Application::Instance()->getFilterList();
  if(croppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping)
  {
    if(!filterListPtr->containsPlugin(k_SimplnxCorePluginId))
    {
      auto errorResult = MakePreflightErrorResult(-18542, "The plugin SimplnxCore was not instantiated in this instance, so image cropping is not available.");
      return errorResult;
    }

    auto cropImageGeomFilter = filterListPtr->createFilter(k_CropImageGeomFilterHandle);
    if(nullptr == cropImageGeomFilter.get())
    {
      auto errorResult = MakePreflightErrorResult(-18543, "Unable to create an instance of the crop image geometry filter, so image cropping is not available.");
      return errorResult;
    }

    Arguments cropImageGeomArgs;
    cropImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(currentImageGeomPath));
    cropImageGeomArgs.insertOrAssign("use_physical_bounds", std::make_any<bool>(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume));
    cropImageGeomArgs.insertOrAssign("crop_x_dim", std::make_any<bool>(croppingOptions.cropX));
    cropImageGeomArgs.insertOrAssign("crop_y_dim", std::make_any<bool>(croppingOptions.cropY));
    cropImageGeomArgs.insertOrAssign("crop_z_dim", std::make_any<bool>(croppingOptions.cropZ));
    if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
    {
      cropImageGeomArgs.insertOrAssign("min_voxel",
                                       std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(croppingOptions.xBoundVoxels[0]), static_cast<uint64>(croppingOptions.yBoundVoxels[0]),
                                                                                        static_cast<uint64>(croppingOptions.zBoundVoxels[0])}));
      cropImageGeomArgs.insertOrAssign("max_voxel",
                                       std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(croppingOptions.xBoundVoxels[1]), static_cast<uint64>(croppingOptions.yBoundVoxels[1]),
                                                                                        static_cast<uint64>(croppingOptions.zBoundVoxels[1])}));
    }
    else
    {
      cropImageGeomArgs.insertOrAssign(
          "min_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(croppingOptions.xBoundPhysical[0]), static_cast<float64>(croppingOptions.yBoundPhysical[0]),
                                                                         static_cast<float64>(croppingOptions.zBoundPhysical[0])}));
      cropImageGeomArgs.insertOrAssign(
          "max_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(croppingOptions.xBoundPhysical[1]), static_cast<float64>(croppingOptions.yBoundPhysical[1]),
                                                                         static_cast<float64>(croppingOptions.zBoundPhysical[1])}));
    }
    cropImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(false));
    cropImageGeomArgs.insertOrAssign("output_image_geometry_path", std::make_any<DataPath>(DataPath({imageGeomPath.getTargetName() + "_cropped"})));

    PreflightResult cropImageResult = cropImageGeomFilter->preflight(tmpDs, cropImageGeomArgs, messageHandler, shouldCancel);
    if(cropImageResult.outputActions.invalid())
    {
      return cropImageResult;
    }
    Result<> actionsResult = cropImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    resultOutputActions = MergeOutputActionResults(resultOutputActions, cropImageResult.outputActions);

    pathsToDelete.push_back(currentImageGeomPath);
    currentImageGeomPath = DataPath({imageGeomPath.getTargetName() + "_cropped"});
  }

  if(pResampleImagesChoiceValue != k_NoResampleModeIndex)
  {
    if(!filterListPtr->containsPlugin(k_SimplnxCorePluginId))
    {
      auto errorResult = MakePreflightErrorResult(-18544, "The plugin SimplnxCore was not instantiated in this instance, so image resampling is not available.");
      return errorResult;
    }

    auto resampleImageGeomFilter = filterListPtr->createFilter(k_ResampleImageGeomFilterHandle);
    if(nullptr == resampleImageGeomFilter.get())
    {
      auto errorResult = MakePreflightErrorResult(-18545, "Unable to create an instance of the resample image geometry filter, so image resampling is not available.");
      return errorResult;
    }

    if(pResampleImagesChoiceValue == k_ScalingModeIndex && pScalingValue < 1.0f)
    {
      // seemingly arbitrary numeric limit, only included for compatibility with ResampleImageGeomFilter
      auto errorResult = MakePreflightErrorResult(-23508, fmt::format("Scaling value must be greater than or equal to 1.0f. Received: {}", pScalingValue));
      return errorResult;
    }

    Arguments resampleImageGeomArgs;
    resampleImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(currentImageGeomPath));
    resampleImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(false));
    resampleImageGeomArgs.insertOrAssign("new_data_container_path", std::make_any<DataPath>(DataPath({imageGeomPath.getTargetName() + "_resampled"})));
    resampleImageGeomArgs.insertOrAssign("resampling_mode_index", std::make_any<ChoicesParameter::ValueType>(pResampleImagesChoiceValue));
    resampleImageGeomArgs.insertOrAssign("scaling", std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{pScalingValue, pScalingValue, 100.0f}));
    resampleImageGeomArgs.insertOrAssign("exact_dimensions", std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{pExactXYDimsValue[0], pExactXYDimsValue[1], 1}));

    // Run resample image geometry filter and process results and messages
    auto resampleImageResult = resampleImageGeomFilter->preflight(tmpDs, resampleImageGeomArgs, messageHandler, shouldCancel);
    if(resampleImageResult.outputActions.invalid())
    {
      return resampleImageResult;
    }

    Result<> actionsResult = resampleImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    resultOutputActions = MergeOutputActionResults(resultOutputActions, resampleImageResult.outputActions);

    pathsToDelete.push_back(currentImageGeomPath);
    currentImageGeomPath = DataPath({imageGeomPath.getTargetName() + "_resampled"});
  }

  const DataPath imageDataPath = currentImageGeomPath.createChildPath(cellDataName).createChildPath(pImageDataArrayNameValue);
  auto& imageData = tmpDs.getDataRefAs<IDataArray>(imageDataPath);
  if(pConvertToGrayScaleValue)
  {
    if(imageData.getDataType() != DataType::uint8)
    {
      return MakePreflightErrorResult(-23504, fmt::format("The input DataType is {} which cannot be converted to grayscale. Please turn off the 'Convert To Grayscale' option.",
                                                          nx::core::DataTypeToString(imageData.getDataType())));
    }

    if(!filterListPtr->containsPlugin(k_SimplnxCorePluginId))
    {
      auto errorResult = MakePreflightErrorResult(-23501, "Color to GrayScale conversion is disabled because the 'SimplnxCore' plugin was not loaded.");
      return errorResult;
    }
    auto grayScaleFilter = filterListPtr->createFilter(k_ColorToGrayScaleFilterHandle);
    if(nullptr == grayScaleFilter.get())
    {
      auto errorResult = MakePreflightErrorResult(-23502, "Color to GrayScale conversion is disabled because the 'Color to GrayScale' filter is missing from the SimplnxCore plugin.");
      return errorResult;
    }

    Arguments grayscaleImageGeomArgs;

    //    static inline constexpr StringLiteral k_ConversionAlgorithm_Key = "conversion_algorithm_index";
    //    static inline constexpr StringLiteral k_ColorChannel_Key = "color_channel";

    grayscaleImageGeomArgs.insertOrAssign("input_data_array_paths", std::make_any<std::vector<DataPath>>({imageDataPath}));
    grayscaleImageGeomArgs.insertOrAssign("output_array_prefix", std::make_any<std::string>("grayscale_"));
    grayscaleImageGeomArgs.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(pColorWeightsValue));

    // Run resample image geometry filter and process results and messages
    auto grayscaleImageResult = grayScaleFilter->preflight(tmpDs, grayscaleImageGeomArgs, messageHandler, shouldCancel);
    if(grayscaleImageResult.outputActions.invalid())
    {
      return grayscaleImageResult;
    }
    Result<> actionsResult = grayscaleImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    resultOutputActions = MergeOutputActionResults(resultOutputActions, grayscaleImageResult.outputActions);

    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(imageDataPath));
    const DataPath grayscaleImageDataPath = currentImageGeomPath.createChildPath(cellDataName).createChildPath("grayscale_" + pImageDataArrayNameValue);
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(grayscaleImageDataPath, pImageDataArrayNameValue));
  }
  else
  {
    if(pChangeDataType && imageData.getComponentShape().at(0) != 1)
    {
      return MakePreflightErrorResult(
          -23506, fmt::format("Changing the array type requires the input image data to be a scalar value OR the image data can be RGB but you must also select 'Convert to Grayscale'"));
    }
  }

  for(const auto& pathToDelete : pathsToDelete)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pathToDelete));
  }

  if(currentImageGeomPath != imageGeomPath)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(currentImageGeomPath, imageGeomPath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKImportImageStackFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  auto origin = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Spacing_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageTransformValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageTransformChoice_Key);
  auto convertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto colorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto resampleImageChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ResampleImagesChoice_Key);
  auto scalingFactor = filterArgs.value<Float32Parameter::ValueType>(k_Scaling_Key);
  auto exactXYDims = filterArgs.value<VectorUInt64Parameter::ValueType>(k_ExactXYDimensions_Key);

  auto changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto destType = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);
  auto croppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  // const DataPath imageDataPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataName);

  std::vector<std::string> files = inputFileListInfo.generate();

  const std::string& firstFile = files.at(0);

  const itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(firstFile.c_str(), itk::ImageIOFactory::IOFileModeEnum::ReadMode);
  imageIO->SetFileName(firstFile.c_str());
  imageIO->ReadImageInformation();

  const itk::ImageIOBase::IOComponentEnum component = imageIO->GetComponentType();

  std::optional<NumericType> numericType = ITK::ConvertIOComponentToNumericType(component);
  if(!numericType.has_value())
  {
    return MakeErrorResult(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
  }

  Result<> readResult;
  if(changeDataType &&
     ExecuteNeighborFunction(nx::core::ITK::detail::PreflightTypeConversionValidateFunctor{}, ConvertNumericTypeToDataType(*numericType), ITK::detail::ConvertChoiceToDataType(destType)))
  {
    switch(ITK::detail::ConvertChoiceToDataType(destType))
    {
    case DataType::uint8: {
      readResult =
          cxITKImportImageStackFilter::ReadImageStack<uint8>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue, colorWeightsValue,
                                                             resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing, messageHandler, shouldCancel);
      break;
    }
    case DataType::uint16: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<uint16>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                       colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                       messageHandler, shouldCancel);
      break;
    }
    case DataType::uint32: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<uint32>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                       colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                       messageHandler, shouldCancel);
      break;
    }
    default: {
      throw std::runtime_error("Unsupported Conversion type");
    }
    }
  }
  else
  {
    switch(*numericType)
    {
    case NumericType::uint8: {
      readResult =
          cxITKImportImageStackFilter::ReadImageStack<uint8>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue, colorWeightsValue,
                                                             resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing, messageHandler, shouldCancel);
      break;
    }
    case NumericType::int8: {
      readResult =
          cxITKImportImageStackFilter::ReadImageStack<int8>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue, colorWeightsValue,
                                                            resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing, messageHandler, shouldCancel);
      break;
    }
    case NumericType::uint16: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<uint16>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                       colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                       messageHandler, shouldCancel);
      break;
    }
    case NumericType::int16: {
      readResult =
          cxITKImportImageStackFilter::ReadImageStack<int16>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue, colorWeightsValue,
                                                             resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing, messageHandler, shouldCancel);
      break;
    }
    case NumericType::uint32: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<uint32>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                       colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                       messageHandler, shouldCancel);
      break;
    }
    case NumericType::int32: {
      readResult =
          cxITKImportImageStackFilter::ReadImageStack<int32>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue, colorWeightsValue,
                                                             resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing, messageHandler, shouldCancel);
      break;
    }
    case NumericType::uint64: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<uint64>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                       colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                       messageHandler, shouldCancel);
      break;
    }
    case NumericType::int64: {
      readResult =
          cxITKImportImageStackFilter::ReadImageStack<int64>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue, colorWeightsValue,
                                                             resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing, messageHandler, shouldCancel);
      break;
    }
    case NumericType::float32: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<float32>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                        colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                        messageHandler, shouldCancel);
      break;
    }
    case NumericType::float64: {
      readResult = cxITKImportImageStackFilter::ReadImageStack<float64>(dataStructure, imageGeomPath, cellDataName, imageDataName, files, imageTransformValue, convertToGrayScaleValue,
                                                                        colorWeightsValue, resampleImageChoice, scalingFactor, exactXYDims, changeDataType, destType, croppingOptions, origin, spacing,
                                                                        messageHandler, shouldCancel);
      break;
    }
    default: {
      throw std::runtime_error("Unsupported array type");
    }
    }
  }

  return readResult;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileListInfoKey = "InputFileListInfo";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_SpacingKey = "Spacing";
constexpr StringLiteral k_ImageTransformChoiceKey = "ImageTransformChoice";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_ImageDataArrayNameKey = "ImageDataArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKImportImageStackFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKImportImageStackFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FileListInfoFilterParameterConverter>(args, json, SIMPL::k_InputFileListInfoKey, k_InputFileListInfo_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_ImageTransformChoiceKey, k_ImageTransformChoice_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellDataName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ImageDataArrayNameKey, k_ImageDataArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
