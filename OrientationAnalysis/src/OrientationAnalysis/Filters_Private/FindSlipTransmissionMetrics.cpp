#include "FindSlipTransmissionMetrics.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSlipTransmissionMetrics::name() const
{
  return FilterTraits<FindSlipTransmissionMetrics>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSlipTransmissionMetrics::className() const
{
  return FilterTraits<FindSlipTransmissionMetrics>::className;
}

//------------------------------------------------------------------------------
Uuid FindSlipTransmissionMetrics::uuid() const
{
  return FilterTraits<FindSlipTransmissionMetrics>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSlipTransmissionMetrics::humanName() const
{
  return "Find Neighbor Slip Transmission Metrics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSlipTransmissionMetrics::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindSlipTransmissionMetrics::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orienation of each Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath({"CellFeatureData", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{complex::int32}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_F1ListArrayName_Key, "F1 List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_F1sptListArrayName_Key, "F1spt List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_F7ListArrayName_Key, "F7 List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_mPrimeListArrayName_Key, "mPrime List", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSlipTransmissionMetrics::clone() const
{
  return std::make_unique<FindSlipTransmissionMetrics>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSlipTransmissionMetrics::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pF1ListArrayNameValue = filterArgs.value<DataPath>(k_F1ListArrayName_Key);
  auto pF1sptListArrayNameValue = filterArgs.value<DataPath>(k_F1sptListArrayName_Key);
  auto pF7ListArrayNameValue = filterArgs.value<DataPath>(k_F7ListArrayName_Key);
  auto pmPrimeListArrayNameValue = filterArgs.value<DataPath>(k_mPrimeListArrayName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindSlipTransmissionMetrics::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pF1ListArrayNameValue = filterArgs.value<DataPath>(k_F1ListArrayName_Key);
  auto pF1sptListArrayNameValue = filterArgs.value<DataPath>(k_F1sptListArrayName_Key);
  auto pF7ListArrayNameValue = filterArgs.value<DataPath>(k_F7ListArrayName_Key);
  auto pmPrimeListArrayNameValue = filterArgs.value<DataPath>(k_mPrimeListArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
