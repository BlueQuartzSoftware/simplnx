#include "ComputeReferenceOrientationsCAxisFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeReferenceOrientationsCAxis.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeReferenceOrientationsCAxisFilter::name() const
{
  return FilterTraits<ComputeReferenceOrientationsCAxisFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeReferenceOrientationsCAxisFilter::className() const
{
  return FilterTraits<ComputeReferenceOrientationsCAxisFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeReferenceOrientationsCAxisFilter::uuid() const
{
  return FilterTraits<ComputeReferenceOrientationsCAxisFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeReferenceOrientationsCAxisFilter::humanName() const
{
  return "Compute Reference Orientations (C-Axis)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeReferenceOrientationsCAxisFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters ComputeReferenceOrientationsCAxisFilter::parameters() const
{
  Parameters params;
 
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ReferenceDir_Key, "Reference Direction", "The reference axis with respect to which the C-axis misorientation is computed",
                                                         std::vector<float32>{0.0F, 0.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "The direction of each Feature's C-axis in the sample reference frame", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_MisorientationArrayName_Key, "Misorientation Array Name",
                                                          "Name of the output feature-level array storing the misorientation angle (in degrees) between each Feature's C-axis and the reference direction",
                                                          "ReferenceOrientationMisorientation"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeReferenceOrientationsCAxisFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeReferenceOrientationsCAxisFilter::clone() const
{
  return std::make_unique<ComputeReferenceOrientationsCAxisFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeReferenceOrientationsCAxisFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pMisorientationArrayNameValue = filterArgs.value<std::string>(k_MisorientationArrayName_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const ShapeType tupleShape = dataStructure.getDataRefAs<Float32Array>(pAvgCAxesArrayPathValue).getTupleShape();
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pAvgCAxesArrayPathValue.replaceName(pMisorientationArrayNameValue));
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeReferenceOrientationsCAxisFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeReferenceOrientationsCAxisInputValues inputValues;

  inputValues.ReferenceDir = filterArgs.value<std::vector<float32>>(k_ReferenceDir_Key);
  inputValues.AvgCAxesArrayPath = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  inputValues.MisorientationArrayPath = inputValues.AvgCAxesArrayPath.replaceName(filterArgs.value<std::string>(k_MisorientationArrayName_Key));

  return ComputeReferenceOrientationsCAxis(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
