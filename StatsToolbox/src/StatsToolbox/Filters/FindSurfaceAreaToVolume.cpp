#include "FindSurfaceAreaToVolume.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindSurfaceAreaToVolume::name() const
{
  return FilterTraits<FindSurfaceAreaToVolume>::name.str();
}

std::string FindSurfaceAreaToVolume::className() const
{
  return FilterTraits<FindSurfaceAreaToVolume>::className;
}

Uuid FindSurfaceAreaToVolume::uuid() const
{
  return FilterTraits<FindSurfaceAreaToVolume>::uuid;
}

std::string FindSurfaceAreaToVolume::humanName() const
{
  return "Find Surface Area to Volume & Sphericity";
}

Parameters FindSurfaceAreaToVolume::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumCellsArrayPath_Key, "Number of Cells", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceAreaVolumeRatioArrayName_Key, "Surface Area to Volume Ratio", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CalculateSphericity_Key, "Calculate Sphericity", "", false));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SphericityArrayName_Key, "Sphericity Array Name", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_CalculateSphericity_Key, k_SphericityArrayName_Key, true);

  return params;
}

IFilter::UniquePointer FindSurfaceAreaToVolume::clone() const
{
  return std::make_unique<FindSurfaceAreaToVolume>();
}

Result<OutputActions> FindSurfaceAreaToVolume::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pNumCellsArrayPathValue = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  auto pSurfaceAreaVolumeRatioArrayNameValue = filterArgs.value<DataPath>(k_SurfaceAreaVolumeRatioArrayName_Key);
  auto pCalculateSphericityValue = filterArgs.value<bool>(k_CalculateSphericity_Key);
  auto pSphericityArrayNameValue = filterArgs.value<DataPath>(k_SphericityArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindSurfaceAreaToVolumeAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindSurfaceAreaToVolume::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pNumCellsArrayPathValue = filterArgs.value<DataPath>(k_NumCellsArrayPath_Key);
  auto pSurfaceAreaVolumeRatioArrayNameValue = filterArgs.value<DataPath>(k_SurfaceAreaVolumeRatioArrayName_Key);
  auto pCalculateSphericityValue = filterArgs.value<bool>(k_CalculateSphericity_Key);
  auto pSphericityArrayNameValue = filterArgs.value<DataPath>(k_SphericityArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
