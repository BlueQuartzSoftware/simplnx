#include "ItkConvertArrayTo8BitImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string ItkConvertArrayTo8BitImage::name() const
{
  return FilterTraits<ItkConvertArrayTo8BitImage>::name.str();
}

std::string ItkConvertArrayTo8BitImage::className() const
{
  return FilterTraits<ItkConvertArrayTo8BitImage>::className;
}

Uuid ItkConvertArrayTo8BitImage::uuid() const
{
  return FilterTraits<ItkConvertArrayTo8BitImage>::uuid;
}

std::string ItkConvertArrayTo8BitImage::humanName() const
{
  return "Convert Array to 8 Bit Image (ImageProcessing)";
}

Parameters ItkConvertArrayTo8BitImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array To Convert", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArrayArrayName_Key, "Converted Attribute Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ItkConvertArrayTo8BitImage::clone() const
{
  return std::make_unique<ItkConvertArrayTo8BitImage>();
}

Result<OutputActions> ItkConvertArrayTo8BitImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkConvertArrayTo8BitImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ItkConvertArrayTo8BitImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
