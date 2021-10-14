#include "RenameAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string RenameAttributeMatrix::name() const
{
  return FilterTraits<RenameAttributeMatrix>::name.str();
}

std::string RenameAttributeMatrix::className() const
{
  return FilterTraits<RenameAttributeMatrix>::className;
}

Uuid RenameAttributeMatrix::uuid() const
{
  return FilterTraits<RenameAttributeMatrix>::uuid;
}

std::string RenameAttributeMatrix::humanName() const
{
  return "Rename Attribute Matrix";
}

Parameters RenameAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Attribute Matrix to Rename", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_NewAttributeMatrix_Key, "New Attribute Matrix Name", "", "SomeString"));

  return params;
}

IFilter::UniquePointer RenameAttributeMatrix::clone() const
{
  return std::make_unique<RenameAttributeMatrix>();
}

Result<OutputActions> RenameAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<StringParameter::ValueType>(k_NewAttributeMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RenameAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> RenameAttributeMatrix::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<StringParameter::ValueType>(k_NewAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
