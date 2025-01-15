#include "CreateAMScanPathsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CreateAMScanPaths.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::name() const
{
  return FilterTraits<CreateAMScanPathsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::className() const
{
  return FilterTraits<CreateAMScanPathsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateAMScanPathsFilter::uuid() const
{
  return FilterTraits<CreateAMScanPathsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::humanName() const
{
  return "Create AM Scan Paths";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateAMScanPathsFilter::defaultTags() const
{
  return {className(), "GCode", "Scan Path", "Scan Vector", "Generate"};
}

//------------------------------------------------------------------------------
Parameters CreateAMScanPathsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<Float32Parameter>(k_HatchSpacing_Key, "Hatch Spacing", "The orthogonal distance between each generated vector.", 0.14f));
  params.insert(std::make_unique<Float32Parameter>(k_StripeWidth_Key, "Hatch Length", "The length of each vector that is created.", 7.0f));
  params.insert(std::make_unique<Float32Parameter>(k_RotationAngle, "Hatch Rotation Angle (Degrees)", "The angle in degrees by which each slice's hatches are rotated", 67.0f));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_CADSliceDataContainerPath_Key, "Slice Data Container", "The input edge geometry from which to create the scan paths", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Edge}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CADSliceIdsArrayPath_Key, "Slice Ids", "Identifies the slice to which each edge belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CADRegionIdsArrayPath_Key, "Region Ids", "Identifies the region to which each edge belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_HatchDataContainerPath_Key, "Scan Vector Geometry", "The created edge geometry representing the scan paths", DataPath({"ScanVectorGeometry"})));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "The name of the attribute matrix containing the scan paths' vertex data", "Vertex Data"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_HatchAttributeMatrixName_Key, "Edge Attribute Matrix", "The name of the attribute matrix containing the scan path's Edge data", "Edge Data"));
  params.insertSeparator(Parameters::Separator{"Vertex Node Data"});
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_RegionIdsArrayName_Key, "Region Ids", "The name of the array identifying the region to which each scan path belongs", "RegionIds"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateAMScanPathsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateAMScanPathsFilter::clone() const
{
  return std::make_unique<CreateAMScanPathsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateAMScanPathsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pCADSliceDataContainerNameValue = filterArgs.value<DataPath>(k_CADSliceDataContainerPath_Key);
  auto pCADSliceIdsArrayPathValue = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  auto pCADRegionIdsArrayPathValue = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  auto pHatchDataContainerNameValue = filterArgs.value<DataPath>(k_HatchDataContainerPath_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pHatchAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  auto pRegionIdsArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_RegionIdsArrayName_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    auto createGeometryAction = std::make_unique<CreateEdgeGeometryAction>(pHatchDataContainerNameValue, 1, 2, pVertexAttributeMatrixNameValue, pHatchAttributeMatrixNameValue,
                                                                           EdgeGeom::k_SharedVertexListName, EdgeGeom::k_SharedEdgeListName);
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }
  const DataPath hatchAttributeMatrixPath = pHatchDataContainerNameValue.createChildPath(pHatchAttributeMatrixNameValue);
  std::vector<size_t> tDims = {1};
  const std::vector<size_t> compDims = {1};
  {
    DataPath path = hatchAttributeMatrixPath.createChildPath(pCADSliceIdsArrayPathValue.getTargetName());
    auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }
  {
    DataPath path = hatchAttributeMatrixPath.createChildPath(pRegionIdsArrayNameValue);
    auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateAMScanPathsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  CreateAMScanPathsInputValues inputValues;

  inputValues.StripeWidth = filterArgs.value<float32>(k_StripeWidth_Key);
  inputValues.HatchSpacing = filterArgs.value<float32>(k_HatchSpacing_Key);
  inputValues.SliceHatchRotationAngle = filterArgs.value<float32>(k_RotationAngle) * nx::core::Constants::k_DegToRadF;
  inputValues.CADSliceDataContainerName = filterArgs.value<DataPath>(k_CADSliceDataContainerPath_Key);
  inputValues.CADSliceIdsArrayPath = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  inputValues.CADRegionIdsArrayPath = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  inputValues.HatchDataContainerName = filterArgs.value<DataPath>(k_HatchDataContainerPath_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.HatchAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  inputValues.RegionIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RegionIdsArrayName_Key);

  return CreateAMScanPaths(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_StripeWidth_Key = "StripeWidth";
constexpr StringLiteral k_HatchSpacing_Key = "HatchSpacing";
constexpr StringLiteral k_CADSliceDataContainerName_Key = "CADSliceDataContainerName";
constexpr StringLiteral k_CADSliceIdsArrayPath_Key = "CADSliceIdsArrayPath";
constexpr StringLiteral k_CADRegionIdsArrayPath_Key = "CADRegionIdsArrayPath";
constexpr StringLiteral k_HatchDataContainerName_Key = "HatchDataContainerName";
constexpr StringLiteral k_VertexAttributeMatrixName_Key = "VertexAttributeMatrixName";
constexpr StringLiteral k_HatchAttributeMatrixName_Key = "HatchAttributeMatrixName";
constexpr StringLiteral k_RegionIdsArrayName_Key = "RegionIdsArrayName";
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> CreateAMScanPathsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateAMScanPathsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_StripeWidth_Key, k_StripeWidth_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_HatchSpacing_Key, k_HatchSpacing_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CADSliceDataContainerName_Key, k_CADSliceDataContainerPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CADSliceIdsArrayPath_Key, k_CADSliceIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CADRegionIdsArrayPath_Key, k_CADRegionIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_HatchDataContainerName_Key, k_HatchDataContainerPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VertexAttributeMatrixName_Key, k_VertexAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_HatchAttributeMatrixName_Key, k_HatchAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_RegionIdsArrayName_Key, k_RegionIdsArrayName_Key));
  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
