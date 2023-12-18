#include "MergeTwinsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/MergeTwins.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NeighborListSelectionParameter.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include "complex/Parameters/NumberParameter.hpp"

#include <random>

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MergeTwinsFilter::name() const
{
  return FilterTraits<MergeTwinsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string MergeTwinsFilter::className() const
{
  return FilterTraits<MergeTwinsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MergeTwinsFilter::uuid() const
{
  return FilterTraits<MergeTwinsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MergeTwinsFilter::humanName() const
{
  return "Merge Twins";
}

//------------------------------------------------------------------------------
std::vector<std::string> MergeTwinsFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Grouping"};
}

//------------------------------------------------------------------------------
Parameters MergeTwinsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Seeded Randomness"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "MergeTwins SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNonContiguousNeighbors_Key, "Use Non-Contiguous Neighbors",
                                                                 "Whether to use a list of non-contiguous or contiguous neighbors for each feature when merging", false));

  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)",
                                                   "Tolerance allowed when comparing the axis part of the axis-angle representation of the misorientation", 3.0F));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)",
                                                   "Tolerance allowed when comparing the angle part of the axis-angle representation of the misorientation", 2.0F));

  params.insertSeparator(Parameters::Separator{"Required Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_ContiguousNeighborListArrayPath_Key, "Contiguous Neighbor List", "List of contiguous neighbors for each Feature.",
                                                                 DataPath({"NeighborList2"}), NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_NonContiguousNeighborListArrayPath_Key, "Non-Contiguous Neighbor List", "List of non-contiguous neighbors for each Feature.",
                                                                 DataPath{}, NeighborListSelectionParameter::AllowedTypes{DataType::int32}));
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_ContiguousNeighborListArrayPath_Key, false);
  params.linkParameters(k_UseNonContiguousNeighbors_Key, k_NonContiguousNeighborListArrayPath_Key, true);

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "Specifies the average orientation of each Feature in quaternion representation",
                                                          DataPath({"Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));

  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Element Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellParentIdsArrayName_Key, "Parent Ids", "The name of the array specifying to which parent each cell belongs", "ParentIds"));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_NewCellFeatureAttributeMatrixName_Key, "Feature Attribute Matrix", "The name of the created cell feature attribute matrix", "NewGrain Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureParentIdsArrayName_Key, "Parent Ids", "The name of the array specifying to which parent each Feature belongs", "ParentIds"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_ActiveArrayName_Key, "Active",
      "The name of the array specifying if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). At the end of the Filter, all Features will be Active",
      "Active"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MergeTwinsFilter::clone() const
{
  return std::make_unique<MergeTwinsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MergeTwinsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pUseNonContiguousNeighborsValue = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  auto pNonContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  auto pContiguousNeighborListArrayPathValue = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  DataPath cellFeatureDataPath = pFeaturePhasesArrayPathValue.getParent();
  auto pCellParentIdsArrayNameValue = pFeatureIdsArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_CellParentIdsArrayName_Key));
  auto pNewCellFeatureAttributeMatrixNameValue = cellFeatureDataPath.getParent().createChildPath(filterArgs.value<std::string>(k_NewCellFeatureAttributeMatrixName_Key));
  auto pFeatureParentIdsArrayNameValue = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key));
  auto pActiveArrayNameValue = pNewCellFeatureAttributeMatrixNameValue.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<size_t> cDims(1, 1);
  const NeighborList<int32>* contiguousNeighborList = dataStructure.getDataAs<NeighborList<int32>>(pContiguousNeighborListArrayPathValue);
  if(contiguousNeighborList == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-6874600, fmt::format("Could not find contiguous neighbor list of type Int32 at path '{}' ", pContiguousNeighborListArrayPathValue.toString())), {}};
  }
  if(pUseNonContiguousNeighborsValue)
  {
    const NeighborList<int32>* nonContiguousNeighborList = dataStructure.getDataAs<NeighborList<int32>>(pNonContiguousNeighborListArrayPathValue);
    if(nonContiguousNeighborList == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-6874601, fmt::format("Could not find non contiguous neighbor list of type Int32 at path '{}' ", pNonContiguousNeighborListArrayPathValue.toString())),
              {}};
    }
  }

  std::vector<size_t> tDims(1, 0);
  auto newCellFeatureAction = std::make_unique<CreateAttributeMatrixAction>(pNewCellFeatureAttributeMatrixNameValue, tDims);
  resultOutputActions.value().appendAction(std::move(newCellFeatureAction));

  std::vector<DataPath> dataArrayPaths;

  // Cell Data
  const Int32Array* featureIds = dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue);
  if(nullptr == featureIds)
  {
    return {MakeErrorResult<OutputActions>(-6874602, fmt::format("Could not find feature ids array of type Int32 at path '{}' ", pFeatureIdsArrayPathValue.toString())), {}};
  }
  auto cellParentIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, featureIds->getIDataStore()->getTupleShape(), cDims, pCellParentIdsArrayNameValue);
  resultOutputActions.value().appendAction(std::move(cellParentIdsAction));

  // Feature Data
  const Int32Array* phases = dataStructure.getDataAs<Int32Array>(pFeaturePhasesArrayPathValue);
  if(nullptr == phases)
  {
    return {MakeErrorResult<OutputActions>(-6874603, fmt::format("Could not find phases array of type Int32 at path '{}' ", pFeaturePhasesArrayPathValue.toString())), {}};
  }
  dataArrayPaths.push_back(pFeaturePhasesArrayPathValue);

  auto featureParentIdsAction = std::make_unique<CreateArrayAction>(DataType::int32, phases->getIDataStore()->getTupleShape(), cDims, pFeatureParentIdsArrayNameValue);
  resultOutputActions.value().appendAction(std::move(featureParentIdsAction));

  cDims[0] = 4;
  const Float32Array* avgQuats = dataStructure.getDataAs<Float32Array>(pAvgQuatsArrayPathValue);
  if(nullptr == avgQuats)
  {
    return {MakeErrorResult<OutputActions>(-6874602, fmt::format("Could not find average quaternions array of type Float32 at path '{}' ", pAvgQuatsArrayPathValue.toString())), {}};
  }
  dataArrayPaths.push_back(pAvgQuatsArrayPathValue);

  // New Feature Data
  cDims[0] = 1;
  auto activeAction = std::make_unique<CreateArrayAction>(DataType::boolean, tDims, cDims, pActiveArrayNameValue);
  resultOutputActions.value().appendAction(std::move(activeAction));

  // Ensemble Data
  const UInt32Array* crystalStructures = dataStructure.getDataAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(nullptr == crystalStructures)
  {
    return {MakeErrorResult<OutputActions>(-6874602, fmt::format("Could not find crystal structures array of type UInt32 at path '{}' ", pCrystalStructuresArrayPathValue.toString())), {}};
  }

  dataStructure.validateNumberOfTuples(dataArrayPaths);

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MergeTwinsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  MergeTwinsInputValues inputValues;
  GroupFeaturesInputValues groupInputValues;

  groupInputValues.UseNonContiguousNeighbors = filterArgs.value<bool>(k_UseNonContiguousNeighbors_Key);
  groupInputValues.NonContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_NonContiguousNeighborListArrayPath_Key);
  groupInputValues.ContiguousNeighborListArrayPath = filterArgs.value<DataPath>(k_ContiguousNeighborListArrayPath_Key);
  inputValues.AxisTolerance = filterArgs.value<float32>(k_AxisTolerance_Key);
  inputValues.AngleTolerance = filterArgs.value<float32>(k_AngleTolerance_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  DataPath cellFeatureDataPath = inputValues.FeaturePhasesArrayPath.getParent();
  inputValues.CellParentIdsArrayName = inputValues.FeatureIdsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_CellParentIdsArrayName_Key));
  inputValues.NewCellFeatureAttributeMatrixName = cellFeatureDataPath.getParent().createChildPath(filterArgs.value<std::string>(k_NewCellFeatureAttributeMatrixName_Key));
  inputValues.FeatureParentIdsArrayName = cellFeatureDataPath.createChildPath(filterArgs.value<std::string>(k_FeatureParentIdsArrayName_Key));
  inputValues.ActiveArrayName = inputValues.NewCellFeatureAttributeMatrixName.createChildPath(filterArgs.value<std::string>(k_ActiveArrayName_Key));
  inputValues.Seed = seed;

  return MergeTwins(dataStructure, messageHandler, shouldCancel, &inputValues, &groupInputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_UseNonContiguousNeighborsKey = "UseNonContiguousNeighbors";
constexpr StringLiteral k_NonContiguousNeighborListArrayPathKey = "NonContiguousNeighborListArrayPath";
constexpr StringLiteral k_ContiguousNeighborListArrayPathKey = "ContiguousNeighborListArrayPath";
constexpr StringLiteral k_AxisToleranceKey = "AxisTolerance";
constexpr StringLiteral k_AngleToleranceKey = "AngleTolerance";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_CellParentIdsArrayNameKey = "CellParentIdsArrayName";
constexpr StringLiteral k_NewCellFeatureAttributeMatrixNameKey = "NewCellFeatureAttributeMatrixName";
constexpr StringLiteral k_FeatureParentIdsArrayNameKey = "FeatureParentIdsArrayName";
constexpr StringLiteral k_ActiveArrayNameKey = "ActiveArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> MergeTwinsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MergeTwinsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseNonContiguousNeighborsKey, k_UseNonContiguousNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NonContiguousNeighborListArrayPathKey,
                                                                                                                   k_NonContiguousNeighborListArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ContiguousNeighborListArrayPathKey, k_ContiguousNeighborListArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_AxisToleranceKey, k_AxisTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_AngleToleranceKey, k_AngleTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellParentIdsArrayNameKey, k_CellParentIdsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NewCellFeatureAttributeMatrixNameKey,
                                                                                                                   k_NewCellFeatureAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FeatureParentIdsArrayNameKey, k_FeatureParentIdsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ActiveArrayNameKey, k_ActiveArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace complex
