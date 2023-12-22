#include "FindAvgCAxesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindAvgCAxes.hpp"
#include "OrientationAnalysis/utilities/SIMPLConversion.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FindAvgCAxesFilter::name() const
{
  return FilterTraits<FindAvgCAxesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindAvgCAxesFilter::className() const
{
  return FilterTraits<FindAvgCAxesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindAvgCAxesFilter::uuid() const
{
  return FilterTraits<FindAvgCAxesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindAvgCAxesFilter::humanName() const
{
  return "Find Average C-Axis Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindAvgCAxesFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindAvgCAxesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Input quaternion array", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrix_Key, "Cell Feature Attribute Matrix", "The path to the cell feature attribute matrix", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "The output average C-Axis values for each feature", "AvgCAxes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindAvgCAxesFilter::clone() const
{
  return std::make_unique<FindAvgCAxesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindAvgCAxesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrix_Key);
  auto pAvgCAxesArrayNameValue = filterArgs.value<std::string>(k_AvgCAxesArrayPath_Key);

  const DataPath avgCAxesPath = pCellFeatureAttributeMatrixPathValue.createChildPath(pAvgCAxesArrayNameValue);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataPaths = {pQuatsArrayPathValue, pFeatureIdsArrayPathValue, pCellPhasesArrayPathValue};
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-6400, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  {
    std::vector<usize> tupleShape = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue).getShape();
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, avgCAxesPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-6401, "Finding the average c-axes requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindAvgCAxesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  FindAvgCAxesInputValues inputValues;

  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CellFeatureDataPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrix_Key);
  inputValues.AvgCAxesArrayPath = inputValues.CellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_AvgCAxesArrayPath_Key));
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return FindAvgCAxes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_AvgCAxesArrayPathKey = "AvgCAxesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> FindAvgCAxesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindAvgCAxesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_CellFeatureAttributeMatrix_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_AvgCAxesArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_AvgCAxesArrayPathKey, k_AvgCAxesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
