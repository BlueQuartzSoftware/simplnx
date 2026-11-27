#include "ComputeFeaturePhasesBinaryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeaturePhasesBinary.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeaturePhasesBinaryFilter::name() const
{
  return FilterTraits<ComputeFeaturePhasesBinaryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeaturePhasesBinaryFilter::className() const
{
  return FilterTraits<ComputeFeaturePhasesBinaryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeaturePhasesBinaryFilter::uuid() const
{
  return FilterTraits<ComputeFeaturePhasesBinaryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeaturePhasesBinaryFilter::humanName() const
{
  return "Compute Feature Phases Binary";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeaturePhasesBinaryFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeaturePhasesBinaryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Cell Feature Ids used only with a Cell mask. Values are not read with a Feature mask.",
                                                          DataPath{}, GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Mask and Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Boolean or uint8 Cell or Feature mask. Zero assigns phase 0; any nonzero value assigns phase 1.",
                                                          DataPath{}, GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::boolean, DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAMPath_Key, "Cell Feature Attribute Matrix",
                                                                    "The Feature Attribute Matrix where the binary phases array will be created. A mask in this matrix selects Feature-mask mode.",
                                                                    DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeaturePhasesArrayName_Key, "Binary Feature Phases Array Name", "Created Data Array name to specify to which Ensemble each Feature belongs",
                                                          "Binary Phases Array"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CreateCellEnsembleAM_Key, "Create Cell Ensemble Attribute Matrix",
                                                                 "Create an empty Attribute Matrix with 2 tuples beside the selected Feature Attribute Matrix.", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAMName_Key, "Cell Ensemble Attribute Matrix Name", "Name of the created Cell Ensemble Attribute Matrix", "Cell Ensemble Data"));
  params.linkParameters(k_CreateCellEnsembleAM_Key, k_CellEnsembleAMName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeaturePhasesBinaryFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeFeaturePhasesBinaryFilter::clone() const
{
  return std::make_unique<ComputeFeaturePhasesBinaryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeaturePhasesBinaryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                         const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  const auto featureAMPath = filterArgs.value<DataPath>(k_CellFeatureAMPath_Key);
  const auto maskPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  const auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath);
  const auto& mask = dataStructure.getDataRefAs<IDataArray>(maskPath);

  // Compare resolved objects so alternate paths to the same Attribute Matrix select the same mode.
  if(dataStructure.getData(maskPath.getParent()) != &featureAM)
  {
    const auto& featureIds = dataStructure.getDataRefAs<IDataArray>(featureIdsPath);
    if(mask.getNumberOfTuples() != featureIds.getNumberOfTuples())
    {
      return {.output_actions = MakeErrorResult<OutputActions>(
                  -53802, fmt::format("Cell mask '{}' has {} tuples, but Feature Ids '{}' has {} tuples. Select arrays with the same tuple count, or select a mask in Feature Attribute Matrix '{}'.",
                                      maskPath.toString(), mask.getNumberOfTuples(), featureIdsPath.toString(), featureIds.getNumberOfTuples(), featureAMPath.toString()))};
    }
  }

  Result<OutputActions> resultOutputActions;
  const auto outputPath = featureAMPath.createChildPath(filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int32, featureAM.getShape(), ShapeType{1}, outputPath));
  if(filterArgs.value<bool>(k_CreateCellEnsembleAM_Key))
  {
    const auto ensemblePath = featureAMPath.replaceName(filterArgs.value<std::string>(k_CellEnsembleAMName_Key));
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(ensemblePath, ShapeType{2}));
  }
  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeaturePhasesBinaryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeFeaturePhasesBinaryInputValues inputValues;
  inputValues.feature_ids_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_MaskArrayPath_Key);
  inputValues.cell_feature_attribute_matrix_path = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAMPath_Key);
  inputValues.feature_phases_array_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeaturePhasesArrayName_Key);

  return ComputeFeaturePhasesBinary(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeFeaturePhasesBinaryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeaturePhasesBinaryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_CellFeatureAMPath_Key));

  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAMName_Key));
  args.insertOrAssign(k_CreateCellEnsembleAM_Key, std::make_any<bool>(true));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
