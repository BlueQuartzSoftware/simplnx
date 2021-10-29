#include "Lesson7.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string Lesson7::name() const
{
  return FilterTraits<Lesson7>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson7::className() const
{
  return FilterTraits<Lesson7>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson7::uuid() const
{
  return FilterTraits<Lesson7>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson7::humanName() const
{
  return "Lesson7";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson7::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson7::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_GeometrySelection_Key, "Geometry", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Lesson7::clone() const
{
  return std::make_unique<Lesson7>();
}

//------------------------------------------------------------------------------
Result<OutputActions> Lesson7::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGeometrySelectionValue = filterArgs.value<DataPath>(k_GeometrySelection_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<Lesson7Action>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> Lesson7::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
