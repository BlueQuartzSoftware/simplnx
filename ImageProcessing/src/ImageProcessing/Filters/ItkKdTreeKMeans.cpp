#include "ItkKdTreeKMeans.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkKdTreeKMeans::name() const
{
  return FilterTraits<ItkKdTreeKMeans>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkKdTreeKMeans::className() const
{
  return FilterTraits<ItkKdTreeKMeans>::className;
}

//------------------------------------------------------------------------------
Uuid ItkKdTreeKMeans::uuid() const
{
  return FilterTraits<ItkKdTreeKMeans>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkKdTreeKMeans::humanName() const
{
  return "K-d Tree K Means (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkKdTreeKMeans::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkKdTreeKMeans::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_Classes_Key, "Number of Classes", "", 1234356));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Classify", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Class Labels", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkKdTreeKMeans::clone() const
{
  return std::make_unique<ItkKdTreeKMeans>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkKdTreeKMeans::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pClassesValue = filterArgs.value<int32>(k_Classes_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkKdTreeKMeansAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkKdTreeKMeans::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pClassesValue = filterArgs.value<int32>(k_Classes_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
