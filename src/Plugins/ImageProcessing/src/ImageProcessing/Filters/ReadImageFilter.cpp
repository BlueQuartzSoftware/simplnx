#include "ReadImageFilter.hpp"

#include "ImageProcessing/Filters/Algorithms/ReadImage.hpp"
#include "ImageProcessing/utils/NrrdUtilities.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
ImageGeometryCropOptions CreateCropOptions(const DataPath& inputGeometryPath, const DataPath& outputGeometryPath, const CropGeometryParameter::ValueType& croppingOptions, bool cropZ,
                                           bool removeOriginalGeometry)
{
  ImageGeometryCropOptions options;
  options.inputImageGeometryPath = inputGeometryPath;
  options.outputImageGeometryPath = outputGeometryPath;
  options.usePhysicalBounds = croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  options.cropX = croppingOptions.cropX;
  options.cropY = croppingOptions.cropY;
  options.cropZ = cropZ;
  options.removeOriginalGeometry = removeOriginalGeometry;
  options.minVoxel = {static_cast<uint64>(croppingOptions.xBoundVoxels[0]), static_cast<uint64>(croppingOptions.yBoundVoxels[0]), static_cast<uint64>(croppingOptions.zBoundVoxels[0])};
  options.maxVoxel = {static_cast<uint64>(croppingOptions.xBoundVoxels[1]), static_cast<uint64>(croppingOptions.yBoundVoxels[1]), static_cast<uint64>(croppingOptions.zBoundVoxels[1])};
  options.minCoordinate = {static_cast<float64>(croppingOptions.xBoundPhysical[0]), static_cast<float64>(croppingOptions.yBoundPhysical[0]), static_cast<float64>(croppingOptions.zBoundPhysical[0])};
  options.maxCoordinate = {static_cast<float64>(croppingOptions.xBoundPhysical[1]), static_cast<float64>(croppingOptions.yBoundPhysical[1]), static_cast<float64>(croppingOptions.zBoundPhysical[1])};
  return options;
}

// Applies optional origin/spacing overrides to the supplied values in-place.
// Called at both the pre- and post-crop stages to mirror the same override semantics.
void ApplyOriginSpacingOverrides(bool shouldChangeOrigin, bool shouldCenterOrigin, bool shouldChangeSpacing, const VectorFloat32Parameter::ValueType& originValues,
                                 const VectorFloat32Parameter::ValueType& spacingValues, const std::vector<usize>& dims, FloatVec3& origin, FloatVec3& spacing)
{
  if(shouldChangeSpacing)
  {
    spacing = FloatVec3{spacingValues[0], spacingValues[1], spacingValues[2]};
  }

  if(shouldChangeOrigin)
  {
    origin = FloatVec3{originValues[0], originValues[1], originValues[2]};
    if(shouldCenterOrigin)
    {
      for(usize i = 0; i < 3; i++)
      {
        origin[i] = -0.5f * spacing[i] * static_cast<float32>(dims[i]);
      }
    }
  }
}

