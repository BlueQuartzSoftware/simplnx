#include "ItkConvertArrayTo8BitImageAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkConvertArrayTo8BitImageAttributeMatrix::name() const
{
  return FilterTraits<ItkConvertArrayTo8BitImageAttributeMatrix>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkConvertArrayTo8BitImageAttributeMatrix::className() const
{
  return FilterTraits<ItkConvertArrayTo8BitImageAttributeMatrix>::className;
}

//------------------------------------------------------------------------------
Uuid ItkConvertArrayTo8BitImageAttributeMatrix::uuid() const
{
  return FilterTraits<ItkConvertArrayTo8BitImageAttributeMatrix>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkConvertArrayTo8BitImageAttributeMatrix::humanName() const
{
  return "Convert Array to 8 Bit Image Attribute Matrix (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkConvertArrayTo8BitImageAttributeMatrix::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkConvertArrayTo8BitImageAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkConvertArrayTo8BitImageAttributeMatrix::clone() const
{
  return std::make_unique<ItkConvertArrayTo8BitImageAttributeMatrix>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ItkConvertArrayTo8BitImageAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkConvertArrayTo8BitImageAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ItkConvertArrayTo8BitImageAttributeMatrix::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
