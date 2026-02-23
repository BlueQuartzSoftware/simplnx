#include "ComputeFeaturePhasesBinaryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeaturePhasesBinary.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Data Array that specifies if the Cell is to be counted in the algorithm", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellDataAMPath_Key, "Cell Data Attribute Matrix",
                                                                    "The Cell Data Attribute Matrix within the Image Geometry where the Binary Phases Array will be created", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeaturePhasesArrayName_Key, "Binary Feature Phases Array Name", "Created Data Array name to specify to which Ensemble each Feature belongs",
                                                          "Binary Phases Array"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeaturePhasesBinaryFilter::parametersVersion() const
{
  return 1;
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
  auto pCellDataAMPathValue = filterArgs.value<DataPath>(k_CellDataAMPath_Key);
  auto pFeaturePhasesArrayNameValue = filterArgs.value<std::string>(k_FeaturePhasesArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto cellDataAM = dataStructure.getDataAs<AttributeMatrix>(pCellDataAMPathValue);
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, cellDataAM->getShape(), std::vector<usize>{1}, pCellDataAMPathValue.createChildPath(pFeaturePhasesArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeaturePhasesBinaryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeaturePhasesBinaryInputValues inputValues;
  inputValues.FeatureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_MaskArrayPath_Key);
  inputValues.CellDataAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellDataAMPath_Key);
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
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellDataAMPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
