#include "ItkGrayToRGB.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkGrayToRGB::name() const
{
  return FilterTraits<ItkGrayToRGB>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkGrayToRGB::className() const
{
  return FilterTraits<ItkGrayToRGB>::className;
}

//------------------------------------------------------------------------------
Uuid ItkGrayToRGB::uuid() const
{
  return FilterTraits<ItkGrayToRGB>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkGrayToRGB::humanName() const
{
  return "Convert Grayscale to RGB (Merge Channels) (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkGrayToRGB::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkGrayToRGB::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_RedArrayPath_Key, "Red Channel", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GreenArrayPath_Key, "Green Channel", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BlueArrayPath_Key, "Blue Channel", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "RGB Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkGrayToRGB::clone() const
{
  return std::make_unique<ItkGrayToRGB>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkGrayToRGB::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRedArrayPathValue = filterArgs.value<DataPath>(k_RedArrayPath_Key);
  auto pGreenArrayPathValue = filterArgs.value<DataPath>(k_GreenArrayPath_Key);
  auto pBlueArrayPathValue = filterArgs.value<DataPath>(k_BlueArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkGrayToRGBAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkGrayToRGB::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRedArrayPathValue = filterArgs.value<DataPath>(k_RedArrayPath_Key);
  auto pGreenArrayPathValue = filterArgs.value<DataPath>(k_GreenArrayPath_Key);
  auto pBlueArrayPathValue = filterArgs.value<DataPath>(k_BlueArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
