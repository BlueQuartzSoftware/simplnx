#include "WriteImageFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteImage.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CreateColorMapParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const std::set<DataType> k_ScalarPixelAllowedTypes = {DataType::int8, DataType::uint8, DataType::int16, DataType::uint16, DataType::int32, DataType::uint32, DataType::float32};
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string WriteImageFilter::name() const
{
  return FilterTraits<WriteImageFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteImageFilter::className() const
{
  return FilterTraits<WriteImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteImageFilter::uuid() const
{
  return FilterTraits<WriteImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteImageFilter::humanName() const
{
  return "Write Image";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteImageFilter::defaultTags() const
{
  return {className(), "io", "output", "write", "export", "image", "jpg", "tiff", "bmp", "png"};
}

//------------------------------------------------------------------------------
Parameters WriteImageFilter::parameters() const
{
  Parameters params;

  using ExtensionListType = std::unordered_set<std::string>;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane", "Selection for plane normal for writing the images (XY, XZ, or YZ)", 0, ChoicesParameter::Choices{"XY", "XZ", "YZ"}));

  params.insertSeparator(Parameters::Separator{"Output File Options"});
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_FileName_Key, "Output File", "Path to the output file to write.", fs::path(), ExtensionListType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<UInt64Parameter>(k_IndexOffset_Key, "Index Offset", "This is the starting index when writing multiple images", 0));
  params.insert(std::make_unique<Int32Parameter>(k_TotalIndexDigits_Key, "Total Number of Index Digits", "This is the total number of digits to use when generating the index", 3));
  params.insert(std::make_unique<StringParameter>(k_LeadingDigitCharacter_Key, "Fill Character", "The character to use for the leading digits if needed", "0"));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_ImageArrayPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, ::k_ScalarPixelAllowedTypes));

  params.insertSeparator(Parameters::Separator{"Color Table (Optional)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateColorTable_Key, "Create Color Table",
                                                                 "When enabled, the single-component input array is converted to RGB using the selected preset before writing.", false));
  params.insert(
      std::make_unique<CreateColorMapParameter>(k_SelectedPreset_Key, "Select Preset...", "Select a preset color scheme to apply to the input array", ColorTableUtilities::GetDefaultRGBPresetName()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Whether to assign the masked color to 'bad' voxels", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Path to the data array used to define voxels as good or bad.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  std::vector<uint8> defaultMask(3, 0);
  params.insert(std::make_unique<VectorUInt8Parameter>(k_InvalidColorValue_Key, "Masked Color (RGB)", "The color to assign to voxels that have a mask value of FALSE", defaultMask,
                                                       std::vector<std::string>{"Red", "Green", "Blue"}));

  params.linkParameters(k_CreateColorTable_Key, k_SelectedPreset_Key, true);
  // NOTE: k_UseMask_Key cannot itself be linked as a child of k_CreateColorTable_Key: it is a linkable
  // group (it gates k_MaskArrayPath_Key/k_InvalidColorValue_Key below) and Parameters::linkParameters()
  // forbids a group from being a child of another group. It therefore stays a top-level toggle, matching
  // the same pattern used by CreateColorMapFilter's "Use Mask Array" parameter.
  params.linkParameters(k_CreateColorTable_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_CreateColorTable_Key, k_InvalidColorValue_Key, true);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseMask_Key, k_InvalidColorValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteImageFilter::parametersVersion() const
{
  // Version 2: Added optional inline color-table parameters (create_color_table, selected_preset, use_mask, mask_array_path, invalid_color_value).
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteImageFilter::clone() const
{
  return std::make_unique<WriteImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                         const ExecutionContext& executionContext) const
{
  auto plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto filePath = filterArgs.value<fs::path>(k_FileName_Key);
  auto indexOffset = filterArgs.value<uint64>(k_IndexOffset_Key);
  auto imageArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto totalDigits = filterArgs.value<int32>(k_TotalIndexDigits_Key);
  auto fillChar = filterArgs.value<StringParameter::ValueType>(k_LeadingDigitCharacter_Key);
  auto createColorTable = filterArgs.value<bool>(k_CreateColorTable_Key);
  auto useMask = filterArgs.value<bool>(k_UseMask_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  // Validate output file format is supported
  auto imageIOResult = CreateImageIO(filePath);
  if(imageIOResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(imageIOResult)), {})};
  }

  // Validate fill character is a single character
  if(fillChar.size() != 1)
  {
    return {MakeErrorResult<OutputActions>(-27010, "The fill character must be a single character.")};
  }

  // Stored fastest to slowest i.e. X Y Z
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& imageArray = dataStructure.getDataRefAs<IDataArray>(imageArrayPath);

  // Validate data type is in the supported set
  DataType arrayDataType = imageArray.getDataType();
  const auto& allowedTypes = ::k_ScalarPixelAllowedTypes;
  if(allowedTypes.find(arrayDataType) == allowedTypes.end())
  {
    return {MakeErrorResult<OutputActions>(
        -27011, fmt::format("Unsupported data type '{}' for image writing. Supported types: int8, uint8, int16, uint16, int32, uint32, float32.", DataTypeToString(arrayDataType)))};
  }

  if(createColorTable)
  {
    // Color mapping requires a single-component scalar input; RGB is produced by the filter.
    if(imageArray.getNumberOfComponents() != 1)
    {
      return {MakeErrorResult<OutputActions>(-27012, fmt::format("When 'Create Color Table' is enabled the input array must have a single component, but '{}' has {} components.",
                                                                 imageArrayPath.toString(), imageArray.getNumberOfComponents()))};
    }

    if(useMask)
    {
      auto tupleValidityCheck = dataStructure.validateNumberOfTuples({imageArrayPath, maskArrayPath});
      if(!tupleValidityCheck)
      {
        return {MakeErrorResult<OutputActions>(-27013, fmt::format("The input array and mask array must have equal tuple counts.\n{}", tupleValidityCheck.error()))};
      }
    }
  }

  const IDataStore& imageArrayStore = imageArray.getIDataStoreRef();

  if(!nx::core::DoDimensionsMatch(imageArrayStore, imageGeom))
  {
    return {MakeErrorResult<OutputActions>(-25600, fmt::format("Image array '{}' dimensions ({}) do not match image geometry '{}' dimensions ({}).", imageArrayPath.toString(),
                                                               StringUtilities::formatTupleShape3D(imageArray.getTupleShape()), imageGeomPath.toString(),
                                                               StringUtilities::formatDimensions3D(imageGeom.getDimensions())))};
  }

  // Compute slice count based on plane and geometry dims
  auto imageGeomDims = imageGeom.getDimensions();
  usize maxSlice = 1;
  switch(plane)
  {
  case 0: // XY
    maxSlice = imageGeomDims[2];
    break;
  case 1: // XZ
    maxSlice = imageGeomDims[1];
    break;
  case 2: // YZ
    maxSlice = imageGeomDims[0];
    break;
  default:
    break;
  }

  // Generate example filename for PreflightValues
  const std::string indexStr = CreateIndexString(maxSlice, static_cast<usize>(totalDigits), fillChar);
  const std::string exampleFileName = fmt::format("{}/{}_{}{}", fs::absolute(filePath).parent_path().string(), filePath.stem().string(), indexStr, filePath.extension().string());

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Example Output File", exampleFileName});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  WriteImageInputValues inputValues;

  inputValues.outputFilePath = filterArgs.value<fs::path>(k_FileName_Key);
  inputValues.planeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.indexOffset = filterArgs.value<uint64>(k_IndexOffset_Key);
  inputValues.totalIndexDigits = filterArgs.value<int32>(k_TotalIndexDigits_Key);
  inputValues.leadingDigitCharacter = filterArgs.value<StringParameter::ValueType>(k_LeadingDigitCharacter_Key);
  inputValues.imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  inputValues.imageDataArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);
  inputValues.createColorTable = filterArgs.value<bool>(k_CreateColorTable_Key);
  inputValues.presetName = filterArgs.value<CreateColorMapParameter::ValueType>(k_SelectedPreset_Key);
  inputValues.useMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.maskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.invalidColor = filterArgs.value<std::vector<uint8>>(k_InvalidColorValue_Key);

  return WriteImage(dataStructure, messageHandler, shouldCancel, inputValues)();
}

} // namespace nx::core
