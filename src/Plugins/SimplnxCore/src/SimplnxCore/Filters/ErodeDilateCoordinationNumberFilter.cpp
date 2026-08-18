#include "ErodeDilateCoordinationNumberFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief Rejects a Coordination Number outside the closed interval [0, 6]. Matches the
 * DREAM3D 6.5.171 dataCheck() guard (ErodeDilateCoordinationNumber.cpp:118-123, code -5555):
 * a voxel has six face neighbours, so no other value can ever be met.
 */
constexpr int32 k_InvalidCoordinationNumberError = -16800;

/**
 * @brief Rejects a Coordination Number of 0 while Loop is enabled. Beyond 6.5.171 parity and
 * deliberate: see deviation ErodeDilateCoordinationNumberFilter-D1. The sweep's trailing counter
 * tests coordination >= CoordinationNumber, so at a threshold of 0 the predicate holds for every
 * voxel of every volume - the counter can never reach 0 and the sweep can never converge. The
 * non-termination is universal, so refusing to start costs the user nothing. 6.5.171 accepts the
 * value and hangs.
 */
constexpr int32 k_NonTerminatingLoopError = -16801;

/**
 * @brief Warns about a Coordination Number of 1 while Loop is enabled. Unlike a threshold of 0 the
 * non-termination here is data dependent: the trailing predicate is satisfied only by voxels on a
 * good/bad boundary, so a volume containing one may oscillate indefinitely; termination is
 * data-dependent. A boundary-free volume
 * (all one feature, or all bad - a realistic post-cleanup input) converges after a single no-op
 * sweep in both implementations. Rejecting it would break legacy pipelines that ran to completion,
 * so the combination is accepted with a warning instead.
 */
constexpr int32 k_OscillatingLoopWarning = -16802;

/**
 * @brief The lowest Coordination Number for which a Loop = true run terminates on every volume.
 */
constexpr int32 k_MinimumLoopCoordinationNumber = 2;

/**
 * @brief The only Coordination Number whose Loop = true termination depends on the input volume.
 */
constexpr int32 k_OscillatingLoopCoordinationNumber = 1;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::name() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::className() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateCoordinationNumberFilter::uuid() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::humanName() const
{
  return "Erode/Dilate Coordination Number";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateCoordinationNumberFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate", "Smooth Bad Data"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateCoordinationNumberFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_CoordinationNumber_Key, "Coordination Number to Consider",
                                                 " Number of neighboring **Cells** that can be of opposite classification before a **Cell** will be removed", 6));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "Keep looping until all criteria is met", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ErodeDilateCoordinationNumberFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateCoordinationNumberFilter::clone() const
{
  return std::make_unique<ErodeDilateCoordinationNumberFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateCoordinationNumberFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCoordinationNumberValue = filterArgs.value<int32>(k_CoordinationNumber_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  if(pCoordinationNumberValue < 0 || pCoordinationNumberValue > 6)
  {
    return {MakeErrorResult<OutputActions>(
        k_InvalidCoordinationNumberError,
        fmt::format("Coordination Number ({}) must be on the interval [0,6]. A cell has six face neighbors, so no other threshold can be met.", pCoordinationNumberValue))};
  }

  if(pCoordinationNumberValue < k_OscillatingLoopCoordinationNumber && pLoopValue)
  {
    return {MakeErrorResult<OutputActions>(
        k_NonTerminatingLoopError,
        fmt::format("Coordination Number ({}) with Loop enabled can never terminate on any input; use a value of at least {} or disable Loop. The loop stops only once no cell has a coordination "
                    "number of at least {}, and every cell of every volume has a coordination number of at least {}, so the stopping condition can never be reached.",
                    pCoordinationNumberValue, k_MinimumLoopCoordinationNumber, pCoordinationNumberValue, pCoordinationNumberValue))};
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pCoordinationNumberValue == k_OscillatingLoopCoordinationNumber && pLoopValue)
  {
    resultOutputActions.warnings().push_back(
        Warning{k_OscillatingLoopWarning,
                fmt::format("Coordination Number ({}) with Loop enabled is not guaranteed to terminate - termination is data-dependent: at this threshold the stopping condition is met only by cells "
                            "on a good/bad boundary, so a boundary-free volume completes after a single no-op sweep, while a volume containing a boundary may oscillate indefinitely because each "
                            "sweep converts boundary cells in both directions and recreates the boundary. The run can be cancelled; use a Coordination Number of at least {}, or disable Loop, to "
                            "guarantee termination.",
                            pCoordinationNumberValue, k_MinimumLoopCoordinationNumber)});
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pFeatureIdsArrayPathValue.getParent(), pIgnoredDataArrayPathsValue);

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumberFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ErodeDilateCoordinationNumberInputValues inputValues;

  inputValues.CoordinationNumber = filterArgs.value<int32>(k_CoordinationNumber_Key);
  inputValues.Loop = filterArgs.value<bool>(k_Loop_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ErodeDilateCoordinationNumber(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CoordinationNumberKey = "CoordinationNumber";
constexpr StringLiteral k_LoopKey = "Loop";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> ErodeDilateCoordinationNumberFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateCoordinationNumberFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_CoordinationNumberKey, k_CoordinationNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_LoopKey, k_Loop_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
