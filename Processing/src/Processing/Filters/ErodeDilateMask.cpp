#include "ErodeDilateMask.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ErodeDilateMask::name() const
{
  return FilterTraits<ErodeDilateMask>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateMask::className() const
{
  return FilterTraits<ErodeDilateMask>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateMask::uuid() const
{
  return FilterTraits<ErodeDilateMask>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateMask::humanName() const
{
  return "Erode/Dilate Mask";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateMask::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateMask::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Direction_Key, "Operation", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "", false));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "", false));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateMask::clone() const
{
  return std::make_unique<ErodeDilateMask>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ErodeDilateMask::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDirectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_Direction_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pXDirOnValue = filterArgs.value<bool>(k_XDirOn_Key);
  auto pYDirOnValue = filterArgs.value<bool>(k_YDirOn_Key);
  auto pZDirOnValue = filterArgs.value<bool>(k_ZDirOn_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ErodeDilateMaskAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateMask::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDirectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_Direction_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pXDirOnValue = filterArgs.value<bool>(k_XDirOn_Key);
  auto pYDirOnValue = filterArgs.value<bool>(k_YDirOn_Key);
  auto pZDirOnValue = filterArgs.value<bool>(k_ZDirOn_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
