#include "FindSchmidsFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindSchmids.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSchmidsFilter::name() const
{
  return FilterTraits<FindSchmidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSchmidsFilter::className() const
{
  return FilterTraits<FindSchmidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindSchmidsFilter::uuid() const
{
  return FilterTraits<FindSchmidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSchmidsFilter::humanName() const
{
  return "Find Schmid Factors";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSchmidsFilter::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindSchmidsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Parameters"});

  params.insert(std::make_unique<VectorFloat32Parameter>(k_LoadingDirection_Key, "Loading Direction", "", std::vector<float32>({1.0F, 1.0F, 1.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreAngleComponents_Key, "Store Angle Components of Schmid Factor", "", false));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_OverrideSystem_Key, "Override Default Slip System", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SlipPlane_Key, "Slip Plane", "", std::vector<float32>({0.0F, 0.0F, 1.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SlipDirection_Key, "Slip Direction", "", std::vector<float32>({1.0F, 0.0F, 0.0F}), std::vector<std::string>({"X", "Y", "Z"})));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath({"CellFeatureData", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{complex::DataType::int32}, ArraySelectionParameter::ComponentTypes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath({"CellFeatureData", "AvgQuats"}),
                                                          ArraySelectionParameter::AllowedTypes{complex::DataType::float32}, ArraySelectionParameter::ComponentTypes{{4}}));
  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{complex::DataType::uint32}, ArraySelectionParameter::ComponentTypes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SchmidsArrayName_Key, "Schmids", "", "Schmids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SlipSystemsArrayName_Key, "Slip Systems", "", "SlipSystems"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PolesArrayName_Key, "Poles", "", "Poles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhisArrayName_Key, "Phis", "", "Schmid_Phis"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_LambdasArrayName_Key, "Lambdas", "", "Schmid_Lambdas"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_StoreAngleComponents_Key, k_PhisArrayName_Key, true);
  params.linkParameters(k_StoreAngleComponents_Key, k_LambdasArrayName_Key, true);
  params.linkParameters(k_OverrideSystem_Key, k_SlipPlane_Key, true);
  params.linkParameters(k_OverrideSystem_Key, k_SlipDirection_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSchmidsFilter::clone() const
{
  return std::make_unique<FindSchmidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSchmidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pLoadingDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDirection_Key);
  auto pStoreAngleComponentsValue = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  auto pOverrideSystemValue = filterArgs.value<bool>(k_OverrideSystem_Key);
  auto pSlipPlaneValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipPlane_Key);
  auto pSlipDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipDirection_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  DataPath cellFeatDataPath = pFeaturePhasesArrayPathValue.getParent();
  auto pSchmidsArrayNameValue = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_SchmidsArrayName_Key));
  auto pSlipSystemsArrayNameValue = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_SlipSystemsArrayName_Key));
  auto pPolesArrayNameValue = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_PolesArrayName_Key));
  auto pPhisArrayNameValue = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_PhisArrayName_Key));
  auto pLambdasArrayNameValue = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_LambdasArrayName_Key));

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  DataPath featureDataGroup = pFeaturePhasesArrayPathValue.getParent();

  const Int32Array& phases = dataStructure.getDataRefAs<Int32Array>(pFeaturePhasesArrayPathValue);
  auto tupleShape = phases.getIDataStore()->getTupleShape();

  // Create output Schmids Array
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pSchmidsArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create output SlipSystems Array
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{1}, pSlipSystemsArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create output SlipSystems Array
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{3}, pPolesArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create output SlipSystems Array
  if(pStoreAngleComponentsValue)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pPhisArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create output Lambdas Array
  if(pStoreAngleComponentsValue)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pLambdasArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  if(pOverrideSystemValue)
  {
    // make sure direction lies in plane
    float cosVec = pSlipPlaneValue[0] * pSlipDirectionValue[0] + pSlipPlaneValue[1] * pSlipDirectionValue[1] + pSlipPlaneValue[2] * pSlipDirectionValue[2];
    if(0.0F != cosVec)
    {
      return {MakeErrorResult<OutputActions>(-13500, "Slip Plane and Slip Direction must be normal")};
    }
  }

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindSchmidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  FindSchmidsInputValues inputValues;

  inputValues.LoadingDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDirection_Key);
  inputValues.StoreAngleComponents = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  inputValues.OverrideSystem = filterArgs.value<bool>(k_OverrideSystem_Key);
  inputValues.SlipPlane = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipPlane_Key);
  inputValues.SlipDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipDirection_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  DataPath cellFeatDataPath = inputValues.FeaturePhasesArrayPath.getParent();
  inputValues.SchmidsArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_SchmidsArrayName_Key));
  inputValues.SlipSystemsArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_SlipSystemsArrayName_Key));
  inputValues.PolesArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_PolesArrayName_Key));
  inputValues.PhisArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_PhisArrayName_Key));
  inputValues.LambdasArrayName = cellFeatDataPath.createChildPath(filterArgs.value<std::string>(k_LambdasArrayName_Key));

  return FindSchmids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
