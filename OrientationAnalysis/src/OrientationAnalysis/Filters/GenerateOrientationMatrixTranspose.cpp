#include "GenerateOrientationMatrixTranspose.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
std::string GenerateOrientationMatrixTranspose::name() const
{
  return FilterTraits<GenerateOrientationMatrixTranspose>::name.str();
}

Uuid GenerateOrientationMatrixTranspose::uuid() const
{
  return FilterTraits<GenerateOrientationMatrixTranspose>::uuid;
}

std::string GenerateOrientationMatrixTranspose::humanName() const
{
  return "Generate Orientation Matrix Transpose";
}

Parameters GenerateOrientationMatrixTranspose::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_OrientationMatrixDataArrayPath_Key, "Quaternion Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Data Array Path", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  return params;
}

IFilter::UniquePointer GenerateOrientationMatrixTranspose::clone() const
{
  return std::make_unique<GenerateOrientationMatrixTranspose>();
}

Result<OutputActions> GenerateOrientationMatrixTranspose::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOrientationMatrixDataArrayPathValue = filterArgs.value<DataPath>(k_OrientationMatrixDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateOrientationMatrixTransposeAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> GenerateOrientationMatrixTranspose::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOrientationMatrixDataArrayPathValue = filterArgs.value<DataPath>(k_OrientationMatrixDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
