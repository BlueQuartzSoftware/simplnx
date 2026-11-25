#include "CropImageGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CropImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
struct CropImageGeometryFilterCache
{
  uint64 x_min;
  uint64 x_max;
  uint64 y_max;
  uint64 y_min;
  uint64 z_max;
  uint64 z_min;
};

std::atomic_int32_t SInstanceId = 0;
std::map<int32, CropImageGeometryFilterCache> SHeaderCache;

/**
 * @brief
 * @param dataStructure
 * @param imageGeomPath
 * @return
 */
FloatVec3Type GetCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  FloatVec3Type data = {0, 0, 0};
  const auto* imagePtr = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(imagePtr != nullptr)
  {
    data = imagePtr->getSpacing();
  }
  return data;
}
} // namespace

//------------------------------------------------------------------------------
CropImageGeometryFilter::CropImageGeometryFilter()
: m_InstanceId(SInstanceId.fetch_add(1))
{
  SHeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
CropImageGeometryFilter::~CropImageGeometryFilter() noexcept
{
  SHeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string CropImageGeometryFilter::name() const
{
  return FilterTraits<CropImageGeometryFilter>::name;
}

//------------------------------------------------------------------------------
std::string CropImageGeometryFilter::className() const
{
  return FilterTraits<CropImageGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CropImageGeometryFilter::uuid() const
{
  return FilterTraits<CropImageGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CropImageGeometryFilter::humanName() const
{
  return "Crop Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropImageGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Crop Image Geometry", "Image Geometry", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters CropImageGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UsePhysicalBounds_Key, "Use Physical Units For Bounds", "If true define physical coordinates for bounds, If false define voxel indices for bounds", true));
  params.insert(std::make_unique<BoolParameter>(k_CropXDim_Key, "Crop X Dimension", "Enable cropping in the X dimension.", true));
  params.insert(std::make_unique<BoolParameter>(k_CropYDim_Key, "Crop Y Dimension", "Enable cropping in the Y dimension.", true));
  params.insert(std::make_unique<BoolParameter>(k_CropZDim_Key, "Crop Z Dimension", "Enable cropping in the Z dimension.", true));
  params.insert(std::make_unique<VectorUInt64ParameterType>(k_MinVoxel_Key, "Min Voxel", "Lower bound of voxels of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                            std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64ParameterType>(k_MaxVoxel_Key, "Max Voxel [Inclusive]", "Upper bound in voxels of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                            std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorFloat64ParameterType>(k_MinCoord_Key, "Min Coordinate (Physical Units)", "Lower bound in real units of the volume to crop.", std::vector<float64>{0.0, 0.0, 0.0},
                                                             std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat64ParameterType>(k_MaxCoord_Key, "Max Coordinate (Physical Units) [Inclusive]", "Upper bound in real units of the volume to crop.",
                                                             std::vector<float64>{0.0, 0.0, 0.0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "DataPath to the source Image Geometry", DataPath(), std::set{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Feature Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_ClearFeatureAttributeMatrix_Key, "Clear Feature Attribute Matrix",
      "Recreates the selected Feature Attribute Matrix in the cropped geometry with no arrays. Cropping changes which cells belong to each feature, so every feature value computed before this "
      "filter is invalid. Recompute any feature data needed later in the pipeline.",
      true));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Cell Feature Data"})));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features",
                                                "Renumber Feature Ids to 0..N and resize the cleared Feature Attribute Matrix to the features that remain after cropping", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Created Image Geometry", "The DataPath to store the created Image Geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UsePhysicalBounds_Key, k_MinVoxel_Key, false);
  params.linkParameters(k_UsePhysicalBounds_Key, k_MaxVoxel_Key, false);

  params.linkParameters(k_UsePhysicalBounds_Key, k_MinCoord_Key, true);
  params.linkParameters(k_UsePhysicalBounds_Key, k_MaxCoord_Key, true);

  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_RenumberFeatures_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CropImageGeometryFilter::parametersVersion() const
{
  return 2;
}

IFilter::UniquePointerType CropImageGeometryFilter::clone() const
{
  return std::make_unique<CropImageGeometryFilter>();
}
IFilter::PreflightResult CropImageGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  ImageGeometryCropOptions options;
  options.input_image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  options.output_image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  options.feature_ids_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  options.cell_feature_attribute_matrix_path = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  options.min_voxel = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_MinVoxel_Key);
  options.max_voxel = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_MaxVoxel_Key);
  options.min_coordinate = filterArgs.value<VectorFloat64ParameterType::ValueType>(k_MinCoord_Key);
  options.max_coordinate = filterArgs.value<VectorFloat64ParameterType::ValueType>(k_MaxCoord_Key);
  options.clear_feature_attribute_matrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  options.report_uncleared_feature_data = true;
  options.renumber_features = options.clear_feature_attribute_matrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  options.remove_original_geometry = filterArgs.value<BoolParameter::ValueType>(k_RemoveOriginalGeometry_Key);
  options.use_physical_bounds = filterArgs.value<BoolParameter::ValueType>(k_UsePhysicalBounds_Key);
  options.crop_x = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  options.crop_y = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  options.crop_z = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  ImageGeometryCropBounds bounds;
  IFilter::PreflightResult result = PreflightImageGeometryCrop(dataStructure, options, bounds);
  SHeaderCache[m_InstanceId] = {.x_min = bounds.x_min, .x_max = bounds.x_max, .y_max = bounds.y_max, .y_min = bounds.y_min, .z_max = bounds.z_max, .z_min = bounds.z_min};
  return result;
}

Result<> CropImageGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  CropImageGeometryInputValues inputValues;

  inputValues.input_image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.output_image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.feature_ids_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.min_voxel = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_MinVoxel_Key);
  inputValues.max_voxel = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_MaxVoxel_Key);
  inputValues.clear_feature_attribute_matrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  inputValues.renumber_features = inputValues.clear_feature_attribute_matrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  inputValues.cell_feature_attribute_matrix_path = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);
  inputValues.remove_original_geometry = filterArgs.value<BoolParameter::ValueType>(k_RemoveOriginalGeometry_Key);
  inputValues.crop_x_dim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  inputValues.crop_y_dim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  inputValues.crop_z_dim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  inputValues.x_min = SHeaderCache[m_InstanceId].x_min;
  inputValues.x_max = SHeaderCache[m_InstanceId].x_max;
  inputValues.y_min = SHeaderCache[m_InstanceId].y_min;
  inputValues.y_max = SHeaderCache[m_InstanceId].y_max;
  inputValues.z_min = SHeaderCache[m_InstanceId].z_min;
  inputValues.z_max = SHeaderCache[m_InstanceId].z_max;

  return CropImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_XMinKey = "XMin";
