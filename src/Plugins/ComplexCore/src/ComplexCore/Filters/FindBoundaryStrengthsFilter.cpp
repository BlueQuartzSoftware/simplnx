#include "FindBoundaryStrengthsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindBoundaryStrengths.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
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
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Loading_Key, "Loading Direction (XYZ)", "The loading axis for the sample", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Data Array that specifies which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));

  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions",
                                                          "Data Array that specifies the average orientation of each Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Data Array that specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each phase", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Face Data Arrays"});
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
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshF1sArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshF1sArrayName_Key);
  auto pSurfaceMeshF1sptsArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshF1sptsArrayName_Key);
  auto pSurfaceMeshF7sArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshF7sArrayName_Key);
  auto pSurfaceMeshmPrimesArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshmPrimesArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> faceLabelsTupShape = dataStructure.getDataAs<Int32Array>(pSurfaceMeshFaceLabelsArrayPathValue)->getTupleShape();
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2},
                                                      pSurfaceMeshFaceLabelsArrayPathValue.getParent().createChildPath(pSurfaceMeshF1sArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2},
                                                      pSurfaceMeshFaceLabelsArrayPathValue.getParent().createChildPath(pSurfaceMeshF1sptsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2},
                                                      pSurfaceMeshFaceLabelsArrayPathValue.getParent().createChildPath(pSurfaceMeshF7sArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2},
                                                      pSurfaceMeshFaceLabelsArrayPathValue.getParent().createChildPath(pSurfaceMeshmPrimesArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

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
