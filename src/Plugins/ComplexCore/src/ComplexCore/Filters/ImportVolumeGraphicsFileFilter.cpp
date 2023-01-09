#include "ImportVolumeGraphicsFileFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ImportVolumeGraphicsFile.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
inline constexpr int32 k_FileDoesNotExist = -91501;
inline constexpr int32 k_IncorrectTokenCount = -91502;
inline constexpr int32 k_VgiParseError = -91503;
inline const std::string k_Millimeter("mm");

// File Block
inline const std::string k_File1Block("[file1]");
inline const std::string k_GeometryBlock("[geometry]");

inline const std::string k_RegionOfInterestStart("RegionOfInterestStart");
inline const std::string k_RegionOfInterestEnd("RegionOfInterestEnd");

inline const std::string k_FileFormat("FileFormat");
inline const std::string k_Size("Size");
inline const std::string k_Name("Name");
inline const std::string k_DataType("Datatype");
inline const std::string k_DataRange("datarange");
inline const std::string k_BitsPerElement("BitsPerElement");
inline const std::string k_Status("status");
inline const std::string k_RelativePosition("relativeposition");
inline const std::string k_Position("position");
inline const std::string k_Resolution("resolution");
inline const std::string k_Scale("scale");
inline const std::string k_Center("center");
inline const std::string k_Rotate("rotate");
inline const std::string k_Unit("unit");

struct FileBlock
{
  SizeVec3 RegionOfInterestStart = {0, 0, 0};
  SizeVec3 RegionOfInterestEnd = {0, 0, 0};
  std::string FileFormat = "raw";
  SizeVec3 Size = {0, 0, 0};
  std::string Name;
  std::string Datatype = "float";
  FloatVec2Type DataRange = {0.0F, 0.0F};
  int8 BitsPerElement = 32;
};

struct GeometryBlock
{
  std::string Status = "visible";
  FloatVec3 RelativePosition = {0.0F, 0.0F, 0.0F};
  FloatVec3 Position = {0.0F, 0.0F, 0.0F};
  FloatVec3 Resolution = {0.0F, 0.0F, 0.0F};
  FloatVec3 Scale = {1.0F, 1.0F, 1.0F};
  FloatVec3 Center = {0.0F, 0.0F, 0.0F};
  std::array<int32, 9> RotationMatrix = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  std::string Unit = "mm";
};

Result<SizeVec3> ParseSizeVec3(const std::vector<std::string>& tokens, usize& lineCount)
{
  SizeVec3 vec = {0, 0, 0};
  const std::string vgiParseErrorStr = "Error parsing usize value ({}) from vgi file.";
  const std::string vgiOutOfRangeErrorStr = "Value ({}) in vgi file is out of range for storage type usize.";

  if(tokens.size() < 5)
  {
    return {MakeErrorResult<SizeVec3>(k_IncorrectTokenCount, fmt::format("Line '{}' in the vgi file does not have at least 5 tokens.", lineCount))};
  }

  try
  {
    vec[0] = std::stoull(tokens[2]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<SizeVec3>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[2]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<SizeVec3>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[2]))};
  }

  try
  {
    vec[1] = std::stoull(tokens[3]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<SizeVec3>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[3]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<SizeVec3>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[3]))};
  }

  try
  {
    vec[2] = std::stoull(tokens[4]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<SizeVec3>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[4]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<SizeVec3>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[4]))};
  }

  return {vec};
}

Result<FloatVec2Type> ParseFloatVec2(const std::vector<std::string>& tokens, usize& lineCount)
{
  FloatVec2Type vec = {0.0F, 0.0F};
  const std::string vgiParseErrorStr = "Error parsing floating point value ({}) from vgi file.";
  const std::string vgiOutOfRangeErrorStr = "Floating point value ({}) in vgi file is out of range for storage type float32.";

  if(tokens.size() < 4)
  {
    return {MakeErrorResult<FloatVec2Type>(k_IncorrectTokenCount, fmt::format("Line '{}' in the vgi file does not have at least 4 tokens.", lineCount))};
  }

  try
  {
    vec[0] = std::stof(tokens[2]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<FloatVec2Type>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[2]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<FloatVec2Type>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[2]))};
  }

  try
  {
    vec[1] = std::stof(tokens[3]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<FloatVec2Type>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[3]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<FloatVec2Type>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[3]))};
  }

  return {vec};
}

