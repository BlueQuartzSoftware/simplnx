#include "TriangleAreaFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

using namespace complex;

namespace complex
{
std::string TriangleAreaFilter::name() const
{
  return FilterTraits<TriangleAreaFilter>::name.str();
}

std::string TriangleAreaFilter::className() const
{
  return FilterTraits<TriangleAreaFilter>::className;
}

Uuid TriangleAreaFilter::uuid() const
{
  return FilterTraits<TriangleAreaFilter>::uuid;
}

std::string TriangleAreaFilter::humanName() const
{
  return "Generate Triangle Areas";
}

Parameters TriangleAreaFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTriangleAreasArrayPath_Key, "Face Areas", "", DataPath{}));

  return params;
}

IFilter::UniquePointer TriangleAreaFilter::clone() const
{
  return std::make_unique<TriangleAreaFilter>();
}

Result<OutputActions> TriangleAreaFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<TriangleAreaFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> TriangleAreaFilter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
