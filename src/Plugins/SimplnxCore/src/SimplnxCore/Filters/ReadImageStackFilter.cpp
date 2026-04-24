#include "ReadImageStackFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadImageStack.hpp"
#include "SimplnxCore/Filters/ConvertColorToGrayScaleFilter.hpp"
#include "SimplnxCore/Filters/ReadImageFilter.hpp"
#include "SimplnxCore/Filters/ResampleImageGeomFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const ChoicesParameter::Choices k_SliceOperationChoices = {"None", "Flip about X axis", "Flip about Y axis"};

constexpr nx::core::StringLiteral k_NoResamplingMode = "Do Not Resample (0)";
constexpr nx::core::StringLiteral k_ScalingMode = "Scaling (1)";
constexpr nx::core::StringLiteral k_ExactDimensions = "Exact X/Y Dimensions (2)";
const nx::core::ChoicesParameter::Choices k_ResamplingChoices = {k_NoResamplingMode, k_ScalingMode, k_ExactDimensions};
const nx::core::ChoicesParameter::ValueType k_NoResampleModeIndex = 0;
const nx::core::ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const nx::core::ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadImageStackFilter::name() const
{
  return FilterTraits<ReadImageStackFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadImageStackFilter::className() const
{
  return FilterTraits<ReadImageStackFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadImageStackFilter::uuid() const
{
  return FilterTraits<ReadImageStackFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadImageStackFilter::humanName() const
{
  return "Read Images [3D Stack]";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadImageStackFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Image", "Tif", "JPEG", "PNG"};
}

//------------------------------------------------------------------------------
Parameters ReadImageStackFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(
      std::make_unique<GeneratedFileListParameter>(k_InputFileListInfo_Key, "Input File List", "The list of 2D image files to be read in to a 3D volume", GeneratedFileListParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  params.insert(std::make_unique<CropGeometryParameter>(
      k_CroppingOptions_Key, "Cropping Options",
      "The cropping options used to crop images.  These include picking the cropping type, the cropping dimensions, and the cropping ranges for each chosen dimension.",
      CropGeometryParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Origin & Spacing Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Set Origin", "Specifies if the origin should be changed", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the 3D volume", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Set Spacing", "Specifies if the spacing should be changed", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "The spacing of the 3D volume", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_OriginSpacingProcessing_Key, "Origin & Spacing Processing", "Whether the origin & spacing should be preprocessed or postprocessed.", 1,
                                                   ChoicesParameter::Choices{"Preprocessed", "Postprocessed"}));

  params.insertSeparator(Parameters::Separator{"Resampling Options"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ResampleImagesChoice_Key, "Resample Images",
                                                                    "Mode can be [0] Do Not Rescale, [1] Scaling as Percent, [2] Exact X/Y Dimensions For Resampling Along Z Axis",
                                                                    ::k_NoResampleModeIndex, ::k_ResamplingChoices));
  params.insert(std::make_unique<Float32Parameter>(
      k_Scaling_Key, "Scaling (%)",
      "The scaling of the 3D volume, in percentages. Percentage must be greater than or equal to 1.0f. Larger percentages will cause more voxels, smaller percentages "
      "will cause less voxels. For example, 10.0 is one-tenth the original number of pixels.  200.0 is double the number of pixels.",
      100.0f));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_ExactXYDimensions_Key, "Exact 2D Dimensions (Pixels)",
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
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, true);
  params.linkParameters(k_ChangeOrigin_Key, k_OriginSpacingProcessing_Key, true);
  params.linkParameters(k_ChangeSpacing_Key, k_OriginSpacingProcessing_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadImageStackFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadImageStackFilter::clone() const
{
  return std::make_unique<ReadImageStackFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadImageStackFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto originSpacingProcessing = static_cast<OriginSpacingProcessing>(filterArgs.value<ChoicesParameter::ValueType>(k_OriginSpacingProcessing_Key));
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto convertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto colorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto resampleImagesChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ResampleImagesChoice_Key);
  auto scalingValue = filterArgs.value<Float32Parameter::ValueType>(k_Scaling_Key);
  auto exactXYDims = filterArgs.value<VectorUInt64Parameter::ValueType>(k_ExactXYDimensions_Key);

  auto changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto imageDataTypeChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);
  auto croppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<std::string> files = inputFileListInfo.generate();

  if(files.empty())
  {
    return {MakeErrorResult<OutputActions>(-1, "GeneratedFileList must not be empty")};
  }

  DataStructure tmpDs;
  std::vector<usize> outputDims;
  std::vector<float32> outputSpacing;
  std::vector<float32> outputOrigin;
  IGeometry::LengthUnit outputUnits = IGeometry::LengthUnit::Micrometer;

  // Preflight the first slice through ReadImageFilter to get the 2D geometry/array actions; the Z
  // dimension is adjusted below from the file-list length (and any Z cropping) before we append our
  // own actions.
  ReadImageSubFilterConfig subConfig{};
  subConfig.filePath = files.at(0);
  subConfig.imageGeometryPath = imageGeomPath;
  subConfig.cellDataName = cellDataName;
  subConfig.imageDataArrayName = imageDataArrayName;
  subConfig.changeOrigin = shouldChangeOrigin;
  subConfig.changeSpacing = shouldChangeSpacing;
  subConfig.origin = origin;
  subConfig.spacing = spacing;
  subConfig.originSpacingProcessing = originSpacingProcessing;
  subConfig.changeDataType = changeDataType;
  subConfig.imageDataType = ChoiceToImageDataType(imageDataTypeChoice);
  subConfig.croppingOptions = croppingOptions;

  const ReadImageFilter imageReader;
  PreflightResult imageReaderResult = imageReader.preflight(tmpDs, BuildReadImageFilterArgs(subConfig), messageHandler, shouldCancel);
  if(imageReaderResult.outputActions.invalid())
  {
    return imageReaderResult;
  }

  const IDataAction* action0Ptr = imageReaderResult.outputActions.value().actions.at(0).get();
  const auto* createImageGeomActionPtr = dynamic_cast<const CreateImageGeometryAction*>(action0Ptr);
  if(createImageGeomActionPtr == nullptr)
  {
    return MakePreflightErrorResult(-23530, "Internal error: expected ReadImageFilter preflight to produce a CreateImageGeometryAction as its first output action.");
  }
  {
    outputDims = createImageGeomActionPtr->dims();
    outputSpacing = createImageGeomActionPtr->spacing();
    outputOrigin = createImageGeomActionPtr->origin();
    outputUnits = createImageGeomActionPtr->units();

    // Compute Z dimension, taking into account possible Z cropping
    usize zDim = files.size();

    if(croppingOptions.cropZ)
    {
      if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
      {
        // Voxel-based Z cropping: zBoundVoxels are inclusive indices
        const auto zMin = static_cast<usize>(croppingOptions.zBoundVoxels[0]);
        const auto zMax = static_cast<usize>(croppingOptions.zBoundVoxels[1]);
        if(zMax >= zMin)
        {
          zDim = zMax - zMin + 1;
        }
      }
      else if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume)
      {
        const float64 zMinPhys = croppingOptions.zBoundPhysical[0];
        const float64 zMaxPhys = croppingOptions.zBoundPhysical[1];

        const float64 originZ = (shouldChangeOrigin && originSpacingProcessing == OriginSpacingProcessing::Preprocessed) ? origin[2] : 0;
        const float64 spacingZ = (shouldChangeSpacing && originSpacingProcessing == OriginSpacingProcessing::Preprocessed) ? spacing[2] : 1;

        if(zMaxPhys < zMinPhys)
        {
          return MakePreflightErrorResult(
              -23520, fmt::format("Invalid Z cropping range: the maximum physical Z value is smaller than the minimum. Please ensure the start Z is less than or equal to the end Z."));
        }

        if(spacingZ <= 0)
        {
          return MakePreflightErrorResult(-23521, fmt::format("Invalid Z spacing ({}). The Z spacing must be greater than zero to apply physical cropping.", spacingZ));
        }

        if(zMinPhys < originZ || zMinPhys > (static_cast<float32>(zDim) * spacingZ + originZ))
        {
          return MakePreflightErrorResult(-23522, fmt::format("The minimum Z cropping value ({}) is outside the image bounds. Valid Z range is [{} to {}] in physical units.", zMinPhys, originZ,
                                                              (static_cast<float32>(zDim) * spacingZ + originZ)));
        }

        if(zMaxPhys < originZ || zMaxPhys > (static_cast<float32>(zDim) * spacingZ + originZ))
        {
          return MakePreflightErrorResult(-23523, fmt::format("The maximum Z cropping value ({}) is outside the image bounds. Valid Z range is [{} to {}] in physical units.", zMaxPhys, originZ,
                                                              (static_cast<float32>(zDim) * spacingZ + originZ)));
        }

        const auto zMinIndex = static_cast<usize>(std::floor((zMinPhys - originZ) / spacingZ));
        if(zMinIndex >= zDim)
        {
          return MakePreflightErrorResult(-23524, fmt::format("The minimum Z cropping value ({}) converts to slice index {} which is outside the valid slice index range [0 to {}].", zMinPhys,
                                                              zMinIndex, (zDim > 0 ? zDim - 1 : 0)));
        }

        const auto zMaxIndex = static_cast<usize>(std::floor((zMaxPhys - originZ) / spacingZ));
        if(zMaxIndex >= zDim)
        {
          return MakePreflightErrorResult(-23525, fmt::format("The maximum Z cropping value ({}) converts to slice index {} which is outside the valid slice index range [0 to {}].", zMaxPhys,
                                                              zMaxIndex, (zDim > 0 ? zDim - 1 : 0)));
        }

        zDim = zMaxIndex - zMinIndex + 1;
      }
    }

    outputDims.back() = zDim;

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(createImageGeomActionPtr->path(), outputDims, createImageGeomActionPtr->origin(),
                                                                                         createImageGeomActionPtr->spacing(), createImageGeomActionPtr->cellAttributeMatrixName(),
                                                                                         createImageGeomActionPtr->units()));
    const IDataAction* action1Ptr = imageReaderResult.outputActions.value().actions.at(1).get();
    const auto* createArrayActionPtr = dynamic_cast<const CreateArrayAction*>(action1Ptr);
    if(createArrayActionPtr == nullptr)
    {
      return MakePreflightErrorResult(-23531, "Internal error: expected ReadImageFilter preflight to produce a CreateArrayAction as its second output action.");
    }
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(createArrayActionPtr->type(), std::vector<usize>(outputDims.rbegin(), outputDims.rend()),
                                                                                 createArrayActionPtr->componentDims(), createArrayActionPtr->path(), createArrayActionPtr->dataFormat(),
                                                                                 createArrayActionPtr->fillValue()));

    Result<> actionsResult = resultOutputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }
  }

  DataPath currentImageGeomPath = imageGeomPath;
  std::vector<DataPath> pathsToDelete;

  if(resampleImagesChoice != k_NoResampleModeIndex)
  {
    if(resampleImagesChoice == k_ScalingModeIndex && scalingValue < 1.0f)
    {
      return MakePreflightErrorResult(-23508, fmt::format("Scaling value must be greater than or equal to 1.0f. Received: {}", scalingValue));
    }

    Arguments resampleImageGeomArgs;
    resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(currentImageGeomPath));
    resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(DataPath({imageGeomPath.getTargetName() + "_resampled"})));
    resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(resampleImagesChoice));
    resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{scalingValue, scalingValue, 100.0f}));
    resampleImageGeomArgs.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key,
                                         std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{exactXYDims[0], exactXYDims[1], outputDims[2]}));

    ResampleImageGeomFilter resampleImageGeomFilter;
    PreflightResult resampleImageResult = resampleImageGeomFilter.preflight(tmpDs, resampleImageGeomArgs, messageHandler, shouldCancel);
    if(resampleImageResult.outputActions.invalid())
    {
      return resampleImageResult;
    }

    action0Ptr = resampleImageResult.outputActions.value().actions.at(0).get();
    createImageGeomActionPtr = dynamic_cast<const CreateImageGeometryAction*>(action0Ptr);
    if(createImageGeomActionPtr == nullptr)
    {
      return MakePreflightErrorResult(-23532, "Internal error: expected ResampleImageGeomFilter preflight to produce a CreateImageGeometryAction as its first output action.");
    }

    std::vector<usize> dims = createImageGeomActionPtr->dims();
    dims.back() = outputDims.back();
    outputDims = dims;
    outputSpacing = createImageGeomActionPtr->spacing();
    outputOrigin = createImageGeomActionPtr->origin();
    outputUnits = createImageGeomActionPtr->units();
    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(createImageGeomActionPtr->path(), outputDims, createImageGeomActionPtr->origin(),
                                                                                         createImageGeomActionPtr->spacing(), createImageGeomActionPtr->cellAttributeMatrixName(),
                                                                                         createImageGeomActionPtr->units()));

    const IDataAction* action1Ptr = resampleImageResult.outputActions.value().actions.at(1).get();
    const auto* createArrayActionPtr = dynamic_cast<const CreateArrayAction*>(action1Ptr);
    if(createArrayActionPtr == nullptr)
    {
      return MakePreflightErrorResult(-23533, "Internal error: expected ResampleImageGeomFilter preflight to produce a CreateArrayAction as its second output action.");
    }
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(createArrayActionPtr->type(), std::vector<usize>(outputDims.rbegin(), outputDims.rend()),
                                                                                 createArrayActionPtr->componentDims(), createArrayActionPtr->path(), createArrayActionPtr->dataFormat(),
                                                                                 createArrayActionPtr->fillValue()));

    tmpDs = DataStructure();
    Result<> actionsResult = resultOutputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    pathsToDelete.push_back(currentImageGeomPath);
    currentImageGeomPath = DataPath({imageGeomPath.getTargetName() + "_resampled"});
  }

  const DataPath imageDataPath = currentImageGeomPath.createChildPath(cellDataName).createChildPath(imageDataArrayName);
  auto& imageData = tmpDs.getDataRefAs<IDataArray>(imageDataPath);
  if(convertToGrayScale)
  {
    if(imageData.getDataType() != DataType::uint8)
    {
      return MakePreflightErrorResult(-23504, fmt::format("The input DataType is {} which cannot be converted to grayscale. Please turn off the 'Convert To Grayscale' option.",
                                                          nx::core::DataTypeToString(imageData.getDataType())));
    }

    Arguments grayscaleImageGeomArgs;
    grayscaleImageGeomArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_InputDataArrayPath_Key, std::make_any<std::vector<DataPath>>({imageDataPath}));
    grayscaleImageGeomArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key, std::make_any<std::string>("grayscale_"));
    grayscaleImageGeomArgs.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(colorWeights));

    ConvertColorToGrayScaleFilter grayScaleFilter;
    PreflightResult grayscaleImageResult = grayScaleFilter.preflight(tmpDs, grayscaleImageGeomArgs, messageHandler, shouldCancel);
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
    const DataPath grayscaleImageDataPath = currentImageGeomPath.createChildPath(cellDataName).createChildPath("grayscale_" + imageDataArrayName);
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(grayscaleImageDataPath, imageDataArrayName));
  }
  else
  {
    if(changeDataType && imageData.getComponentShape().at(0) != 1)
    {
      return MakePreflightErrorResult(
          -23506, fmt::format("Changing the array type requires the input image data to be a scalar value OR the image data can be RGB but you must also select 'Convert to Grayscale'"));
    }
  }

  for(const DataPath& pathToDelete : pathsToDelete)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pathToDelete));
  }

  if(originSpacingProcessing == OriginSpacingProcessing::Postprocessed && (shouldChangeOrigin || shouldChangeSpacing))
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<UpdateImageGeomAction>(shouldChangeOrigin ? FloatVec3(origin) : std::optional<FloatVec3>{},
                                                                                             shouldChangeSpacing ? FloatVec3(spacing) : std::optional<FloatVec3>{}, currentImageGeomPath));
    outputSpacing = spacing;
    outputOrigin = origin;
  }

  if(currentImageGeomPath != imageGeomPath)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(currentImageGeomPath, imageGeomPath.getTargetName()));
  }

  preflightUpdatedValues.push_back({"Output Geometry", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(outputDims, outputSpacing, outputOrigin, outputUnits)});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadImageStackFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);

  ReadImageStackInputValues inputValues;
  inputValues.fileList = inputFileListInfo.generate();
  inputValues.imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  inputValues.cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  inputValues.changeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.changeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  inputValues.spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.originSpacingProcessing = static_cast<OriginSpacingProcessing>(filterArgs.value<ChoicesParameter::ValueType>(k_OriginSpacingProcessing_Key));
  inputValues.imageTransform = static_cast<ImageFlipTransform>(filterArgs.value<ChoicesParameter::ValueType>(k_ImageTransformChoice_Key));
  inputValues.convertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  inputValues.colorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.resampleImagesChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ResampleImagesChoice_Key);
  inputValues.scaling = filterArgs.value<Float32Parameter::ValueType>(k_Scaling_Key);
  inputValues.exactXYDimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(k_ExactXYDimensions_Key);
  inputValues.changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  inputValues.imageDataType = ChoiceToImageDataType(filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key));
  inputValues.croppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  return ReadImageStack(dataStructure, messageHandler, shouldCancel, inputValues)();
}

} // namespace nx::core
