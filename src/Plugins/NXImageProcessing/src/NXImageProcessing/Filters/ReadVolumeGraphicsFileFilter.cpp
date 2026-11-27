#include "ReadVolumeGraphicsFileFilter.hpp"

#include "NXImageProcessing/Filters/Algorithms/ReadVolumeGraphicsFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr int32 k_FileDoesNotExist = -91501;
constexpr int32 k_IncorrectTokenCount = -91502;
constexpr int32 k_VgiParseError = -91503;
constexpr int32 k_UnsupportedDataType = -91505;
constexpr StringLiteral k_Millimeter = "mm";

// File Block
constexpr StringLiteral k_File1Block = "[file1]";
constexpr StringLiteral k_GeometryBlock = "[geometry]";

constexpr StringLiteral k_RegionOfInterestStart = "RegionOfInterestStart";
constexpr StringLiteral k_RegionOfInterestEnd = "RegionOfInterestEnd";

constexpr StringLiteral k_FileFormat = "FileFormat";
constexpr StringLiteral k_Size = "Size";
constexpr StringLiteral k_Name = "Name";
constexpr StringLiteral k_DataType = "Datatype";
constexpr StringLiteral k_DataRange = "datarange";
constexpr StringLiteral k_BitsPerElement = "BitsPerElement";
constexpr StringLiteral k_Status = "status";
constexpr StringLiteral k_RelativePosition = "relativeposition";
constexpr StringLiteral k_Position = "position";
constexpr StringLiteral k_Resolution = "resolution";
constexpr StringLiteral k_Scale = "scale";
constexpr StringLiteral k_Center = "center";
constexpr StringLiteral k_Rotate = "rotate";
constexpr StringLiteral k_Unit = "unit";

struct FileBlock
{
  SizeVec3 region_of_interest_start = {0, 0, 0};
  SizeVec3 region_of_interest_end = {0, 0, 0};
  std::string file_format = "raw";
  SizeVec3 size = {0, 0, 0};
  std::string name;
  std::string datatype = "float";
  FloatVec2Type data_range = {0.0F, 0.0F};
  int8 bits_per_element = 32;
};

struct GeometryBlock
{
  std::string status = "visible";
  FloatVec3 relative_position = {0.0F, 0.0F, 0.0F};
  FloatVec3 position = {0.0F, 0.0F, 0.0F};
  FloatVec3 resolution = {0.0F, 0.0F, 0.0F};
  FloatVec3 scale = {1.0F, 1.0F, 1.0F};
  FloatVec3 center = {0.0F, 0.0F, 0.0F};
  std::array<int32, 9> rotation_matrix = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  std::string unit = "mm";
};

template <usize Dimension, bool USizeType = false>
struct ParserArgs
{
  static constexpr usize k_UseDimension = Dimension;
  static constexpr bool k_UseUSize = USizeType;
};

using ParseIntArrayType = ParserArgs<9>;
using ParseFloatVec2Type = ParserArgs<2>;
using ParseFloatVec3Type = ParserArgs<3>;
using ParseSizeVec3Type = ParserArgs<3, true>;

template <typename T, typename ParserArgs = ParseIntArrayType>
Result<std::array<T, ParserArgs::k_UseDimension>> ParseArray(const std::vector<std::string>& tokens, usize& lineCount)
{
  const auto dimension = ParserArgs::k_UseDimension;
  const auto useSize = ParserArgs::k_UseUSize;
  using VecTType = std::array<T, dimension>;

  VecTType vec = {0}; // auto initializes all values to 0

  const std::string vgiParseErrorStr = "Error parsing {} value ({}) from vgi file.";
  const std::string vgiOutOfRangeErrorStr = "Value ({}) in vgi file is out of range for storage type {}.";

  std::string parseUnit = "float32"; // default to save calls

  if constexpr(useSize)
  {
    parseUnit = "usize";
  }

  if(tokens.size() < dimension + 2)
  {
    return {MakeErrorResult<VecTType>(k_IncorrectTokenCount, fmt::format("Line '{}' in the vgi file does not have at least {} tokens.", lineCount, dimension + 2))};
  }

  for(usize index = 0; index < dimension; index++)
  {
    try
    {
      if constexpr(useSize)
      {
        vec[index] = std::stoull(tokens[index + 2]);
      }
      else
      {
        vec[index] = std::stof(tokens[index + 2]);
      }
    } catch(const std::invalid_argument& e)
    {
      return {MakeErrorResult<VecTType>(k_VgiParseError, fmt::format(fmt::runtime(vgiParseErrorStr), parseUnit, tokens[index + 2]))};
    } catch(const std::out_of_range& e)
    {
      return {MakeErrorResult<VecTType>(k_VgiParseError, fmt::format(fmt::runtime(vgiOutOfRangeErrorStr), tokens[index + 2], parseUnit))};
    }
  }

  return {vec};
}

