#include "RotateEulerRefFrameFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RotateEulerRefFrameFilter::name() const
{
  return FilterTraits<RotateEulerRefFrameFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RotateEulerRefFrameFilter::className() const
{
  return FilterTraits<RotateEulerRefFrameFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RotateEulerRefFrameFilter::uuid() const
{
  return FilterTraits<RotateEulerRefFrameFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RotateEulerRefFrameFilter::humanName() const
{
  return "Rotate Euler Reference Frame";
}

//------------------------------------------------------------------------------
std::vector<std::string> RotateEulerRefFrameFilter::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters RotateEulerRefFrameFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_RotationAngle_Key, "Rotation Angle (Degrees)", "", 0.0f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxis_Key, "Rotation Axis (ijk)", "", std::vector<float32>{0.0F, 0.0F, 1.0F}, std::vector<std::string>{"i", "j", "k"}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RotateEulerRefFrameFilter::clone() const
{
  return std::make_unique<RotateEulerRefFrameFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RotateEulerRefFrameFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pRotationAngleValue = filterArgs.value<float32>(k_RotationAngle_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RotateEulerRefFrameFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  // Instantiate our Algorithm class.
  complex::RotateEulerRefFrame algorithm(dataStructure, messageHandler, shouldCancel);

  complex::RotateEulerRefFrameInputValues* inputValues = algorithm.inputValues();

  inputValues->eulerAngleDataPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues->rotationAngle = filterArgs.value<float32>(k_RotationAngle_Key);
  inputValues->rotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);

  return algorithm();
}
} // namespace complex
