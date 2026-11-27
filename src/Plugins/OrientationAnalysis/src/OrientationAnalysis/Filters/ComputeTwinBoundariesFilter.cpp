#include "ComputeTwinBoundariesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeTwinBoundaries.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeTwinBoundariesFilter::name() const
{
  return FilterTraits<ComputeTwinBoundariesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeTwinBoundariesFilter::className() const
{
  return FilterTraits<ComputeTwinBoundariesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeTwinBoundariesFilter::uuid() const
{
  return FilterTraits<ComputeTwinBoundariesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeTwinBoundariesFilter::humanName() const
{
  return "Compute Twin Boundaries";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeTwinBoundariesFilter::defaultTags() const
{
  return {className(), "Statistics", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeTwinBoundariesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Algorithm Modifiers"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindCoherence_Key, "Find Coherence", "If true, compute the coherence between the Face normal and the misorientation axis", true));
  params.insert(std::make_unique<Float32ParameterType>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "Degree of tolerance for angular distance from the [111] axis", 0.0f));
  params.insert(std::make_unique<Float32ParameterType>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "Degree of tolerance for angular deviation from 60 degrees", 0.0f));
  params.insert(std::make_unique<ChoicesParameter>(k_BoundariesArrayType_Key, "Output Type for Twin Boundaries Array",
                                                   "The Twin Boundaries Array is essentially a mask; This allows for determining how the mask is stored; uint8 recommended.", 0ULL,
                                                   ChoicesParameter::ChoicesType{"boolean", "uint8"}));

  params.insertSeparator(Parameters::Separator{"Input Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "Specifies to which Feature each Face of Triangle belongs to",
                                                          DataPath({"Face Data", "FaceLabels"}), ArraySelectionParameter::AllowedTypeSetType{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceNormalsArrayPath_Key, "Face Normals", "Specifies the Normal of each face. Only required if Find Coherence is checked",
                                                          DataPath({"Face Data", "FaceNormals"}), ArraySelectionParameter::AllowedTypeSetType{DataType::float64},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{3}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of the Feature in quaternion representation",
                                                          DataPath({"Feature Data", "AvgQuats"}), ArraySelectionParameter::AllowedTypeSetType{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs", DataPath({"Feature Data", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Specifies the crystal structure for each phase (in enumeration representation)",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypeSetType{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_TwinBoundariesName_Key, "Twin Boundaries Array name",
                                                          "The name of the array that will act as a mask for which triangles do make up a twin boundary (true)", "Twin Boundaries"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_TwinBoundariesIncoherenceName_Key, "Twin Boundaries Incoherence Array name",
      "The name of array that contains the crystal direction parallel to the Face normal determined relative to the misorientation; values are 180 by default", "Twin Boundaries Incoherence"));

  params.linkParameters(k_FindCoherence_Key, k_FaceNormalsArrayPath_Key, true);
  params.linkParameters(k_FindCoherence_Key, k_TwinBoundariesIncoherenceName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeTwinBoundariesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeTwinBoundariesFilter::clone() const
{
  return std::make_unique<ComputeTwinBoundariesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeTwinBoundariesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pFindCoherence = filterArgs.value<BoolParameter::ValueType>(k_FindCoherence_Key);
  auto pBoundariesArrayTypeChoice = filterArgs.value<ChoicesParameter::ValueType>(k_BoundariesArrayType_Key);
  auto pFaceLabelsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FaceLabelsArrayPath_Key);
  auto pTwinBoundariesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesName_Key);
  auto pTwinBoundariesIncoherenceName = filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesIncoherenceName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto* faceLabelsPtr = dataStructure.getDataAs<IDataArray>(pFaceLabelsArrayPath);
  if(faceLabelsPtr == nullptr)
  {
    return MakePreflightErrorResult(-94730, "Input Face Labels Array must be valid.");
  }

  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  if(pBoundariesArrayTypeChoice == 0ULL)
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::boolean, faceLabelsPtr->getTupleShape(), std::vector<usize>{1ULL}, pFaceLabelsArrayPath.getParent().createChildPath(pTwinBoundariesName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  else if(pBoundariesArrayTypeChoice == 1ULL)
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::uint8, faceLabelsPtr->getTupleShape(), std::vector<usize>{1ULL}, pFaceLabelsArrayPath.getParent().createChildPath(pTwinBoundariesName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  if(pFindCoherence)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsPtr->getTupleShape(), std::vector<usize>{1ULL},
                                                                 pFaceLabelsArrayPath.getParent().createChildPath(pTwinBoundariesIncoherenceName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-93214, "Finding the twin boundaries requires Cubic-Low m-3 or Cubic-High m-3m type crystal structures. Make sure your data is of one of these two types."});

  // Return both the resultOutputActions via std::move()
  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeTwinBoundariesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeTwinBoundariesInputValues inputValues;

  inputValues.find_coherence = filterArgs.value<BoolParameter::ValueType>(k_FindCoherence_Key);
  inputValues.axis_tolerance = filterArgs.value<Float32ParameterType::ValueType>(k_AxisTolerance_Key);
  inputValues.angle_tolerance = filterArgs.value<Float32ParameterType::ValueType>(k_AngleTolerance_Key);
  inputValues.face_labels_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FaceLabelsArrayPath_Key);
  inputValues.face_normals_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FaceNormalsArrayPath_Key);
  inputValues.avg_quats_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_AvgQuatsArrayPath_Key);
  inputValues.feature_phases_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeaturePhasesArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_CrystalStructuresArrayPath_Key);

  const DataPath parentPath = inputValues.face_labels_array_path.getParent();

  inputValues.twin_boundaries_array_path = parentPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesName_Key));
  inputValues.twin_boundary_incoherence_array_path = parentPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesIncoherenceName_Key));

  return ComputeTwinBoundaries(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_FindCoherenceKey = "FindCoherence";
constexpr StringLiteralType k_AxisToleranceKey = "AxisTolerance";
constexpr StringLiteralType k_AngleToleranceKey = "AngleTolerance";
constexpr StringLiteralType k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteralType k_SurfaceMeshFaceNormalsArrayPathKey = "SurfaceMeshFaceNormalsArrayPath";
constexpr StringLiteralType k_SurfaceMeshTwinBoundaryArrayNameKey = "SurfaceMeshTwinBoundaryArrayName";
constexpr StringLiteralType k_SurfaceMeshTwinBoundaryIncoherenceArrayNameKey = "SurfaceMeshTwinBoundaryIncoherenceArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeTwinBoundariesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeTwinBoundariesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Algorithm Modifiers
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_FindCoherenceKey, k_FindCoherence_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<Float32ParameterType::ValueType>>(args, json, simpl::k_AxisToleranceKey, k_AxisTolerance_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<Float32ParameterType::ValueType>>(args, json, simpl::k_AngleToleranceKey, k_AngleTolerance_Key));

  // Input Face Data
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceLabelsArrayPathKey, k_FaceLabelsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceNormalsArrayPathKey, k_FaceNormalsArrayPath_Key));

  // Input Feature Data
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));

  // Input Ensemble Data
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));

  // Output Face Data
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_SurfaceMeshTwinBoundaryArrayNameKey, k_TwinBoundariesName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_SurfaceMeshTwinBoundaryIncoherenceArrayNameKey,
                                                                                                                   k_TwinBoundariesIncoherenceName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
