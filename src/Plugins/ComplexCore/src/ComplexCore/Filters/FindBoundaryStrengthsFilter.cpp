#include "FindBoundaryStrengthsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindBoundaryStrengths.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryStrengthsFilter::name() const
{
  return FilterTraits<FindBoundaryStrengthsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryStrengthsFilter::className() const
{
  return FilterTraits<FindBoundaryStrengthsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryStrengthsFilter::uuid() const
{
  return FilterTraits<FindBoundaryStrengthsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryStrengthsFilter::humanName() const
{
  return "Find Feature Boundary Strength Metrics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryStrengthsFilter::defaultTags() const
{
  return {"Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryStrengthsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Loading_Key, "Loading Direction (XYZ)", "", std::vector<float64>{0.0, 0.0, 0.0}, std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{2}}));

  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshF1sArrayName_Key, "F1s", "", "F1s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshF1sptsArrayName_Key, "F1spts", "", "F1s points"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshF7sArrayName_Key, "F7s", "", "F7s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshmPrimesArrayName_Key, "mPrimes", "", "mPrimes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryStrengthsFilter::clone() const
{
  return std::make_unique<FindBoundaryStrengthsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundaryStrengthsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pLoadingValue = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Loading_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshF1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF1sArrayName_Key);
  auto pSurfaceMeshF1sptsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF1sptsArrayName_Key);
  auto pSurfaceMeshF7sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshF7sArrayName_Key);
  auto pSurfaceMeshmPrimesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshmPrimesArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindBoundaryStrengthsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  FindBoundaryStrengthsInputValues inputValues;

  inputValues.Loading = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Loading_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshF1sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshF1sArrayName_Key);
  inputValues.SurfaceMeshF1sptsArrayName = filterArgs.value<DataPath>(k_SurfaceMeshF1sptsArrayName_Key);
  inputValues.SurfaceMeshF7sArrayName = filterArgs.value<DataPath>(k_SurfaceMeshF7sArrayName_Key);
  inputValues.SurfaceMeshmPrimesArrayName = filterArgs.value<DataPath>(k_SurfaceMeshmPrimesArrayName_Key);

  return FindBoundaryStrengths(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
