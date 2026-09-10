#include "WriteDAMASKFilesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteDAMASKFiles.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
// SIMPLConversion::DataContainerSelectionFilterParameterConverter requires this parameter header.
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <array>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
std::string WriteDAMASKFilesFilter::name() const
{
  return FilterTraits<WriteDAMASKFilesFilter>::name.str();
}

std::string WriteDAMASKFilesFilter::className() const
{
  return FilterTraits<WriteDAMASKFilesFilter>::className;
}

Uuid WriteDAMASKFilesFilter::uuid() const
{
  return FilterTraits<WriteDAMASKFilesFilter>::uuid;
}

std::string WriteDAMASKFilesFilter::humanName() const
{
  return "Write DAMASK Files";
}

std::vector<std::string> WriteDAMASKFilesFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "DAMASK", "Simulation"};
}

Parameters WriteDAMASKFilesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_DataFormat_Key, "Data Format", "Write one microstructure per cell or per grain.", 0, ChoicesParameter::Choices{"pointwise", "grainwise"}));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The existing directory that receives both DAMASK files.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_GeometryFileName_Key, "Geometry File Name", "Writes <name>.geom and material.config in the output path.", ""));
  params.insert(std::make_unique<Int32Parameter>(k_HomogenizationIndex_Key, "Homogenization Index", "The homogenization index in the geometry header. The index must be 1 or greater.", 1));
  params.insert(std::make_unique<BoolParameter>(k_CompressGeomFile_Key, "Compress Geom File", "Write the pointwise geometry IDs as a compact range.", false));
  params.linkParameters(k_DataFormat_Key, k_CompressGeomFile_Key, std::make_any<ChoicesParameter::ValueType>(0));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Input Image Geometry", "The Image Geometry that defines the DAMASK grid.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "The feature ID for each cell.", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles", "The three Euler angles in radians for each cell.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "The phase ID for each cell.", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

IFilter::VersionType WriteDAMASKFilesFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer WriteDAMASKFilesFilter::clone() const
{
  return std::make_unique<WriteDAMASKFilesFilter>();
}

IFilter::PreflightResult WriteDAMASKFilesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  if(!fs::exists(outputPath))
  {
    return MakePreflightErrorResult(-12070, fmt::format("The output path '{}' does not exist.", outputPath.string()));
  }
  if(!fs::is_directory(outputPath))
  {
    return MakePreflightErrorResult(-12071, fmt::format("The output path '{}' is not a directory.", outputPath.string()));
  }

  const auto geometryFileName = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  if(geometryFileName.empty())
  {
    return MakePreflightErrorResult(-12072, "The Geometry File Name ('') is empty. Specify a name for the .geom file.");
  }

  const auto homogenizationIndex = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  if(homogenizationIndex < 1)
  {
    return MakePreflightErrorResult(-12079, fmt::format("The Homogenization Index ({}) must be 1 or greater. DAMASK numbers homogenization entries from 1.", homogenizationIndex));
  }

  const auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  const usize cellCount = imageGeom.getNumberOfCells();
  if(cellCount == 0)
  {
    return MakePreflightErrorResult(
        -12080, fmt::format("The Image Geometry '{}' has 0 cells. The dimensions are ({}).", imageGeometryPath.toString(), StringUtilities::formatDimensions3D(imageGeom.getDimensions())));
  }
  const std::array<std::pair<StringLiteral, int32>, 3> arrayKeys = {{{k_FeatureIdsArrayPath_Key, -12073}, {k_CellEulerAnglesArrayPath_Key, -12078}, {k_CellPhasesArrayPath_Key, -12077}}};
  for(const auto& [arrayKey, errorCode] : arrayKeys)
  {
    const auto arrayPath = filterArgs.value<DataPath>(arrayKey);
    const auto& array = dataStructure.getDataRefAs<IDataArray>(arrayPath);
    if(array.getNumberOfTuples() != cellCount)
    {
      return MakePreflightErrorResult(
          errorCode, fmt::format("The array '{}' has {} tuples, but the Image Geometry '{}' has {} cells.", arrayPath.toString(), array.getNumberOfTuples(), imageGeometryPath.toString(), cellCount));
    }
  }

  return {};
}

Result<> WriteDAMASKFilesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  WriteDAMASKFilesInputValues inputValues;
  inputValues.DataFormat = filterArgs.value<ChoicesParameter::ValueType>(k_DataFormat_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.GeometryFileName = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  inputValues.HomogenizationIndex = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  inputValues.CompressGeomFile = filterArgs.value<bool>(k_CompressGeomFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  return WriteDAMASKFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DataFormatKey = "DataFormat";
constexpr StringLiteral k_OutputPathKey = "OutputPath";
constexpr StringLiteral k_GeometryFileNameKey = "GeometryFileName";
constexpr StringLiteral k_HomogenizationIndexKey = "HomogenizationIndex";
constexpr StringLiteral k_CompressGeomFileKey = "CompressGeomFile";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteDAMASKFilesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteDAMASKFilesFilter().getDefaultArguments();
  std::vector<Result<>> results;

  if(json.contains(SIMPL::k_DataFormatKey))
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DataFormatKey, k_DataFormat_Key));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_GeometryFileNameKey, k_GeometryFileName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_HomogenizationIndexKey, k_HomogenizationIndex_Key));
  if(json.contains(SIMPL::k_CompressGeomFileKey))
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_CompressGeomFileKey, k_CompressGeomFile_Key));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
