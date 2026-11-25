#include "WriteINLFileFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WriteINLFile.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteINLFileFilter::name() const
{
  return FilterTraits<WriteINLFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteINLFileFilter::className() const
{
  return FilterTraits<WriteINLFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteINLFileFilter::uuid() const
{
  return FilterTraits<WriteINLFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteINLFileFilter::humanName() const
{
  return "Write INL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteINLFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteINLFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "The output .inl file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "The selected image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles", "Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)",
                                                          DataPath{}, GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{3}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaterialNameArrayPath_Key, "Material Names", "Name of each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedArrayTypes{IArray::ArrayType::StringArray}, ArraySelectionParameter::AllowedDataTypes::StringOnly(),
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "The number of Features per Ensemble", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteINLFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteINLFileFilter::clone() const
{
  return std::make_unique<WriteINLFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteINLFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                           const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto cellPhasesPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto eulerAnglesPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, imageGeometryPath, {featureIdsPath, cellPhasesPath, eulerAnglesPath}, -78431); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteINLFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  WriteINLFileInputValues inputValues;

  inputValues.output_file = atomicFile.tempFilePath();
  inputValues.image_geom_path = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.cell_euler_angles_array_path = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.material_name_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_MaterialNameArrayPath_Key);
  inputValues.num_features_array_path = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);

  auto result = WriteINLFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
  if(result.valid())
  {
    if(shouldCancel)
    {
      return MakeErrorResult(-1, "Filter cancelled");
    }
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }
  return result;
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputFileKey = "OutputFile";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_MaterialNameArrayPathKey = "MaterialNameArrayPath";
constexpr StringLiteralType k_NumFeaturesArrayPathKey = "NumFeaturesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteINLFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteINLFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputFileKey, k_OutputFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_MaterialNameArrayPathKey, k_MaterialNameArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_NumFeaturesArrayPathKey, k_NumFeaturesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_NumFeaturesArrayPathKey, k_ImageGeomPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