// Builds the ImageGeom + cell DataArray for the NRRD backend (.nrrd/.nhdr). This is the geometry
// logic of the (now retired) standalone NRRD reader, folded in here so ReadImage produces identical
// geometry for NRRD volumes: header parse -> dims/origin/spacing -> optional crop delegation -> the create-actions and
// the non-axis-aligned ('space directions') warning (code -35750). The raster-oriented
// origin/spacing/data-type override parameters are intentionally not applied here; the NRRD path is
// kept byte-for-byte compatible with the dedicated NRRD reader.
IFilter::PreflightResult NrrdPreflight(const fs::path& inputFilePath, const DataPath& imageGeomPath, const std::string& cellAttrMatName, const std::string& imageDataArrayName,
                                       const CropGeometryParameter::ValueType& croppingOptions, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  Result<OutputActions> resultOutputActions;
  std::vector<IFilter::PreflightValue> preflightUpdatedValues;

  if(!fs::exists(inputFilePath))
  {
    return {MakeErrorResult<OutputActions>(-35720, fmt::format("Input NRRD file does not exist: '{}'", inputFilePath.string()))};
  }

  auto metadataResult = nrrd::ReadNrrdHeader(inputFilePath);
  if(metadataResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
  }
  const nrrd::NrrdMetadata md = std::move(metadataResult).value();

  std::vector<usize> dims = {md.dimensions[0], md.dimensions[1], md.dimensions[2]};
  std::vector<float32> origin = {md.origin[0], md.origin[1], md.origin[2]};
  std::vector<float32> spacing = {md.spacing[0], md.spacing[1], md.spacing[2]};

  preflightUpdatedValues.push_back({"Full Input Geometry", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});

  if(croppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping)
  {
    DataStructure tmpDs;
    OutputActions tmpActions;
    {
      auto tmpGeomAction = std::make_unique<CreateImageGeometryAction>(imageGeomPath, dims, origin, spacing, cellAttrMatName);
      tmpActions.appendAction(std::move(tmpGeomAction));
    }
    Result<> tmpActionsResult = tmpActions.applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(tmpActionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(tmpActionsResult), {})};
    }

    const DataPath croppedGeomPath({imageGeomPath.getTargetName() + "_cropped"});
    const ImageGeometryCropOptions cropOptions = CreateCropOptions(imageGeomPath, croppedGeomPath, croppingOptions, croppingOptions.cropZ, false);
    ImageGeometryCropBounds cropBounds;
    IFilter::PreflightResult cropImageResult = PreflightImageGeometryCrop(tmpDs, cropOptions, cropBounds);
    if(cropImageResult.outputActions.invalid())
    {
      return cropImageResult;
    }
    Result<> actionsResult = cropImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    const auto& croppedGeom = tmpDs.getDataRefAs<ImageGeom>(croppedGeomPath);
    dims = croppedGeom.getDimensions().toContainer<std::vector<usize>>();
    origin = croppedGeom.getOrigin().toContainer<std::vector<float32>>();
    spacing = croppedGeom.getSpacing().toContainer<std::vector<float32>>();
  }

  {
    auto createGeomAction = std::make_unique<CreateImageGeometryAction>(imageGeomPath, dims, origin, spacing, cellAttrMatName);
    resultOutputActions.value().appendAction(std::move(createGeomAction));
  }

  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  const std::vector<usize> componentDims = {md.componentCount};
  const DataPath dataArrayPath = imageGeomPath.createChildPath(cellAttrMatName).createChildPath(imageDataArrayName);
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(md.dataType, tupleDims, componentDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});
  preflightUpdatedValues.push_back({"NRRD Header", fmt::format("type: {}  |  components: {}  |  encoding: {}  |  byte-swap: {}", md.typeString, md.componentCount,
                                                               (md.encoding == nrrd::Encoding::Gzip ? "gzip" : "raw"), md.byteSwapRequired ? "yes" : "no")});

  if(md.affineHasRotation)
  {
    resultOutputActions.warnings().push_back(Warning{
        -35750, fmt::format("NRRD file '{}' has non-axis-aligned 'space directions'. The ImageGeom in simplnx is axis-aligned; the rotation/permutation component has been dropped and only the "
                            "per-axis spacing (column magnitudes) and origin were extracted. Voxels are loaded in their stored order.",
                            md.filePath)});
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadImageFilter::name() const
{
  return FilterTraits<ReadImageFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadImageFilter::className() const
{
  return FilterTraits<ReadImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadImageFilter::uuid() const
{
  return FilterTraits<ReadImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadImageFilter::humanName() const
{
  return "Read Image";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadImageFilter::defaultTags() const
{
  return {className(), "io", "input", "read", "import", "image", "jpg", "tiff", "bmp", "png", "nrrd", "nhdr", "volume"};
}

//------------------------------------------------------------------------------
Parameters ReadImageFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<FileSystemPathParameter>(k_FileName_Key, "File", "Input image file (2D raster, multi-page TIFF, or NRRD volume)", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".png"}, {".tiff"}, {".tif"}, {".bmp"}, {".jpeg"}, {".jpg"}, {".nrrd"}, {".nhdr"}},
                                                          FileSystemPathParameter::PathType::InputFile, false));

  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry",
                                                   to_underlying(IGeometry::LengthUnit::Micrometer), IGeometry::GetAllLengthUnitStrings()));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeDataType_Key, "Set Image Data Type", "Set the final created image data type.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ImageDataType_Key, "Output Data Type", "Numeric Type of data to create", 0ULL,
                                                   ChoicesParameter::Choices{"uint8", "uint16", "uint32"})); // Sequence Dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Origin & Spacing Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Set Origin", "Specifies if the origin should be changed", false));
  params.insert(
      std::make_unique<BoolParameter>(k_CenterOrigin_Key, "Put Input Origin at the Center of Geometry", "Specifies if the origin should be aligned with the corner (false) or center (true)", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin (Physical Units)", "Specifies the new origin values in physical units.", std::vector<float32>{0.0F, 0.0F, 0.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Set Spacing", "Specifies if the spacing should be changed", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing (Physical Units)", "Specifies the new spacing values in physical units.", std::vector<float32>{1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_OriginSpacingProcessing_Key, "Origin & Spacing Processing", "Whether the origin & spacing should be preprocessed or postprocessed.", 1,
                                                   ChoicesParameter::Choices{"Preprocessed", "Postprocessed"}));

  params.linkParameters(k_ChangeDataType_Key, k_ImageDataType_Key, true);

  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeOrigin_Key, k_CenterOrigin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeOrigin_Key, k_OriginSpacingProcessing_Key, true);
  params.linkParameters(k_ChangeSpacing_Key, k_OriginSpacingProcessing_Key, true);

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  auto croppingOptions = CropGeometryParameter::ValueType{};
  croppingOptions.is2D = true;
  params.insert(std::make_unique<CropGeometryParameter>(
      k_CroppingOptions_Key, "Cropping Options",
      "The cropping options used to crop images.  These include picking the cropping type, the cropping dimensions, and the cropping ranges for each chosen dimension.", croppingOptions));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Created Cell Attribute Matrix", "The name of the created cell attribute matrix", ImageGeom::k_CellAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageDataArrayPath_Key, "Created Cell Data",
                                                          "The name of the created image data array. Will be stored in the created Cell Attribute Matrix", "ImageData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadImageFilter::clone() const
{
  auto filter = std::make_unique<ReadImageFilter>();
  filter->m_ReadSinglePageOnly = m_ReadSinglePageOnly;
  return filter;
}

