#include "ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ConvertHexGridToSquareGridFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGrid::name() const
{
  return FilterTraits<ConvertHexGridToSquareGrid>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGrid::className() const
{
  return FilterTraits<ConvertHexGridToSquareGrid>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertHexGridToSquareGrid::uuid() const
{
  return FilterTraits<ConvertHexGridToSquareGrid>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGrid::humanName() const
{
  return "Convert Hexagonal Grid Data to Square Grid Data (TSL - .ang)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertHexGridToSquareGrid::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters ConvertHexGridToSquareGrid::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ConvertHexGridToSquareGridFilterParameter>(k_HexGridStack_Key, "Convert Hex Grid ANG Files", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertHexGridToSquareGrid::clone() const
{
  return std::make_unique<ConvertHexGridToSquareGrid>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ConvertHexGridToSquareGrid::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pHexGridStackValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_HexGridStack_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ConvertHexGridToSquareGridAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGrid::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pHexGridStackValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_HexGridStack_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
