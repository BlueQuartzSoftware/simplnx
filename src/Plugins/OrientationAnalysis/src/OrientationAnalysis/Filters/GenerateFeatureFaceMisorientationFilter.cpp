#include "GenerateFeatureFaceMisorientationFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GenerateFeatureFaceMisorientation.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string GenerateFeatureFaceMisorientationFilter::name() const
{
  return FilterTraits<GenerateFeatureFaceMisorientationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateFeatureFaceMisorientationFilter::className() const
{
  return FilterTraits<GenerateFeatureFaceMisorientationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateFeatureFaceMisorientationFilter::uuid() const
{
  return FilterTraits<GenerateFeatureFaceMisorientationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateFeatureFaceMisorientationFilter::humanName() const
{
  return "Generate Feature Face Misorientation (Face)";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateFeatureFaceMisorientationFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateFeatureFaceMisorientationFilter::parameters() const
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
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key, "Misorientation Colors", "A set of RGB color schemes encoded as floats for each Face",
                                                          "FaceMisorientationColors"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateFeatureFaceMisorientationFilter::clone() const
{
  return std::make_unique<GenerateFeatureFaceMisorientationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateFeatureFaceMisorientationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceMisorientationColorsArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
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

  DataPath faceMisorientationColorsArrayPath = pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSurfaceMeshFaceMisorientationColorsArrayNameValue);
  auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabels->getTupleShape(), std::vector<usize>{3}, faceMisorientationColorsArrayPath);
  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateFeatureFaceMisorientationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  GenerateFeatureFaceMisorientationInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SurfaceMeshFaceMisorientationColorsArrayName = filterArgs.value<std::string>(k_SurfaceMeshFaceMisorientationColorsArrayName_Key);

  return GenerateFeatureFaceMisorientation(dataStructure, messageHandler, shouldCancel, &inputValues)();
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

Result<Arguments> GenerateFeatureFaceMisorientationFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = GenerateFeatureFaceMisorientationFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceMisorientationColorsArrayNameKey,
                                                                                                                   k_SurfaceMeshFaceMisorientationColorsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
