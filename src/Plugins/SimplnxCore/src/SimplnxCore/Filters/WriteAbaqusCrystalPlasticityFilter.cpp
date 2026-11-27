#include "WriteAbaqusCrystalPlasticityFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteAbaqusCrystalPlasticity.hpp"
#include "SimplnxCore/utils/AbaqusElementSets.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
// SIMPLConversion::DataContainerSelectionFilterParameterConverter requires this parameter header.
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <array>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
std::string WriteAbaqusCrystalPlasticityFilter::name() const
{
  return FilterTraits<WriteAbaqusCrystalPlasticityFilter>::name.str();
}

std::string WriteAbaqusCrystalPlasticityFilter::className() const
{
  return FilterTraits<WriteAbaqusCrystalPlasticityFilter>::className;
}

Uuid WriteAbaqusCrystalPlasticityFilter::uuid() const
{
  return FilterTraits<WriteAbaqusCrystalPlasticityFilter>::uuid;
}

std::string WriteAbaqusCrystalPlasticityFilter::humanName() const
{
  return "Write Abaqus Crystal Plasticity File";
}

std::vector<std::string> WriteAbaqusCrystalPlasticityFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Abaqus", "Crystal Plasticity", "Simulation"};
}

Parameters WriteAbaqusCrystalPlasticityFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The directory for the Abaqus input files.", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix",
                                                  "The prefix for each output file. It must be a nonempty plain file name without whitespace or path separators, and cannot be '.' or '..'.",
                                                  "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "The Abaqus job name.", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseReducedIntegration_Key, "Use Reduced Integration Elements",
                                                                 "When true, writes C3D8R elements and includes an hourglass stiffness value for each grain section.", false));
  params.insert(
      std::make_unique<Int32ParameterType>(k_HourglassStiffness_Key, "Hourglass Stiffness Value", "The hourglass stiffness for C3D8R elements, in the model's stress units such as Pa or MPa.", 250));
  params.insert(std::make_unique<BoolParameter>(k_WriteEulerAnglesInDegrees_Key, "Write Euler Angles in Degrees",
                                                "When true, converts the Euler angles from radians to degrees before writing them. When false, writes them in radians.", false));
  params.insert(std::make_unique<Int32ParameterType>(k_NumDepvar_Key, "Number of Solution Dependent State Variables", "The number of solution-dependent state variables.", 1));
  params.insert(std::make_unique<Int32ParameterType>(k_NumUserOutVar_Key, "Number of User Output Variables", "The number of user output variables.", 1));

  params.insertSeparator(Parameters::Separator{"Material Constant Layout"});
  params.insert(std::make_unique<BoolParameter>(k_UseUnsymmetricSolver_Key, "Use Unsymmetric Solver",
                                                "When true, appends 'unsymm' to each *User Material line so Abaqus uses the unsymmetric equation solver, which is recommended for crystal plasticity.",
                                                true));
  params.insert(
      std::make_unique<BoolParameter>(k_IncludeGrainAndPhaseIds_Key, "Include Grain and Phase IDs as Constants", "When true, the grain ID and phase ID are written as constants 1 and 2.", true));
  params.insert(std::make_unique<Int32ParameterType>(k_EulerAnglesStartIndex_Key, "Euler Angles Start Index",
                                                     "The 1-based position of the first Euler angle in the *User Material constant list. The three angles occupy this position and the next two.", 3));

  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(1, 6, "User Constant {}"));
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Values"}));
  const DynamicTableInfo::TableDataType defaultTable(6, DynamicTableInfo::RowType(1, 0.0));
  params.insert(std::make_unique<DynamicTableParameter>(
      k_MaterialConstants_Key, "Material Constants",
      "The user constants in table order. Grain and phase IDs, when included, occupy constants 1 and 2. The three Euler angles are inserted at Euler Angles Start Index "
      "(radians unless Write Euler Angles in Degrees is enabled). The User Constant number is the table row. Its final position in the *User Material list depends on "
      "Include Grain and Phase IDs as Constants and Euler Angles Start Index, as described in the documentation.",
      defaultTable, tableInfo));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Selected Image Geometry", "The Image Geometry that defines the voxel mesh.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "The feature ID for each cell.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles", "The Euler angles in radians for each cell.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "The phase ID for each cell.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.linkParameters(k_UseReducedIntegration_Key, k_HourglassStiffness_Key, true);

  return params;
}

IFilter::VersionType WriteAbaqusCrystalPlasticityFilter::parametersVersion() const
{
  // Version 1 is the first simplnx version. It adds integration type, hourglass stiffness, Euler-angle units,
  // the unsymmetric solver flag, grain/phase ID inclusion, and the Euler-angle start index beyond the legacy filter.
  // Defaults are C3D8, radians, unsymmetric solver on, IDs included, and Euler-angle start index 3.

  return 1;
}

IFilter::UniquePointerType WriteAbaqusCrystalPlasticityFilter::clone() const
{
  return std::make_unique<WriteAbaqusCrystalPlasticityFilter>();
}

