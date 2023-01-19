#include "RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

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
  return {"#Processing", "#Conversion", "#Euler", "#Angles", "#ReferenceFrame"};
}

//------------------------------------------------------------------------------
Parameters RotateEulerRefFrameFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxisAngle_Key, "Rotation Axis-Angle [<ijk>w]", "Axis-Angle in sample reference frame to rotate about.",
                                                         VectorFloat32Parameter::ValueType{0.0f, 0.0f, 0.0f, 90.0F}, std::vector<std::string>{"i", "j", "k", "w (Deg)"}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

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
  auto pRotationAxisAngleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);
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
  complex::RotateEulerRefFrameInputValues inputValues;

  inputValues.eulerAngleDataPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.rotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);

  // Let the Algorithm instance do the work
  return RotateEulerRefFrame(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
