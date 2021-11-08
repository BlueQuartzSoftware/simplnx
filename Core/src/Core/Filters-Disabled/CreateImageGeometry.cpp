#include "CreateImageGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateImageGeometry::name() const
{
  return FilterTraits<CreateImageGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateImageGeometry::className() const
{
  return FilterTraits<CreateImageGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CreateImageGeometry::uuid() const
{
  return FilterTraits<CreateImageGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateImageGeometry::humanName() const
{
  return "Create Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateImageGeometry::defaultTags() const
{
  return {"#Core", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters CreateImageGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataContainer_Key, "Data Container Destination", "", DataPath{}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_BoxDimensions_Key, "Box Size in Length Units", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateImageGeometry::clone() const
{
  return std::make_unique<CreateImageGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateImageGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedDataContainerValue = filterArgs.value<DataPath>(k_SelectedDataContainer_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_BoxDimensions_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateImageGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateImageGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedDataContainerValue = filterArgs.value<DataPath>(k_SelectedDataContainer_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_BoxDimensions_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