Result<FileBlock> ParseFileBlock(std::ifstream& inputFile, usize& lineCount)
{
  FileBlock fileBlock;
  std::string buf;

  std::streampos currentPos = inputFile.tellg();
  getline(inputFile, buf);
  lineCount++;
  buf = StringUtilities::trimmed(buf);

  while(buf[0] != '[' && !inputFile.eof())
  {
    std::vector<std::string> tokens = StringUtilities::Split(buf, ' ');
    if(tokens[0] == k_FileFormat)
    {
      fileBlock.file_format = tokens[2];
    }
    else if(tokens[0] == k_Name)
    {
      fileBlock.name = tokens[2];
    }
    else if(tokens[0] == k_DataType)
    {
      fileBlock.datatype = tokens[2];
    }
    else if(tokens[0] == k_DataRange)
    {
      auto result = ParseArray<float32, ParseFloatVec2Type>(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(ConvertResult(std::move(result)), {});
      }
      fileBlock.data_range = result.value();
    }
    else if(tokens[0] == k_BitsPerElement)
    {
      try
      {
        fileBlock.bits_per_element = static_cast<int8>(std::stoi(tokens[2]));
      } catch(const std::invalid_argument& e)
      {
        return {MakeErrorResult<FileBlock>(k_VgiParseError, fmt::format("Error parsing 'Bits Per Element' integer value ({}) from vgi file.", tokens[2]))};
      } catch(const std::out_of_range& e)
      {
        return {MakeErrorResult<FileBlock>(k_VgiParseError, fmt::format("'Bits Per Element' integer value ({}) in vgi file is out of range for storage type int8.", tokens[2]))};
      }
    }
    else if(std::find(tokens.begin(), tokens.end(), "=") != tokens.end())
    {
      auto result = ParseArray<usize, ParseSizeVec3Type>(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(ConvertResult(std::move(result)), {});
      }
      if(tokens[0] == k_RegionOfInterestStart)
      {
        fileBlock.region_of_interest_start = result.value();
      }
      else if(tokens[0] == k_RegionOfInterestEnd)
      {
        fileBlock.region_of_interest_end = result.value();
      }
      else if(tokens[0] == k_Size)
      {
        fileBlock.size = result.value();
      }
    }

    currentPos = inputFile.tellg();
    getline(inputFile, buf); // Read the next line
    lineCount++;
    buf = StringUtilities::trimmed(buf);
  }
  inputFile.seekg(currentPos); // Put the file position back before the last read.
  return {fileBlock};
}

