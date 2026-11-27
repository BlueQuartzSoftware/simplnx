#include "WriteOnScaleTableFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteOnScaleTableFile.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
// This header declares DataContainerSelectionFilterParameterConverter.
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The directory for the OnScale table file.", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix", "The prefix for the output file.", "SomeString"));
  params.insert(std::make_unique<VectorInt32ParameterType>(k_NumKeypoints_Key, "Number of Keypoints", "The number of keypoints along the X, Y, and Z axes.", VectorInt32ParameterType::ValueType{2, 2, 2},
                                                       VectorInt32ParameterType::NamesType{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputGeometryPath_Key, "Input Geometry", "The Image or Rectilinear Grid Geometry to export.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image, IGeometry::Type::RectGrid}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "The feature ID for each cell.", DataPath{},
      ArraySelectionParameter::AllowedTypeSetType{DataType::int8, DataType::int16, DataType::int32, DataType::int64, DataType::uint8, DataType::uint16, DataType::uint32, DataType::uint64},
      ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_PhaseNamesArrayPath_Key, "Phase Names", "The name for each positive feature ID.", DataPath{}));

  return params;
}

IFilter::VersionType WriteOnScaleTableFileFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointerType WriteOnScaleTableFileFilter::clone() const
{
  return std::make_unique<WriteOnScaleTableFileFilter>();
}

IFilter::PreflightResult WriteOnScaleTableFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  Result<OutputActions> resultOutputActions;
  const auto outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  if(fs::exists(outputPath) && !fs::is_directory(outputPath))
  {
    return MakePreflightErrorResult(-12023, fmt::format("The output path '{}' is not a directory.", outputPath.string()));
  }

  const auto geometryPath = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  const auto& geometry = dataStructure.getDataRefAs<IGridGeometry>(geometryPath);
  const auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  // ArraySelectionParameter also accepts StringArray and NeighborList for numeric selections.
  // Keep the local type check until the parameter validates supported array kinds.
  // TODO(#1833): Revisit this guard after the shared fix preserves generic array support.
  const auto* featureIdsPtr = dataStructure.getDataAs<IDataArray>(featureIdsPath);
  if(featureIdsPtr == nullptr)
  {
    const auto& selectedObject = dataStructure.getDataRefAs<DataObject>(featureIdsPath);
    return MakePreflightErrorResult(-12042, fmt::format("The selected Feature Ids object '{}' has type '{}', but a scalar integer DataArray is required. Select a scalar integer DataArray.",
                                                        featureIdsPath.toString(), selectedObject.getTypeName()));
  }
  auto cellTupleResult = ValidateCellTupleCount(geometry, geometryPath, featureIdsPtr->getNumberOfTuples(), featureIdsPath, -12024);
  if(cellTupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(cellTupleResult), {})};
  }

  const auto phaseNamesPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);
  const auto* phaseNamesPtr = dataStructure.getDataAs<StringArray>(phaseNamesPath);
  if(phaseNamesPtr == nullptr)
  {
    return MakePreflightErrorResult(-12025, fmt::format("The phase names object '{}' is not a StringArray. Select a StringArray that contains the phase names.", phaseNamesPath.toString()));
  }
  if(phaseNamesPtr->getNumberOfTuples() == 0)
  {
    resultOutputActions.warnings().push_back(
        Warning{.code = -12038, .message = fmt::format("The phase names StringArray '{}' has 0 tuples. Every name will be written as 'Phase_<id>'.", phaseNamesPath.toString())});
  }

  const SizeVec3Type dimensions = geometry.getDimensions();
  if(geometry.getGeomType() == IGeometry::Type::RectGrid && (dimensions[0] < dimensions[1] || dimensions[1] < dimensions[2]))
  {
    return MakePreflightErrorResult(-12026,
                                    fmt::format("The Rectilinear Grid Geometry '{}' has dimensions ({}). OnScale export requires X >= Y >= Z. Only Image Geometries can be reordered automatically.",
                                                geometryPath.toString(), string_utilities::FormatDimensions3D(dimensions)));
  }

  return {.output_actions = std::move(resultOutputActions)};
}

Result<> WriteOnScaleTableFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteOnScaleTableFileInputValues inputValues;
  inputValues.output_path = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.file_prefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.num_keypoints = filterArgs.value<VectorInt32ParameterType::ValueType>(k_NumKeypoints_Key);
  inputValues.input_geometry_path = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.phase_names_array_path = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  return WriteOnScaleTableFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputPathKey = "OutputPath";
constexpr StringLiteralType k_OutputFilePrefixKey = "OutputFilePrefix";
constexpr StringLiteralType k_NumKeypointsKey = "NumKeypoints";
constexpr StringLiteralType k_PzflexFeatureIdsArrayPathKey = "PzflexFeatureIdsArrayPath";
constexpr StringLiteralType k_PhaseNamesArrayPathKey = "PhaseNamesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteOnScaleTableFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteOnScaleTableFileFilter().getDefaultArguments();
  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_OutputFilePrefixKey, k_FilePrefix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntVec3FilterParameterConverterType>(args, json, simpl::k_NumKeypointsKey, k_NumKeypoints_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_PzflexFeatureIdsArrayPathKey, k_InputGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_PzflexFeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_PhaseNamesArrayPathKey, k_PhaseNamesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
