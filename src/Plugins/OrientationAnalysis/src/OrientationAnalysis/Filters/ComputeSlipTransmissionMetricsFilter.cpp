#include "ComputeSlipTransmissionMetricsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeSlipTransmissionMetrics.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NeighborListSelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeSlipTransmissionMetricsFilter::name() const
{
  return FilterTraits<ComputeSlipTransmissionMetricsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeSlipTransmissionMetricsFilter::className() const
{
  return FilterTraits<ComputeSlipTransmissionMetricsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeSlipTransmissionMetricsFilter::uuid() const
{
  return FilterTraits<ComputeSlipTransmissionMetricsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeSlipTransmissionMetricsFilter::humanName() const
{
  return "Compute Neighbor Slip Transmission Metrics";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeSlipTransmissionMetricsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Find", "Generate", "Calculate", "Determine", "Luster-Morris"};
}

//------------------------------------------------------------------------------
Parameters ComputeSlipTransmissionMetricsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Data Array that specifies the average orientation of each Feature as a quaternion",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "Data Array that specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NeighborListArrayPath_Key, "Feature Neighbor List", "List of the contiguous neighboring Features for a given Feature", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each phase", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_mPrimeListArrayName_Key, "Luster-Morris M-Prime", "DataArray Name to store the calculated Luster-Morris Parameters", "mPrime List"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_F1ListArrayName_Key, "Fracture Initiation Parameter (F1)", "DataArray Name to store the calculated F1 Values", "F1 List"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_F1sptListArrayName_Key, "Fracture Initiation Parameter (F1spt)", "DataArray Name to store the calculated F1spt Values", "F1spt List"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_F7ListArrayName_Key, "Fracture Initiation Parameter (F7)", "DataArray Name to store the calculated F7 Values", "F7 List"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeSlipTransmissionMetricsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeSlipTransmissionMetricsFilter::clone() const
{
  return std::make_unique<ComputeSlipTransmissionMetricsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeSlipTransmissionMetricsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto pNeighborListPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pFeaturePhasesPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pF1ListNameValue = filterArgs.value<std::string>(k_F1ListArrayName_Key);
  auto pF1sptListNameValue = filterArgs.value<std::string>(k_F1sptListArrayName_Key);
  auto pF7ListNameValue = filterArgs.value<std::string>(k_F7ListArrayName_Key);
  auto pmPrimeListNameValue = filterArgs.value<std::string>(k_mPrimeListArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  usize tupShape = dataStructure.getDataAs<Int32NeighborList>(pNeighborListPathValue)->getNumberOfTuples();

  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupShape, pNeighborListPathValue.replaceName(pF1ListNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupShape, pNeighborListPathValue.replaceName(pF1sptListNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupShape, pNeighborListPathValue.replaceName(pF7ListNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupShape, pNeighborListPathValue.replaceName(pmPrimeListNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeSlipTransmissionMetricsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  ComputeSlipTransmissionMetricsInputValues inputValues;

  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.F1ListArrayName = inputValues.NeighborListArrayPath.replaceName(filterArgs.value<std::string>(k_F1ListArrayName_Key));
  inputValues.F1sptListArrayName = inputValues.NeighborListArrayPath.replaceName(filterArgs.value<std::string>(k_F1sptListArrayName_Key));
  inputValues.F7ListArrayName = inputValues.NeighborListArrayPath.replaceName(filterArgs.value<std::string>(k_F7ListArrayName_Key));
  inputValues.mPrimeListArrayName = inputValues.NeighborListArrayPath.replaceName(filterArgs.value<std::string>(k_mPrimeListArrayName_Key));

  return ComputeSlipTransmissionMetrics(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_NeighborListArrayPathKey = "NeighborListArrayPath";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_F1ListArrayNameKey = "F1ListArrayName";
constexpr StringLiteral k_F1sptListArrayNameKey = "F1sptListArrayName";
constexpr StringLiteral k_F7ListArrayNameKey = "F7ListArrayName";
constexpr StringLiteral k_mPrimeListArrayNameKey = "mPrimeListArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeSlipTransmissionMetricsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeSlipTransmissionMetricsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NeighborListArrayPathKey, k_NeighborListArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_F1ListArrayNameKey, k_F1ListArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_F1sptListArrayNameKey, k_F1sptListArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_F7ListArrayNameKey, k_F7ListArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_mPrimeListArrayNameKey, k_mPrimeListArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
