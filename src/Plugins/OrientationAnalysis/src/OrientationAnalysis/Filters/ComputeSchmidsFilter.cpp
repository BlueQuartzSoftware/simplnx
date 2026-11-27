#include "ComputeSchmidsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeSchmids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/VectorParameter.hpp"

#include <cmath>
#include <limits>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeSchmidsFilter::name() const
{
  return FilterTraits<ComputeSchmidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeSchmidsFilter::className() const
{
  return FilterTraits<ComputeSchmidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeSchmidsFilter::uuid() const
{
  return FilterTraits<ComputeSchmidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeSchmidsFilter::humanName() const
{
  return "Compute Schmid Factors";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeSchmidsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeSchmidsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<VectorFloat32ParameterType>(k_LoadingDirection_Key, "Loading Direction", "A non-zero loading direction in the sample reference frame",
                                                         std::vector<float32>({1.0F, 1.0F, 1.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_StoreAngleComponents_Key, "Store Angle Components of Schmid Factor", "Whether to store the angle components for each Feature", false));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_OverrideSystem_Key, "Override Default Slip System", "Allows the user to manually input the slip plane and slip direction", false));
  params.insert(std::make_unique<VectorFloat32ParameterType>(k_SlipPlane_Key, "Slip Plane", "A non-zero vector defining the slip plane normal", std::vector<float32>({0.0F, 0.0F, 1.0F}),
                                                         std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32ParameterType>(k_SlipDirection_Key, "Slip Direction", "A non-zero vector defining the slip direction", std::vector<float32>({1.0F, 0.0F, 0.0F}),
                                                         std::vector<std::string>({"X", "Y", "Z"})));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies the Ensemble to which each Feature belongs", DataPath({"Cell Feature Data", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of each Feature in quaternion representation",
                                                          DataPath({"Cell Feature Data", "AvgQuats"}), ArraySelectionParameter::AllowedTypeSetType{nx::core::DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypeSetType{nx::core::DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_SchmidsArrayName_Key, "Schmids", "The name of the array containing the value of the Schmid factor for the most favorably oriented slip system (i.e., the one with the highest Schmid factor)",
      "Schmids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SlipSystemsArrayName_Key, "Slip Systems",
                                                          "The name of the array containing the enumeration of the slip system that has the highest Schmid factor", "SlipSystems"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_PolesArrayName_Key, "Poles", "The name of the array containing the normalized loading direction in the crystal frame, multiplied by 100 and truncated toward zero", "Poles"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_PhisArrayName_Key, "Phis", "The name of the array containing cos(phi) when the default slip systems are used or phi in radians when they are overridden", "Schmid_Phis"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_LambdasArrayName_Key, "Lambdas", "The name of the array containing cos(lambda) when the default slip systems are used or lambda in radians when they are overridden", "Schmid_Lambdas"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_StoreAngleComponents_Key, k_PhisArrayName_Key, true);
  params.linkParameters(k_StoreAngleComponents_Key, k_LambdasArrayName_Key, true);
  params.linkParameters(k_OverrideSystem_Key, k_SlipPlane_Key, true);
  params.linkParameters(k_OverrideSystem_Key, k_SlipDirection_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeSchmidsFilter::parametersVersion() const
{
  // Version 2 clarifies the vector constraints and the mode-dependent output-array semantics.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeSchmidsFilter::clone() const
{
  return std::make_unique<ComputeSchmidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeSchmidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                             const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pLoadingDirectionValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_LoadingDirection_Key);
  auto pStoreAngleComponentsValue = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  auto pOverrideSystemValue = filterArgs.value<bool>(k_OverrideSystem_Key);
  auto pSlipPlaneValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_SlipPlane_Key);
  auto pSlipDirectionValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_SlipDirection_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  DataPath cellFeatureDataPath = pFeaturePhasesArrayPathValue.getParent();
  auto pSchmidsArrayNameValue = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_SchmidsArrayName_Key));
  auto pSlipSystemsArrayNameValue = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_SlipSystemsArrayName_Key));
  auto pPolesArrayNameValue = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_PolesArrayName_Key));
  auto pPhisArrayNameValue = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_PhisArrayName_Key));
  auto pLambdasArrayNameValue = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_LambdasArrayName_Key));

  nx::core::Result<OutputActions> resultOutputActions;

  const Int32ArrayType& phasesRef = dataStructure.getDataRefAs<Int32ArrayType>(pFeaturePhasesArrayPathValue);
  const UInt32ArrayType& crystalStructuresRef = dataStructure.getDataRefAs<UInt32ArrayType>(pCrystalStructuresArrayPathValue);

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples({pFeaturePhasesArrayPathValue, pAvgQuatsArrayPathValue});
  if(!tupleValidityCheck)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-13503, fmt::format("The Feature Phases array '{}' and Average Quaternions array '{}' must have equal tuple counts. {}",
                                                                                pFeaturePhasesArrayPathValue.toString(), pAvgQuatsArrayPathValue.toString(), tupleValidityCheck.error()))};
  }

  if(phasesRef.getNumberOfTuples() == 0)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                -13504, fmt::format("The Feature Phases array '{}' and Average Quaternions array '{}' each contain 0 tuples. They must contain at least the Feature 0 sentinel tuple.",
                                    pFeaturePhasesArrayPathValue.toString(), pAvgQuatsArrayPathValue.toString()))};
  }

  if(crystalStructuresRef.getNumberOfTuples() == 0)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                -13505, fmt::format("The Crystal Structures array '{}' contains 0 tuples. It must contain at least one Ensemble tuple.", pCrystalStructuresArrayPathValue.toString()))};
  }

  const auto vectorMagnitude = [](const VectorFloat32ParameterType::ValueType& vector) {
    return std::sqrt(static_cast<float64>(vector[0]) * vector[0] + static_cast<float64>(vector[1]) * vector[1] + static_cast<float64>(vector[2]) * vector[2]);
  };

  if(vectorMagnitude(pLoadingDirectionValue) < std::numeric_limits<float32>::epsilon())
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-13506, fmt::format("Loading Direction <{}, {}, {}> has zero length. Its components must define a non-zero direction.",
                                                                                pLoadingDirectionValue[0], pLoadingDirectionValue[1], pLoadingDirectionValue[2]))};
  }

  const auto tupleShape = phasesRef.getIDataStore()->getTupleShape();

  // All five output arrays are created with an explicit "0" fill value. The algorithm skips any
  // Feature whose crystal structure is outside the supported Laue-group range, and Feature 0 is a
  // sentinel slot that is never computed. Legacy SIMPL FindSchmids allocated with initValue 0.
  const std::string kZeroFill = "0";

  // Create output Schmids Array
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pSchmidsArrayNameValue, "", kZeroFill);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create output SlipSystems Array
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{1}, pSlipSystemsArrayNameValue, "", kZeroFill);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create output Poles Array
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{3}, pPolesArrayNameValue, "", kZeroFill);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create output Phis Array
  if(pStoreAngleComponentsValue)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pPhisArrayNameValue, "", kZeroFill);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create output Lambdas Array
  if(pStoreAngleComponentsValue)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pLambdasArrayNameValue, "", kZeroFill);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  if(pOverrideSystemValue)
  {
    if(vectorMagnitude(pSlipPlaneValue) < std::numeric_limits<float32>::epsilon())
    {
      return {.output_actions = MakeErrorResult<OutputActions>(
                  -13507, fmt::format("Slip Plane <{}, {}, {}> has zero length. Its components must define a non-zero plane normal.", pSlipPlaneValue[0], pSlipPlaneValue[1], pSlipPlaneValue[2]))};
    }
    if(vectorMagnitude(pSlipDirectionValue) < std::numeric_limits<float32>::epsilon())
    {
      return {.output_actions = MakeErrorResult<OutputActions>(-13508, fmt::format("Slip Direction <{}, {}, {}> has zero length. Its components must define a non-zero direction.",
                                                                                  pSlipDirectionValue[0], pSlipDirectionValue[1], pSlipDirectionValue[2]))};
    }

    // make sure direction lies in plane
    const float32 dotProduct = pSlipPlaneValue[0] * pSlipDirectionValue[0] + pSlipPlaneValue[1] * pSlipDirectionValue[1] + pSlipPlaneValue[2] * pSlipDirectionValue[2];
    if(0.0F != dotProduct)
    {
      return {.output_actions = MakeErrorResult<OutputActions>(
                  -13500, fmt::format("Slip Plane <{}, {}, {}> and Slip Direction <{}, {}, {}> have a dot product of {}. The vectors must be perpendicular.", pSlipPlaneValue[0], pSlipPlaneValue[1],
                                      pSlipPlaneValue[2], pSlipDirectionValue[0], pSlipDirectionValue[1], pSlipDirectionValue[2], dotProduct))};
    }
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeSchmidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeSchmidsInputValues inputValues;

  inputValues.loading_direction = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_LoadingDirection_Key);
  inputValues.store_angle_components = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  inputValues.override_system = filterArgs.value<bool>(k_OverrideSystem_Key);
  inputValues.slip_plane = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_SlipPlane_Key);
  inputValues.slip_direction = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_SlipDirection_Key);
  inputValues.feature_phases_array_path = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.avg_quats_array_path = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  DataPath cellFeatureDataPath = inputValues.feature_phases_array_path.getParent();
  inputValues.schmids_array_name = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_SchmidsArrayName_Key));
  inputValues.slip_systems_array_name = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_SlipSystemsArrayName_Key));
  inputValues.poles_array_name = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_PolesArrayName_Key));
  inputValues.phis_array_name = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_PhisArrayName_Key));
  inputValues.lambdas_array_name = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_LambdasArrayName_Key));

  return ComputeSchmids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_LoadingDirectionKey = "LoadingDirection";
