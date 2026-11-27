#include "WriteAbaqusHexahedronFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteAbaqusHexahedron.hpp"
#include "SimplnxCore/utils/AbaqusElementSets.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteAbaqusHexahedronFilter::name() const
{
  return FilterTraits<WriteAbaqusHexahedronFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteAbaqusHexahedronFilter::className() const
{
  return FilterTraits<WriteAbaqusHexahedronFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteAbaqusHexahedronFilter::uuid() const
{
  return FilterTraits<WriteAbaqusHexahedronFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteAbaqusHexahedronFilter::humanName() const
{
  return "Write Abaqus Hexahedron";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteAbaqusHexahedronFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteAbaqusHexahedronFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(
      std::make_unique<BoolParameter>(k_WriteDummyNode_Key, "Write Dummy Node", "When true writes a dummy node used for stress - strain curves as the last node in the `_.nodes.inp` file.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseReducedIntegration_Key, "Use Reduced Integration Elements",
                                                                 "When true, writes C3D8R elements and includes an hourglass stiffness value for each grain section.", false));
  params.insert(
      std::make_unique<Int32ParameterType>(k_HourglassStiffness_Key, "Hourglass Stiffness Value", "The hourglass stiffness for C3D8R elements, in the model's stress units such as Pa or MPa.", 250));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "The name of the job", "SomeString"));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix",
                                                  "The prefix for each output file. It must be a nonempty plain file name without whitespace or path separators, and cannot be '.' or '..'.",
                                                  "SomeString"));
  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Selected Image Geometry", "The input Image Geometry that will be written.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.linkParameters(k_UseReducedIntegration_Key, k_HourglassStiffness_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteAbaqusHexahedronFilter::parametersVersion() const
{
  return 4;

  // Version 2 adds k_WriteDummyNode_Key. The default value preserves the legacy dummy node output.

  // Version 3 adds k_UseReducedIntegration_Key. The default value writes standard C3D8 elements without hourglass stiffness.

  // Version 4 changes the k_WriteDummyNode_Key default to false. Version 1 pipelines, which lack the key, now omit the dummy node.
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteAbaqusHexahedronFilter::clone() const
{
  return std::make_unique<WriteAbaqusHexahedronFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteAbaqusHexahedronFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto geometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto inputArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(geometryPath);
  if(auto sizeResult = abaqus_element_sets::ValidateMeshSize(imageGeom.getDimensions()); sizeResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(sizeResult), {})};
  }

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, geometryPath, {inputArrayPath}, -1124); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  const auto prefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  if(auto prefixResult = abaqus_element_sets::ValidateFilePrefix(prefix, -1122); prefixResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(prefixResult), {})};
  }

  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);

  // Check Output Path
  if(fs::exists(pOutputPathValue) && !fs::is_directory(pOutputPathValue))
  {
    return MakePreflightErrorResult(-1112, "The supplied directory path isn't a directory");
  }

  if(auto targetsResult = abaqus_element_sets::ValidateOutputTargets(pOutputPathValue, prefix, -1123); targetsResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(targetsResult), {})};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteAbaqusHexahedronFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteAbaqusHexahedronInputValues inputValues;

  inputValues.use_reduced_integration = filterArgs.value<bool>(k_UseReducedIntegration_Key);
  inputValues.hourglass_stiffness = filterArgs.value<int32>(k_HourglassStiffness_Key);
  inputValues.job_name = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.output_path = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.file_prefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.write_dummy_node = filterArgs.value<bool>(k_WriteDummyNode_Key);

  return WriteAbaqusHexahedron(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_HourglassStiffnessKey = "HourglassStiffness";
constexpr StringLiteralType k_JobNameKey = "JobName";
constexpr StringLiteralType k_OutputPathKey = "OutputPath";
constexpr StringLiteralType k_FilePrefixKey = "FilePrefix";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteAbaqusHexahedronFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAbaqusHexahedronFilter().getDefaultArguments();
  args.insertOrAssign(k_WriteDummyNode_Key, std::make_any<bool>(true)); // Legacy SIMPL always wrote the dummy node.

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_HourglassStiffnessKey, k_HourglassStiffness_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_JobNameKey, k_JobName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_FilePrefixKey, k_FilePrefix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
