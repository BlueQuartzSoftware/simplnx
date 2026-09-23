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
const ChoicesParameter::Choices k_Choices = {k_SpacingMode, k_ScalingMode, k_ExactDimensions};
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

  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "New Spacing",
                                                         "The new spacing values (dx, dy, dz). Larger spacing will cause less voxels, smaller spacing will cause more voxels.",
                                                         std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Scaling_Key, "Scale Factor (percentages)",
                                               "The scale factor values (dx, dy, dz) to resample the geometry, in percentages. Larger percentages will cause more voxels, smaller percentages "
                                               "will cause less voxels.  A percentage of 100 in any dimension will not resample the geometry in that dimension. Percentages must be larger than 0.",
                                               std::vector<float32>{100.0F, 100.0F, 100.0F}, std::vector<std::string>{"X%", "Y%", "Z%"}));

  params.insert(std::make_unique<VectorUInt64Parameter>(k_ExactDimensions_Key, "Exact Dimensions (pixels)", "The exact dimension size values (dx, dy, dz) to resample the geometry, in pixels.",
                                                        std::vector<uint64>{100, 100, 100}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry to resample", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Optional Renumber Features"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrix_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometry_Key, "Created Image Geometry", "The location of the resampled geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ResamplingMode_Key, k_Spacing_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
  params.linkParameters(k_ResamplingMode_Key, k_Scaling_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
  params.linkParameters(k_ResamplingMode_Key, k_ExactDimensions_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureAttributeMatrix_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometry_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ResampleImageGeomFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ResampleImageGeomFilter::clone() const
{
  return std::make_unique<ResampleImageGeomFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ResampleImageGeomFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ResampleImageGeomInputValues inputValues;
  inputValues.ResamplingMode = filterArgs.value<ChoicesParameter::ValueType>(k_ResamplingMode_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Scaling = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scaling_Key);
  inputValues.ExactDimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(k_ExactDimensions_Key);
  inputValues.RemoveOriginalImageGeom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.CreatedImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  inputValues.RenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrix = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);
  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  return PreflightImageGeometryResample(dataStructure, inputValues);
}

//------------------------------------------------------------------------------
Result<> ResampleImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter*, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ResampleImageGeomInputValues inputValues;
  inputValues.ResamplingMode = filterArgs.value<ChoicesParameter::ValueType>(k_ResamplingMode_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Scaling = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scaling_Key);
  inputValues.ExactDimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(k_ExactDimensions_Key);
  inputValues.RemoveOriginalImageGeom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.CreatedImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  inputValues.RenumberFeatures = filterArgs.value<bool>(k_RenumberFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrix = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);
  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  return ResampleImageGeometry(dataStructure, inputValues, messageHandler, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SpacingKey = "Spacing";
constexpr StringLiteral k_RenumberFeaturesKey = "RenumberFeatures";
constexpr StringLiteral k_SaveAsNewDataContainerKey = "SaveAsNewDataContainer";
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
constexpr StringLiteral k_NewDataContainerPathKey = "NewDataContainerPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ResampleImageGeomFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ResampleImageGeomFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_RenumberFeaturesKey, k_RenumberFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InvertedBooleanFilterParameterConverter>(args, json, SIMPL::k_SaveAsNewDataContainerKey, k_RemoveOriginalGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey,
                                                                                                                                      k_SelectedImageGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_FeatureAttributeMatrix_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerPathKey, k_CreatedImageGeometry_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
