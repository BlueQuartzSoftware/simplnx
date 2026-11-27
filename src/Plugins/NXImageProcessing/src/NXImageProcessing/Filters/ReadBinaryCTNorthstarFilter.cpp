#include "ReadBinaryCTNorthstarFilter.hpp"

#include "NXImageProcessing/Filters/Algorithms/ReadBinaryCTNorthstar.hpp"

#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <array>
#include <cmath>
#include <fstream>
#include <limits>

using namespace nx::core;

namespace
{
struct ReadBinaryCTNorthstarFilterCache
{
  std::vector<std::pair<fs::path, usize>> data_file_paths;
  SizeVec3 original_geometry_dims;
  SizeVec3 imported_geometry_dims;
  IntVec3 start_voxel_coord;
  IntVec3 end_voxel_coord;
};

std::atomic_int32_t SInstanceId = 0;
std::map<int32, ReadBinaryCTNorthstarFilterCache> SHeaderCache;
constexpr std::array<char, 3> k_Axes = {'X', 'Y', 'Z'};

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
  return StringUtilities::Split(line, delimiter);
}

// -----------------------------------------------------------------------------
std::string GenerateGeometryInfoString(const ReadBinaryCTNorthstarFilter::ImageGeometryInfo& info, const IGeometry::LengthUnit& lengthUnit)
{
  SizeVec3 dims = info.Dimensions;
  FloatVec3 origin = info.Origin;
  FloatVec3 spacing = info.Spacing;
  std::string lengthUnitStr = IGeometry::LengthUnitToString(lengthUnit);
  std::string desc;
  for(usize axis = 0; axis < k_Axes.size(); ++axis)
  {
    const float64 extent = static_cast<float64>(dims[axis]) * spacing[axis];
    const float64 end = static_cast<float64>(origin[axis]) + extent;
    desc.append(fmt::format("{} Range: {} to {:.9g} (Delta: {:.9g} {}) 0-{} Voxels\n", k_Axes[axis], origin[axis], end, extent, lengthUnitStr, dims[axis] - 1));
  }
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
        Result<usize> result = StringInterpretationUtilities::Convert<usize>(tokens[1]);
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
Result<std::pair<std::vector<float32>, std::vector<float32>>> ReadLocation(std::ifstream& inStream, const fs::path& headerFilePath, usize& lineCount, bool readMin, bool readMax)
{
  using LocationType = std::pair<std::vector<float32>, std::vector<float32>>;
  LocationType location{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}};
  constexpr std::array<usize, 3> k_AxisOrder{1, 2, 0};
  for(auto tokens = ParseNextLine(inStream, ' ', lineCount);; tokens = ParseNextLine(inStream, ' ', lineCount))
  {
    if(!tokens.empty() && tokens[0] == "</Location>")
    {
      return {std::move(location)};
    }
    if(tokens.size() < 4 || (tokens[0] != "<Min>" && tokens[0] != "<Max>"))
    {
      return MakeErrorResult<LocationType>(-1,
                                           fmt::format("Unexpected data at line {} in 'Location' section of Binary CT Northstar file '{}'. Expected '<Min>' or '<Max>' followed by three coordinates.",
                                                       lineCount, headerFilePath.string()));
    }

    const bool isMin = tokens[0] == "<Min>";
    if(isMin ? !readMin : !readMax)
    {
      continue;
    }

    auto& coordinates = isMin ? location.first : location.second;
    for(usize i = 0; i < k_AxisOrder.size(); ++i)
    {
      const usize axis = k_AxisOrder[i];
      const auto value = StringInterpretationUtilities::Convert<float32>(tokens[i + 1]);
      if(value.invalid())
      {
        const auto& error = value.errors().front();
        const auto field = fmt::format("{} {} coordinate", isMin ? "minimum" : "maximum", k_Axes[axis]);
        const auto detail = fmt::format("Value '{}' at line {} in the 'Location' section could not be converted to float32.", tokens[i + 1], lineCount);
        const auto message = isMin ? ImageIO::FormatReaderValueError(headerFilePath, field, detail, ImageIO::k_SetOriginLabel, ImageIO::k_SetSpacingLabel) :
                                     ImageIO::FormatReaderValueError(headerFilePath, field, detail, ImageIO::k_SetSpacingLabel);
        return MakeErrorResult<LocationType>(error.code, message);
      }
      coordinates[axis] = value.value();
    }
  }
}