//------------------------------------------------------------------------------
void ReadImageFilter::setReadSinglePageOnly(bool value)
{
  m_ReadSinglePageOnly = value;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                        const ExecutionContext& executionContext) const
{
  auto fileName = filterArgs.value<fs::path>(k_FileName_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldCenterOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto originValues = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto spacingValues = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto originSpacingProcessing = static_cast<OriginSpacingProcessing>(filterArgs.value<ChoicesParameter::ValueType>(k_OriginSpacingProcessing_Key));
  auto changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto imageDataTypeChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);
  auto lengthUnitIndex = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto croppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  // Dispatch on the file extension: .nrrd/.nhdr use the NRRD backend (geometry built exactly like
  // the dedicated NRRD reader); everything else is a raster image read below. (.mha is out of scope.)
  if(DetermineReadImageBackend(fileName) == ReadImageBackend::Nrrd)
  {
    return NrrdPreflight(fileName, imageGeomPath, cellDataName, imageDataArrayName, croppingOptions, messageHandler, shouldCancel);
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto imageIOResult = CreateImageIO(fileName);
  if(imageIOResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(imageIOResult)), {})};
  }
  auto& imageIO = imageIOResult.value();

  auto metadataResult = imageIO->readMetadata(fileName);
  if(metadataResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
  }
  const auto& metadata = metadataResult.value();

  // Z is the number of real (non-pyramid/non-mask) pages the metadata reports. A single-page hint
  // (set programmatically by ReadImageStack) forces Z == 1 so each stacked file yields one slice.
  const usize resolvedDepth = m_ReadSinglePageOnly ? usize{1} : metadata.numPages;
  std::vector<usize> dims = {metadata.width, metadata.height, resolvedDepth};
  FloatVec3 origin = metadata.origin.value_or(FloatVec3{0.0f, 0.0f, 0.0f});
  FloatVec3 spacing = metadata.spacing.value_or(FloatVec3{1.0f, 1.0f, 1.0f});

  if(originSpacingProcessing == OriginSpacingProcessing::Preprocessed)
  {
    ApplyOriginSpacingOverrides(shouldChangeOrigin, shouldCenterOrigin, shouldChangeSpacing, originValues, spacingValues, dims, origin, spacing);
  }

  // Cropping uses the shared planner on a temporary Image Geometry.
  // Z cropping applies only to a volume with more than one page.
  bool cropImage = croppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping;
  const bool zCropActive = cropImage && croppingOptions.cropZ && dims[2] > 1;
  bool cropAnyDim = cropImage && (croppingOptions.cropX || croppingOptions.cropY || zCropActive);
  if(cropAnyDim)
  {
    DataStructure tmpDs;
    DataPath tmpGeomPath = DataPath({"tmpGeom"});
    ImageGeom* tmpGeomPtr = ImageGeom::Create(tmpDs, tmpGeomPath.getTargetName());
    AttributeMatrix* amPtr = AttributeMatrix::Create(tmpDs, "CellData", std::vector<usize>(dims.crbegin(), dims.crend()), tmpGeomPtr->getId());
    tmpGeomPtr->setCellData(*amPtr);
    tmpGeomPtr->setDimensions(dims);
    tmpGeomPtr->setOrigin(origin);
    tmpGeomPtr->setSpacing(spacing);

    const ImageGeometryCropOptions cropOptions = CreateCropOptions(tmpGeomPath, {}, croppingOptions, zCropActive, true);
    ImageGeometryCropBounds cropBounds;
    IFilter::PreflightResult cropImageResult = PreflightImageGeometryCrop(tmpDs, cropOptions, cropBounds);
    if(cropImageResult.outputActions.invalid())
    {
      return cropImageResult;
    }

    Result<> actionsResult = cropImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    const auto& croppedGeom = tmpDs.getDataRefAs<ImageGeom>(tmpGeomPath);
    dims = croppedGeom.getDimensions().toContainer<std::vector<usize>>();
    spacing = croppedGeom.getSpacing().toContainer<std::vector<float32>>();
    origin = croppedGeom.getOrigin().toContainer<std::vector<float32>>();
  }

  if(originSpacingProcessing == OriginSpacingProcessing::Postprocessed)
  {
    ApplyOriginSpacingOverrides(shouldChangeOrigin, shouldCenterOrigin, shouldChangeSpacing, originValues, spacingValues, dims, origin, spacing);
  }

  DataType dataType = metadata.dataType;
  if(changeDataType)
  {
    dataType = ChoiceToImageDataType(imageDataTypeChoice);
  }

  auto lengthUnit = static_cast<IGeometry::LengthUnit>(lengthUnitIndex);

  // DataArray dimensions are stored slowest-to-fastest (Z, Y, X); ImageGeometry stores fastest-to-slowest (X, Y, Z).
  std::vector<usize> arrayDims(dims.crbegin(), dims.crend());
  std::vector<usize> componentDims = {metadata.numComponents};

  {
    auto originVec = std::vector<float32>{origin[0], origin[1], origin[2]};
    auto spacingVec = std::vector<float32>{spacing[0], spacing[1], spacing[2]};
    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(imageGeomPath, dims, originVec, spacingVec, cellDataName, lengthUnit));
  }

  {
    DataPath imageDataArrayPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataArrayName);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, arrayDims, componentDims, imageDataArrayPath));
  }

  std::string summary = fmt::format("Image Dimensions: {} x {} x {} (pages: {})\n"
                                    "Pixel Components: {}\n"
                                    "Source Data Type: {}\n"
                                    "Output Data Type: {}\n"
                                    "Origin: [{}, {}, {}]\n"
                                    "Spacing: [{}, {}, {}]",
                                    metadata.width, metadata.height, resolvedDepth, metadata.numPages, metadata.numComponents, DataTypeToString(metadata.dataType), DataTypeToString(dataType),
                                    origin[0], origin[1], origin[2], spacing[0], spacing[1], spacing[2]);

  preflightUpdatedValues.push_back({"Image Information", summary});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadImageInputValues inputValues;

  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);

  inputValues.inputFilePath = filterArgs.value<fs::path>(k_FileName_Key);
  inputValues.imageGeometryPath = imageGeomPath;
  inputValues.imageDataArrayPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataArrayName);
  inputValues.cellDataName = cellDataName;
  inputValues.changeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.centerOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  auto originValues = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.origin = FloatVec3{originValues[0], originValues[1], originValues[2]};
  inputValues.changeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto spacingValues = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.spacing = FloatVec3{spacingValues[0], spacingValues[1], spacingValues[2]};
  inputValues.originSpacingProcessing = static_cast<OriginSpacingProcessing>(filterArgs.value<ChoicesParameter::ValueType>(k_OriginSpacingProcessing_Key));
  inputValues.changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  inputValues.imageDataType = ChoiceToImageDataType(filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key));
  inputValues.croppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);
  inputValues.readSinglePageOnly = m_ReadSinglePageOnly;

  return ReadImage(dataStructure, messageHandler, shouldCancel, inputValues)();
}

} // namespace nx::core
