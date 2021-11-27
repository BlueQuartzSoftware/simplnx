#include "SliceTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
IFilter::PreflightResult SliceTriangleGeometry::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
