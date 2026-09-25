#include "ComputeCAxisMisorientationsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeCAxisMisorientations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

namespace
{
// Squared-magnitude threshold below which the reference direction is treated as zero.
constexpr float64 k_MinRefDirSquaredNorm = 1.0e-20;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeCAxisMisorientationsFilter::name() const
{
  return FilterTraits<ComputeCAxisMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeCAxisMisorientationsFilter::className() const
{
  return FilterTraits<ComputeCAxisMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeCAxisMisorientationsFilter::uuid() const
{
  return FilterTraits<ComputeCAxisMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeCAxisMisorientationsFilter::humanName() const
{
  return "Compute C-Axis Misorientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeCAxisMisorientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters ComputeCAxisMisorientationsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ReferenceDir_Key, "Reference Direction", "The reference axis with respect to which the C-axis misorientation is computed",
                                                         std::vector<float32>{0.0F, 0.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "The direction of each Feature's C-axis in the sample reference frame", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_MisorientationArrayName_Key, "Misorientation Array Name",
      "Name of the output feature-level array storing the misorientation angle (in degrees) between each Feature's C-axis and the reference direction", "ReferenceOrientationMisorientation"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeCAxisMisorientationsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeCAxisMisorientationsFilter::clone() const
{
  return std::make_unique<ComputeCAxisMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeCAxisMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pMisorientationArrayNameValue = filterArgs.value<std::string>(k_MisorientationArrayName_Key);

  auto pReferenceDirValue = filterArgs.value<std::vector<float32>>(k_ReferenceDir_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const float64 rx = pReferenceDirValue[0];
  const float64 ry = pReferenceDirValue[1];
  const float64 rz = pReferenceDirValue[2];
  if((rx * rx + ry * ry + rz * rz) < k_MinRefDirSquaredNorm)
  {
    return {MakeErrorResult<OutputActions>(-77000, "Reference direction has zero magnitude. Provide a non-zero reference direction.")};
  }

  const ShapeType tupleShape = dataStructure.getDataRefAs<Float32Array>(pAvgCAxesArrayPathValue).getTupleShape();
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pAvgCAxesArrayPathValue.replaceName(pMisorientationArrayNameValue));
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeCAxisMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeCAxisMisorientationsInputValues inputValues;

  inputValues.ReferenceDir = filterArgs.value<std::vector<float32>>(k_ReferenceDir_Key);
  inputValues.AvgCAxesArrayPath = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  inputValues.MisorientationArrayPath = inputValues.AvgCAxesArrayPath.replaceName(filterArgs.value<std::string>(k_MisorientationArrayName_Key));

  return ComputeCAxisMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
