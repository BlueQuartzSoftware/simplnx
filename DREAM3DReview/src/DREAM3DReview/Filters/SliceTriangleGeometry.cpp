#include "SliceTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string SliceTriangleGeometry::name() const
{
  return FilterTraits<SliceTriangleGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string SliceTriangleGeometry::className() const
{
  return FilterTraits<SliceTriangleGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid SliceTriangleGeometry::uuid() const
{
  return FilterTraits<SliceTriangleGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string SliceTriangleGeometry::humanName() const
{
  return "Slice Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> SliceTriangleGeometry::defaultTags() const
{
  return {"#Sampling", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters SliceTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SliceDirection_Key, "Slice Direction (ijk)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SliceRange_Key, "Slice Range", "", 0, ChoicesParameter::Choices{"Full Range", "User Defined Range"}));
  params.insert(std::make_unique<Float32Parameter>(k_Zstart_Key, "Slicing Start", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Zend_Key, "Slicing End", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_SliceResolution_Key, "Slice Spacing", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_HaveRegionIds_Key, "Have Region Ids", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CADDataContainerName_Key, "CAD Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_RegionIdArrayPath_Key, "Region Ids", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_SliceDataContainerName_Key, "Slice Geometry", "", "SomeString"));
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeAttributeMatrixName_Key, "Edge Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SliceIdArrayName_Key, "Slice Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SliceAttributeMatrixName_Key, "Slice Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SliceRange_Key, k_Zstart_Key, 1);
  params.linkParameters(k_SliceRange_Key, k_Zend_Key, 1);
  params.linkParameters(k_HaveRegionIds_Key, k_RegionIdArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SliceTriangleGeometry::clone() const
{
  return std::make_unique<SliceTriangleGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SliceTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSliceDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SliceDirection_Key);
  auto pSliceRangeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SliceRange_Key);
  auto pZstartValue = filterArgs.value<float32>(k_Zstart_Key);
  auto pZendValue = filterArgs.value<float32>(k_Zend_Key);
  auto pSliceResolutionValue = filterArgs.value<float32>(k_SliceResolution_Key);
  auto pHaveRegionIdsValue = filterArgs.value<bool>(k_HaveRegionIds_Key);
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  auto pRegionIdArrayPathValue = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  auto pSliceDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_SliceDataContainerName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pSliceIdArrayNameValue = filterArgs.value<DataPath>(k_SliceIdArrayName_Key);
  auto pSliceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_SliceAttributeMatrixName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<SliceTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> SliceTriangleGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSliceDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SliceDirection_Key);
  auto pSliceRangeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SliceRange_Key);
  auto pZstartValue = filterArgs.value<float32>(k_Zstart_Key);
  auto pZendValue = filterArgs.value<float32>(k_Zend_Key);
  auto pSliceResolutionValue = filterArgs.value<float32>(k_SliceResolution_Key);
  auto pHaveRegionIdsValue = filterArgs.value<bool>(k_HaveRegionIds_Key);
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  auto pRegionIdArrayPathValue = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  auto pSliceDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_SliceDataContainerName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pSliceIdArrayNameValue = filterArgs.value<DataPath>(k_SliceIdArrayName_Key);
  auto pSliceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_SliceAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
