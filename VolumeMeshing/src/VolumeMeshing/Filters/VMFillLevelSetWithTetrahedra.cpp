#include "VMFillLevelSetWithTetrahedra.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string VMFillLevelSetWithTetrahedra::name() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::name.str();
}

//------------------------------------------------------------------------------
std::string VMFillLevelSetWithTetrahedra::className() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::className;
}

//------------------------------------------------------------------------------
Uuid VMFillLevelSetWithTetrahedra::uuid() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::uuid;
}

//------------------------------------------------------------------------------
std::string VMFillLevelSetWithTetrahedra::humanName() const
{
  return "Fill Level Set with Tetrahedra (VolumeMeshing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> VMFillLevelSetWithTetrahedra::defaultTags() const
{
  return {"#Volume Meshing", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters VMFillLevelSetWithTetrahedra::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_AutoDetectFeatures_Key, "Auto Detect Features", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_AngleThreshold_Key, "Angle Threshold", "", 2.3456789));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainerName_Key, "Triangle Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_LevelSetArrayPath_Key, "Level Set", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_TetrahedralDataContainerName_Key, "Tetrahedral Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TetFeatureIdsName_Key, "Feature Ids", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AutoDetectFeatures_Key, k_TriangleDataContainerName_Key, true);
  params.linkParameters(k_AutoDetectFeatures_Key, k_AngleThreshold_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VMFillLevelSetWithTetrahedra::clone() const
{
  return std::make_unique<VMFillLevelSetWithTetrahedra>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VMFillLevelSetWithTetrahedra::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pAutoDetectFeaturesValue = filterArgs.value<bool>(k_AutoDetectFeatures_Key);
  auto pAngleThresholdValue = filterArgs.value<float64>(k_AngleThreshold_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pLevelSetArrayPathValue = filterArgs.value<DataPath>(k_LevelSetArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTetrahedralDataContainerNameValue = filterArgs.value<DataPath>(k_TetrahedralDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pTetFeatureIdsNameValue = filterArgs.value<StringParameter::ValueType>(k_TetFeatureIdsName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> VMFillLevelSetWithTetrahedra::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAutoDetectFeaturesValue = filterArgs.value<bool>(k_AutoDetectFeatures_Key);
  auto pAngleThresholdValue = filterArgs.value<float64>(k_AngleThreshold_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pLevelSetArrayPathValue = filterArgs.value<DataPath>(k_LevelSetArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTetrahedralDataContainerNameValue = filterArgs.value<DataPath>(k_TetrahedralDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pTetFeatureIdsNameValue = filterArgs.value<StringParameter::ValueType>(k_TetFeatureIdsName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
