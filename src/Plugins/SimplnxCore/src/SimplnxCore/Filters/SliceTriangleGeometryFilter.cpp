#include "SliceTriangleGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

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
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SliceDirection_Key, "Slice Direction (ijk)", "", std::vector<float32>{0.0F, 0.0F, 1.0F}, std::vector<std::string>{"i", "j", "k"}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SliceRange_Key, "Slice Range", "", 0, ChoicesParameter::Choices{"Full Range", "User Defined Range"}));
  params.insert(std::make_unique<Float32Parameter>(k_Zstart_Key, "Slicing Start", "", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_Zend_Key, "Slicing End", "", 0.0));
  params.insert(std::make_unique<Float32Parameter>(k_SliceResolution_Key, "Slice Spacing", "", 1.0f));

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry", "", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::TriangleGeom}));

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Optional Input Region Ids"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_HaveRegionIds_Key, "Have Region Ids", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_RegionIdArrayPath_Key, "Region Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Edge Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputEdgeGeometryPath_Key, "Created Edge Geometry", "The name of the created Edge Geometry", DataPath({"Edge Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "", "Edge Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SliceIdArrayName_Key, "Slice Ids", "", "Slice Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SliceAttributeMatrixName_Key, "Slice Attribute Matrix", "", "Slice Feature Data"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_HaveRegionIds_Key, k_RegionIdArrayPath_Key, true);

  params.linkParameters(k_SliceRange_Key, k_Zstart_Key, 1ULL);
  params.linkParameters(k_SliceRange_Key, k_Zend_Key, 1ULL);
  return params;
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
  auto pSliceDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SliceDirection_Key);
  auto pSliceRangeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SliceRange_Key);
  auto pZstartValue = filterArgs.value<float32>(k_Zstart_Key);
  auto pZendValue = filterArgs.value<float32>(k_Zend_Key);
  auto pSliceResolutionValue = filterArgs.value<float32>(k_SliceResolution_Key);
  auto pHaveRegionIdsValue = filterArgs.value<bool>(k_HaveRegionIds_Key);
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  auto pRegionIdArrayPathValue = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  auto pSliceDataContainerNameValue = filterArgs.value<DataGroupCreationParameter::ValueType>(k_OutputEdgeGeometryPath_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_EdgeAttributeMatrixName_Key);
  auto pSliceIdArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceIdArrayName_Key);
  auto pSliceAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceAttributeMatrixName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  nx::core::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // create the edge geometry
  {
    auto createGeometryAction = std::make_unique<CreateEdgeGeometryAction>(pSliceDataContainerNameValue, 1, 2, "Vertex Data", pEdgeAttributeMatrixNameValue,
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

  DataPath featureSliceAttrMatPath = pSliceDataContainerNameValue.createChildPath(pSliceAttributeMatrixNameValue);
  {
    auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(featureSliceAttrMatPath, tDims);
    resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SliceTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{

  SliceTriangleGeometryInputValues inputValues;

  inputValues.SliceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SliceDirection_Key);
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

} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> SliceTriangleGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SliceTriangleGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
