#include "ScalarSegmentFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ScalarSegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
} // namespace

namespace nx::core
{

std::string ScalarSegmentFeaturesFilter::name() const
{
  return FilterTraits<ScalarSegmentFeaturesFilter>::name;
}

std::string ScalarSegmentFeaturesFilter::className() const
{
  return FilterTraits<ScalarSegmentFeaturesFilter>::className;
}

Uuid ScalarSegmentFeaturesFilter::uuid() const
{
  return FilterTraits<ScalarSegmentFeaturesFilter>::uuid;
}

std::string ScalarSegmentFeaturesFilter::humanName() const
{
  return "Segment Features (Scalar)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ScalarSegmentFeaturesFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Segmentation"};
}

Parameters ScalarSegmentFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<NumberParameter<int32>>(k_ScalarToleranceKey, "Scalar Tolerance", "Tolerance for segmenting input Cell Data", 1));
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatures_Key, "Randomize Feature Ids", "Specifies if feature IDs should be randomized during calculations", false));

  params.insert(std::make_unique<ChoicesParameter>(k_NeighborScheme_Key, "Neighbor Scheme", "How many neighbors to use", segment_features::k_6NeighborIndex, segment_features::k_OperationChoices));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Determines if a mask array is used for segmenting", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "Path to the DataArray Mask", DataPath(),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, std::make_any<bool>(true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeomPath_Key, "Input Image Geometry", "DataPath to input Image Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPathKey, "Scalar Array to Segment", "Path to the DataArray to segment", DataPath(), nx::core::GetIntegerDataTypes(),
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsName_Key, "Cell Feature IDs", "Path to the created Feature IDs path", "FeatureIds"));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellFeatureName_Key, "Feature Attribute Matrix", "Created Cell Feature Attribute Matrix", "Cell Feature Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ActiveArrayName_Key, "Active", "Created array", "Active"));

  params.insert(std::make_unique<BoolParameter>(k_IsPeriodic_Key, "Is Periodic", "Should segment features wrap around the image data", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ScalarSegmentFeaturesFilter::parametersVersion() const
{
  return 2;
}

IFilter::UniquePointerType ScalarSegmentFeaturesFilter::clone() const
{
  return std::make_unique<ScalarSegmentFeaturesFilter>();
}

IFilter::PreflightResult ScalarSegmentFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto inputDataPath = filterArgs.value<DataPath>(k_InputArrayPathKey);
  auto featureIdsName = filterArgs.value<std::string>(k_FeatureIdsName_Key);
  auto cellFeaturesName = filterArgs.value<std::string>(k_CellFeatureName_Key);
  auto activeArrayName = filterArgs.value<std::string>(k_ActiveArrayName_Key);
  DataPath featureIdsPath = inputDataPath.replaceName(featureIdsName);

  auto gridGeomPath = filterArgs.value<DataPath>(k_GridGeomPath_Key);
  DataPath cellFeaturesPath = gridGeomPath.createChildPath(cellFeaturesName);
  DataPath activeArrayPath = cellFeaturesPath.createChildPath(activeArrayName);

  std::vector<DataPath> cellArrayPaths = {inputDataPath};
  if(filterArgs.value<bool>(k_UseMask_Key))
  {
    cellArrayPaths.emplace_back(filterArgs.value<DataPath>(k_MaskArrayPath_Key));
  }
  if(auto tupleResult = ValidateCellTupleCount(dataStructure, gridGeomPath, cellArrayPaths, -654); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  const auto& gridGeometry = dataStructure.getDataRefAs<IGridGeometry>(gridGeomPath);
  auto gridDims = gridGeometry.getDimensions();
  const std::vector<usize> cellTupleDims = {gridDims[2], gridDims[1], gridDims[0]};
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, cellTupleDims, std::vector<usize>{1}, featureIdsPath, "", "0");

  auto createFeatureGroupAction = std::make_unique<CreateAttributeMatrixAction>(cellFeaturesPath, std::vector<usize>{1});
  auto createActiveAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{1}, std::vector<usize>{1}, activeArrayPath, "", "1");

  nx::core::Result<OutputActions> resultOutputActions;

  resultOutputActions.value().appendAction(std::move(createFeatureGroupAction));
  resultOutputActions.value().appendAction(std::move(createActiveAction));
  resultOutputActions.value().appendAction(std::move(createFeatureIdsAction));

  return {.output_actions = std::move(resultOutputActions)};
}

// -----------------------------------------------------------------------------
Result<> ScalarSegmentFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ScalarSegmentFeaturesInputValues inputValues;

  inputValues.input_data_path = filterArgs.value<DataPath>(k_InputArrayPathKey);
  inputValues.scalar_tolerance = filterArgs.value<int>(k_ScalarToleranceKey);
  inputValues.randomize_feature_ids = filterArgs.value<bool>(k_RandomizeFeatures_Key);
  inputValues.feature_ids_array_path = inputValues.input_data_path.replaceName(filterArgs.value<std::string>(k_FeatureIdsName_Key));
  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_GridGeomPath_Key);
  inputValues.cell_feature_attribute_matrix_path = inputValues.image_geometry_path.createChildPath(filterArgs.value<std::string>(k_CellFeatureName_Key));
  inputValues.active_array_path = inputValues.cell_feature_attribute_matrix_path.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));
  inputValues.is_periodic = filterArgs.value<bool>(k_IsPeriodic_Key);
  inputValues.neighbor_scheme = static_cast<SegmentFeatures::NeighborScheme>(filterArgs.value<ChoicesParameter::ValueType>(k_NeighborScheme_Key));

  return ScalarSegmentFeatures(dataStructure, &inputValues, shouldCancel, messageHandler)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_ScalarToleranceKey = "ScalarTolerance";
constexpr StringLiteralType k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteralType k_RandomizeFeatureIdsKey = "RandomizeFeatureIds";
constexpr StringLiteralType k_ScalarArrayPathKey = "ScalarArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteralType k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteralType k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteralType k_ActiveArrayNameKey = "ActiveArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ScalarSegmentFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ScalarSegmentFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_ScalarToleranceKey, k_ScalarToleranceKey));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseGoodVoxelsKey, k_UseMask_Key));
  Result<> randomizeResult = simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_RandomizeFeatureIdsKey, k_RandomizeFeatures_Key);
  if(randomizeResult.valid())
  {
    // DREAM3D 6.5 pipelines omit this setting. The importer retains the setting
    // when the input contains it.
    results.push_back(std::move(randomizeResult));
  }
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ScalarArrayPathKey, k_InputArrayPathKey));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayNameKey, k_FeatureIdsName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellFeatureAttributeMatrixNameKey, k_CellFeatureName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_ScalarArrayPathKey, k_GridGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_ActiveArrayNameKey, k_ActiveArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