constexpr StringLiteralType k_YMinKey = "YMin";
constexpr StringLiteralType k_ZMinKey = "ZMin";
constexpr StringLiteralType k_XMaxKey = "XMax";
constexpr StringLiteralType k_YMaxKey = "YMax";
constexpr StringLiteralType k_ZMaxKey = "ZMax";
// constexpr StringLiteral k_OldBoxDimensionsKey = "OldBoxDimensions";
// constexpr StringLiteral k_NewBoxDimensionsKey = "NewBoxDimensions";
// constexpr StringLiteral k_UpdateOriginKey = "UpdateOrigin";
constexpr StringLiteralType k_SaveAsNewDataContainerKey = "SaveAsNewDataContainer";
constexpr StringLiteralType k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteralType k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteralType k_RenumberFeaturesKey = "RenumberFeatures";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
} // namespace simpl
} // namespace

Result<Arguments> CropImageGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CropImageGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::Convert3Parameters<simpl_conversion::UInt64ToVec3FilterParameterConverterType>(args, json, simpl::k_XMinKey, simpl::k_YMinKey, simpl::k_ZMinKey, k_MinVoxel_Key));
  results.push_back(simpl_conversion::Convert3Parameters<simpl_conversion::UInt64ToVec3FilterParameterConverterType>(args, json, simpl::k_XMaxKey, simpl::k_YMaxKey, simpl::k_ZMaxKey, k_MaxVoxel_Key));
  // DREAM.3D 6.x crops only by voxel index.
  args.insertOrAssign(k_UsePhysicalBounds_Key, std::make_any<bool>(false));
  // k_UpdateOrigin_Key currently disabled in NX.
  // results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UpdateOriginKey, k_UpdateOrigin_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::InvertedBooleanFilterParameterConverter>(args, json, simpl::k_SaveAsNewDataContainerKey, k_RemoveOriginalGeometry_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DCPathBuilderFilterParameterConverter>(args, json, simpl::k_NewDataContainerNameKey, k_CreatedImageGeometryPath_Key));
  // Cell attribute matrix parameter is not applicable in NX
  // Legacy renumbering also controls whether Feature data is cleared.
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_RenumberFeaturesKey, k_ClearFeatureAttributeMatrix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_RenumberFeaturesKey, k_RenumberFeatures_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_CellFeatureAttributeMatrixPathKey, k_FeatureAttributeMatrixPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
