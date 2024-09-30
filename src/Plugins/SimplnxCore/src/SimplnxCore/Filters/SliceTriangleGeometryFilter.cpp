#include "SliceTriangleGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
constexpr ChoicesParameter::ValueType k_UserDefinedRange = 1;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string SliceTriangleGeometryFilter::name() const
{
  return FilterTraits<SliceTriangleGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string SliceTriangleGeometryFilter::className() const
{
  return FilterTraits<SliceTriangleGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SliceTriangleGeometryFilter::uuid() const
{
  return FilterTraits<SliceTriangleGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SliceTriangleGeometryFilter::humanName() const
{
  return "Slice Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> SliceTriangleGeometryFilter::defaultTags() const
{
  return {className(), "Sampling", "Geometry"};
}

//------------------------------------------------------------------------------
Parameters SliceTriangleGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SliceRange_Key, "Slice Range", "Type of slice range to use, either Full Range or User Defined Range", 0,
                                                                    ChoicesParameter::Choices{"Full Range", "User Defined Range"}));
  params.insert(std::make_unique<Float32Parameter>(k_Zstart_Key, "Slicing Start", "The z axis start value", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_Zend_Key, "Slicing End", "The z axis stop value", 0.0));
  params.insert(std::make_unique<Float32Parameter>(k_SliceResolution_Key, "Slice Spacing", "The spacing between slices", 1.0f));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});

  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry", "The input triangle geometry to be sliced", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Optional Input Region Ids"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_HaveRegionIds_Key, "Have Region Ids", "Whether to supply an id array that propagates to the created edges", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_RegionIdArrayPath_Key, "Region Ids", "Optional identifier array, if Have Region Ids is selected", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Edge Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputEdgeGeometryPath_Key, "Created Edge Geometry", "The name of the created Edge Geometry", DataPath({"Edge Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "Attribute Matrix to store information about the created edges", "Edge Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SliceIdArrayName_Key, "Slice Ids", "Identifies the slice to which each edge belongs", "Slice Ids"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SliceAttributeMatrixName_Key, "Slice Attribute Matrix", "Attribute Matrix to store information about the created edges", "Slice Feature Data"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_HaveRegionIds_Key, k_RegionIdArrayPath_Key, true);

  params.linkParameters(k_SliceRange_Key, k_Zstart_Key, k_UserDefinedRange);
  params.linkParameters(k_SliceRange_Key, k_Zend_Key, k_UserDefinedRange);
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SliceTriangleGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SliceTriangleGeometryFilter::clone() const
{
  return std::make_unique<SliceTriangleGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SliceTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pSliceRangeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SliceRange_Key);
  auto pZStartValue = filterArgs.value<float32>(k_Zstart_Key);
  auto pZEndValue = filterArgs.value<float32>(k_Zend_Key);
  auto pHaveRegionIdsValue = filterArgs.value<bool>(k_HaveRegionIds_Key);
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  auto pRegionIdArrayPathValue = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  auto pSliceDataContainerNameValue = filterArgs.value<DataGroupCreationParameter::ValueType>(k_OutputEdgeGeometryPath_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_EdgeAttributeMatrixName_Key);
  auto pSliceIdArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceIdArrayName_Key);
  auto pSliceAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceAttributeMatrixName_Key);

  Result<OutputActions> resultOutputActions;

  if(pSliceRangeValue == k_UserDefinedRange)
  {
    if(pZStartValue >= pZEndValue)
    {
      return MakePreflightErrorResult(-62100, "Z end must be larger than Z start.");
    }
  }

  // create the edge geometry
  {
    auto createGeometryAction = std::make_unique<CreateEdgeGeometryAction>(pSliceDataContainerNameValue, 1, 2, INodeGeometry0D::k_VertexDataName, pEdgeAttributeMatrixNameValue,
                                                                           CreateEdgeGeometryAction::k_DefaultVerticesName, CreateEdgeGeometryAction::k_DefaultEdgesName);
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }

  std::vector<size_t> tDims = {1};
  const std::vector<size_t> compDims = {1};
  {
    DataPath path = pSliceDataContainerNameValue.createChildPath(pEdgeAttributeMatrixNameValue).createChildPath(pSliceIdArrayNameValue);
    auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }

  if(pHaveRegionIdsValue)
  {
    DataPath path = pSliceDataContainerNameValue.createChildPath(pEdgeAttributeMatrixNameValue).createChildPath(pRegionIdArrayPathValue.getTargetName());
    auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }

  DataPath featureSliceAttrMatPath = pSliceDataContainerNameValue.createChildPath(pSliceAttributeMatrixNameValue);
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(featureSliceAttrMatPath, tDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> SliceTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  SliceTriangleGeometryInputValues inputValues;

  inputValues.SliceRange = filterArgs.value<ChoicesParameter::ValueType>(k_SliceRange_Key);
  inputValues.Zstart = filterArgs.value<float32>(k_Zstart_Key);
  inputValues.Zend = filterArgs.value<float32>(k_Zend_Key);
  inputValues.SliceResolution = filterArgs.value<float32>(k_SliceResolution_Key);
  inputValues.HaveRegionIds = filterArgs.value<bool>(k_HaveRegionIds_Key);
  inputValues.CADDataContainerName = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  inputValues.RegionIdArrayPath = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  inputValues.SliceDataContainerName = filterArgs.value<DataGroupCreationParameter::ValueType>(k_OutputEdgeGeometryPath_Key);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_EdgeAttributeMatrixName_Key);
  inputValues.SliceIdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceIdArrayName_Key);
  inputValues.SliceAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceAttributeMatrixName_Key);

  return SliceTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SliceRange_Key = "SliceRange";
constexpr StringLiteral k_Zstart_Key = "Zstart";
constexpr StringLiteral k_Zend_Key = "Zend";
constexpr StringLiteral k_SliceResolution_Key = "SliceResolution";
constexpr StringLiteral k_HaveRegionIds_Key = "HaveRegionIds";
constexpr StringLiteral k_RegionIdArrayPath_Key = "RegionIdArrayPath";

constexpr StringLiteral k_TriangleGeometryDataPath_Key = "CADDataContainerName";

constexpr StringLiteral k_OutputEdgeGeometryPath_Key = "SliceDataContainerName";
constexpr StringLiteral k_EdgeAttributeMatrixName_Key = "EdgeAttributeMatrixName";

constexpr StringLiteral k_SliceAttributeMatrixName_Key = "SliceAttributeMatrixName";
constexpr StringLiteral k_SliceIdArrayName_Key = "SliceIdArrayName";
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> SliceTriangleGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SliceTriangleGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_SliceRange_Key, k_SliceRange_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_Zstart_Key, k_Zstart_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_Zend_Key, k_Zend_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_SliceResolution_Key, k_SliceResolution_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_HaveRegionIds_Key, k_HaveRegionIds_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_RegionIdArrayPath_Key, k_RegionIdArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TriangleGeometryDataPath_Key, k_TriangleGeometryDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_OutputEdgeGeometryPath_Key, k_OutputEdgeGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_EdgeAttributeMatrixName_Key, k_EdgeAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SliceAttributeMatrixName_Key, k_SliceAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SliceIdArrayName_Key, k_SliceIdArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
