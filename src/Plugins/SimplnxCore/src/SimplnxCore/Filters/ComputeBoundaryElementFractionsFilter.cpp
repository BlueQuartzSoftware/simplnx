#include "ComputeBoundaryElementFractionsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeBoundaryElementFractions.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeBoundaryElementFractionsFilter::name() const
{
  return FilterTraits<ComputeBoundaryElementFractionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeBoundaryElementFractionsFilter::className() const
{
  return FilterTraits<ComputeBoundaryElementFractionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeBoundaryElementFractionsFilter::uuid() const
{
  return FilterTraits<ComputeBoundaryElementFractionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeBoundaryElementFractionsFilter::humanName() const
{
  return "Compute Feature Boundary Element Fractions";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeBoundaryElementFractionsFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters ComputeBoundaryElementFractionsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_BoundaryCellsArrayPath_Key, "Surface Elements",
                                                          "DataArray containing the number of neighboring Elements of a given Element that belong to a different Feature than itself", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Attribute Matrix"});
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureDataAMPath_Key, "Feature Data", "Parent Attribute Matrix for the Surface Element Fractions Array to be created in", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BoundaryCellFractionsArrayName_Key, "Surface Element Fractions",
                                                          "Name of created Data Array containing fraction of Elements belonging to the Feature that are \"surface\" Elements",
                                                          "Surface Element Fractions"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeBoundaryElementFractionsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeBoundaryElementFractionsFilter::clone() const
{
  return std::make_unique<ComputeBoundaryElementFractionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeBoundaryElementFractionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pFeatureDataAMPathValue = filterArgs.value<DataPath>(k_FeatureDataAMPath_Key);
  auto pBoundaryCellFractionsArrayPathValue = filterArgs.value<std::string>(k_BoundaryCellFractionsArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> numTuples = dataStructure.getDataAs<AttributeMatrix>(pFeatureDataAMPathValue)->getShape();
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, numTuples, std::vector<usize>{1}, pFeatureDataAMPathValue.createChildPath(pBoundaryCellFractionsArrayPathValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeBoundaryElementFractionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeBoundaryElementFractionsInputValues inputValues;
  // Replace the keys below with the variables from the header.
  inputValues.BoundaryCellFractionsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_BoundaryCellFractionsArrayName_Key);
  inputValues.BoundaryCellsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_BoundaryCellsArrayPath_Key);
  inputValues.FeatureDataAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureDataAMPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsArrayPath_Key);

  return ComputeBoundaryElementFractions(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_BoundaryCellsArrayPathKey = "BoundaryCellsArrayPath";
constexpr StringLiteral k_BoundaryCellFractionsArrayPathKey = "BoundaryCellFractionsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeBoundaryElementFractionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeBoundaryElementFractionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_BoundaryCellsArrayPathKey, k_FeatureDataAMPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_BoundaryCellsArrayPathKey, k_BoundaryCellsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_BoundaryCellFractionsArrayPathKey,
                                                                                                                                  k_BoundaryCellFractionsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
