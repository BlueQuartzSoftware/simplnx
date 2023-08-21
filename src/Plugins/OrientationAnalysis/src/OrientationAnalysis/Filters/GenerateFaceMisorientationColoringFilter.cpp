#include "GenerateFaceMisorientationColoringFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GenerateFaceMisorientationColoring.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateFaceMisorientationColoringFilter::name() const
{
  return FilterTraits<GenerateFaceMisorientationColoringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateFaceMisorientationColoringFilter::className() const
{
  return FilterTraits<GenerateFaceMisorientationColoringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateFaceMisorientationColoringFilter::uuid() const
{
  return FilterTraits<GenerateFaceMisorientationColoringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateFaceMisorientationColoringFilter::humanName() const
{
  return "Generate Misorientation Colors (Face)";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateFaceMisorientationColoringFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateFaceMisorientationColoringFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Specifies which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of each Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which phase each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key, "Misorientation Colors", "A set of RGB color schemes encoded as floats for each Face",
                                                          "FaceMisorientationColors"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateFaceMisorientationColoringFilter::clone() const
{
  return std::make_unique<GenerateFaceMisorientationColoringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateFaceMisorientationColoringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceMisorientationColorsArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // make sure all the cell data has same number of tuples (i.e. they should all be coming from the same Image Geometry)
  std::vector<DataPath> imageArrayPaths = {pAvgQuatsArrayPathValue, pFeaturePhasesArrayPathValue};
  if(!dataStructure.validateNumberOfTuples(imageArrayPaths))
  {
    return MakePreflightErrorResult(
        -98410, "The input image geometry cell feature data arrays have inconsistent numbers of tuples.  Make sure the average quaternions and phases arrays all have the same number of tuples.");
  }

  const auto faceLabels = dataStructure.getDataAs<Int32Array>(pSurfaceMeshFaceLabelsArrayPathValue);
  if(faceLabels == nullptr)
  {
    return MakePreflightErrorResult(-98411, fmt::format("Could not find the face labels data array at path '{}'", pSurfaceMeshFaceLabelsArrayPathValue.toString()));
  }

  DataPath faceMisorientationColorsArrayPath = pSurfaceMeshFaceLabelsArrayPathValue.getParent().createChildPath(pSurfaceMeshFaceMisorientationColorsArrayNameValue);
  auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabels->getTupleShape(), std::vector<usize>{3}, faceMisorientationColorsArrayPath);
  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateFaceMisorientationColoringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  GenerateFaceMisorientationColoringInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceMisorientationColorsArrayName = filterArgs.value<std::string>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key);

  return GenerateFaceMisorientationColoring(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
