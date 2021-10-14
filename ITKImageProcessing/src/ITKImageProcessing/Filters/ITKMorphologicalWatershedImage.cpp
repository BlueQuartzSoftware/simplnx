#include "ITKMorphologicalWatershedImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKMorphologicalWatershedImage::name() const
{
  return FilterTraits<ITKMorphologicalWatershedImage>::name.str();
}

std::string ITKMorphologicalWatershedImage::className() const
{
  return FilterTraits<ITKMorphologicalWatershedImage>::className;
}

Uuid ITKMorphologicalWatershedImage::uuid() const
{
  return FilterTraits<ITKMorphologicalWatershedImage>::uuid;
}

std::string ITKMorphologicalWatershedImage::humanName() const
{
  return "ITK::Morphological Watershed Image Filter";
}

Parameters ITKMorphologicalWatershedImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Level_Key, "Level", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_MarkWatershedLine_Key, "MarkWatershedLine", "", false));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKMorphologicalWatershedImage::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedImage>();
}

Result<OutputActions> ITKMorphologicalWatershedImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pLevelValue = filterArgs.value<float64>(k_Level_Key);
  auto pMarkWatershedLineValue = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto pFullyConnectedValue = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKMorphologicalWatershedImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKMorphologicalWatershedImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLevelValue = filterArgs.value<float64>(k_Level_Key);
  auto pMarkWatershedLineValue = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto pFullyConnectedValue = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
