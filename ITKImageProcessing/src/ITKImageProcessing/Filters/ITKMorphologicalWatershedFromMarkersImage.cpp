#include "ITKMorphologicalWatershedFromMarkersImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKMorphologicalWatershedFromMarkersImage::name() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImage>::name.str();
}

Uuid ITKMorphologicalWatershedFromMarkersImage::uuid() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImage>::uuid;
}

std::string ITKMorphologicalWatershedFromMarkersImage::humanName() const
{
  return "ITK::Morphological Watershed From Markers Image Filter";
}

Parameters ITKMorphologicalWatershedFromMarkersImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_MarkWatershedLine_Key, "MarkWatershedLine", "", false));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MarkerCellArrayPath_Key, "Marker Array", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKMorphologicalWatershedFromMarkersImage::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedFromMarkersImage>();
}

Result<OutputActions> ITKMorphologicalWatershedFromMarkersImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMarkWatershedLineValue = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto pFullyConnectedValue = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMarkerCellArrayPathValue = filterArgs.value<DataPath>(k_MarkerCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKMorphologicalWatershedFromMarkersImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKMorphologicalWatershedFromMarkersImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMarkWatershedLineValue = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto pFullyConnectedValue = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMarkerCellArrayPathValue = filterArgs.value<DataPath>(k_MarkerCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
