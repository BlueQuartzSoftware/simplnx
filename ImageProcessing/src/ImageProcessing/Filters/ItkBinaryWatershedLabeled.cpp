#include "ItkBinaryWatershedLabeled.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkBinaryWatershedLabeled::name() const
{
  return FilterTraits<ItkBinaryWatershedLabeled>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkBinaryWatershedLabeled::className() const
{
  return FilterTraits<ItkBinaryWatershedLabeled>::className;
}

//------------------------------------------------------------------------------
Uuid ItkBinaryWatershedLabeled::uuid() const
{
  return FilterTraits<ItkBinaryWatershedLabeled>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkBinaryWatershedLabeled::humanName() const
{
  return "Binary Watershed Labeled (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkBinaryWatershedLabeled::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkBinaryWatershedLabeled::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Watershed", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_PeakTolerance_Key, "Peak Noise Tolerance", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Watershed Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkBinaryWatershedLabeled::clone() const
{
  return std::make_unique<ItkBinaryWatershedLabeled>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ItkBinaryWatershedLabeled::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pPeakToleranceValue = filterArgs.value<float32>(k_PeakTolerance_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkBinaryWatershedLabeledAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkBinaryWatershedLabeled::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pPeakToleranceValue = filterArgs.value<float32>(k_PeakTolerance_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
