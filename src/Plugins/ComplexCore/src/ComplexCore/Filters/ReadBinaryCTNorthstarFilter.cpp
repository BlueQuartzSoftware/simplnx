#include "ReadBinaryCTNorthstarFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ReadBinaryCTNorthstar.hpp"

#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include <fstream>

using namespace complex;

namespace
{
struct ReadBinaryCTNorthstarFilterCache
{
  std::vector<std::pair<fs::path, usize>> DataFilePaths;
  SizeVec3 OriginalGeometryDims;
  SizeVec3 ImportedGeometryDims;
  IntVec3 StartVoxelCoord;
  IntVec3 EndVoxelCoord;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ReadBinaryCTNorthstarFilterCache> s_HeaderCache;

// -----------------------------------------------------------------------------
std::string GetNextLine(std::ifstream& inStream, usize& lineCount)
{
  std::string buffer;
  getline(inStream, buffer);
  lineCount++;
  buffer = StringUtilities::trimmed(buffer);
  return buffer;
}

// -----------------------------------------------------------------------------
std::vector<std::string> ParseNextLine(std::ifstream& inStream, char delimiter, usize& lineCount)
{
  std::string line = GetNextLine(inStream, lineCount);
  return StringUtilities::split(line, delimiter);
}

// -----------------------------------------------------------------------------
std::string GenerateGeometryInfoString(const ReadBinaryCTNorthstarFilter::ImageGeometryInfo& info, const IGeometry::LengthUnit& lengthUnit)
{
  SizeVec3 dims = info.Dimensions;
  FloatVec3 origin = info.Origin;
  FloatVec3 spacing = info.Spacing;
  std::string lengthUnitStr = IGeometry::LengthUnitToString(lengthUnit);
  std::string desc = fmt::format("X Range: {} to {} (Delta: {} {}) 0-{} Voxels\n", origin[0], origin[0] + (static_cast<float32>(dims[0]) * spacing[0]), static_cast<float32>(dims[0]) * spacing[0],
                                 lengthUnitStr, dims[0] - 1);
  desc.append(fmt::format("Y Range: {} to {} (Delta: {} {}) 0-{} Voxels\n", origin[1], origin[1] + (static_cast<float32>(dims[1]) * spacing[1]), static_cast<float32>(dims[1]) * spacing[1],
                          lengthUnitStr, dims[1] - 1));
  desc.append(fmt::format("Z Range: {} to {} (Delta: {} {}) 0-{} Voxels\n", origin[2], origin[2] + (static_cast<float32>(dims[2]) * spacing[2]), static_cast<float32>(dims[2]) * spacing[2],
                          lengthUnitStr, dims[2] - 1));
  return desc;
}

// -----------------------------------------------------------------------------
std::string GenerateDataFileListInfoString(const std::vector<std::pair<fs::path, usize>>& dataFiles)
{
  std::string desc;
  for(const auto& dataFileInput : dataFiles)
  {
    desc.append(dataFileInput.first.filename().string());
    if(fs::exists(dataFileInput.first))
    {
      desc.append(" [ok]\n");
    }
    else
    {
      desc.append(" [!]\n");
    }
  }
  return desc;
}

// -----------------------------------------------------------------------------
Result<std::vector<std::pair<fs::path, usize>>> ReadFileList(std::ifstream& inStream, const fs::path& headerFilePath, usize& lineCount)
{
  std::vector<std::pair<fs::path, usize>> fileList;

  std::string line = GetNextLine(inStream, lineCount);
  while(!StringUtilities::starts_with(line, "</Files>"))
  {
    if(StringUtilities::starts_with(line, "<Name>"))
    {
      std::pair<fs::path, usize> filePair;

      filePair.first = headerFilePath.parent_path() / fs::path(StringUtilities::replace(line, "<Name>", ""));

      std::vector<std::string> tokens = ParseNextLine(inStream, ' ', lineCount);
      if(tokens.size() != 2)
      {
        return MakeErrorResult<std::vector<std::pair<fs::path, usize>>>(
            -3000, fmt::format("Expecting NbSlices data in the 'Files' section of Binary CT Northstar file '{}': {} tokens on line {} when {} tokens were expected.", headerFilePath.string(),
                               tokens.size(), lineCount, 2));
      }

      if(tokens[0] == "<NbSlices>")
      {
        Result<usize> result = ConvertTo<usize>::convert(tokens[1]);
        if(result.invalid())
        {
          const Error& err = result.errors()[0];
          return MakeErrorResult<std::vector<std::pair<fs::path, usize>>>(
              err.code, fmt::format("Error reading NbSlices data in 'Files' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
        }
        filePair.second = result.value();
      }
      else
      {
        return MakeErrorResult<std::vector<std::pair<fs::path, usize>>>(
            -3001, fmt::format("Expecting NbSlices data in the 'Files' section of Binary CT Northstar file '{}': line {} does not start with 'NbSlices' tag.", headerFilePath.string(), lineCount));
      }

      fileList.push_back(filePair);
    }

    line = GetNextLine(inStream, lineCount);
  }

  return {fileList};
}

// -----------------------------------------------------------------------------
Result<std::pair<std::vector<float32>, std::vector<float32>>> ReadLocation(std::ifstream& inStream, const fs::path& headerFilePath, usize& lineCount)
{
  std::vector<float32> minLocation = {0.0f, 0.0f, 0.0f};
  std::vector<float32> maxLocation = {0.0f, 0.0f, 0.0f};
  std::string buffer;

  std::vector<std::string> tokens = ParseNextLine(inStream, ' ', lineCount);
  while(tokens[0] != "</Location>")
  {
    if(tokens[0] == "<Min>")
    {
      Result<float32> result = ConvertTo<float32>::convert(tokens[1]);
      if(result.invalid())
      {
        const Error& err = result.errors()[0];
        return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
            err.code, fmt::format("Error reading minimum Y coordinate in 'Location' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
      }
      minLocation[1] = result.value();

      result = ConvertTo<float32>::convert(tokens[2]);
      if(result.invalid())
      {
        const Error& err = result.errors()[0];
        return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
            err.code, fmt::format("Error reading minimum Z coordinate in 'Location' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
      }
      minLocation[2] = result.value();

      result = ConvertTo<float32>::convert(tokens[3]);
      if(result.invalid())
      {
        const Error& err = result.errors()[0];
        return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
            err.code, fmt::format("Error reading minimum X coordinate in 'Location' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
      }
      minLocation[0] = result.value();
    }
    else if(tokens[0] == "<Max>")
    {
      Result<float32> result = ConvertTo<float32>::convert(tokens[1]);
      if(result.invalid())
      {
        const Error& err = result.errors()[0];
        return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
            err.code, fmt::format("Error reading maximum Y coordinate in 'Location' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
      }
      maxLocation[1] = result.value();

      result = ConvertTo<float32>::convert(tokens[2]);
      if(result.invalid())
      {
        const Error& err = result.errors()[0];
        return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
            err.code, fmt::format("Error reading maximum Z coordinate in 'Location' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
      }
      maxLocation[2] = result.value();

      result = ConvertTo<float32>::convert(tokens[3]);
      if(result.invalid())
      {
        const Error& err = result.errors()[0];
        return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
            err.code, fmt::format("Error reading maximum X coordinate in 'Location' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
      }
      maxLocation[0] = result.value();
    }
    else
    {
      return MakeErrorResult<std::pair<std::vector<float32>, std::vector<float32>>>(
          -1, fmt::format("Unexpected data at line {} in 'Location' section of Binary CT Northstar file '{}'.  Expecting only 'Min' and 'Max' data fields.", lineCount, headerFilePath.string()));
    }

    tokens = ParseNextLine(inStream, ' ', lineCount);
  }

  return {std::make_pair(minLocation, maxLocation)};
}

// -----------------------------------------------------------------------------
Result<std::vector<usize>> ReadVoxels(const std::vector<std::string>& tokens, const fs::path& headerFilePath, usize& lineCount)
{
  std::vector<usize> voxels = {0, 0, 0};

  Result<usize> result = ConvertTo<usize>::convert(tokens[1]);
  if(result.invalid())
  {
    const Error& err = result.errors()[0];
    return MakeErrorResult<std::vector<usize>>(
        err.code, fmt::format("Error reading Y coordinate in 'Voxels' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
  }
  voxels[1] = result.value();

  result = ConvertTo<usize>::convert(tokens[2]);
  if(result.invalid())
  {
    const Error& err = result.errors()[0];
    return MakeErrorResult<std::vector<usize>>(
        err.code, fmt::format("Error reading Z coordinate in 'Voxels' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
  }
  voxels[2] = result.value();

  result = ConvertTo<usize>::convert(tokens[3]);
  if(result.invalid())
  {
    const Error& err = result.errors()[0];
    return MakeErrorResult<std::vector<usize>>(
        err.code, fmt::format("Error reading X coordinate in 'Voxels' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
  }
  voxels[0] = result.value();

  return {voxels};
}

// -----------------------------------------------------------------------------
Result<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>> ReadHeaderMetaData(const fs::path& headerFilePath)
{
  ReadBinaryCTNorthstarFilter::ImageGeometryInfo imageGeomInfo;

  std::string buffer;
  std::string trimmedBuffer;
  std::vector<std::string> tokens;

  std::vector<usize> voxels = {0, 0, 0};
  std::vector<float32> maxLocation = {0.0f, 0.0f, 0.0f};
  std::vector<float32> minLocation = {0.0f, 0.0f, 0.0f};
  std::vector<float32> spacing = {0.0f, 0.0f, 0.0f};

  std::ifstream inHeaderStream(headerFilePath.string());
  usize lineCount = 0;

  std::vector<std::pair<fs::path, usize>> dataFiles;
  while(!inHeaderStream.eof())
  {
    tokens = ParseNextLine(inHeaderStream, ' ', lineCount);
    for(usize i = 0; i < tokens.size(); i++)
    {
      if(tokens[i] == "<Voxels>")
      {
        Result<std::vector<usize>> result = ReadVoxels(tokens, headerFilePath, lineCount);
        if(result.invalid())
        {
          return ConvertResultTo<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(std::move(ConvertResult(std::move(result))), {});
        }
        voxels = result.value();
      }
      else if(tokens[i] == "<Location>")
      {
        auto result = ReadLocation(inHeaderStream, headerFilePath, lineCount);
        if(result.invalid())
        {
          return ConvertResultTo<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(std::move(ConvertResult(std::move(result))), {});
        }
        minLocation = result.value().first;
        maxLocation = result.value().second;
      }
      else if(tokens[i] == "<Files>")
      {
        auto result = ReadFileList(inHeaderStream, headerFilePath, lineCount);
        if(result.invalid())
        {
          return ConvertResultTo<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(std::move(ConvertResult(std::move(result))), {});
        }
        dataFiles = result.value();
      }
    }
  }

  for(usize i = 0; i < 3; i++)
  {
    spacing[i] = (maxLocation[i] - minLocation[i]) / static_cast<float32>(voxels[i]);
  }

  imageGeomInfo.Dimensions = voxels;
  imageGeomInfo.Spacing = spacing;
  imageGeomInfo.Origin = minLocation;

  return {std::make_pair(dataFiles, imageGeomInfo)};
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
ReadBinaryCTNorthstarFilter::ReadBinaryCTNorthstarFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadBinaryCTNorthstarFilter::~ReadBinaryCTNorthstarFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ReadBinaryCTNorthstarFilter::name() const
{
  return FilterTraits<ReadBinaryCTNorthstarFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadBinaryCTNorthstarFilter::className() const
{
  return FilterTraits<ReadBinaryCTNorthstarFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadBinaryCTNorthstarFilter::uuid() const
{
  return FilterTraits<ReadBinaryCTNorthstarFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadBinaryCTNorthstarFilter::humanName() const
{
  return "Read North Star Imaging CT (.nsihdr/.nsidat)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadBinaryCTNorthstarFilter::defaultTags() const
{
  return {className(), "#IO", "#Input", "#Read", "#Import", "#Northstar", "#CT", "#Binary"};
}

//------------------------------------------------------------------------------
Parameters ReadBinaryCTNorthstarFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputHeaderFile_Key, "Input Header File", "The path to the .nsihdr file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".nsihdr"}, FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ImportSubvolume_Key, "Import Subvolume", "Import a subvolume instead of the entire volume", false));
  params.insert(
      std::make_unique<VectorInt32Parameter>(k_StartVoxelCoord_Key, "Starting XYZ Voxel for Subvolume", "The starting subvolume voxel", std::vector<int32>({0, 0, 0}), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_EndVoxelCoord_Key, "Ending XYZ Voxel for Subvolume", "The ending subvolume voxel (inclusive)", std::vector<int32>({1, 1, 1}),
                                                       std::vector<std::string>(3)));

  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry", 0, IGeometry::GetAllLengthUnitStrings()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Image Geometry Path", "The path that will be used to create the Image Geometry.", DataPath{{"CT Image Geometry"}}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "The name used to create the Cell Attribute Matrix.", "CT Scan Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_DensityArrayName_Key, "Density Array", "The name used to create the Density data array.", "Density"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ImportSubvolume_Key, k_StartVoxelCoord_Key, true);
  params.linkParameters(k_ImportSubvolume_Key, k_EndVoxelCoord_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadBinaryCTNorthstarFilter::clone() const
{
  return std::make_unique<ReadBinaryCTNorthstarFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadBinaryCTNorthstarFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);

  if(!fs::exists(pInputHeaderFileValue))
  {
    return {MakeErrorResult<OutputActions>(-38701, "The input header file does not exist")};
  }

  Result<std::pair<std::vector<std::pair<fs::path, usize>>, ImageGeometryInfo>> result = ReadHeaderMetaData(pInputHeaderFileValue);
  if(result.invalid())
  {
    const Error& err = result.errors()[0];
    return {MakeErrorResult<OutputActions>(err.code, fmt::format("Error reading input header file '{}': {}", pInputHeaderFileValue.string(), err.message))};
  }

  ImageGeometryInfo geometryInfo = result.value().second;

  ImageGeometryInfo importedGeometryInfo;
  if(pImportSubvolumeValue)
  {
    importedGeometryInfo.Dimensions = {static_cast<usize>(pEndVoxelCoordValue[0] - pStartVoxelCoordValue[0] + 1), static_cast<usize>(pEndVoxelCoordValue[1] - pStartVoxelCoordValue[1] + 1),
                                       static_cast<usize>(pEndVoxelCoordValue[2] - pStartVoxelCoordValue[2] + 1)};
    importedGeometryInfo.Spacing = geometryInfo.Spacing;
    importedGeometryInfo.Origin = {geometryInfo.Origin[0] + (static_cast<float32>(pStartVoxelCoordValue[0]) * importedGeometryInfo.Spacing[0]),
                                   geometryInfo.Origin[1] + (static_cast<float32>(pStartVoxelCoordValue[1]) * importedGeometryInfo.Spacing[1]),
                                   geometryInfo.Origin[2] + (static_cast<float32>(pStartVoxelCoordValue[2]) * importedGeometryInfo.Spacing[2])};
    s_HeaderCache[m_InstanceId].StartVoxelCoord = pStartVoxelCoordValue;
    s_HeaderCache[m_InstanceId].EndVoxelCoord = pEndVoxelCoordValue;
  }
  else
  {
    importedGeometryInfo = geometryInfo;
    s_HeaderCache[m_InstanceId].StartVoxelCoord = {0, 0, 0};
    s_HeaderCache[m_InstanceId].EndVoxelCoord = {static_cast<int32_t>(geometryInfo.Dimensions[0] - 1), static_cast<int32_t>(geometryInfo.Dimensions[1] - 1),
                                                 static_cast<int32_t>(geometryInfo.Dimensions[2] - 1)};
  }

  s_HeaderCache[m_InstanceId].OriginalGeometryDims = geometryInfo.Dimensions;
  s_HeaderCache[m_InstanceId].ImportedGeometryDims = importedGeometryInfo.Dimensions;

  // Sanity check the Start/End Voxels
  if(pImportSubvolumeValue)
  {
    if(pStartVoxelCoordValue[0] > pEndVoxelCoordValue[0])
    {
      return {MakeErrorResult<OutputActions>(-38712, fmt::format("Starting X Voxel > Ending X Voxel ({} > {})", pStartVoxelCoordValue[0], pEndVoxelCoordValue[0]))};
    }
    if(pStartVoxelCoordValue[1] > pEndVoxelCoordValue[1])
    {
      return {MakeErrorResult<OutputActions>(-38713, fmt::format("Starting Y Voxel > Ending Y Voxel ({} > {})", pStartVoxelCoordValue[1], pEndVoxelCoordValue[1]))};
    }
    if(pStartVoxelCoordValue[2] > pEndVoxelCoordValue[2])
    {
      return {MakeErrorResult<OutputActions>(-38714, fmt::format("Starting Z Voxel > Ending Z Voxel ({} > {})", pStartVoxelCoordValue[2], pEndVoxelCoordValue[2]))};
    }

    if(pStartVoxelCoordValue[0] < 0 || pStartVoxelCoordValue[1] < 0 || pStartVoxelCoordValue[2] < 0)
    {
      return {MakeErrorResult<OutputActions>(-38715, fmt::format("Start Voxel < ZERO ({}, {}, {})", pStartVoxelCoordValue[0], pStartVoxelCoordValue[1], pStartVoxelCoordValue[2]))};
    }

    std::vector<usize> origDims = geometryInfo.Dimensions;
    if(pEndVoxelCoordValue[0] >= origDims[0])
    {
      return {MakeErrorResult<OutputActions>(-38716, fmt::format("Ending X Voxel > Original Volume Dimension ({} >= {})", pEndVoxelCoordValue[0], origDims[0] - 1))};
    }
    if(pEndVoxelCoordValue[1] >= origDims[1])
    {
      return {MakeErrorResult<OutputActions>(-38717, fmt::format("Ending Y Voxel > Original Volume Dimension ({} >= {})", pEndVoxelCoordValue[1], origDims[1] - 1))};
    }
    if(pEndVoxelCoordValue[2] >= origDims[2])
    {
      return {MakeErrorResult<OutputActions>(-38718, fmt::format("Ending Z Voxel > Original Volume Dimension ({} >= {})", pEndVoxelCoordValue[2], origDims[2] - 1))};
    }
  }

  std::vector<std::pair<fs::path, usize>> dataFiles = result.value().first;
  for(auto& dataFilePair : dataFiles)
  {
    if(!fs::exists(dataFilePair.first))
    {
      return {MakeErrorResult<OutputActions>(-38704, fmt::format("The input binary CT file: {} could not be found in the path: {}. The input binary CT file name was obtained from the provided "
                                                                 "header.  Please ensure the file is in the same path as the header and the name has not been altered.",
                                                                 dataFilePair.first.string(), pInputHeaderFileValue.parent_path().string()))};
    }
  }

  s_HeaderCache[m_InstanceId].DataFilePaths = dataFiles;

  // Set the output actions
  complex::Result<OutputActions> resultOutputActions;
  auto createImageGeometryAction =
      std::make_unique<CreateImageGeometryAction>(pImageGeometryPathValue, importedGeometryInfo.Dimensions, importedGeometryInfo.Origin, importedGeometryInfo.Spacing, pCellAttributeMatrixNameValue);
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  DataPath densityArrayPath = pImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(
      DataType::float32, std::vector<usize>{importedGeometryInfo.Dimensions[2], importedGeometryInfo.Dimensions[1], importedGeometryInfo.Dimensions[0]}, std::vector<usize>{1}, densityArrayPath));

  // Set the preflight updated values
  std::string volumeDescription = GenerateGeometryInfoString(geometryInfo, static_cast<IGeometry::LengthUnit>(pLengthUnitValue));
  std::string dataFileInfo = GenerateDataFileListInfoString(dataFiles);

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Original Volume", volumeDescription});
  preflightUpdatedValues.push_back({"Data Files", dataFileInfo});

  if(pImportSubvolumeValue)
  {
    std::string importedVolumeDescription = GenerateGeometryInfoString(importedGeometryInfo, static_cast<IGeometry::LengthUnit>(pLengthUnitValue));
    preflightUpdatedValues.push_back({"Subvolume", importedVolumeDescription});
  }

  // Return output actions and preflight updated values
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadBinaryCTNorthstarFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ReadBinaryCTNorthstarInputValues inputValues;

  inputValues.InputHeaderFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_DensityArrayName_Key);
  DataPath densityArrayPath = inputValues.ImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  inputValues.DensityArrayPath = densityArrayPath;

  inputValues.DataFilePaths = s_HeaderCache[m_InstanceId].DataFilePaths;
  inputValues.OriginalGeometryDims = s_HeaderCache[m_InstanceId].OriginalGeometryDims;
  inputValues.ImportedGeometryDims = s_HeaderCache[m_InstanceId].ImportedGeometryDims;
  inputValues.ImportSubvolume = filterArgs.value<bool>(k_ImportSubvolume_Key);
  inputValues.StartVoxelCoord = s_HeaderCache[m_InstanceId].StartVoxelCoord;
  inputValues.EndVoxelCoord = s_HeaderCache[m_InstanceId].EndVoxelCoord;
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);

  return ReadBinaryCTNorthstar(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputHeaderFileKey = "InputHeaderFile";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_DensityArrayNameKey = "DensityArrayName";
constexpr StringLiteral k_LengthUnitKey = "LengthUnit";
constexpr StringLiteral k_VolumeDescriptionKey = "VolumeDescription";
constexpr StringLiteral k_DataFileInfoKey = "DataFileInfo";
constexpr StringLiteral k_ImportSubvolumeKey = "ImportSubvolume";
constexpr StringLiteral k_StartVoxelCoordKey = "StartVoxelCoord";
constexpr StringLiteral k_EndVoxelCoordKey = "EndVoxelCoord";
constexpr StringLiteral k_ImportedVolumeDescriptionKey = "ImportedVolumeDescription";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadBinaryCTNorthstarFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadBinaryCTNorthstarFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputHeaderFileKey, k_InputHeaderFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_DensityArrayNameKey, k_DensityArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_LengthUnitKey, k_LengthUnit_Key));
  // Volume description parameter is not applicable in NX
  // Data file info parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ImportSubvolumeKey, k_ImportSubvolume_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec3FilterParameterConverter>(args, json, SIMPL::k_StartVoxelCoordKey, k_StartVoxelCoord_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec3FilterParameterConverter>(args, json, SIMPL::k_EndVoxelCoordKey, k_EndVoxelCoord_Key));
  // Imported Volume Description parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
