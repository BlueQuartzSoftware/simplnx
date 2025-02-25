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
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "Degree of tolerance for angular distance from the [111] axis", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "Degree of tolerance for angular deviation from 60 degrees", 0.0f));
  params.insert(std::make_unique<ChoicesParameter>(k_BoundariesArrayType_Key, "Output Type for Twin Boundaries Array",
                                                   "The Twin Boundaries Array is essentially a mask; This allows for determining how the mask is stored; uint8 recommended.", 0ULL,
                                                   ChoicesParameter::Choices{"boolean", "uint8"}));

  params.insertSeparator(Parameters::Separator{"Input Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "Specifies to which Feature each Face of Triangle belongs to",
                                                          DataPath({"Face Data", "FaceLabels"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceNormalsArrayPath_Key, "Face Normals", "Specifies the Normal of each face. Only required if Find Coherence is checked",
                                                          DataPath({"Face Data", "FaceNormals"}), ArraySelectionParameter::AllowedTypes{DataType::float64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of the Feature in quaternion representation",
                                                          DataPath({"Feature Data", "AvgQuats"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs", DataPath({"Feature Data", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Specifies the crystal structure for each phase (in enumeration representation)",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

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
IFilter::UniquePointer ComputeTwinBoundariesFilter::clone() const
{
  return std::make_unique<ComputeTwinBoundariesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeTwinBoundariesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pFindCoherence = filterArgs.value<BoolParameter::ValueType>(k_FindCoherence_Key);
  auto pBoundariesArrayTypeChoice = filterArgs.value<ChoicesParameter::ValueType>(k_BoundariesArrayType_Key);
  auto pFaceLabelsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FaceLabelsArrayPath_Key);
  auto pTwinBoundariesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesName_Key);
  auto pTwinBoundariesIncoherenceName = filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesIncoherenceName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto* faceLabels = dataStructure.getDataAs<IDataArray>(pFaceLabelsArrayPath);
  if(faceLabels == nullptr)
  {
    return MakePreflightErrorResult(-94730, "Input Face Labels Array must be valid.");
  }

  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  if(pBoundariesArrayTypeChoice == 0ULL)
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::boolean, faceLabels->getTupleShape(), std::vector<usize>{1ULL}, pFaceLabelsArrayPath.getParent().createChildPath(pTwinBoundariesName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  else if(pBoundariesArrayTypeChoice == 1ULL)
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::uint8, faceLabels->getTupleShape(), std::vector<usize>{1ULL}, pFaceLabelsArrayPath.getParent().createChildPath(pTwinBoundariesName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  if(pFindCoherence)
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, faceLabels->getTupleShape(), std::vector<usize>{1ULL}, pFaceLabelsArrayPath.getParent().createChildPath(pTwinBoundariesIncoherenceName));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-93214, "Finding the twin boundaries requires Cubic-Low m-3 or Cubic-High m-3m type crystal structures. Make sure your data is of one of these two types."});

  // Return both the resultOutputActions via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeTwinBoundariesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ComputeTwinBoundariesInputValues inputValues;

  inputValues.FindCoherence = filterArgs.value<BoolParameter::ValueType>(k_FindCoherence_Key);
  inputValues.AxisTolerance = filterArgs.value<Float32Parameter::ValueType>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<Float32Parameter::ValueType>(k_AngleTolerance_Key);
  inputValues.FaceLabelsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FaceLabelsArrayPath_Key);
  inputValues.FaceNormalsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FaceNormalsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CrystalStructuresArrayPath_Key);

  const DataPath parentPath = inputValues.FaceLabelsArrayPath.getParent();

  inputValues.TwinBoundariesArrayPath = parentPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesName_Key));
  inputValues.TwinBoundaryIncoherenceArrayPath = parentPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_TwinBoundariesIncoherenceName_Key));

  return ComputeTwinBoundaries(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FindCoherenceKey = "FindCoherence";
constexpr StringLiteral k_AxisToleranceKey = "AxisTolerance";
constexpr StringLiteral k_AngleToleranceKey = "AngleTolerance";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceNormalsArrayPathKey = "SurfaceMeshFaceNormalsArrayPath";
constexpr StringLiteral k_SurfaceMeshTwinBoundaryArrayNameKey = "SurfaceMeshTwinBoundaryArrayName";
constexpr StringLiteral k_SurfaceMeshTwinBoundaryIncoherenceArrayNameKey = "SurfaceMeshTwinBoundaryIncoherenceArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeTwinBoundariesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeTwinBoundariesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Algorithm Modifiers
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindCoherenceKey, k_FindCoherence_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<Float32Parameter::ValueType>>(args, json, SIMPL::k_AxisToleranceKey, k_AxisTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<Float32Parameter::ValueType>>(args, json, SIMPL::k_AngleToleranceKey, k_AngleTolerance_Key));

  // Input Face Data
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_FaceLabelsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceNormalsArrayPathKey, k_FaceNormalsArrayPath_Key));

  // Input Feature Data
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));

  // Input Ensemble Data
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));

  // Output Face Data
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTwinBoundaryArrayNameKey,
                                                                                                                                  k_TwinBoundariesName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTwinBoundaryIncoherenceArrayNameKey,
                                                                                                                                  k_TwinBoundariesIncoherenceName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
