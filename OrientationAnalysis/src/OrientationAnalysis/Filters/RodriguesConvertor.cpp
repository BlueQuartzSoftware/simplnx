#include "RodriguesConvertor.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RodriguesConvertor::name() const
{
  return FilterTraits<RodriguesConvertor>::name.str();
}

//------------------------------------------------------------------------------
std::string RodriguesConvertor::className() const
{
  return FilterTraits<RodriguesConvertor>::className;
}

//------------------------------------------------------------------------------
Uuid RodriguesConvertor::uuid() const
{
  return FilterTraits<RodriguesConvertor>::uuid;
}

//------------------------------------------------------------------------------
std::string RodriguesConvertor::humanName() const
{
  return "Rodrigues Convertor";
}

//------------------------------------------------------------------------------
std::vector<std::string> RodriguesConvertor::defaultTags() const
{
  return {"#Processing", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters RodriguesConvertor::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_RodriguesDataArrayPath_Key, "Rodrigues Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Data Array Path", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RodriguesConvertor::clone() const
{
  return std::make_unique<RodriguesConvertor>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RodriguesConvertor::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRodriguesDataArrayPathValue = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RodriguesConvertorAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RodriguesConvertor::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRodriguesDataArrayPathValue = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
