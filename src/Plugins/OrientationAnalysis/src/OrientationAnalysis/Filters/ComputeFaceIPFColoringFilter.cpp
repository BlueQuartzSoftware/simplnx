#include "ComputeFaceIPFColoringFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeFaceIPFColoring.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFaceIPFColoringFilter::name() const
{
  return FilterTraits<ComputeFaceIPFColoringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFaceIPFColoringFilter::className() const
{
  return FilterTraits<ComputeFaceIPFColoringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFaceIPFColoringFilter::uuid() const
{
  return FilterTraits<ComputeFaceIPFColoringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFaceIPFColoringFilter::humanName() const
{
  return "Compute IPF Colors (Face)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFaceIPFColoringFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography", "Generate", "IPF Colors", "Surface Mesh", "Triangles"};
}

//------------------------------------------------------------------------------
Parameters ComputeFaceIPFColoringFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_ColorKey_Key, "Color Key",
                                                   "Which IPF color scheme to use:\n"
                                                   "  TSL: Primary-corner SST coloring (EDAX/OIM Analysis default).\n"
                                                   "  PUCM: Perceptually-uniform color map (Patala / MTEX-style).\n"
                                                   "  Nolze-Hielscher: MTEX HSV-style coloring.",
                                                   0, ChoicesParameter::ChoicesType{"TSL", "PUCM", "Nolze-Hielscher"}));

  params.insertSeparator(Parameters::Separator{"Input Triangle Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Specifies which Features are on either side of each Face", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "Specifies the normal of each Face", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)",
                                                          DataPath{}, GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which phase each Feature belongs", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FirstFaceIPFColorsArrayName_Key, "First Set of IPF Colors", "The first set of RGB color schemes encoded as unsigned chars for each Face",
                                                          "Face IPF Colors (0)"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SecondFaceIPFColorsArrayName_Key, "Second Set of IPF Colors", "The second set of RGB color schemes encoded as unsigned chars for each Face",
                                                          "Face IPF Colors (1)"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFaceIPFColoringFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeFaceIPFColoringFilter::clone() const
{
  return std::make_unique<ComputeFaceIPFColoringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFaceIPFColoringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                     const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pFirstFaceIPFColorsArrayNameValue = filterArgs.value<std::string>(k_FirstFaceIPFColorsArrayName_Key);
  auto pSecondFaceIPFColorsArrayNameValue = filterArgs.value<std::string>(k_SecondFaceIPFColorsArrayName_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // make sure all the face data has same number of tuples (i.e. they should all be coming from the same Triangle Geometry)
  std::vector<DataPath> triangleArrayPaths = {pSurfaceMeshFaceLabelsArrayPathValue, pSurfaceMeshFaceNormalsArrayPathValue};
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(triangleArrayPaths);
  if(!tupleValidityCheck)
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(-2430, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // make sure all the cell data has same number of tuples (i.e. they should all be coming from the same Image Geometry)
  std::vector<DataPath> imageArrayPaths = {pFeatureEulerAnglesArrayPathValue, pFeaturePhasesArrayPathValue};
  tupleValidityCheck = dataStructure.validateNumberOfTuples(imageArrayPaths);
  if(!tupleValidityCheck)
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(-2432, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& faceLabels = dataStructure.getDataRefAs<IDataArray>(pSurfaceMeshFaceLabelsArrayPathValue);
  DataPath faceIpfColorsArrayPath = pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pFirstFaceIPFColorsArrayNameValue);
  auto firstAction = std::make_unique<CreateArrayAction>(DataType::uint8, faceLabels.getTupleShape(), std::vector<usize>{3}, faceIpfColorsArrayPath);
  resultOutputActions.value().appendAction(std::move(firstAction));

  faceIpfColorsArrayPath = pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSecondFaceIPFColorsArrayNameValue);
  auto secondAction = std::make_unique<CreateArrayAction>(DataType::uint8, faceLabels.getTupleShape(), std::vector<usize>{3}, faceIpfColorsArrayPath);
  resultOutputActions.value().appendAction(std::move(secondAction));

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFaceIPFColoringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeFaceIPFColoringInputValues inputValues;

  inputValues.surface_mesh_face_labels_array_path = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.surface_mesh_face_normals_array_path = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.feature_euler_angles_array_path = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.feature_phases_array_path = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.first_face_ipf_colors_array_name = filterArgs.value<std::string>(k_FirstFaceIPFColorsArrayName_Key);
  inputValues.second_face_ipf_colors_array_name = filterArgs.value<std::string>(k_SecondFaceIPFColorsArrayName_Key);

  const auto colorKeyChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ColorKey_Key);
  switch(colorKeyChoice)
  {
  case 0:
    inputValues.color_key = ebsdlib::ColorKeyKind::TSL;
    break;
  case 1:
    inputValues.color_key = ebsdlib::ColorKeyKind::PUCM;
    break;
  case 2:
    inputValues.color_key = ebsdlib::ColorKeyKind::NolzeHielscher;
    break;
  default:
    return MakeErrorResult(-24340, fmt::format("Invalid Color Key choice: {}. Valid range is [0, 2].", colorKeyChoice));
  }

  return ComputeFaceIPFColoring(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteralType k_SurfaceMeshFaceNormalsArrayPathKey = "SurfaceMeshFaceNormalsArrayPath";
constexpr StringLiteralType k_FeatureEulerAnglesArrayPathKey = "FeatureEulerAnglesArrayPath";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_SurfaceMeshFaceIPFColorsArrayNameKey = "SurfaceMeshFaceIPFColorsArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeFaceIPFColoringFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFaceIPFColoringFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceNormalsArrayPathKey, k_SurfaceMeshFaceNormalsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureEulerAnglesArrayPathKey, k_FeatureEulerAnglesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceIPFColorsArrayNameKey, k_FirstFaceIPFColorsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
