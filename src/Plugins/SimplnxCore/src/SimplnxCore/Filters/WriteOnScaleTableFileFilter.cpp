#include "WriteOnScaleTableFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteOnScaleTableFile.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
// This header declares DataContainerSelectionFilterParameterConverter.
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
std::string WriteOnScaleTableFileFilter::name() const
{
  return FilterTraits<WriteOnScaleTableFileFilter>::name.str();
}

std::string WriteOnScaleTableFileFilter::className() const
{
  return FilterTraits<WriteOnScaleTableFileFilter>::className;
}

Uuid WriteOnScaleTableFileFilter::uuid() const
{
  return FilterTraits<WriteOnScaleTableFileFilter>::uuid;
}

std::string WriteOnScaleTableFileFilter::humanName() const
{
  return "Write OnScale Table File";
}

std::vector<std::string> WriteOnScaleTableFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "OnScale", "PZFLEX", "Simulation"};
}

Parameters WriteOnScaleTableFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The existing directory for the OnScale table file.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix", "The prefix for the output file.", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumKeypoints_Key, "Number of Keypoints", "The number of keypoints along the X, Y, and Z axes.", VectorInt32Parameter::ValueType{2, 2, 2},
                                                       VectorInt32Parameter::NamesType{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputGeometryPath_Key, "Input Geometry", "The Image or Rectilinear Grid Geometry to export.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "The feature ID for each cell.", DataPath{},
      ArraySelectionParameter::AllowedTypes{DataType::int8, DataType::int16, DataType::int32, DataType::int64, DataType::uint8, DataType::uint16, DataType::uint32, DataType::uint64},
      ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_PhaseNamesArrayPath_Key, "Phase Names", "The name for each positive feature ID.", DataPath{}));

  return params;
}

IFilter::VersionType WriteOnScaleTableFileFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer WriteOnScaleTableFileFilter::clone() const
{
  return std::make_unique<WriteOnScaleTableFileFilter>();
}

IFilter::PreflightResult WriteOnScaleTableFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  Result<OutputActions> resultOutputActions;
  const auto outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  if(!fs::exists(outputPath))
  {
    return MakePreflightErrorResult(-12022, fmt::format("The output path '{}' does not exist.", outputPath.string()));
  }
  if(!fs::is_directory(outputPath))
  {
    return MakePreflightErrorResult(-12023, fmt::format("The output path '{}' is not a directory.", outputPath.string()));
  }

  const auto geometryPath = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  const auto& geometry = dataStructure.getDataRefAs<IGridGeometry>(geometryPath);
  const auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  const auto& featureIds = dataStructure.getDataRefAs<IDataArray>(featureIdsPath);
  if(featureIds.getNumberOfTuples() != geometry.getNumberOfCells())
  {
    return MakePreflightErrorResult(-12024, fmt::format("The array '{}' has {} tuples, but the grid geometry '{}' has {} cells.", featureIdsPath.toString(), featureIds.getNumberOfTuples(),
                                                        geometryPath.toString(), geometry.getNumberOfCells()));
  }

  const auto phaseNamesPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);
  const auto* phaseNames = dataStructure.getDataAs<StringArray>(phaseNamesPath);
  if(phaseNames == nullptr)
  {
    return MakePreflightErrorResult(-12025, fmt::format("The phase names object '{}' is not a StringArray. Select a StringArray that contains the phase names.", phaseNamesPath.toString()));
  }
  if(phaseNames->getNumberOfTuples() == 0)
  {
    resultOutputActions.warnings().push_back(Warning{-12038, fmt::format("The phase names StringArray '{}' has 0 tuples. Every name will be written as 'Phase_<id>'.", phaseNamesPath.toString())});
  }

  const SizeVec3 dimensions = geometry.getDimensions();
  if(geometry.getGeomType() == IGeometry::Type::RectGrid && !(dimensions[0] >= dimensions[1] && dimensions[1] >= dimensions[2]))
  {
    return MakePreflightErrorResult(-12026,
                                    fmt::format("The Rectilinear Grid Geometry '{}' has dimensions ({}). OnScale export requires X >= Y >= Z. Only Image Geometries can be reordered automatically.",
                                                geometryPath.toString(), StringUtilities::formatDimensions3D(dimensions)));
  }

  return {std::move(resultOutputActions)};
}

Result<> WriteOnScaleTableFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  WriteOnScaleTableFileInputValues inputValues;
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FilePrefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.NumKeypoints = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  inputValues.InputGeometryPath = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.PhaseNamesArrayPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  return WriteOnScaleTableFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputPathKey = "OutputPath";
constexpr StringLiteral k_OutputFilePrefixKey = "OutputFilePrefix";
constexpr StringLiteral k_NumKeypointsKey = "NumKeypoints";
constexpr StringLiteral k_PzflexFeatureIdsArrayPathKey = "PzflexFeatureIdsArrayPath";
constexpr StringLiteral k_PhaseNamesArrayPathKey = "PhaseNamesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteOnScaleTableFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteOnScaleTableFileFilter().getDefaultArguments();
  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_OutputFilePrefixKey, k_FilePrefix_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec3FilterParameterConverter>(args, json, SIMPL::k_NumKeypointsKey, k_NumKeypoints_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_PzflexFeatureIdsArrayPathKey, k_InputGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_PzflexFeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_PhaseNamesArrayPathKey, k_PhaseNamesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
