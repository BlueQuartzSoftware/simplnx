#include "ReplaceElementAttributesWithNeighborValuesFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

namespace complex
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<Float32Parameter>(k_MinConfidence_Key, "Threshold Value", "The value to of the threshold", 0.1F));
  params.insert(std::make_unique<ChoicesParameter>(k_SelectedComparison_Key, "Comparison Operator", "The operator to use for comparisons. 0=Less, 1=Greater Than", 0, ::k_OperationChoices));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "The algorithm will keep looping until all pixels have been evaluated", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ComparisonDataPath, "Input Comparison Array", "The DataPath to the input array to use for comparison", DataPath{},
                                                          complex::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));

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

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  complex::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pComparisonDataPath.getParent(), {});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return ReplaceElementAttributesWithNeighborValues(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
