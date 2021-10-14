#include "GenerateQuaternionConjugate.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
std::string GenerateQuaternionConjugate::name() const
{
  return FilterTraits<GenerateQuaternionConjugate>::name.str();
}

std::string GenerateQuaternionConjugate::className() const
{
  return FilterTraits<GenerateQuaternionConjugate>::className;
}

Uuid GenerateQuaternionConjugate::uuid() const
{
  return FilterTraits<GenerateQuaternionConjugate>::uuid;
}

std::string GenerateQuaternionConjugate::humanName() const
{
  return "Generate Quaternion Conjugate";
}

Parameters GenerateQuaternionConjugate::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuaternionDataArrayPath_Key, "Quaternion Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Data Array Path", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  return params;
}

IFilter::UniquePointer GenerateQuaternionConjugate::clone() const
{
  return std::make_unique<GenerateQuaternionConjugate>();
}

Result<OutputActions> GenerateQuaternionConjugate::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_QuaternionDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateQuaternionConjugateAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> GenerateQuaternionConjugate::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_QuaternionDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
