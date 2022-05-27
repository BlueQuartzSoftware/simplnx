#include "ResampleImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ResampleImageGeom::name() const
{
  return FilterTraits<ResampleImageGeom>::name.str();
}

//------------------------------------------------------------------------------
std::string ResampleImageGeom::className() const
{
  return FilterTraits<ResampleImageGeom>::className;
}

//------------------------------------------------------------------------------
Uuid ResampleImageGeom::uuid() const
{
  return FilterTraits<ResampleImageGeom>::uuid;
}

//------------------------------------------------------------------------------
std::string ResampleImageGeom::humanName() const
{
  return "Resample Data (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ResampleImageGeom::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters ResampleImageGeom::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewDataContainer_Key, "Save as New Data Container", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataContainerPath_Key, "Data Container", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureIdsArrayPath_Key, true);
  params.linkParameters(k_SaveAsNewDataContainer_Key, k_NewDataContainerPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ResampleImageGeom::clone() const
{
  return std::make_unique<ResampleImageGeom>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ResampleImageGeom::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pNewDataContainerPathValue = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

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
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string currentGeomtryInfo;
  std::string newGeomtryInfo;

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pNewDataContainerPathValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"CurrentGeomtryInfo", currentGeomtryInfo});
  preflightUpdatedValues.push_back({"NewGeomtryInfo", newGeomtryInfo});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ResampleImageGeom::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pNewDataContainerPathValue = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
