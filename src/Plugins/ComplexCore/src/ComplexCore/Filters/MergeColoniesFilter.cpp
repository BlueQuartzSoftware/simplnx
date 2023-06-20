#include "MergeColoniesFilter.hpp"

#include "ComplexCore/Filters/Algorithms/MergeColonies.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MergeColoniesFilter::name() const
{
  return FilterTraits<MergeColoniesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string MergeColoniesFilter::className() const
{
  return FilterTraits<MergeColoniesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MergeColoniesFilter::uuid() const
{
  return FilterTraits<MergeColoniesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MergeColoniesFilter::humanName() const
{
  return "Merge Colonies";
}

//------------------------------------------------------------------------------
std::vector<std::string> MergeColoniesFilter::defaultTags() const
{
  return {"Reconstruction", "Grouping"};
}

//------------------------------------------------------------------------------
Parameters MergeColoniesFilter::parameters() const
{
  Parameters params;

  /**
   * Please separate the parameters into groups generally of the following:
   *
   * params.insertSeparator(Parameters::Separator{"Input Parameters"});
   * params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
   *
   * .. or create appropriate separators as needed. The UI in COMPLEX no longer
   * does this for the developer by using catgories as in SIMPL
   */

  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors", "", false #error Check default values));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 0.0f #error Check default values));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 0.0f #error Check default values));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_IdentifyGlobAlpha_Key, "Identify Glob Alpha", "", false #error Check default values));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_GlobAlphaArrayName_Key, "Glob Alpha", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);
  params.linkParameters(k_IdentifyGlobAlpha_Key, k_GlobAlphaArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MergeColoniesFilter::clone() const
{
  return std::make_unique<MergeColoniesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MergeColoniesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pUseNonContiguousNeighborsValue = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  auto pContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pIdentifyGlobAlphaValue = filterArgs.value<bool>(k_IdentifyGlobAlpha_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellParentIdsArrayNameValue = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  auto pGlobAlphaArrayNameValue = filterArgs.value<DataPath>(k_GlobAlphaArrayName_Key);
  auto pNewCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);



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
  // None found in this filter based on the filter parameters

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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MergeColoniesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{

  MergeColoniesInputValues inputValues;

    inputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  inputValues.NonContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  inputValues.ContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.IdentifyGlobAlpha = filterArgs.value<bool>(k_IdentifyGlobAlpha_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CellParentIdsArrayName = filterArgs.value<DataPath>(k_CellParentIdsArrayName_Key);
  inputValues.GlobAlphaArrayName = filterArgs.value<DataPath>(k_GlobAlphaArrayName_Key);
  inputValues.NewCellFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_NewCellFeatureAttributeMatrixName_Key);
  inputValues.FeatureParentIdsArrayName = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  inputValues.ActiveArrayName = filterArgs.value<DataPath>(k_ActiveArrayName_Key);


  return MergeColonies(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