// -----------------------------------------------------------------------------
Result<std::vector<usize>> ReadVoxels(const std::vector<std::string>& tokens, const fs::path& headerFilePath, usize& lineCount)
{
  std::vector<usize> voxels = {0, 0, 0};

  Result<usize> result = StringInterpretationUtilities::Convert<usize>(tokens[1]);
  if(result.invalid())
  {
    const Error& err = result.errors()[0];
    return MakeErrorResult<std::vector<usize>>(
        err.code, fmt::format("Error reading Y coordinate in 'Voxels' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
  }
  voxels[1] = result.value();

  result = StringInterpretationUtilities::Convert<usize>(tokens[2]);
  if(result.invalid())
  {
    const Error& err = result.errors()[0];
    return MakeErrorResult<std::vector<usize>>(
        err.code, fmt::format("Error reading Z coordinate in 'Voxels' section of Binary CT Northstar file '{}' at line {}: {}", headerFilePath.string(), lineCount, err.message));
  }
  voxels[2] = result.value();

  result = StringInterpretationUtilities::Convert<usize>(tokens[3]);
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
Result<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>> ReadHeaderMetaData(const fs::path& headerFilePath,
                                                                                                                              const ImageIO::GeometryOverrides& overrides)
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

  // The minimum defines both origin and spacing. The maximum affects only spacing.
  const bool readMax = !overrides.Spacing.has_value();
  const bool readMin = !overrides.Origin.has_value() || readMax;
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
          return ConvertResultTo<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(ConvertResult(std::move(result)), {});
        }
        voxels = result.value();
      }
      else if(tokens[i] == "<Location>")
      {
        auto result = ReadLocation(inHeaderStream, headerFilePath, lineCount, readMin, readMax);
        if(result.invalid())
        {
          return ConvertResultTo<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(ConvertResult(std::move(result)), {});
        }
        minLocation = result.value().first;
        maxLocation = result.value().second;
      }
      else if(tokens[i] == "<Files>")
      {
        auto result = ReadFileList(inHeaderStream, headerFilePath, lineCount);
        if(result.invalid())
        {
          return ConvertResultTo<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(ConvertResult(std::move(result)), {});
        }
        dataFiles = result.value();
      }
    }
  }

  for(usize i = 0; i < 3; i++)
  {
    if(voxels[i] == 0)
    {
      constexpr int32 k_ZeroVoxelCount = -38741;
      return MakeErrorResult<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(
          k_ZeroVoxelCount, fmt::format("The {} voxel count in '{}' must be greater than zero. Received {}.", k_Axes[i], headerFilePath.string(), voxels[i]));
    }
    if(readMax)
    {
      const float64 extent = static_cast<float64>(maxLocation[i]) - static_cast<float64>(minLocation[i]);
      const float64 spacingValue = extent / static_cast<float64>(voxels[i]);
      if(std::isfinite(spacingValue) && (spacingValue < std::numeric_limits<float32>::lowest() || spacingValue > std::numeric_limits<float32>::max()))
      {
        return MakeErrorResult<std::pair<std::vector<std::pair<fs::path, usize>>, ReadBinaryCTNorthstarFilter::ImageGeometryInfo>>(
            ImageGeom::k_InvalidSpacing,
            ImageIO::FormatReaderValueError(headerFilePath, "spacing", fmt::format("Computed {} spacing ({}) is outside the float32 range.", k_Axes[i], spacingValue), ImageIO::k_SetSpacingLabel));
      }
      spacing[i] = static_cast<float32>(spacingValue);
    }
  }

  imageGeomInfo.Dimensions = voxels;
  imageGeomInfo.Spacing = spacing;
  imageGeomInfo.Origin = minLocation;

  return {std::make_pair(dataFiles, imageGeomInfo)};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadBinaryCTNorthstarFilter::ReadBinaryCTNorthstarFilter()
