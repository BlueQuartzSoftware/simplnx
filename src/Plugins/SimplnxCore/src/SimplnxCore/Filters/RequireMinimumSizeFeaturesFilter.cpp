#include "RequireMinimumSizeFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <algorithm>
#include <vector>

namespace nx::core
{

using FeatureIdsArrayType = Int32ArrayType;
using NumCellsArrayType = Int32ArrayType;

namespace
{
constexpr int32 k_BadMinAllowedFeatureSize = -5555;

} // namespace

std::string RequireMinimumSizeFeaturesFilter::name() const
{
  return FilterTraits<RequireMinimumSizeFeaturesFilter>::name;
}

std::string RequireMinimumSizeFeaturesFilter::className() const
{
  return FilterTraits<RequireMinimumSizeFeaturesFilter>::className;
}

Uuid RequireMinimumSizeFeaturesFilter::uuid() const
{
  return FilterTraits<RequireMinimumSizeFeaturesFilter>::uuid;
}

std::string RequireMinimumSizeFeaturesFilter::humanName() const
{
  return "Remove Minimum Size Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> RequireMinimumSizeFeaturesFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "MinSize", "Feature Removal"};
}

Parameters RequireMinimumSizeFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<NumberParameter<int64>>(k_MinAllowedFeaturesSize_Key, "Minimum Allowed Features Size", "Minimum allowed features size", 0));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplySinglePhase_Key, "Apply to Single Phase", "Apply to Single Phase", false));
  params.insert(std::make_unique<NumberParameter<int32>>(k_SinglePhaseNumber_Key, "Phase Index", "Target phase to remove", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Input Image Geometry", "The input image geometry (cell)", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "DataPath to FeatureIds DataArray", DataPath({"FeatureIds"}), GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureNumCellsPath_Key, "Feature Num. Cells Array", "DataPath to NumCells DataArray", DataPath({"NumElements"}), GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases", "DataPath to Feature Phases DataArray", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  // Link the checkbox to the other parameters
  params.linkParameters(k_ApplySinglePhase_Key, k_SinglePhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplySinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RequireMinimumSizeFeaturesFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointerType RequireMinimumSizeFeaturesFilter::clone() const
{
  return std::make_unique<RequireMinimumSizeFeaturesFilter>();
}

IFilter::PreflightResult RequireMinimumSizeFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                         const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto featurePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto featureNumCellsPath = filterArgs.value<DataPath>(k_FeatureNumCellsPath_Key);
  auto applyToSinglePhase = filterArgs.value<bool>(k_ApplySinglePhase_Key);
  auto minAllowedFeatureSize = filterArgs.value<int64>(k_MinAllowedFeaturesSize_Key);

  auto featureIdsResult = ValidateArrayInGeometryCellData(dataStructure, imageGeomPath, featureIdsPath, -5562, "Cell Feature Ids");
  if(featureIdsResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(featureIdsResult), {})};
  }

  auto featureAttributeMatrixResult = ValidateFeatureAttributeMatrixInGeometry(dataStructure, imageGeomPath, featureNumCellsPath.getParent(), -5563, "Feature Num. Cells", featureNumCellsPath);
  if(featureAttributeMatrixResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(featureAttributeMatrixResult), {})};
  }

  // The Cell Attribute Matrix can have more than one DataPath. Use the geometry's Cell Attribute Matrix path for Feature Ids.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath).getCellDataPath();
  featureIdsPath = MapToGeometryCellDataPath(cellDataPath, featureIdsPath);

  std::vector<DataPath> dataArrayPaths;

  if(minAllowedFeatureSize < 0)
  {
    std::string ss = fmt::format("The minimum Feature size ({}) must be 0 or positive", minAllowedFeatureSize);
    return {.output_actions = MakeErrorResult<OutputActions>(-k_BadMinAllowedFeatureSize, ss)};
  }

  dataArrayPaths.push_back(featureNumCellsPath);

  if(applyToSinglePhase)
  {
    dataArrayPaths.push_back(featurePhasesPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  // Create the preflightResult object
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  nx::core::AppendStaleFeatureArraysPreflightValue(dataStructure, featureNumCellsPath.getParent(), featureIdsPath, preflightUpdatedValues);

  // This section will warn the user about the removal of NeighborLists
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, featureNumCellsPath, resultOutputActions);
  if(result.output_actions.invalid())
  {
    return result;
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modified_actions, cellDataPath, {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
Result<> RequireMinimumSizeFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  RequireMinimumSizeFeaturesInputValues inputValues;

  inputValues.min_allowed_features_size = filterArgs.value<Int64ParameterType::ValueType>(k_MinAllowedFeaturesSize_Key);
  inputValues.feature_ids_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsPath_Key);
  inputValues.input_image_geometry_path = filterArgs.value<GeometrySelectionParameter::ValueType>(k_ImageGeomPath_Key);
  inputValues.feature_num_cells_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureNumCellsPath_Key);
  inputValues.feature_phases_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeaturePhasesPath_Key);
  inputValues.apply_single_phase = filterArgs.value<BoolParameter::ValueType>(k_ApplySinglePhase_Key);
  inputValues.phase_number = filterArgs.value<Int32ParameterType::ValueType>(k_SinglePhaseNumber_Key);

  // The algorithm finds the Cell arrays through the parent of Feature Ids. Use the geometry's Cell Attribute Matrix path for Feature Ids.
  const auto cellDataPath = dataStructure.getDataRefAs<ImageGeom>(inputValues.input_image_geometry_path).getCellDataPath();
  inputValues.feature_ids_path = MapToGeometryCellDataPath(cellDataPath, inputValues.feature_ids_path);

  return RequireMinimumSizeFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_MinAllowedFeatureSizeKey = "MinAllowedFeatureSize";
constexpr StringLiteralType k_ApplyToSinglePhaseKey = "ApplyToSinglePhase";
constexpr StringLiteralType k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteralType k_NumCellsArrayPathKey = "NumCellsArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> RequireMinimumSizeFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RequireMinimumSizeFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int64>>(args, json, simpl::k_MinAllowedFeatureSizeKey, k_MinAllowedFeaturesSize_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_ApplyToSinglePhaseKey, k_ApplySinglePhase_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_PhaseNumberKey, k_SinglePhaseNumber_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_ImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_NumCellsArrayPathKey, k_FeatureNumCellsPath_Key));
  // Ignored Array Paths parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
