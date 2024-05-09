#include "ComputeFeatureNeighborCAxisMisalignmentsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NeighborListSelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborCAxisMisalignmentsFilter::name() const
{
  return FilterTraits<ComputeFeatureNeighborCAxisMisalignmentsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborCAxisMisalignmentsFilter::className() const
{
  return FilterTraits<ComputeFeatureNeighborCAxisMisalignmentsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureNeighborCAxisMisalignmentsFilter::uuid() const
{
  return FilterTraits<ComputeFeatureNeighborCAxisMisalignmentsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborCAxisMisalignmentsFilter::humanName() const
{
  return "Find Feature Neighbor C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureNeighborCAxisMisalignmentsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureNeighborCAxisMisalignmentsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindAvgMisals_Key, "Find Average Misalignment Per Feature",
                                                                 "Whether the average of the C-axis misalignments with the neighboring Features should be stored for each Feature", false));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NeighborListArrayPath_Key, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", DataPath{},
                                                                 NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Defines the average orientation of the Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each **Feature** belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CAxisMisalignmentListArrayName_Key, "C-Axis Misalignment List",
                                                          "List of the C-axis misalignment angles (in degrees) with the contiguous neighboring Features for a given Feature", "CAxisMisalignmentList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgCAxisMisalignmentsArrayName_Key, "Average C-Axis Misalignments",
                                                          "Number weighted average of neighbor C-axis misalignments. Only created if Find Average Misalignment Per Feature is checked",
                                                          "AvgCAxisMisalignments"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindAvgMisals_Key, k_AvgCAxisMisalignmentsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureNeighborCAxisMisalignmentsFilter::clone() const
{
  return std::make_unique<ComputeFeatureNeighborCAxisMisalignmentsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureNeighborCAxisMisalignmentsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pFindAvgMisalsValue = filterArgs.value<bool>(k_FindAvgMisals_Key);
  auto pNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCAxisMisalignmentListArrayNameValue = filterArgs.value<std::string>(k_CAxisMisalignmentListArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples({pNeighborListArrayPathValue, pAvgQuatsArrayPathValue, pFeaturePhasesArrayPathValue});
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-1560, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& featurePhases = dataStructure.getDataRefAs<Int32Array>(pFeaturePhasesArrayPathValue);
  {
    auto createArrayAction =
        std::make_unique<CreateNeighborListAction>(DataType::float32, featurePhases.getNumberOfTuples(), pFeaturePhasesArrayPathValue.replaceName(pCAxisMisalignmentListArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(pFindAvgMisalsValue)
  {
    auto pAvgCAxisMisalignmentsArrayNameValue = filterArgs.value<std::string>(k_AvgCAxisMisalignmentsArrayName_Key);
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, featurePhases.getTupleShape(), std::vector<usize>{1}, pFeaturePhasesArrayPathValue.replaceName(pAvgCAxisMisalignmentsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-1561, "Finding the feature neighbor c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these types."});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureNeighborCAxisMisalignmentsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                     const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  ComputeFeatureNeighborCAxisMisalignmentsInputValues inputValues;

  inputValues.FindAvgMisals = filterArgs.value<bool>(k_FindAvgMisals_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CAxisMisalignmentListArrayName = inputValues.FeaturePhasesArrayPath.replaceName(filterArgs.value<std::string>(k_CAxisMisalignmentListArrayName_Key));
  inputValues.AvgCAxisMisalignmentsArrayName = inputValues.FeaturePhasesArrayPath.replaceName(filterArgs.value<std::string>(k_AvgCAxisMisalignmentsArrayName_Key));

  return ComputeFeatureNeighborCAxisMisalignments(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FindAvgMisalsKey = "FindAvgMisals";
constexpr StringLiteral k_NeighborListArrayPathKey = "NeighborListArrayPath";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_CAxisMisalignmentListArrayNameKey = "CAxisMisalignmentListArrayName";
constexpr StringLiteral k_AvgCAxisMisalignmentsArrayNameKey = "AvgCAxisMisalignmentsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureNeighborCAxisMisalignmentsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureNeighborCAxisMisalignmentsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindAvgMisalsKey, k_FindAvgMisals_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NeighborListArrayPathKey, k_NeighborListArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_CAxisMisalignmentListArrayNameKey, k_CAxisMisalignmentListArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_AvgCAxisMisalignmentsArrayNameKey, k_AvgCAxisMisalignmentsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
