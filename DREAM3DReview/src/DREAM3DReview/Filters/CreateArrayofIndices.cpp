#include "CreateArrayofIndices.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateArrayofIndices::name() const
{
  return FilterTraits<CreateArrayofIndices>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateArrayofIndices::className() const
{
  return FilterTraits<CreateArrayofIndices>::className;
}

//------------------------------------------------------------------------------
Uuid CreateArrayofIndices::uuid() const
{
  return FilterTraits<CreateArrayofIndices>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateArrayofIndices::humanName() const
{
  return "Create Array of Indices";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateArrayofIndices::defaultTags() const
{
  return {"#Unsupported", "#DREAM3DReview"};
}

//------------------------------------------------------------------------------
Parameters CreateArrayofIndices::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArrayCreationParameter>(k_IndexArrayPath_Key, "Index Array Path", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateArrayofIndices::clone() const
{
  return std::make_unique<CreateArrayofIndices>();
}

//------------------------------------------------------------------------------
Result<OutputActions> CreateArrayofIndices::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pIndexArrayPathValue = filterArgs.value<DataPath>(k_IndexArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateArrayofIndicesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateArrayofIndices::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pIndexArrayPathValue = filterArgs.value<DataPath>(k_IndexArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
