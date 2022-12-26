#include "ImportVolumeGraphicsFileFilter.hpp"

#include <fstream>

#include "Algorithms/ImportVolumeGraphicsFile.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
static inline constexpr int32 k_FileDoesNotExist = -91501;
static inline constexpr int32 k_VgiParseError = -91503;
static inline const std::string k_Millimeter("mm");

// File Block
inline const std::string k_file1Block("[file1]");
inline const std::string k_geometryBlock("[geometry]");
// inline const std::string k_representationBlock("[representation]");

inline const std::string k_RegionOfInterestStart("RegionOfInterestStart");
inline const std::string k_RegionOfInterestEnd("RegionOfInterestEnd");

inline const std::string k_FileFormat("FileFormat");
inline const std::string k_Size("Size");
inline const std::string k_Name("Name");
inline const std::string k_Datatype("Datatype");
inline const std::string k_datarange("datarange");
inline const std::string k_BitsPerElement("BitsPerElement");
inline const std::string k_status("status");
inline const std::string k_relativeposition("relativeposition");
inline const std::string k_position("position");
inline const std::string k_resolution("resolution");
inline const std::string k_scale("scale");
inline const std::string k_center("center");
inline const std::string k_rotate("rotate");
inline const std::string k_unit("unit");

struct FileBlock
{
  SizeVec3 RegionOfInterestStart = {0, 0, 0};
  SizeVec3 RegionOfInterestEnd = {0, 0, 0};
  std::string FileFormat = "raw";
  SizeVec3 Size = {0, 0, 0};
  std::string Name = "AS_N610_02.vol";
  std::string Datatype = "float";
  FloatVec2Type datarange = {0.0F, 0.0F};
  int8 BitsPerElement = 32;
};

struct GeometryBlock
{
  std::string status = "visible";
  FloatVec3 relativeposition = {0.0F, 0.0F, 0.0F};
  FloatVec3 position = {0.0F, 0.0F, 0.0F};
  FloatVec3 resolution = {0.0F, 0.0F, 0.0F};
  FloatVec3 scale = {1.0F, 1.0F, 1.0F};
  FloatVec3 center = {0.0F, 0.0F, 0.0F};
  std::array<int32, 9> rotate = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  std::string unit = "mm";
};