constexpr StringLiteralType k_StoreAngleComponentsKey = "StoreAngleComponents";
constexpr StringLiteralType k_OverrideSystemKey = "OverrideSystem";
constexpr StringLiteralType k_SlipPlaneKey = "SlipPlane";
constexpr StringLiteralType k_SlipDirectionKey = "SlipDirection";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_SchmidsArrayNameKey = "SchmidsArrayName";
constexpr StringLiteralType k_SlipSystemsArrayNameKey = "SlipSystemsArrayName";
constexpr StringLiteralType k_PolesArrayNameKey = "PolesArrayName";
constexpr StringLiteralType k_PhisArrayNameKey = "PhisArrayName";
constexpr StringLiteralType k_LambdasArrayNameKey = "LambdasArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeSchmidsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeSchmidsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_LoadingDirectionKey, k_LoadingDirection_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_StoreAngleComponentsKey, k_StoreAngleComponents_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_OverrideSystemKey, k_OverrideSystem_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_SlipPlaneKey, k_SlipPlane_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_SlipDirectionKey, k_SlipDirection_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_SchmidsArrayNameKey, k_SchmidsArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_SlipSystemsArrayNameKey, k_SlipSystemsArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_PolesArrayNameKey, k_PolesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_PhisArrayNameKey, k_PhisArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_LambdasArrayNameKey, k_LambdasArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