IFilter::PreflightResult WriteAbaqusCrystalPlasticityFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                           const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  const auto prefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  if(auto prefixResult = abaqus_element_sets::ValidateFilePrefix(prefix, -12018); prefixResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(prefixResult), {})};
  }

  const auto outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  if(fs::exists(outputPath) && !fs::is_directory(outputPath))
  {
    return MakePreflightErrorResult(-12001, fmt::format("The output path '{}' is not a directory.", outputPath.string()));
  }

  if(auto targetsResult = abaqus_element_sets::ValidateOutputTargets(outputPath, prefix, -12019); targetsResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(targetsResult), {})};
  }

  const int32 numDepvar = filterArgs.value<int32>(k_NumDepvar_Key);
  if(numDepvar < 0)
  {
    return MakePreflightErrorResult(-12014, fmt::format("The number of solution-dependent state variables ({}) must be 0 or greater.", numDepvar));
  }

  const int32 numUserOutVar = filterArgs.value<int32>(k_NumUserOutVar_Key);
  if(numUserOutVar < 0)
  {
    return MakePreflightErrorResult(-12015, fmt::format("The number of user output variables ({}) must be 0 or greater.", numUserOutVar));
  }

  const auto materialConstants = filterArgs.value<DynamicTableParameter::ValueType>(k_MaterialConstants_Key);

  const bool includeGrainAndPhaseIds = filterArgs.value<bool>(k_IncludeGrainAndPhaseIds_Key);
  const int32 eulerAnglesStartIndex = filterArgs.value<int32>(k_EulerAnglesStartIndex_Key);
  const int64 prefixCount = includeGrainAndPhaseIds ? 2 : 0;
  const int64 minStartIndex = prefixCount + 1;
  const int64 maxStartIndex = prefixCount + static_cast<int64>(materialConstants.size()) + 1;
  if(eulerAnglesStartIndex < minStartIndex || eulerAnglesStartIndex > maxStartIndex)
  {
    return MakePreflightErrorResult(-12017, fmt::format("The Euler Angles Start Index ({}) must be between {} and {}. With {} material constants{}, the three Euler angles must start after "
                                                        "any grain and phase IDs and must not extend past the end of the constant list.",
                                                        eulerAnglesStartIndex, minStartIndex, maxStartIndex, materialConstants.size(), includeGrainAndPhaseIds ? " and the grain and phase IDs" : ""));
  }

  const auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  if(auto sizeResult = abaqus_element_sets::ValidateMeshSize(imageGeom.getDimensions()); sizeResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(sizeResult), {})};
  }
  const usize cellCount = imageGeom.getNumberOfCells();

  constexpr std::array<std::pair<StringLiteralType, int32>, 3> k_ArrayKeys = {{{k_FeatureIdsArrayPath_Key, -12002}, {k_CellEulerAnglesArrayPath_Key, -12003}, {k_CellPhasesArrayPath_Key, -12004}}};
  for(const auto& [arrayKey, errorCode] : k_ArrayKeys)
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

Result<> WriteAbaqusCrystalPlasticityFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteAbaqusCrystalPlasticityInputValues inputValues;
  inputValues.output_path = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.file_prefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.job_name = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.use_reduced_integration = filterArgs.value<bool>(k_UseReducedIntegration_Key);
  inputValues.hourglass_stiffness = filterArgs.value<int32>(k_HourglassStiffness_Key);
  inputValues.write_euler_angles_in_degrees = filterArgs.value<bool>(k_WriteEulerAnglesInDegrees_Key);
  inputValues.num_depvar = filterArgs.value<int32>(k_NumDepvar_Key);
  inputValues.num_user_out_var = filterArgs.value<int32>(k_NumUserOutVar_Key);
  inputValues.use_unsymmetric_solver = filterArgs.value<bool>(k_UseUnsymmetricSolver_Key);
  inputValues.include_grain_and_phase_ids = filterArgs.value<bool>(k_IncludeGrainAndPhaseIds_Key);
  inputValues.euler_angles_start_index = filterArgs.value<int32>(k_EulerAnglesStartIndex_Key);
  inputValues.material_constants = filterArgs.value<DynamicTableParameter::ValueType>(k_MaterialConstants_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.cell_euler_angles_array_path = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  return WriteAbaqusCrystalPlasticity(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputPathKey = "OutputPath";
constexpr StringLiteralType k_OutputFilePrefixKey = "OutputFilePrefix";
constexpr StringLiteralType k_JobNameKey = "JobName";
constexpr StringLiteralType k_NumDepvarKey = "NumDepvar";
constexpr StringLiteralType k_NumUserOutVarKey = "NumUserOutVar";
constexpr StringLiteralType k_MatConstKey = "MatConst";
constexpr StringLiteralType k_AbqFeatureIdsArrayPathKey = "AbqFeatureIdsArrayPath";
constexpr StringLiteralType k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteAbaqusCrystalPlasticityFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAbaqusCrystalPlasticityFilter().getDefaultArguments();
  // Legacy SimulationIO wrote C3D8R elements without hourglass stiffness. Conversion keeps C3D8R and adds the default hourglass stiffness record.
  args.insertOrAssign(k_UseReducedIntegration_Key, std::make_any<bool>(true));
  // Legacy SimulationIO wrote Euler angles in degrees.
  args.insertOrAssign(k_WriteEulerAnglesInDegrees_Key, std::make_any<bool>(true));
  // Legacy SimulationIO did not request the unsymmetric solver.
  args.insertOrAssign(k_UseUnsymmetricSolver_Key, std::make_any<bool>(false));
  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_OutputFilePrefixKey, k_FilePrefix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_JobNameKey, k_JobName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumDepvarKey, k_NumDepvar_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumUserOutVarKey, k_NumUserOutVar_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DynamicTableFilterParameterConverter>(args, json, simpl::k_MatConstKey, k_MaterialConstants_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_AbqFeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_AbqFeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