Result<FloatVec3> ParseFloatVec3(const std::vector<std::string>& tokens, usize& lineCount)
{
  FloatVec3 vec = {0.0F, 0.0F, 0.0F};
  const std::string vgiParseErrorStr = "Error parsing floating point value ({}) from vgi file.";
  const std::string vgiOutOfRangeErrorStr = "Floating point value ({}) in vgi file is out of range for storage type float32.";

  if(tokens.size() < 5)
  {
    return {MakeErrorResult<FloatVec3>(k_IncorrectTokenCount, fmt::format("Line '{}' in the vgi file does not have at least 5 tokens.", lineCount))};
  }

  try
  {
    vec[0] = std::stof(tokens[2]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<FloatVec3>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[2]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<FloatVec3>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[2]))};
  }

  try
  {
    vec[1] = std::stof(tokens[3]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<FloatVec3>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[3]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<FloatVec3>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[3]))};
  }

  try
  {
    vec[2] = std::stof(tokens[4]);
  } catch(const std::invalid_argument& e)
  {
    return {MakeErrorResult<FloatVec3>(k_VgiParseError, fmt::format(vgiParseErrorStr, tokens[4]))};
  } catch(const std::out_of_range& e)
  {
    return {MakeErrorResult<FloatVec3>(k_VgiParseError, fmt::format(vgiOutOfRangeErrorStr, tokens[4]))};
  }

  return {vec};
}

