#include "ItkFindMaxima.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkFindMaxima::name() const
{
  return FilterTraits<ItkFindMaxima>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkFindMaxima::className() const
{
  return FilterTraits<ItkFindMaxima>::className;
}

//------------------------------------------------------------------------------
Uuid ItkFindMaxima::uuid() const
{
  return FilterTraits<ItkFindMaxima>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkFindMaxima::humanName() const
{
  return "Find Maxima (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkFindMaxima::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkFindMaxima::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Input Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Tolerance_Key, "Noise Tolerance", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Output Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkFindMaxima::clone() const
{
  return std::make_unique<ItkFindMaxima>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ItkFindMaxima::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pToleranceValue = filterArgs.value<float32>(k_Tolerance_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkFindMaximaAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkFindMaxima::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pToleranceValue = filterArgs.value<float32>(k_Tolerance_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
