#include "ReplaceElementAttributesWithNeighborValuesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReplaceElementAttributesWithNeighborValuesFilter::name() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReplaceElementAttributesWithNeighborValuesFilter::className() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReplaceElementAttributesWithNeighborValuesFilter::uuid() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReplaceElementAttributesWithNeighborValuesFilter::humanName() const
{
  return "Replace Element Attributes with Neighbor (Threshold)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReplaceElementAttributesWithNeighborValuesFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Replace Values"};
}

//------------------------------------------------------------------------------
Parameters ReplaceElementAttributesWithNeighborValuesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<Float32Parameter>(k_MinConfidence_Key, "Threshold Value", "The value to of the threshold", 0.1F));
  params.insert(std::make_unique<ChoicesParameter>(k_SelectedComparison_Key, "Comparison Operator", "The operator to use for comparisons. 0=Less, 1=Greater Than", 0, detail::k_OperationChoices));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "The algorithm will keep looping until all pixels have been evaluated", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ComparisonDataPath, "Input Comparison Array", "The DataPath to the input array to use for comparison", DataPath{},
                                                          nx::core::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReplaceElementAttributesWithNeighborValuesFilter::clone() const
{
  return std::make_unique<ReplaceElementAttributesWithNeighborValuesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReplaceElementAttributesWithNeighborValuesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                         const std::atomic_bool& shouldCancel) const
{
  auto pComparisonDataPath = filterArgs.value<DataPath>(k_ComparisonDataPath);

  nx::core::Result<OutputActions> resultOutputActions;

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pComparisonDataPath.getParent(), {});

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ReplaceElementAttributesWithNeighborValuesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                       const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  ReplaceElementAttributesWithNeighborValuesInputValues inputValues;

  inputValues.MinConfidence = filterArgs.value<float32>(k_MinConfidence_Key);
  inputValues.SelectedComparison = filterArgs.value<ChoicesParameter::ValueType>(k_SelectedComparison_Key);
  inputValues.Loop = filterArgs.value<bool>(k_Loop_Key);
  inputValues.InputArrayPath = filterArgs.value<DataPath>(k_ComparisonDataPath);
  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ReplaceElementAttributesWithNeighborValues(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinConfidenceKey = "MinConfidence";
constexpr StringLiteral k_SelectedComparisonKey = "SelectedComparison";
constexpr StringLiteral k_LoopKey = "Loop";
constexpr StringLiteral k_ConfidenceIndexArrayPathKey = "ConfidenceIndexArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ReplaceElementAttributesWithNeighborValuesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReplaceElementAttributesWithNeighborValuesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MinConfidenceKey, k_MinConfidence_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_SelectedComparisonKey, k_SelectedComparison_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_LoopKey, k_Loop_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ConfidenceIndexArrayPathKey, k_ComparisonDataPath));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_ConfidenceIndexArrayPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