Result<GeometryBlock> ParseGeometryBlock(std::ifstream& inputFile, const fs::path& headerPath, usize& lineCount, bool replaceSpacing)
{
  GeometryBlock block;
  std::string buf;

  const std::streampos currentPos = inputFile.tellg();
  getline(inputFile, buf);
  lineCount++;
  buf = StringUtilities::trimmed(buf);
  std::vector<std::string> tokens;

  while(buf[0] != '[' && !inputFile.eof())
  {
    tokens = StringUtilities::Split(buf, ' ');
    if(tokens[0] == k_Status)
    {
      block.status = tokens[2];
    }
    else if(tokens[0] == k_Unit)
    {
      block.unit = tokens[2];
    }
    else if(tokens[0] == k_Rotate)
    {
      auto result = ParseArray<int32, ParseIntArrayType>(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(ConvertResult(std::move(result)), {});
      }
      block.rotation_matrix = result.value();
    }
    else
    {
      auto result = ParseArray<float32, ParseFloatVec3Type>(tokens, lineCount);
      if(result.invalid())
      {
        auto& error = result.errors().front();
        const bool isResolutionConversionError = tokens[0] == k_Resolution && error.code == k_VgiParseError;
        if(isResolutionConversionError && !replaceSpacing)
        {
          error.message = ImageIO::FormatReaderValueError(headerPath, k_Resolution, error.message, ImageIO::k_SetSpacingLabel);
        }
        if(!isResolutionConversionError || !replaceSpacing)
        {
          return ConvertInvalidResult<GeometryBlock>(std::move(result));
        }
      }
      else if(tokens[0] == k_RelativePosition)
      {
        block.relative_position = result.value();
      }
      else if(tokens[0] == k_Position)
      {
        block.position = result.value();
      }
      else if(tokens[0] == k_Resolution)
      {
        block.resolution = result.value();
      }
      else if(tokens[0] == k_Scale)
      {
        block.scale = result.value();
      }
      else if(tokens[0] == k_Center)
      {
        block.center = result.value();
      }
    }

    getline(inputFile, buf); // Read the next line
    lineCount++;
    buf = StringUtilities::trimmed(buf);
  }

  inputFile.seekg(currentPos); // Put the file position back before the last read.
  return {block};
}

// -----------------------------------------------------------------------------
Result<ReadVolumeGraphicsFileFilter::HeaderMetadata> ReadHeaderMetaData(const fs::path& vgHeaderFilePath, bool replaceSpacing)
{
  std::ifstream vgHeaderFile(vgHeaderFilePath);
  usize lineCount = 0;

  std::string buf;
  FileBlock fileBlock;
  GeometryBlock geomBlock;

  while(getline(vgHeaderFile, buf))
  {
    lineCount++;
    buf = StringUtilities::trimmed(buf);

    if(buf == k_File1Block)
    {
      auto result = ParseFileBlock(vgHeaderFile, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<ReadVolumeGraphicsFileFilter::HeaderMetadata>(ConvertResult(std::move(result)), {});
      }
      fileBlock = result.value();
    }

    else if(buf == k_GeometryBlock)
    {
      auto result = ParseGeometryBlock(vgHeaderFile, vgHeaderFilePath, lineCount, replaceSpacing);
      if(result.invalid())
      {
        return ConvertResultTo<ReadVolumeGraphicsFileFilter::HeaderMetadata>(ConvertResult(std::move(result)), {});
      }
      geomBlock = result.value();
    }
  }

  if(fileBlock.datatype != "float" || fileBlock.bits_per_element != 32)
  {
    return MakeErrorResult<ReadVolumeGraphicsFileFilter::HeaderMetadata>(k_UnsupportedDataType,
                                                                         fmt::format("Unsupported Volume Graphics Datatype '{}' with BitsPerElement {} in '{}'. Only 32-bit float data is supported.",
                                                                                     fileBlock.datatype, fileBlock.bits_per_element, vgHeaderFilePath.string()));
  }

  vgHeaderFile.clear();
  vgHeaderFile.seekg(0);

  ReadVolumeGraphicsFileFilter::HeaderMetadata metadata;
  metadata.Dimensions = fileBlock.size;
  metadata.Spacing = geomBlock.resolution;
  if(geomBlock.unit == k_Millimeter)
  {
    metadata.Units = IGeometry::LengthUnit::Millimeter;
  }

  metadata.DataFilePath = (fs::absolute(vgHeaderFilePath.parent_path()) / fileBlock.name).string();
  return {metadata};
}

struct ReadVolumeGraphicsFileFilterCache
{
  fs::path vgi_data_file_path;
};