template <typename T, usize Dimension = 9>
Result<std::array<T, Dimension>> ParseIntArray(const std::vector<std::string>& tokens, usize& lineCount)
{
  std::array<T, Dimension> vec;

  if(tokens.size() < (Dimension + 1))
  {
    return {MakeErrorResult<std::array<T, Dimension>>(k_IncorrectTokenCount, fmt::format("Line '{}' in the vgi file does not have at least {} tokens.", lineCount, Dimension + 1))};
  }

  for(usize i = 0; i < Dimension; i++)
  {
    try
    {
      vec[i] = std::stof(tokens[i + 2]);
    } catch(const std::invalid_argument& e)
    {
      return {MakeErrorResult<std::array<T, Dimension>>(k_VgiParseError, fmt::format("Error parsing floating point value ({}) from vgi file.", tokens[i + 2]))};
    } catch(const std::out_of_range& e)
    {
      return {MakeErrorResult<std::array<T, Dimension>>(k_VgiParseError, fmt::format("Floating point value ({}) in vgi file is out of range for storage type float32.", tokens[i + 2]))};
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
    std::vector<std::string> tokens = StringUtilities::split(buf, ' ');
    if(tokens[0] == k_RegionOfInterestStart)
    {
      auto result = ParseSizeVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock.RegionOfInterestStart = result.value();
    }
    else if(tokens[0] == k_RegionOfInterestEnd)
    {
      auto result = ParseSizeVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock.RegionOfInterestEnd = result.value();
    }
    else if(tokens[0] == k_FileFormat)
    {
      fileBlock.FileFormat = tokens[2];
    }
    else if(tokens[0] == k_Size)
    {
      auto result = ParseSizeVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock.Size = result.value();
    }
    else if(tokens[0] == k_Name)
    {
      fileBlock.Name = tokens[2];
    }
    else if(tokens[0] == k_DataType)
    {
      fileBlock.Datatype = tokens[2];
    }
    else if(tokens[0] == k_DataRange)
    {
      auto result = ParseFloatVec2(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock.DataRange = result.value();
    }
    else if(tokens[0] == k_BitsPerElement)
    {
      try
      {
        fileBlock.BitsPerElement = static_cast<int8>(std::stoi(tokens[2]));
      } catch(const std::invalid_argument& e)
      {
        return {MakeErrorResult<FileBlock>(k_VgiParseError, fmt::format("Error parsing 'Bits Per Element' integer value ({}) from vgi file.", tokens[2]))};
      } catch(const std::out_of_range& e)
      {
        return {MakeErrorResult<FileBlock>(k_VgiParseError, fmt::format("'Bits Per Element' integer value ({}) in vgi file is out of range for storage type int8.", tokens[2]))};
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

Result<GeometryBlock> ParseGeometryBlock(std::ifstream& inputFile, usize& lineCount)
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
    tokens = StringUtilities::split(buf, ' ');
    if(tokens[0] == k_Status)
    {
      block.Status = tokens[2];
    }
    else if(tokens[0] == k_RelativePosition)
    {
      auto result = ParseFloatVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.RelativePosition = result.value();
    }
    else if(tokens[0] == k_Position)
    {
      auto result = ParseFloatVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.Position = result.value();
    }
    else if(tokens[0] == k_Resolution)
    {
      auto result = ParseFloatVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.Resolution = result.value();
    }
    else if(tokens[0] == k_Scale)
    {
      auto result = ParseFloatVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.Scale = result.value();
    }
    else if(tokens[0] == k_Center)
    {
      auto result = ParseFloatVec3(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.Center = result.value();
    }
    else if(tokens[0] == k_Rotate)
    {
      auto result = ParseIntArray<int32, 9>(tokens, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.RotationMatrix = result.value();
    }
    else if(tokens[0] == k_Unit)
    {
      block.Unit = tokens[2];
    }

    inputFile.tellg();
    getline(inputFile, buf); // Read the next line
    lineCount++;
    buf = StringUtilities::trimmed(buf);
  }

  inputFile.seekg(currentPos); // Put the file position back before the last read.
  return {block};
}

// -----------------------------------------------------------------------------
Result<ImportVolumeGraphicsFileFilter::HeaderMetadata> ReadHeaderMetaData(const std::string& vgHeaderFilePath)
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
        return ConvertResultTo<ImportVolumeGraphicsFileFilter::HeaderMetadata>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock = result.value();
    }

    else if(buf == k_GeometryBlock)
    {
      auto result = ParseGeometryBlock(vgHeaderFile, lineCount);
      if(result.invalid())
      {
        return ConvertResultTo<ImportVolumeGraphicsFileFilter::HeaderMetadata>(std::move(ConvertResult(std::move(result))), {});
      }
      geomBlock = result.value();
    }
  }

  vgHeaderFile.clear();
  vgHeaderFile.seekg(0);

  ImportVolumeGraphicsFileFilter::HeaderMetadata metadata;
  metadata.Dimensions = fileBlock.Size;
  metadata.Spacing = geomBlock.Resolution;
  if(geomBlock.Unit == k_Millimeter)
  {
    metadata.Units = IGeometry::LengthUnit::Millimeter;
  }

  const fs::path fiHdr = fs::path(vgHeaderFilePath);
  metadata.DataFilePath = (fs::absolute(fiHdr.parent_path()) / fileBlock.Name).string();
  return {metadata};
}

struct ImportVolumeGraphicsFileFilterCache
{
  fs::path VgiDataFilePath;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ImportVolumeGraphicsFileFilterCache> s_HeaderCache;

} // namespace

namespace complex
{
//------------------------------------------------------------------------------
ImportVolumeGraphicsFileFilter::ImportVolumeGraphicsFileFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ImportVolumeGraphicsFileFilter::~ImportVolumeGraphicsFileFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ImportVolumeGraphicsFileFilter::name() const
{
  return FilterTraits<ImportVolumeGraphicsFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportVolumeGraphicsFileFilter::className() const
{
  return FilterTraits<ImportVolumeGraphicsFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportVolumeGraphicsFileFilter::uuid() const
{
  return FilterTraits<ImportVolumeGraphicsFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportVolumeGraphicsFileFilter::humanName() const
{
  return "Import Volume Graphics File (.vgi/.vol)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportVolumeGraphicsFileFilter::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import", "#CT", "#VolumeGraphics", "#vgi"};
}

//------------------------------------------------------------------------------
Parameters ImportVolumeGraphicsFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_VGHeaderFile_Key, "VolumeGraphics .vgi File", "The input VolumeGraphics file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".vgi"}, FileSystemPathParameter::PathType::InputFile));
  params.insertSeparator(Parameters::Separator{"Created Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewImageGeometry_Key, "Image Geometry", "Path to create the Image Geometry", DataPath({"VolumeGraphics"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The attribute matrix created as a child of the image geometry", "CT Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_DensityArrayName_Key, "Density", "The data array created as a child of the attribute matrix", "Density"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportVolumeGraphicsFileFilter::clone() const
{
  return std::make_unique<ImportVolumeGraphicsFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportVolumeGraphicsFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pVGHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_VGHeaderFile_Key);
  auto pNewImageGeometryPathValue = filterArgs.value<DataPath>(k_NewImageGeometry_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_DensityArrayName_Key);

  Result<HeaderMetadata> metadataResult = ReadHeaderMetaData(pVGHeaderFileValue.string());
  if(metadataResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(metadataResult))), {})};
  }

  HeaderMetadata metadata = metadataResult.value();

  complex::Result<OutputActions> resultOutputActions;

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(
      pNewImageGeometryPathValue, CreateImageGeometryAction::DimensionType{metadata.Dimensions[0], metadata.Dimensions[1], metadata.Dimensions[2]}, CreateImageGeometryAction::OriginType{0, 0, 0},
      CreateImageGeometryAction::SpacingType{metadata.Spacing[0], metadata.Spacing[1], metadata.Spacing[2]}, pCellAttributeMatrixNameValue);
  resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));

  const fs::path fiData = fs::path(metadata.DataFilePath);
  s_HeaderCache[m_InstanceId].VgiDataFilePath = fiData; // Store the data file path in the cache

  if(!fs::exists(fiData))
  {
    return {MakeErrorResult<OutputActions>(k_FileDoesNotExist, fmt::format("The Volume Graphics data file does not exist. '{}'", metadata.DataFilePath))};
  }

  const DataPath dap = pNewImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  auto createArrayAction =
      std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{metadata.Dimensions[0], metadata.Dimensions[1], metadata.Dimensions[2]}, std::vector<usize>{1}, dap);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  return {std::move(resultOutputActions), {}};
}

//------------------------------------------------------------------------------
Result<> ImportVolumeGraphicsFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{

  ImportVolumeGraphicsFileInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_NewImageGeometry_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.DensityArrayName = filterArgs.value<std::string>(k_DensityArrayName_Key);
  inputValues.VGDataFile = s_HeaderCache[m_InstanceId].VgiDataFilePath;

  return ImportVolumeGraphicsFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
