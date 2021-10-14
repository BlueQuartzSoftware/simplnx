#include "ItkConvertArrayTo8BitImageAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string ItkConvertArrayTo8BitImageAttributeMatrix::name() const
{
  return FilterTraits<ItkConvertArrayTo8BitImageAttributeMatrix>::name.str();
}

Uuid ItkConvertArrayTo8BitImageAttributeMatrix::uuid() const
{
  return FilterTraits<ItkConvertArrayTo8BitImageAttributeMatrix>::uuid;
}

std::string ItkConvertArrayTo8BitImageAttributeMatrix::humanName() const
{
  return "Convert Array to 8 Bit Image Attribute Matrix (ImageProcessing)";
}

Parameters ItkConvertArrayTo8BitImageAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ItkConvertArrayTo8BitImageAttributeMatrix::clone() const
{
  return std::make_unique<ItkConvertArrayTo8BitImageAttributeMatrix>();
}

Result<OutputActions> ItkConvertArrayTo8BitImageAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkConvertArrayTo8BitImageAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ItkConvertArrayTo8BitImageAttributeMatrix::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
