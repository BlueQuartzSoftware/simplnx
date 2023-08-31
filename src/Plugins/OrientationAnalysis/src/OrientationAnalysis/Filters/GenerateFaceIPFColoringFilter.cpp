#include "GenerateFaceIPFColoringFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GenerateFaceIPFColoring.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateFaceIPFColoringFilter::name() const
{
  return FilterTraits<GenerateFaceIPFColoringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateFaceIPFColoringFilter::className() const
{
  return FilterTraits<GenerateFaceIPFColoringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateFaceIPFColoringFilter::uuid() const
{
  return FilterTraits<GenerateFaceIPFColoringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateFaceIPFColoringFilter::humanName() const
{
  return "Generate IPF Colors (Face)";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateFaceIPFColoringFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateFaceIPFColoringFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Specifies which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "Specifies the normal of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which phase each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SurfaceMeshFaceIPFColorsArrayName_Key, "IPF Colors", "A set of two RGB color schemes encoded as unsigned chars for each Face", "FaceIPFColors"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateFaceIPFColoringFilter::clone() const
{
  return std::make_unique<GenerateFaceIPFColoringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateFaceIPFColoringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceIPFColorsArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshFaceIPFColorsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // make sure all the face data has same number of tuples (i.e. they should all be coming from the same Triangle Geometry)
  std::vector<DataPath> triangleArrayPaths = {pSurfaceMeshFaceLabelsArrayPathValue, pSurfaceMeshFaceNormalsArrayPathValue};
  auto numTupleCheckResult = dataStructure.validateNumberOfTuples(triangleArrayPaths);
  if(!numTupleCheckResult.first)
  {
    return {MakeErrorResult<OutputActions>(-2430, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", numTupleCheckResult.second))};
  }

  const auto faceLabels = dataStructure.getDataAs<Int32Array>(pSurfaceMeshFaceLabelsArrayPathValue);
  if(faceLabels == nullptr)
  {
    return MakePreflightErrorResult(-2431, fmt::format("Could not find the face labels data array at path '{}'", pSurfaceMeshFaceLabelsArrayPathValue.toString()));
  }

  // make sure all the cell data has same number of tuples (i.e. they should all be coming from the same Image Geometry)
  std::vector<DataPath> imageArrayPaths = {pFeatureEulerAnglesArrayPathValue, pFeaturePhasesArrayPathValue};
  numTupleCheckResult = dataStructure.validateNumberOfTuples(imageArrayPaths);
  if(!numTupleCheckResult.first)
  {
    return {MakeErrorResult<OutputActions>(-2432, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", numTupleCheckResult.second))};
  }

  DataPath faceIpfColorsArrayPath = pSurfaceMeshFaceLabelsArrayPathValue.getParent().createChildPath(pSurfaceMeshFaceIPFColorsArrayNameValue);
  auto action = std::make_unique<CreateArrayAction>(DataType::uint8, faceLabels->getTupleShape(), std::vector<usize>{6}, faceIpfColorsArrayPath);
  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateFaceIPFColoringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  GenerateFaceIPFColoringInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceIPFColorsArrayName = filterArgs.value<std::string>(k_SurfaceMeshFaceIPFColorsArrayName_Key);

  return GenerateFaceIPFColoring(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
