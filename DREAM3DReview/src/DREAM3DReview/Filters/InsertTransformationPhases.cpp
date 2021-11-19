#include "InsertTransformationPhases.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string InsertTransformationPhases::name() const
{
  return FilterTraits<InsertTransformationPhases>::name.str();
}

//------------------------------------------------------------------------------
std::string InsertTransformationPhases::className() const
{
  return FilterTraits<InsertTransformationPhases>::className;
}

//------------------------------------------------------------------------------
Uuid InsertTransformationPhases::uuid() const
{
  return FilterTraits<InsertTransformationPhases>::uuid;
}

//------------------------------------------------------------------------------
std::string InsertTransformationPhases::humanName() const
{
  return "Insert Transformation Phases";
}

//------------------------------------------------------------------------------
std::vector<std::string> InsertTransformationPhases::defaultTags() const
{
  return {"#Unsupported", "#Packing"};
}

//------------------------------------------------------------------------------
Parameters InsertTransformationPhases::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_ParentPhase_Key, "Parent Phase", "", 1234356));
  params.insert(std::make_unique<ChoicesParameter>(k_TransCrystalStruct_Key, "Transformation Phase Crystal Structure", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Float32Parameter>(k_TransformationPhaseMisorientation_Key, "Transformation Phase Misorientation", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_DefineHabitPlane_Key, "Define Habit Plane", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_TransformationPhaseHabitPlane_Key, "Transformation Phase Habit Plane", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseAllVariants_Key, "Use All Variants", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_CoherentFrac_Key, "Coherent Fraction", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_TransformationPhaseThickness_Key, "Transformation Phase Thickness", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_NumTransformationPhasesPerFeature_Key, "Average Number Of Transformation Phases Per Feature", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_PeninsulaFrac_Key, "Peninsula Transformation Phase Fraction", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_StatsGenCellEnsembleAttributeMatrixPath_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhaseTypesArrayPath_Key, "Phase Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ShapeTypesArrayPath_Key, "Shape Types", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumFeaturesArrayPath_Key, "Number of Features", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumFeaturesPerParentArrayPath_Key, "Number of Features Per Parent", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DefineHabitPlane_Key, k_TransformationPhaseHabitPlane_Key, true);
  params.linkParameters(k_DefineHabitPlane_Key, k_UseAllVariants_Key, true);
  params.linkParameters(k_UseAllVariants_Key, k_CoherentFrac_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InsertTransformationPhases::clone() const
{
  return std::make_unique<InsertTransformationPhases>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InsertTransformationPhases::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pParentPhaseValue = filterArgs.value<int32>(k_ParentPhase_Key);
  auto pTransCrystalStructValue = filterArgs.value<ChoicesParameter::ValueType>(k_TransCrystalStruct_Key);
  auto pTransformationPhaseMisorientationValue = filterArgs.value<float32>(k_TransformationPhaseMisorientation_Key);
  auto pDefineHabitPlaneValue = filterArgs.value<bool>(k_DefineHabitPlane_Key);
  auto pTransformationPhaseHabitPlaneValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_TransformationPhaseHabitPlane_Key);
  auto pUseAllVariantsValue = filterArgs.value<bool>(k_UseAllVariants_Key);
  auto pCoherentFracValue = filterArgs.value<float32>(k_CoherentFrac_Key);
  auto pTransformationPhaseThicknessValue = filterArgs.value<float32>(k_TransformationPhaseThickness_Key);
  auto pNumTransformationPhasesPerFeatureValue = filterArgs.value<int32>(k_NumTransformationPhasesPerFeature_Key);
  auto pPeninsulaFracValue = filterArgs.value<float32>(k_PeninsulaFrac_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pStatsGenCellEnsembleAttributeMatrixPathValue = filterArgs.value<DataPath>(k_StatsGenCellEnsembleAttributeMatrixPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_PhaseTypesArrayPath_Key);
  auto pShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_ShapeTypesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pNumFeaturesPerParentArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesPerParentArrayPath_Key);

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
Result<> InsertTransformationPhases::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pParentPhaseValue = filterArgs.value<int32>(k_ParentPhase_Key);
  auto pTransCrystalStructValue = filterArgs.value<ChoicesParameter::ValueType>(k_TransCrystalStruct_Key);
  auto pTransformationPhaseMisorientationValue = filterArgs.value<float32>(k_TransformationPhaseMisorientation_Key);
  auto pDefineHabitPlaneValue = filterArgs.value<bool>(k_DefineHabitPlane_Key);
  auto pTransformationPhaseHabitPlaneValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_TransformationPhaseHabitPlane_Key);
  auto pUseAllVariantsValue = filterArgs.value<bool>(k_UseAllVariants_Key);
  auto pCoherentFracValue = filterArgs.value<float32>(k_CoherentFrac_Key);
  auto pTransformationPhaseThicknessValue = filterArgs.value<float32>(k_TransformationPhaseThickness_Key);
  auto pNumTransformationPhasesPerFeatureValue = filterArgs.value<int32>(k_NumTransformationPhasesPerFeature_Key);
  auto pPeninsulaFracValue = filterArgs.value<float32>(k_PeninsulaFrac_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pStatsGenCellEnsembleAttributeMatrixPathValue = filterArgs.value<DataPath>(k_StatsGenCellEnsembleAttributeMatrixPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pPhaseTypesArrayPathValue = filterArgs.value<DataPath>(k_PhaseTypesArrayPath_Key);
  auto pShapeTypesArrayPathValue = filterArgs.value<DataPath>(k_ShapeTypesArrayPath_Key);
  auto pNumFeaturesArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  auto pFeatureParentIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureParentIdsArrayName_Key);
  auto pNumFeaturesPerParentArrayPathValue = filterArgs.value<DataPath>(k_NumFeaturesPerParentArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
