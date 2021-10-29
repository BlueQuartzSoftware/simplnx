#include "ItkDiscreteGaussianBlur.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkDiscreteGaussianBlur::name() const
{
  return FilterTraits<ItkDiscreteGaussianBlur>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkDiscreteGaussianBlur::className() const
{
  return FilterTraits<ItkDiscreteGaussianBlur>::className;
}

//------------------------------------------------------------------------------
Uuid ItkDiscreteGaussianBlur::uuid() const
{
  return FilterTraits<ItkDiscreteGaussianBlur>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkDiscreteGaussianBlur::humanName() const
{
  return "Discrete Gaussian Blur (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkDiscreteGaussianBlur::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkDiscreteGaussianBlur::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewArray_Key, "Save as New Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Blur", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Blurred Array", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Stdev_Key, "Standard Deviation", "", 1.23345f));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveAsNewArray_Key, k_NewCellArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkDiscreteGaussianBlur::clone() const
{
  return std::make_unique<ItkDiscreteGaussianBlur>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkDiscreteGaussianBlur::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);
  auto pStdevValue = filterArgs.value<float32>(k_Stdev_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkDiscreteGaussianBlurAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkDiscreteGaussianBlur::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);
  auto pStdevValue = filterArgs.value<float32>(k_Stdev_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
