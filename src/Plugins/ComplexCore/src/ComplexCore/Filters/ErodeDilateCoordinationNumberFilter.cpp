#include "ErodeDilateCoordinationNumberFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ErodeDilateCoordinationNumber.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::name() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::className() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateCoordinationNumberFilter::uuid() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::humanName() const
{
  return "Erode/Dilate Coordination Number";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateCoordinationNumberFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate", "Smooth Bad Data"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateCoordinationNumberFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_CoordinationNumber_Key, "Coordination Number to Consider", "", 6));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "Keep looping until all criteria is met", false));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, MultiArraySelectionParameter::AllowedDataTypes{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateCoordinationNumberFilter::clone() const
{
  return std::make_unique<ErodeDilateCoordinationNumberFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateCoordinationNumberFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto pCoordinationNumberValue = filterArgs.value<int32>(k_CoordinationNumber_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  if(pCoordinationNumberValue < 0 || pCoordinationNumberValue > 6)
  {
    MakeErrorResult(-16800, fmt::format("Coordination Number must be between 0 and 6. Current Value: {}", pCoordinationNumberValue));
  }

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumberFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  ErodeDilateCoordinationNumberInputValues inputValues;

  inputValues.CoordinationNumber = filterArgs.value<int32>(k_CoordinationNumber_Key);
  inputValues.Loop = filterArgs.value<bool>(k_Loop_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return ErodeDilateCoordinationNumber(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
