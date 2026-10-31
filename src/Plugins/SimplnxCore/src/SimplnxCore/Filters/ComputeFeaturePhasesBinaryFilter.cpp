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
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Mask and Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Boolean or uint8 Cell or Feature mask. Zero assigns phase 0; any nonzero value assigns phase 1.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
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
IFilter::UniquePointer ComputeFeaturePhasesBinaryFilter::clone() const
{
  return std::make_unique<ComputeFeaturePhasesBinaryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeaturePhasesBinaryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
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
      return {MakeErrorResult<OutputActions>(
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
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeaturePhasesBinaryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeaturePhasesBinaryInputValues inputValues;
  inputValues.FeatureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_MaskArrayPath_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAMPath_Key);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeaturePhasesArrayName_Key);

  return ComputeFeaturePhasesBinary(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CellEnsembleAttributeMatrixNameKey = "CellEnsembleAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeaturePhasesBinaryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeaturePhasesBinaryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_CellFeatureAMPath_Key));

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellEnsembleAttributeMatrixNameKey, k_CellEnsembleAMName_Key));
  args.insertOrAssign(k_CreateCellEnsembleAM_Key, std::make_any<bool>(true));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
