#include "FindSizes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindSizes::name() const
{
  return FilterTraits<FindSizes>::name.str();
}

Uuid FindSizes::uuid() const
{
  return FilterTraits<FindSizes>::uuid;
}

std::string FindSizes::humanName() const
{
  return "Find Feature Sizes";
}

Parameters FindSizes::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Save Element Sizes", "", false));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EquivalentDiametersArrayName_Key, "Equivalent Diameters", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumElementsArrayName_Key, "Number of Elements", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolumesArrayName_Key, "Volumes", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindSizes::clone() const
{
  return std::make_unique<FindSizes>();
}

Result<OutputActions> FindSizes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSaveElementSizesValue = filterArgs.value<bool>(k_SaveElementSizes_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pEquivalentDiametersArrayNameValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayName_Key);
  auto pNumElementsArrayNameValue = filterArgs.value<DataPath>(k_NumElementsArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindSizesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindSizes::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSaveElementSizesValue = filterArgs.value<bool>(k_SaveElementSizes_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pEquivalentDiametersArrayNameValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayName_Key);
  auto pNumElementsArrayNameValue = filterArgs.value<DataPath>(k_NumElementsArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
