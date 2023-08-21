#include "ErodeDilateMaskFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ErodeDilateMask.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::name() const
{
  return FilterTraits<ErodeDilateMaskFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::className() const
{
  return FilterTraits<ErodeDilateMaskFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateMaskFilter::uuid() const
{
  return FilterTraits<ErodeDilateMaskFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::humanName() const
{
  return "Erode/Dilate Mask";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateMaskFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateMaskFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether to dilate (0) or erode (1)", 0ULL, ::k_OperationChoices));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "", 1));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "Whether to erode/dilate in the X direction", true));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "Whether to erode/dilate in the Y direction", true));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "Whether to erode/dilate in the Z direction", true));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array Path", "Boolean array where true voxels are used. False voxels are ignored.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateMaskFilter::clone() const
{
  return std::make_unique<ErodeDilateMaskFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateMaskFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pOperationValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pXDirOnValue = filterArgs.value<bool>(k_XDirOn_Key);
  auto pYDirOnValue = filterArgs.value<bool>(k_YDirOn_Key);
  auto pZDirOnValue = filterArgs.value<bool>(k_ZDirOn_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateMaskFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  ErodeDilateMaskInputValues inputValues;

  inputValues.Operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.NumIterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.XDirOn = filterArgs.value<bool>(k_XDirOn_Key);
  inputValues.YDirOn = filterArgs.value<bool>(k_YDirOn_Key);
  inputValues.ZDirOn = filterArgs.value<bool>(k_ZDirOn_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return ErodeDilateMask(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
