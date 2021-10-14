#include "ComputeUmeyamaTransform.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string ComputeUmeyamaTransform::name() const
{
  return FilterTraits<ComputeUmeyamaTransform>::name.str();
}

Uuid ComputeUmeyamaTransform::uuid() const
{
  return FilterTraits<ComputeUmeyamaTransform>::uuid;
}

std::string ComputeUmeyamaTransform::humanName() const
{
  return "Compute Umeyama Transform";
}

Parameters ComputeUmeyamaTransform::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_UseScaling_Key, "Use Scaling", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SourcePointSet_Key, "Moving Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DestPointSet_Key, "Fixed Geometry", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Transformation Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationAttributeMatrixName_Key, "Transformation Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationMatrixName_Key, "Transformation Matrix", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ComputeUmeyamaTransform::clone() const
{
  return std::make_unique<ComputeUmeyamaTransform>();
}

Result<OutputActions> ComputeUmeyamaTransform::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pUseScalingValue = filterArgs.value<bool>(k_UseScaling_Key);
  auto pSourcePointSetValue = filterArgs.value<DataPath>(k_SourcePointSet_Key);
  auto pDestPointSetValue = filterArgs.value<DataPath>(k_DestPointSet_Key);
  auto pTransformationAttributeMatrixNameValue = filterArgs.value<DataPath>(k_TransformationAttributeMatrixName_Key);
  auto pTransformationMatrixNameValue = filterArgs.value<DataPath>(k_TransformationMatrixName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ComputeUmeyamaTransformAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ComputeUmeyamaTransform::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseScalingValue = filterArgs.value<bool>(k_UseScaling_Key);
  auto pSourcePointSetValue = filterArgs.value<DataPath>(k_SourcePointSet_Key);
  auto pDestPointSetValue = filterArgs.value<DataPath>(k_DestPointSet_Key);
  auto pTransformationAttributeMatrixNameValue = filterArgs.value<DataPath>(k_TransformationAttributeMatrixName_Key);
  auto pTransformationMatrixNameValue = filterArgs.value<DataPath>(k_TransformationMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
