#include "CropImageGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CropImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
struct CropImageGeometryFilterCache
{
  uint64 xMin;
  uint64 xMax;
  uint64 yMax;
  uint64 yMin;
  uint64 zMax;
  uint64 zMin;
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, CropImageGeometryFilterCache> s_HeaderCache;

const std::string k_TempGeometryName = ".cropped_image_geometry";

/**
 * @brief
 * @param dataStructure
 * @param imageGeomPath
 * @return
 */
FloatVec3 GetCurrentVolumeDataContainerResolutions(const DataStructure& dataStructure, const DataPath& imageGeomPath)
{
  FloatVec3 data = {0, 0, 0};
  const auto* image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  if(image != nullptr)
  {
    data = image->getSpacing();
  }
  return data;
}
} // namespace

//------------------------------------------------------------------------------
CropImageGeometryFilter::CropImageGeometryFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
CropImageGeometryFilter::~CropImageGeometryFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
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
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinVoxel_Key, "Min Voxel", "Lower bound of voxels of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxVoxel_Key, "Max Voxel [Inclusive]", "Upper bound in voxels of the volume to crop out", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_MinCoord_Key, "Min Coordinate (Physical Units)", "Lower bound in real units of the volume to crop.", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_MaxCoord_Key, "Max Coordinate (Physical Units) [Inclusive]", "Upper bound in real units of the volume to crop.",
                                                         std::vector<float64>{0.0, 0.0, 0.0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after filter is completed", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "DataPath to the source Image Geometry", DataPath(), std::set{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Optional Renumber Features"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "Specifies if the feature IDs should be renumbered", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Created Image Geometry", "The DataPath to store the created Image Geometry", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UsePhysicalBounds_Key, k_MinVoxel_Key, false);
  params.linkParameters(k_UsePhysicalBounds_Key, k_MaxVoxel_Key, false);

  params.linkParameters(k_UsePhysicalBounds_Key, k_MinCoord_Key, true);
  params.linkParameters(k_UsePhysicalBounds_Key, k_MaxCoord_Key, true);

  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CropImageGeometryFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer CropImageGeometryFilter::clone() const
{
  return std::make_unique<CropImageGeometryFilter>();
}
IFilter::PreflightResult CropImageGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ImageGeometryCropOptions options;
  options.inputImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  options.outputImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  options.featureIdsPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  options.cellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  options.minVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MinVoxel_Key);
  options.maxVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MaxVoxel_Key);
  options.minCoordinate = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MinCoord_Key);
  options.maxCoordinate = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaxCoord_Key);
  options.renumberFeatures = filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  options.removeOriginalGeometry = filterArgs.value<BoolParameter::ValueType>(k_RemoveOriginalGeometry_Key);
  options.usePhysicalBounds = filterArgs.value<BoolParameter::ValueType>(k_UsePhysicalBounds_Key);
  options.cropX = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  options.cropY = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  options.cropZ = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  ImageGeometryCropBounds bounds;
  IFilter::PreflightResult result = PreflightImageGeometryCrop(dataStructure, options, bounds);
  s_HeaderCache[m_InstanceId] = {bounds.xMin, bounds.xMax, bounds.yMax, bounds.yMin, bounds.zMax, bounds.zMin};
  return result;
}

Result<> CropImageGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  CropImageGeometryInputValues inputValues;

  inputValues.InputImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.OutputImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.MinVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MinVoxel_Key);
  inputValues.MaxVoxel = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MaxVoxel_Key);
  inputValues.RenumberFeatures = filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrixPath_Key);
  inputValues.RemoveOriginalGeometry = filterArgs.value<BoolParameter::ValueType>(k_RemoveOriginalGeometry_Key);
  inputValues.CropXDim = filterArgs.value<BoolParameter::ValueType>(k_CropXDim_Key);
  inputValues.CropYDim = filterArgs.value<BoolParameter::ValueType>(k_CropYDim_Key);
  inputValues.CropZDim = filterArgs.value<BoolParameter::ValueType>(k_CropZDim_Key);

  inputValues.XMin = s_HeaderCache[m_InstanceId].xMin;
  inputValues.XMax = s_HeaderCache[m_InstanceId].xMax;
  inputValues.YMin = s_HeaderCache[m_InstanceId].yMin;
  inputValues.YMax = s_HeaderCache[m_InstanceId].yMax;
  inputValues.ZMin = s_HeaderCache[m_InstanceId].zMin;
  inputValues.ZMax = s_HeaderCache[m_InstanceId].zMax;

  return CropImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_XMinKey = "XMin";
constexpr StringLiteral k_YMinKey = "YMin";
constexpr StringLiteral k_ZMinKey = "ZMin";
constexpr StringLiteral k_XMaxKey = "XMax";
constexpr StringLiteral k_YMaxKey = "YMax";
constexpr StringLiteral k_ZMaxKey = "ZMax";
// constexpr StringLiteral k_OldBoxDimensionsKey = "OldBoxDimensions";
// constexpr StringLiteral k_NewBoxDimensionsKey = "NewBoxDimensions";
// constexpr StringLiteral k_UpdateOriginKey = "UpdateOrigin";
constexpr StringLiteral k_SaveAsNewDataContainerKey = "SaveAsNewDataContainer";
constexpr StringLiteral k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteral k_RenumberFeaturesKey = "RenumberFeatures";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
} // namespace SIMPL
} // namespace

Result<Arguments> CropImageGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CropImageGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMinKey, SIMPL::k_YMinKey, SIMPL::k_ZMinKey, k_MinVoxel_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMaxKey, SIMPL::k_YMaxKey, SIMPL::k_ZMaxKey, k_MaxVoxel_Key));
  // k_UpdateOrigin_Key currently disabled in NX.
  // results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UpdateOriginKey, k_UpdateOrigin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InvertedBooleanFilterParameterConverter>(args, json, SIMPL::k_SaveAsNewDataContainerKey, k_RemoveOriginalGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerNameKey, k_CreatedImageGeometryPath_Key));
  // Cell attribute matrix parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_RenumberFeaturesKey, k_RenumberFeatures_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixPathKey, k_FeatureAttributeMatrixPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
