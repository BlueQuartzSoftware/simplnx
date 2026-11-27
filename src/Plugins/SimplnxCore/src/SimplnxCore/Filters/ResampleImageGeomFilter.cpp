#include "ResampleImageGeomFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
const std::string k_SpacingMode("Spacing (0)");
const std::string k_ScalingMode("Scaling (1)");
const std::string k_ExactDimensions("Exact Dimensions (2)");
const ChoicesParameter::ChoicesType k_Choices = {k_SpacingMode, k_ScalingMode, k_ExactDimensions};
const ChoicesParameter::ValueType k_SpacingModeIndex = 0;
const ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ResampleImageGeomFilter::name() const
{
  return FilterTraits<ResampleImageGeomFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ResampleImageGeomFilter::className() const
{
  return FilterTraits<ResampleImageGeomFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ResampleImageGeomFilter::uuid() const
{
  return FilterTraits<ResampleImageGeomFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ResampleImageGeomFilter::humanName() const
{
  return "Resample Data (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ResampleImageGeomFilter::defaultTags() const
{
  return {className(), "Sampling", "Spacing", "Image Geometry", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ResampleImageGeomFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ResamplingMode_Key, "Resampling Mode", "Mode can be [0] Spacing, [1] Scaling as Percent, [2] Exact Dimensions as voxels", k_SpacingModeIndex, ::k_Choices));

  params.insert(std::make_unique<VectorFloat32ParameterType>(k_Spacing_Key, "New Spacing",
                                                             "The new spacing values (dx, dy, dz). Larger spacing will cause less voxels, smaller spacing will cause more voxels.",
                                                             std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(
      std::make_unique<VectorFloat32ParameterType>(k_Scaling_Key, "Scale Factor (percentages)",
                                                   "The scale factor values (dx, dy, dz) to resample the geometry, in percentages. Larger percentages will cause more voxels, smaller percentages "
                                                   "will cause less voxels.  A percentage of 100 in any dimension will not resample the geometry in that dimension. Percentages must be larger than 0.",
                                                   std::vector<float32>{100.0F, 100.0F, 100.0F}, std::vector<std::string>{"X%", "Y%", "Z%"}));

  params.insert(std::make_unique<VectorUInt64ParameterType>(k_ExactDimensions_Key, "Exact Dimensions (pixels)", "The exact dimension size values (dx, dy, dz) to resample the geometry, in pixels.",
                                                            std::vector<uint64>{100, 100, 100}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry to resample", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Feature Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_ClearFeatureAttributeMatrix_Key, "Clear Feature Attribute Matrix",
      "Recreates the selected Feature Attribute Matrix in the resampled geometry with no arrays. Resampling changes which cells belong to each feature, so every feature value computed before this "
      "filter is invalid. Recompute any feature data needed later in the pipeline.",
      true));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrix_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Cell Feature Data"})));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features",
                                                "Renumber Feature Ids to 0..N and resize the cleared Feature Attribute Matrix to the features that remain after resampling", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometry_Key, "Created Image Geometry", "The location of the resampled geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ResamplingMode_Key, k_Spacing_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
  params.linkParameters(k_ResamplingMode_Key, k_Scaling_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
  params.linkParameters(k_ResamplingMode_Key, k_ExactDimensions_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureAttributeMatrix_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_RenumberFeatures_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometry_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ResampleImageGeomFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ResampleImageGeomFilter::clone() const
{
  return std::make_unique<ResampleImageGeomFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ResampleImageGeomFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  ResampleImageGeomInputValues inputValues;
  inputValues.resampling_mode = filterArgs.value<ChoicesParameter::ValueType>(k_ResamplingMode_Key);
  inputValues.spacing = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Spacing_Key);
  inputValues.scaling = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Scaling_Key);
  inputValues.exact_dimensions = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_ExactDimensions_Key);
  inputValues.remove_original_image_geom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.created_image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  inputValues.clear_feature_attribute_matrix = filterArgs.value<bool>(k_ClearFeatureAttributeMatrix_Key);
  inputValues.renumber_features = inputValues.clear_feature_attribute_matrix && filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cell_feature_attribute_matrix = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);
  inputValues.selected_image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.report_uncleared_feature_data = true;
  return PreflightImageGeometryResample(dataStructure, inputValues);
}

//------------------------------------------------------------------------------
Result<> ResampleImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ResampleImageGeomInputValues inputValues;
  inputValues.resampling_mode = filterArgs.value<ChoicesParameter::ValueType>(k_ResamplingMode_Key);
  inputValues.spacing = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Spacing_Key);
  inputValues.scaling = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Scaling_Key);
  inputValues.exact_dimensions = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_ExactDimensions_Key);
  inputValues.remove_original_image_geom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.created_image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  inputValues.clear_feature_attribute_matrix = filterArgs.value<bool>(k_ClearFeatureAttributeMatrix_Key);
  inputValues.renumber_features = inputValues.clear_feature_attribute_matrix && filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cell_feature_attribute_matrix = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);
  inputValues.selected_image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  return ResampleImageGeometry(dataStructure, inputValues, messageHandler, shouldCancel);
}

namespace
{
namespace simpl
{
// DREAM.3D 6.4 and 6.5 ChangeResolution write "Resolution" and "NewDataContainerName".
// The later "Spacing" and "NewDataContainerPath" keys are still accepted.
constexpr StringLiteralType k_ResolutionKey = "Resolution";
constexpr StringLiteralType k_SpacingKey = "Spacing";
constexpr StringLiteralType k_RenumberFeaturesKey = "RenumberFeatures";
constexpr StringLiteralType k_SaveAsNewDataContainerKey = "SaveAsNewDataContainer";
constexpr StringLiteralType k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
constexpr StringLiteralType k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteralType k_NewDataContainerPathKey = "NewDataContainerPath";
} // namespace simpl
} // namespace

Result<Arguments> ResampleImageGeomFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ResampleImageGeomFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(
      args, json, json.contains(simpl::k_ResolutionKey) ? simpl::k_ResolutionKey : simpl::k_SpacingKey, k_Spacing_Key));
  // Legacy renumbering also controls whether Feature data is cleared.
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_RenumberFeaturesKey, k_ClearFeatureAttributeMatrix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_RenumberFeaturesKey, k_RenumberFeatures_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::InvertedBooleanFilterParameterConverter>(args, json, simpl::k_SaveAsNewDataContainerKey, k_RemoveOriginalGeometry_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixPathKey,
                                                                                                                                        k_SelectedImageGeometryPath_Key));

  // Legacy ChangeResolution uses these Feature paths only when RenumberFeatures is on.
  bool renumberDisabled = false;
  if(json.contains(simpl::k_RenumberFeaturesKey))
  {
    const auto renumber = simpl_conversion::LinkedBooleanFilterParameterConverter::Convert(json[simpl::k_RenumberFeaturesKey]);
    renumberDisabled = renumber.valid() && !renumber.value();
  }
  if(!renumberDisabled || json.contains(simpl::k_CellFeatureAttributeMatrixPathKey))
  {
    results.push_back(
        simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_CellFeatureAttributeMatrixPathKey, k_FeatureAttributeMatrix_Key));
  }
  if(!renumberDisabled || json.contains(simpl::k_FeatureIdsArrayPathKey))
  {
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  }
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DCPathBuilderFilterParameterConverter>(
      args, json, json.contains(simpl::k_NewDataContainerNameKey) ? simpl::k_NewDataContainerNameKey : simpl::k_NewDataContainerPathKey, k_CreatedImageGeometry_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