: m_InstanceId(SInstanceId.fetch_add(1))
{
  SHeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadBinaryCTNorthstarFilter::~ReadBinaryCTNorthstarFilter() noexcept
{
  SHeaderCache.erase(m_InstanceId);
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
  return {className(), "IO", "Input", "Read", "Import", "Northstar", "CT", "Binary"};
}

//------------------------------------------------------------------------------
Parameters ReadBinaryCTNorthstarFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputHeaderFile_Key, "Input Header File", "The path to the .nsihdr file", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{".nsihdr"}, FileSystemPathParameter::PathType::InputFile));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ImportSubvolume_Key, "Import Subvolume", "Import a subvolume instead of the entire volume", false));
  params.insert(
      std::make_unique<VectorInt32ParameterType>(k_StartVoxelCoord_Key, "Starting XYZ Voxel for Subvolume", "The starting subvolume voxel", std::vector<int32>({0, 0, 0}), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorInt32ParameterType>(k_EndVoxelCoord_Key, "Ending XYZ Voxel for Subvolume", "The ending subvolume voxel (inclusive)", std::vector<int32>({1, 1, 1}),
                                                       std::vector<std::string>(3)));

  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry", 0, IGeometry::GetAllLengthUnitStrings()));

  params.insertSeparator(Parameters::Separator{"Geometry Metadata Overrides"});
  ImageIO::InsertGeometryOverrideParameters(params);

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
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
IFilter::VersionType ReadBinaryCTNorthstarFilter::parametersVersion() const
{
  // Version 2: Added Set Origin and Set Spacing.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ReadBinaryCTNorthstarFilter::clone() const
{
  return std::make_unique<ReadBinaryCTNorthstarFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadBinaryCTNorthstarFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pInputHeaderFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_DensityArrayName_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImportSubvolumeValue = filterArgs.value<bool>(k_ImportSubvolume_Key);
  auto pStartVoxelCoordValue = filterArgs.value<VectorInt32ParameterType::ValueType>(k_StartVoxelCoord_Key);
  auto pEndVoxelCoordValue = filterArgs.value<VectorInt32ParameterType::ValueType>(k_EndVoxelCoord_Key);

  if(!fs::exists(pInputHeaderFileValue))
  {
    return {.outputActions = MakeErrorResult<OutputActions>(-38701, "The input header file does not exist")};
  }

  const auto overrides = ImageIO::GeometryOverrides::FromArguments(filterArgs);
  auto result = ReadHeaderMetaData(pInputHeaderFileValue, overrides);
  if(result.invalid())
  {
    return {.outputActions = ConvertInvalidResult<OutputActions>(std::move(result))};
  }

  ImageGeometryInfo geometryInfo = result.value().second;
  if(auto geometryResult = overrides.apply(pInputHeaderFileValue, geometryInfo.Origin, geometryInfo.Spacing); geometryResult.invalid())
  {
    return {.outputActions = ConvertInvalidResult<OutputActions>(std::move(geometryResult))};
  }

  ImageGeometryInfo importedGeometryInfo;
  if(pImportSubvolumeValue)
  {
    importedGeometryInfo.Dimensions = {static_cast<usize>(pEndVoxelCoordValue[0] - pStartVoxelCoordValue[0] + 1), static_cast<usize>(pEndVoxelCoordValue[1] - pStartVoxelCoordValue[1] + 1),
                                       static_cast<usize>(pEndVoxelCoordValue[2] - pStartVoxelCoordValue[2] + 1)};
    importedGeometryInfo.Spacing = geometryInfo.Spacing;
    importedGeometryInfo.Origin = {geometryInfo.Origin[0] + (static_cast<float32>(pStartVoxelCoordValue[0]) * importedGeometryInfo.Spacing[0]),
                                   geometryInfo.Origin[1] + (static_cast<float32>(pStartVoxelCoordValue[1]) * importedGeometryInfo.Spacing[1]),
                                   geometryInfo.Origin[2] + (static_cast<float32>(pStartVoxelCoordValue[2]) * importedGeometryInfo.Spacing[2])};
    SHeaderCache[m_InstanceId].start_voxel_coord = pStartVoxelCoordValue;
    SHeaderCache[m_InstanceId].end_voxel_coord = pEndVoxelCoordValue;
  }
  else
  {
    importedGeometryInfo = geometryInfo;
    SHeaderCache[m_InstanceId].start_voxel_coord = {0, 0, 0};
    SHeaderCache[m_InstanceId].end_voxel_coord = {static_cast<int32_t>(geometryInfo.Dimensions[0] - 1), static_cast<int32_t>(geometryInfo.Dimensions[1] - 1),
                                                  static_cast<int32_t>(geometryInfo.Dimensions[2] - 1)};
  }

  SHeaderCache[m_InstanceId].original_geometry_dims = geometryInfo.Dimensions;
  SHeaderCache[m_InstanceId].imported_geometry_dims = importedGeometryInfo.Dimensions;

  // Sanity check the Start/End Voxels
  if(pImportSubvolumeValue)
  {
    if(pStartVoxelCoordValue[0] > pEndVoxelCoordValue[0])
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38712, fmt::format("Starting X Voxel > Ending X Voxel ({} > {})", pStartVoxelCoordValue[0], pEndVoxelCoordValue[0]))};
    }
    if(pStartVoxelCoordValue[1] > pEndVoxelCoordValue[1])
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38713, fmt::format("Starting Y Voxel > Ending Y Voxel ({} > {})", pStartVoxelCoordValue[1], pEndVoxelCoordValue[1]))};
    }
    if(pStartVoxelCoordValue[2] > pEndVoxelCoordValue[2])
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38714, fmt::format("Starting Z Voxel > Ending Z Voxel ({} > {})", pStartVoxelCoordValue[2], pEndVoxelCoordValue[2]))};
    }

    if(pStartVoxelCoordValue[0] < 0 || pStartVoxelCoordValue[1] < 0 || pStartVoxelCoordValue[2] < 0)
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38715, fmt::format("Start Voxel < ZERO ({}, {}, {})", pStartVoxelCoordValue[0], pStartVoxelCoordValue[1], pStartVoxelCoordValue[2]))};
    }

    std::vector<usize> origDims = geometryInfo.Dimensions;
    if(pEndVoxelCoordValue[0] >= origDims[0])
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38716, fmt::format("Ending X Voxel > Original Volume Dimension ({} >= {})", pEndVoxelCoordValue[0], origDims[0] - 1))};
    }
    if(pEndVoxelCoordValue[1] >= origDims[1])
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38717, fmt::format("Ending Y Voxel > Original Volume Dimension ({} >= {})", pEndVoxelCoordValue[1], origDims[1] - 1))};
    }
    if(pEndVoxelCoordValue[2] >= origDims[2])
    {
      return {.outputActions = MakeErrorResult<OutputActions>(-38718, fmt::format("Ending Z Voxel > Original Volume Dimension ({} >= {})", pEndVoxelCoordValue[2], origDims[2] - 1))};
    }
  }

  std::vector<std::pair<fs::path, usize>> dataFiles = result.value().first;
  for(auto& dataFilePair : dataFiles)
  {
    if(!fs::exists(dataFilePair.first))
    {
      return {.outputActions =
                  MakeErrorResult<OutputActions>(-38704, fmt::format("The input binary CT file: {} could not be found in the path: {}. The input binary CT file name was obtained from the provided "
                                                                     "header.  Please ensure the file is in the same path as the header and the name has not been altered.",
                                                                     dataFilePair.first.string(), pInputHeaderFileValue.parent_path().string()))};
    }
  }

  SHeaderCache[m_InstanceId].data_file_paths = dataFiles;

  // Set the output actions
  nx::core::Result<OutputActions> resultOutputActions;
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
  return {.outputActions = std::move(resultOutputActions), .outputValues = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadBinaryCTNorthstarFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ReadBinaryCTNorthstarInputValues inputValues;

  inputValues.InputHeaderFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_DensityArrayName_Key);
  DataPath densityArrayPath = inputValues.ImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  inputValues.DensityArrayPath = densityArrayPath;

  inputValues.DataFilePaths = SHeaderCache[m_InstanceId].data_file_paths;
  inputValues.OriginalGeometryDims = SHeaderCache[m_InstanceId].original_geometry_dims;
  inputValues.ImportedGeometryDims = SHeaderCache[m_InstanceId].imported_geometry_dims;
  inputValues.ImportSubvolume = filterArgs.value<bool>(k_ImportSubvolume_Key);
  inputValues.StartVoxelCoord = SHeaderCache[m_InstanceId].start_voxel_coord;
  inputValues.EndVoxelCoord = SHeaderCache[m_InstanceId].end_voxel_coord;
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);

  return ReadBinaryCTNorthstar(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_InputHeaderFileKey = "InputHeaderFile";
constexpr StringLiteralType k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteralType k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteralType k_DensityArrayNameKey = "DensityArrayName";
constexpr StringLiteralType k_LengthUnitKey = "LengthUnit";
constexpr StringLiteralType k_VolumeDescriptionKey = "VolumeDescription";
constexpr StringLiteralType k_DataFileInfoKey = "DataFileInfo";
constexpr StringLiteralType k_ImportSubvolumeKey = "ImportSubvolume";
constexpr StringLiteralType k_StartVoxelCoordKey = "StartVoxelCoord";
constexpr StringLiteralType k_EndVoxelCoordKey = "EndVoxelCoord";
constexpr StringLiteralType k_ImportedVolumeDescriptionKey = "ImportedVolumeDescription";
} // namespace simpl
} // namespace

Result<Arguments> ReadBinaryCTNorthstarFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadBinaryCTNorthstarFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::InputFileFilterParameterConverter>(args, json, simpl::k_InputHeaderFileKey, k_InputHeaderFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringToDataPathFilterParameterConverter>(args, json, simpl::k_DataContainerNameKey, k_ImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_DensityArrayNameKey, k_DensityArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_LengthUnitKey, k_LengthUnit_Key));
  // Volume description parameter is not applicable in NX
  // Data file info parameter is not applicable in NX
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_ImportSubvolumeKey, k_ImportSubvolume_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntVec3FilterParameterConverter>(args, json, simpl::k_StartVoxelCoordKey, k_StartVoxelCoord_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntVec3FilterParameterConverter>(args, json, simpl::k_EndVoxelCoordKey, k_EndVoxelCoord_Key));
  // Imported Volume Description parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