Result<SizeVec3> ParseSizeVec3(const std::vector<std::string>& tokens)
{
  SizeVec3 vec = {0, 0, 0};
  std::string vgiParseErrorStr = "Error parsing usize value ({}) from vgi file.";
  std::string vgiOutOfRangeErrorStr = "Value ({}) in vgi file is out of range for storage type usize.";

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

Result<FloatVec2Type> ParseFloatVec2(const std::vector<std::string>& tokens)
{
  FloatVec2Type vec = {0.0F, 0.0F};
  std::string vgiParseErrorStr = "Error parsing floating point value ({}) from vgi file.";
  std::string vgiOutOfRangeErrorStr = "Floating point value ({}) in vgi file is out of range for storage type float32.";

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

Result<FloatVec3> ParseFloatVec3(const std::vector<std::string>& tokens)
{
  FloatVec3 vec = {0.0F, 0.0F, 0.0F};
  std::string vgiParseErrorStr = "Error parsing floating point value ({}) from vgi file.";
  std::string vgiOutOfRangeErrorStr = "Floating point value ({}) in vgi file is out of range for storage type float32.";

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
Result<std::array<T, Dimension>> ParseIntArray(const std::vector<std::string>& tokens)
{
  std::array<T, Dimension> vec;

  for(usize i = 0; i < Dimension; i++)
  {
    try
    {
      vec[i] = std::stof(tokens[i + 2]);
    } catch(const std::invalid_argument& e)
    {
      return {MakeErrorResult<std::array<T, Dimension>>(k_VgiParseError, fmt::format("Error parsing floating point value ({}) from vgi file.", tokens[3]))};
    } catch(const std::out_of_range& e)
    {
      return {MakeErrorResult<std::array<T, Dimension>>(k_VgiParseError, fmt::format("Floating point value ({}) in vgi file is out of range for storage type float32.", tokens[3]))};
    }
  }

  return {vec};
}

Result<FileBlock> ParseFileBlock(std::ifstream& in)
{
  FileBlock fileBlock;
  std::string buf;

  std::streampos currentPos = in.tellg();
  getline(in, buf);
  buf = StringUtilities::trimmed(buf);

  while(buf[0] != '[' && !in.eof())
  {
    std::vector<std::string> tokens = StringUtilities::split(buf, ' ');
    if(tokens[0] == k_RegionOfInterestStart)
    {
      auto result = ParseSizeVec3(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock.RegionOfInterestStart = result.value();
    }
    else if(tokens[0] == k_RegionOfInterestEnd)
    {
      auto result = ParseSizeVec3(tokens);
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
      auto result = ParseSizeVec3(tokens);
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
    else if(tokens[0] == k_Datatype)
    {
      fileBlock.Datatype = tokens[2];
    }
    else if(tokens[0] == k_datarange)
    {
      auto result = ParseFloatVec2(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<FileBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock.datarange = result.value();
    }
    else if(tokens[0] == k_BitsPerElement)
    {
      try
      {
        fileBlock.BitsPerElement = std::stoi(tokens[2]);
      } catch(const std::invalid_argument& e)
      {
        return {MakeErrorResult<FileBlock>(k_VgiParseError, fmt::format("Error parsing 'Bits Per Element' integer value ({}) from vgi file.", tokens[2]))};
      } catch(const std::out_of_range& e)
      {
        return {MakeErrorResult<FileBlock>(k_VgiParseError, fmt::format("'Bits Per Element' integer value ({}) in vgi file is out of range for storage type int8.", tokens[2]))};
      }
    }

    currentPos = in.tellg();
    getline(in, buf); // Read the next line
    buf = StringUtilities::trimmed(buf);
  }
  in.seekg(currentPos); // Put the file position back before the last read.
  return {fileBlock};
}

Result<GeometryBlock> ParseGeometryBlock(std::ifstream& in)
{
  GeometryBlock block;
  std::string buf;

  std::streampos currentPos = in.tellg();
  getline(in, buf);
  buf = StringUtilities::trimmed(buf);
  std::vector<std::string> tokens;

  while(buf[0] != '[' && !in.eof())
  {
    tokens = StringUtilities::split(buf, ' ');
    if(tokens[0] == k_status)
    {
      block.status = tokens[2];
    }
    else if(tokens[0] == k_relativeposition)
    {
      auto result = ParseFloatVec3(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.relativeposition = result.value();
    }
    else if(tokens[0] == k_position)
    {
      auto result = ParseFloatVec3(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.position = result.value();
    }
    else if(tokens[0] == k_resolution)
    {
      auto result = ParseFloatVec3(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.resolution = result.value();
    }
    else if(tokens[0] == k_scale)
    {
      auto result = ParseFloatVec3(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.scale = result.value();
    }
    else if(tokens[0] == k_center)
    {
      auto result = ParseFloatVec3(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.center = result.value();
    }
    else if(tokens[0] == k_rotate)
    {
      auto result = ParseIntArray<int32, 9>(tokens);
      if(result.invalid())
      {
        return ConvertResultTo<GeometryBlock>(std::move(ConvertResult(std::move(result))), {});
      }
      block.rotate = result.value();
    }
    else if(tokens[0] == k_unit)
    {
      block.unit = tokens[2];
    }

    in.tellg();
    getline(in, buf); // Read the next line
    buf = StringUtilities::trimmed(buf);
  }

  in.seekg(currentPos); // Put the file position back before the last read.
  return {block};
}

// -----------------------------------------------------------------------------
Result<ImportVolumeGraphicsFileFilter::HeaderMetadata> ReadHeaderMetaData(const std::string& vgHeaderFilePath)
{
  std::ifstream vgHeaderFile(vgHeaderFilePath);

  std::string buf;
  FileBlock fileBlock;
  GeometryBlock geomBlock;

  while(getline(vgHeaderFile, buf))
  {
    buf = StringUtilities::trimmed(buf);

    if(buf == k_file1Block)
    {
      auto result = ParseFileBlock(vgHeaderFile);
      if(result.invalid())
      {
        return ConvertResultTo<ImportVolumeGraphicsFileFilter::HeaderMetadata>(std::move(ConvertResult(std::move(result))), {});
      }
      fileBlock = result.value();
    }

    else if(buf == k_geometryBlock)
    {
      auto result = ParseGeometryBlock(vgHeaderFile);
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
  metadata.dims = fileBlock.Size;
  metadata.spacing = geomBlock.resolution;
  if(geomBlock.unit == k_Millimeter)
  {
    metadata.units = IGeometry::LengthUnit::Millimeter;
  }

  fs::path fiHdr = fs::path(vgHeaderFilePath);
  metadata.dataFilePath = (fs::absolute(fiHdr.parent_path()) / fileBlock.Name).string();
  return {metadata};
}

struct ImportVolumeGraphicsFileFilterCache
{
  fs::path vgiDataFilePath;
};

static std::atomic_int32_t s_InstanceId = 0;
static std::map<int32, ImportVolumeGraphicsFileFilterCache> s_HeaderCache;

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
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportVolumeGraphicsFileFilter::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<FileSystemPathParameter>(k_VGHeaderFile_Key, "VolumeGraphics .vgi File", "", fs::path("DefaultInputFileName"), FileSystemPathParameter::ExtensionsType{".vgi"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewImageGeometry_Key, "Image Geometry", "Path to create the Image Geometry", DataPath({"VolumeGraphics"})));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
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
      pNewImageGeometryPathValue, CreateImageGeometryAction::DimensionType{metadata.dims[0], metadata.dims[1], metadata.dims[2]}, CreateImageGeometryAction::OriginType{0, 0, 0},
      CreateImageGeometryAction::SpacingType{metadata.spacing[0], metadata.spacing[1], metadata.spacing[2]}, pCellAttributeMatrixNameValue);
  resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));

  fs::path fiData = fs::path(metadata.dataFilePath);
  s_HeaderCache[m_InstanceId].vgiDataFilePath = fiData; // Store the data file path in the cache

  if(!fs::exists(fiData))
  {
    return {MakeErrorResult<OutputActions>(k_FileDoesNotExist, fmt::format("The Volume Graphics data file does not exist. '{}'", metadata.dataFilePath))};
  }

  DataPath dap = pNewImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{metadata.dims[0], metadata.dims[1], metadata.dims[2]}, std::vector<usize>{1}, dap);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  return {std::move(resultOutputActions), {}};
}

//------------------------------------------------------------------------------
Result<> ImportVolumeGraphicsFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{

  ImportVolumeGraphicsFileInputValues inputValues;

  inputValues.VGHeaderFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_VGHeaderFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_NewImageGeometry_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.DensityArrayName = filterArgs.value<std::string>(k_DensityArrayName_Key);
  inputValues.VGDataFile = s_HeaderCache[m_InstanceId].vgiDataFilePath;

  return ImportVolumeGraphicsFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
