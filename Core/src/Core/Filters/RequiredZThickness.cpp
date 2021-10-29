#include "RequiredZThickness.hpp"

#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RequiredZThickness::name() const
{
  return FilterTraits<RequiredZThickness>::name.str();
}

//------------------------------------------------------------------------------
std::string RequiredZThickness::className() const
{
  return FilterTraits<RequiredZThickness>::className;
}

//------------------------------------------------------------------------------
Uuid RequiredZThickness::uuid() const
{
  return FilterTraits<RequiredZThickness>::uuid;
}

//------------------------------------------------------------------------------
std::string RequiredZThickness::humanName() const
{
  return "Required Z Dimension (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> RequiredZThickness::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters RequiredZThickness::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RequiredZThickness::clone() const
{
  return std::make_unique<RequiredZThickness>();
}

//------------------------------------------------------------------------------
Result<OutputActions> RequiredZThickness::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RequiredZThicknessAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RequiredZThickness::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
