#include "CopyAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string CopyAttributeMatrix::name() const
{
  return FilterTraits<CopyAttributeMatrix>::name.str();
}

std::string CopyAttributeMatrix::className() const
{
  return FilterTraits<CopyAttributeMatrix>::className;
}

Uuid CopyAttributeMatrix::uuid() const
{
  return FilterTraits<CopyAttributeMatrix>::uuid;
}

std::string CopyAttributeMatrix::humanName() const
{
  return "Copy Attribute Matrix";
}

Parameters CopyAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Attribute Matrix to Copy", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewAttributeMatrix_Key, "Copied Attribute Matrix", "", DataPath{}));

  return params;
}

IFilter::UniquePointer CopyAttributeMatrix::clone() const
{
  return std::make_unique<CopyAttributeMatrix>();
}

Result<OutputActions> CopyAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<DataPath>(k_NewAttributeMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CopyAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CopyAttributeMatrix::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<DataPath>(k_NewAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
