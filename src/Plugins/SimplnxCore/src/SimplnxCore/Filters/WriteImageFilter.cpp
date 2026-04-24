#include "WriteImageFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteImage.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"

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
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_FileName_Key, "Output File", "Path to the output file to write.", fs::path(), ExtensionListType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<UInt64Parameter>(k_IndexOffset_Key, "Index Offset", "This is the starting index when writing multiple images", 0));
  params.insert(std::make_unique<Int32Parameter>(k_TotalIndexDigits_Key, "Total Number of Index Digits", "This is the total number of digits to use when generating the index", 3));
  params.insert(std::make_unique<StringParameter>(k_LeadingDigitCharacter_Key, "Fill Character", "The character to use for the leading digits if needed", "0"));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_ImageArrayPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, ::k_ScalarPixelAllowedTypes));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteImageFilter::parametersVersion() const
{
  return 1;
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

  return WriteImage(dataStructure, messageHandler, shouldCancel, inputValues)();
}

} // namespace nx::core
