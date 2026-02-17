#include "RequireMinimumSizeFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <algorithm>
#include <vector>

namespace nx::core
{

using FeatureIdsArrayType = Int32Array;
using NumCellsArrayType = Int32Array;
using PhasesArrayType = Int32Array;

namespace
{
constexpr int32 k_BadMinAllowedFeatureSize = -5555;
constexpr int32 k_ParentlessPathError = -5557;

} // namespace

std::string RequireMinimumSizeFeaturesFilter::name() const
{
  return FilterTraits<RequireMinimumSizeFeaturesFilter>::name;
}

std::string RequireMinimumSizeFeaturesFilter::className() const
{
  return FilterTraits<RequireMinimumSizeFeaturesFilter>::className;
}

Uuid RequireMinimumSizeFeaturesFilter::uuid() const
{
  return FilterTraits<RequireMinimumSizeFeaturesFilter>::uuid;
}

std::string RequireMinimumSizeFeaturesFilter::humanName() const
{
  return "Remove Minimum Size Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> RequireMinimumSizeFeaturesFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "MinSize", "Feature Removal"};
}

Parameters RequireMinimumSizeFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<NumberParameter<int64>>(k_MinAllowedFeaturesSize_Key, "Minimum Allowed Features Size", "Minimum allowed features size", 0));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplySinglePhase_Key, "Apply to Single Phase", "Apply to Single Phase", false));
  params.insert(std::make_unique<NumberParameter<int32>>(k_SinglePhaseNumber_Key, "Phase Index", "Target phase to remove", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Input Image Geometry", "The input image geometry (cell)", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "DataPath to FeatureIds DataArray", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureNumCellsPath_Key, "Feature Num. Cells Array", "DataPath to NumCells DataArray", DataPath({"NumElements"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases", "DataPath to Feature Phases DataArray", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Link the checkbox to the other parameters
  params.linkParameters(k_ApplySinglePhase_Key, k_SinglePhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplySinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RequireMinimumSizeFeaturesFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer RequireMinimumSizeFeaturesFilter::clone() const
{
  return std::make_unique<RequireMinimumSizeFeaturesFilter>();
}

IFilter::PreflightResult RequireMinimumSizeFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto featurePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  auto featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto featureNumCellsPath = filterArgs.value<DataPath>(k_FeatureNumCellsPath_Key);
  auto applyToSinglePhase = filterArgs.value<bool>(k_ApplySinglePhase_Key);
  auto minAllowedFeatureSize = filterArgs.value<int64>(k_MinAllowedFeaturesSize_Key);

  std::vector<DataPath> dataArrayPaths;

  if(minAllowedFeatureSize < 0)
  {
    std::string ss = fmt::format("The minimum Feature size (%1) must be 0 or positive", minAllowedFeatureSize);
    return {MakeErrorResult<OutputActions>(-k_BadMinAllowedFeatureSize, ss)};
  }

  dataArrayPaths.push_back(featureNumCellsPath);

  if(applyToSinglePhase)
  {
    const auto* featurePhasesPtr = dataStructure.getDataAs<PhasesArrayType>(featurePhasesPath);
    if(featurePhasesPtr != nullptr)
    {
      dataArrayPaths.push_back(featurePhasesPath);
    }
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(-2071, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  DataPath featureGroupDataPath = featureNumCellsPath.getParent();
  const auto* featureDataGroup = dataStructure.getDataAs<BaseGroup>(featureGroupDataPath);
  if(nullptr == featureDataGroup)
  {
    return {MakeErrorResult<OutputActions>(k_ParentlessPathError, "The provided NumCells DataPath does not have a parent.")};
  }

  // Create the preflightResult object
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::string featureModificationWarning = "By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature "
                                           "level data should be rerun to ensure accurate final results from your pipeline.";
  preflightUpdatedValues.emplace_back(PreflightValue{"Feature Data Modification Warning", featureModificationWarning});

  // This section will warn the user about the removal of NeighborLists
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, featureNumCellsPath, resultOutputActions);
  if(result.outputActions.invalid())
  {
    return result;
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, featureIdsPath.getParent(), {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
Result<> RequireMinimumSizeFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RequireMinimumSizeFeaturesInputValues inputValues;

  inputValues.MinAllowedFeaturesSize = filterArgs.value<Int64Parameter::ValueType>(k_MinAllowedFeaturesSize_Key);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsPath_Key);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_ImageGeomPath_Key);
  inputValues.FeatureNumCellsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureNumCellsPath_Key);
  inputValues.FeaturePhasesPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeaturePhasesPath_Key);
  inputValues.ApplySinglePhase = filterArgs.value<BoolParameter::ValueType>(k_ApplySinglePhase_Key);
  inputValues.PhaseNumber = filterArgs.value<Int32Parameter::ValueType>(k_SinglePhaseNumber_Key);

  return RequireMinimumSizeFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinAllowedFeatureSizeKey = "MinAllowedFeatureSize";
constexpr StringLiteral k_ApplyToSinglePhaseKey = "ApplyToSinglePhase";
constexpr StringLiteral k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_NumCellsArrayPathKey = "NumCellsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> RequireMinimumSizeFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RequireMinimumSizeFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int64>>(args, json, SIMPL::k_MinAllowedFeatureSizeKey, k_MinAllowedFeaturesSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ApplyToSinglePhaseKey, k_ApplySinglePhase_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_PhaseNumberKey, k_SinglePhaseNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NumCellsArrayPathKey, k_FeatureNumCellsPath_Key));
  // Ignored Array Paths parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
