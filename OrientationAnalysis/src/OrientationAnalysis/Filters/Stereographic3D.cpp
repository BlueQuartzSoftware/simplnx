#include "Stereographic3D.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string Stereographic3D::name() const
{
  return FilterTraits<Stereographic3D>::name.str();
}

Uuid Stereographic3D::uuid() const
{
  return FilterTraits<Stereographic3D>::uuid;
}

std::string Stereographic3D::humanName() const
{
  return "Stereographic 3D Coordinates";
}

Parameters Stereographic3D::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CoordinatesArrayName_Key, "Coordinates", "", DataPath{}));

  return params;
}

IFilter::UniquePointer Stereographic3D::clone() const
{
  return std::make_unique<Stereographic3D>();
}

Result<OutputActions> Stereographic3D::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCoordinatesArrayNameValue = filterArgs.value<DataPath>(k_CoordinatesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<Stereographic3DAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> Stereographic3D::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCoordinatesArrayNameValue = filterArgs.value<DataPath>(k_CoordinatesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