std::atomic_int32_t SInstanceId = 0;
std::map<int32, ReadVolumeGraphicsFileFilterCache> SHeaderCache;

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadVolumeGraphicsFileFilter::ReadVolumeGraphicsFileFilter()
: m_InstanceId(SInstanceId.fetch_add(1))
{
  SHeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadVolumeGraphicsFileFilter::~ReadVolumeGraphicsFileFilter() noexcept
{
  SHeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ReadVolumeGraphicsFileFilter::name() const
{
  return FilterTraits<ReadVolumeGraphicsFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadVolumeGraphicsFileFilter::className() const
{
  return FilterTraits<ReadVolumeGraphicsFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadVolumeGraphicsFileFilter::uuid() const
{
  return FilterTraits<ReadVolumeGraphicsFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadVolumeGraphicsFileFilter::humanName() const
{
  return "Read Volume Graphics File (.vgi/.vol)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadVolumeGraphicsFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "CT", "VolumeGraphics", "vgi"};
}

//------------------------------------------------------------------------------
Parameters ReadVolumeGraphicsFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_VGHeaderFile_Key, "Volume Graphics .vgi File", "The input VolumeGraphics file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".vgi"}, FileSystemPathParameter::PathType::InputFile));
  params.insertSeparator(Parameters::Separator{"Geometry Metadata Overrides"});
  ImageIO::InsertGeometryOverrideParameters(params);

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "Path to create the Image Geometry", DataPath({"VolumeGraphics"})));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The attribute matrix created as a child of the image geometry", "CT Data"));
  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_DensityArrayName_Key, "Density", "The data array created as a child of the attribute matrix", "Density"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadVolumeGraphicsFileFilter::parametersVersion() const
{
  // Version 2: Added Set Origin and Set Spacing.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadVolumeGraphicsFileFilter::clone() const
{
  return std::make_unique<ReadVolumeGraphicsFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadVolumeGraphicsFileFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                     const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pVGHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_VGHeaderFile_Key);
  auto pNewImageGeometryPathValue = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_DensityArrayName_Key);

  const auto overrides = ImageIO::GeometryOverrides::FromArguments(filterArgs);
  auto metadataResult = ReadHeaderMetaData(pVGHeaderFileValue, overrides.Spacing.has_value());
  if(metadataResult.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
  }

  HeaderMetadata metadata = metadataResult.value();
  FloatVec3 origin{0.0F, 0.0F, 0.0F};
  if(auto geometryResult = overrides.apply(pVGHeaderFileValue, origin, metadata.Spacing); geometryResult.invalid())
  {
    return {.outputActions = ConvertInvalidResult<OutputActions>(std::move(geometryResult))};
  }

  nx::core::Result<OutputActions> resultOutputActions;

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(
      pNewImageGeometryPathValue, CreateImageGeometryAction::DimensionType{metadata.Dimensions[0], metadata.Dimensions[1], metadata.Dimensions[2]}, origin.toContainer<std::vector<float32>>(),
      CreateImageGeometryAction::SpacingType{metadata.Spacing[0], metadata.Spacing[1], metadata.Spacing[2]}, pCellAttributeMatrixNameValue);
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  const fs::path fiData = fs::path(metadata.DataFilePath);
  SHeaderCache[m_InstanceId].vgi_data_file_path = fiData; // Store the data file path in the cache

  if(!fs::exists(fiData))
  {
    return {.outputActions = MakeErrorResult<OutputActions>(k_FileDoesNotExist, fmt::format("The Volume Graphics data file does not exist. '{}'", metadata.DataFilePath))};
  }

  const DataPath dap = pNewImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  auto createArrayAction =
      std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{metadata.Dimensions[2], metadata.Dimensions[1], metadata.Dimensions[0]}, std::vector<usize>{1}, dap);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {.outputActions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ReadVolumeGraphicsFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{

  ReadVolumeGraphicsFileInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.DensityArrayName = filterArgs.value<std::string>(k_DensityArrayName_Key);
  inputValues.VGDataFile = SHeaderCache[m_InstanceId].vgi_data_file_path;

  return ReadVolumeGraphicsFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteral k_VGHeaderFileKey = "VGHeaderFile";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_DensityArrayNameKey = "DensityArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ReadVolumeGraphicsFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadVolumeGraphicsFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, simpl::k_VGHeaderFileKey, k_VGHeaderFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, simpl::k_DataContainerNameKey, k_CreatedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_DensityArrayNameKey, k_DensityArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
