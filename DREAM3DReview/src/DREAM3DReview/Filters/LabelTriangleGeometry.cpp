#include "LabelTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string LabelTriangleGeometry::name() const
{
  return FilterTraits<LabelTriangleGeometry>::name.str();
}

Uuid LabelTriangleGeometry::uuid() const
{
  return FilterTraits<LabelTriangleGeometry>::uuid;
}

std::string LabelTriangleGeometry::humanName() const
{
  return "Label CAD Geometry";
}

Parameters LabelTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CADDataContainerName_Key, "CAD Geometry", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_TriangleAttributeMatrixName_Key, "Triangle Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_RegionIdArrayName_Key, "Region Ids", "", "SomeString"));

  return params;
}

IFilter::UniquePointer LabelTriangleGeometry::clone() const
{
  return std::make_unique<LabelTriangleGeometry>();
}

Result<OutputActions> LabelTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  auto pTriangleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleAttributeMatrixName_Key);
  auto pRegionIdArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RegionIdArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<LabelTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> LabelTriangleGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCADDataContainerNameValue = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  auto pTriangleAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TriangleAttributeMatrixName_Key);
  auto pRegionIdArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RegionIdArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
