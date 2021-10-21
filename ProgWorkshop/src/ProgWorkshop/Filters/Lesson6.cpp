#include "Lesson6.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string Lesson6::name() const
{
  return FilterTraits<Lesson6>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson6::className() const
{
  return FilterTraits<Lesson6>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson6::uuid() const
{
  return FilterTraits<Lesson6>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson6::humanName() const
{
  return "Lesson6";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson6::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson6::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_GeometrySelection_Key, "Geometry", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Lesson6::clone() const
{
  return std::make_unique<Lesson6>();
}

//------------------------------------------------------------------------------
Result<OutputActions> Lesson6::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGeometrySelectionValue = filterArgs.value<DataPath>(k_GeometrySelection_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<Lesson6Action>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> Lesson6::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pGeometrySelectionValue = filterArgs.value<DataPath>(k_GeometrySelection_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
