#include "WriteAbaqusCrystalPlasticityFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteAbaqusCrystalPlasticity.hpp"

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
  params.insert(std::make_unique<StringParameter>(k_FilePrefix_Key, "Output File Prefix", "The prefix for each output file.", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_JobName_Key, "Job Name", "The Abaqus job name.", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseReducedIntegration_Key, "Use Reduced Integration Elements",
                                                                 "When true, writes C3D8R elements and includes an hourglass stiffness value for each grain section.", true));
  params.insert(std::make_unique<Int32Parameter>(k_HourglassStiffness_Key, "Hourglass Stiffness Value", "The hourglass stiffness value for C3D8R elements.", 250));
  params.insert(std::make_unique<Int32Parameter>(k_NumDepvar_Key, "Number of Solution Dependent State Variables", "The number of solution-dependent state variables.", 1));
  params.insert(std::make_unique<Int32Parameter>(k_NumUserOutVar_Key, "Number of User Output Variables", "The number of user output variables.", 1));

  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(0, 6, "Constant {}"));
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Values"}));
  const DynamicTableInfo::TableDataType defaultTable(6, DynamicTableInfo::RowType(1, 0.0));
  params.insert(std::make_unique<DynamicTableParameter>(k_MaterialConstants_Key, "Material Constants",
                                                        "The material constants. The filter automatically prepends grain ID, phase ID, Euler 1, Euler 2, and Euler 3 in degrees before these values.",
                                                        defaultTable, tableInfo));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Selected Image Geometry", "The Image Geometry that defines the voxel mesh.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "The feature ID for each cell.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles", "The Euler angles in radians for each cell.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "The phase ID for each cell.", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.linkParameters(k_UseReducedIntegration_Key, k_HourglassStiffness_Key, true);

  return params;
}

IFilter::VersionType WriteAbaqusCrystalPlasticityFilter::parametersVersion() const
{
  return 2;

  // Version 2 adds the integration type and hourglass stiffness parameters. Reduced integration preserves the legacy element type.
}

IFilter::UniquePointer WriteAbaqusCrystalPlasticityFilter::clone() const
{
  return std::make_unique<WriteAbaqusCrystalPlasticityFilter>();
}

IFilter::PreflightResult WriteAbaqusCrystalPlasticityFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto outputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  if(!fs::exists(outputPath))
  {
    return MakePreflightErrorResult(-12000, fmt::format("The output path '{}' does not exist.", outputPath.string()));
  }
  if(!fs::is_directory(outputPath))
  {
    return MakePreflightErrorResult(-12001, fmt::format("The output path '{}' is not a directory.", outputPath.string()));
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
  for(usize rowIndex = 0; rowIndex < materialConstants.size(); rowIndex++)
  {
    const usize columnCount = materialConstants[rowIndex].size();
    if(columnCount != 1)
    {
      return MakePreflightErrorResult(-12016, fmt::format("Material Constants row {} has {} columns. Each row must have exactly 1 column.", rowIndex, columnCount));
    }
  }

  const auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  const usize cellCount = imageGeom.getNumberOfCells();

  const std::array<std::pair<StringLiteral, int32>, 3> arrayKeys = {{{k_FeatureIdsArrayPath_Key, -12002}, {k_CellEulerAnglesArrayPath_Key, -12003}, {k_CellPhasesArrayPath_Key, -12004}}};
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

Result<> WriteAbaqusCrystalPlasticityFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  WriteAbaqusCrystalPlasticityInputValues inputValues;
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FilePrefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.JobName = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.UseReducedIntegration = filterArgs.value<bool>(k_UseReducedIntegration_Key);
  inputValues.HourglassStiffness = filterArgs.value<int32>(k_HourglassStiffness_Key);
  inputValues.NumDepvar = filterArgs.value<int32>(k_NumDepvar_Key);
  inputValues.NumUserOutVar = filterArgs.value<int32>(k_NumUserOutVar_Key);
  inputValues.MaterialConstants = filterArgs.value<DynamicTableParameter::ValueType>(k_MaterialConstants_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  return WriteAbaqusCrystalPlasticity(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_OutputPathKey = "OutputPath";
constexpr StringLiteral k_OutputFilePrefixKey = "OutputFilePrefix";
constexpr StringLiteral k_JobNameKey = "JobName";
constexpr StringLiteral k_NumDepvarKey = "NumDepvar";
constexpr StringLiteral k_NumUserOutVarKey = "NumUserOutVar";
constexpr StringLiteral k_MatConstKey = "MatConst";
constexpr StringLiteral k_AbqFeatureIdsArrayPathKey = "AbqFeatureIdsArrayPath";
constexpr StringLiteral k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteAbaqusCrystalPlasticityFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteAbaqusCrystalPlasticityFilter().getDefaultArguments();
  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_OutputFilePrefixKey, k_FilePrefix_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_JobNameKey, k_JobName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumDepvarKey, k_NumDepvar_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumUserOutVarKey, k_NumUserOutVar_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DynamicTableFilterParameterConverter>(args, json, SIMPL::k_MatConstKey, k_MaterialConstants_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_AbqFeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AbqFeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
