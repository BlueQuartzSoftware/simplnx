#include "ComputeFeatureFaceMisorientationFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureFaceMisorientationFilter::name() const
{
  return FilterTraits<ComputeFeatureFaceMisorientationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureFaceMisorientationFilter::className() const
{
  return FilterTraits<ComputeFeatureFaceMisorientationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureFaceMisorientationFilter::uuid() const
{
  return FilterTraits<ComputeFeatureFaceMisorientationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureFaceMisorientationFilter::humanName() const
{
  return "Compute Feature Face Misorientation (Face)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureFaceMisorientationFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography", "Generate"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureFaceMisorientationFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Triangle Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Specifies which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of each Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which phase each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Face Data"});

  params.insert(std::make_unique<DataObjectNameParameter>(k_MisorientationArrayName_Key, "Misorientation",
                                                          "The name of the array containing the misorientation angle (in degrees) between the 2 features.", "Face Misorientations"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeatureFaceMisorientationFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Description:
  //
  // Change 1:
  // Renamed parameter key
  //   k_SurfaceMeshFaceMisorientationColorsArrayName_Key ("surface_mesh_face_misorientation_colors_array_name")
  //   -> k_MisorientationArrayName_Key ("misorientation_array_name")
  //
  // Change 2:
  // Output array component shape changed from {3} ("axis * angle" colors) to {1} (misorientation angle in degrees).
  //
  // Solution: Pipelines using the old key must be updated to the new key. No automatic migration is provided.
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureFaceMisorientationFilter::clone() const
{
  return std::make_unique<ComputeFeatureFaceMisorientationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureFaceMisorientationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceMisorientationColorsArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_MisorientationArrayName_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // make sure all the cell data has same number of tuples (i.e. they should all be coming from the same Image Geometry)
  std::vector<DataPath> imageArrayPaths = {pAvgQuatsArrayPathValue, pFeaturePhasesArrayPathValue};
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(imageArrayPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-98410, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto faceLabels = dataStructure.getDataAs<Int32Array>(pSurfaceMeshFaceLabelsArrayPathValue);
  if(faceLabels == nullptr)
  {
    return MakePreflightErrorResult(-98411, fmt::format("Could not find the face labels data array at path '{}'", pSurfaceMeshFaceLabelsArrayPathValue.toString()));
  }

  {
    DataPath faceMisorientationColorsArrayPath = pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSurfaceMeshFaceMisorientationColorsArrayNameValue);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabels->getTupleShape(), std::vector<usize>{1}, faceMisorientationColorsArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureFaceMisorientationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeatureFaceMisorientationInputValues inputValues;

  inputValues.surfaceMeshFaceLabelsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.avgQuatsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_AvgQuatsArrayPath_Key);
  inputValues.featurePhasesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeaturePhasesArrayPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CrystalStructuresArrayPath_Key);
  inputValues.misorientationArrayPath = inputValues.surfaceMeshFaceLabelsArrayPath.replaceName(filterArgs.value<DataObjectNameParameter::ValueType>(k_MisorientationArrayName_Key));

  return ComputeFeatureFaceMisorientation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceMisorientationColorsArrayNameKey = "SurfaceMeshFaceMisorientationColorsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureFaceMisorientationFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureFaceMisorientationFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceMisorientationColorsArrayNameKey,
                                                                                                                   k_MisorientationArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
