#include "RequireMinimumSizeFeaturesFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <algorithm>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <vector>

namespace nx::core
{

using FeatureIdsArrayType = Int32Array;
using NumCellsArrayType = Int32Array;
using PhasesArrayType = Int32Array;

namespace
{
constexpr int32 k_BadMinAllowedFeatureSize = -5555;
constexpr int32 k_BadNumCellsPath = -5556;
constexpr int32 k_ParentlessPathError = -5557;
constexpr int32 k_NeighborListRemoval = -5558;
constexpr int32 k_FetchChildArrayError = -5559;

void assign_badpoints(DataStructure& dataStructure, const DataPath& featureIdsPath, SizeVec3 dimensions, const NumCellsArrayType& numCellsArrayRef)
{
  FeatureIdsArrayType* featureIdsPtr = dataStructure.getDataAs<FeatureIdsArrayType>(featureIdsPath);

  usize totalPoints = featureIdsPtr->getNumberOfTuples();
  FeatureIdsArrayType::store_type* featureIds = featureIdsPtr->getDataStore();

  std::array<int64_t, 3> dims = {
      static_cast<int64>(dimensions[0]),
      static_cast<int64>(dimensions[1]),
      static_cast<int64>(dimensions[2]),
  };

  std::vector<int32_t> neighbors(totalPoints * featureIdsPtr->getNumberOfComponents(), -1);

  int32 good = 1;
  int32 current = 0;
  int32 most = 0;
  int64 neighpoint = 0;

  std::array<int64_t, 6> neighpoints = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};

  usize counter = 1;
  int64 count = 0;
  int64 kstride = 0;
  int64 jstride = 0;
  int32 featurename = 0;
  int32 feature = 0;
  int32 neighbor = 0;
  std::vector<int32> n(numCellsArrayRef.getNumberOfTuples(), 0);

  while(counter != 0)
  {
    counter = 0;
    for(int64 k = 0; k < dims[2]; k++)
    {
      kstride = dims[0] * dims[1] * k;
      for(int64 j = 0; j < dims[1]; j++)
      {
        jstride = dims[0] * j;
        for(int64 i = 0; i < dims[0]; i++)
        {
          count = kstride + jstride + i;
          featurename = featureIds->getValue(count);
          if(featurename < 0)
          {
            counter++;
            most = 0;
            for(size_t l = 0; l < neighpoints.size(); l++)
            {
              good = 1;
              neighpoint = count + neighpoints[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = featureIds->getValue(neighpoint);
                if(feature >= 0)
                {
                  n[feature]++;
                  current = n[feature];
                  if(current > most)
                  {
                    most = current;
                    neighbors[count] = neighpoint;
                  }
                }
              }
            }
            for(int32 l = 0; l < neighpoints.size(); l++)
            {
              good = 1;
              neighpoint = count + neighpoints[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = featureIds->getValue(neighpoint);
                if(feature >= 0)
                {
                  n[feature] = 0;
                }
              }
            }
          }
        }
      }
    }
    DataPath attrMatPath = featureIdsPath.getParent();
    BaseGroup* parentGroup = dataStructure.getDataAs<BaseGroup>(attrMatPath);
    std::vector<std::string> voxelArrayNames;
    for(const auto& [identifier, sharedChild] : *parentGroup)
    {
      if(std::dynamic_pointer_cast<IDataArray>(sharedChild))
      {
        voxelArrayNames.push_back(sharedChild->getName());
      }
    }

    // TODO: This loop could be parallelized. Look at NeighborOrientationCorrelation filter
    for(size_t j = 0; j < totalPoints; j++)
    {
      featurename = featureIds->getValue(j);
      neighbor = neighbors[j];
      if(neighbor >= 0)
      {
        if(featurename < 0 && featureIds->getValue(neighbor) >= 0)
        {
          for(auto& voxelArrayName : voxelArrayNames)
          {
            auto arrayPath = attrMatPath.createChildPath(voxelArrayName);
            IDataArray* arr = dataStructure.getDataAs<IDataArray>(arrayPath);
            arr->copyTuple(neighbor, j);
          }
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
std::vector<bool> remove_smallfeatures(FeatureIdsArrayType& featureIdsArrayRef, const NumCellsArrayType& numCellsArrayRef, const PhasesArrayType* featurePhaseArrayPtr, int32_t phaseNumber,
                                       bool applyToSinglePhase, int64 minAllowedFeatureSize, Error& errorReturn)
{
  size_t totalPoints = featureIdsArrayRef.getNumberOfTuples();
  FeatureIdsArrayType::store_type& featureIdsStoreRef = featureIdsArrayRef.getDataStoreRef();

  bool good = false;
  int32 gnum;

  size_t totalFeatures = numCellsArrayRef.getNumberOfTuples();
  const NumCellsArrayType::store_type& numCells = numCellsArrayRef.getDataStoreRef();

  const PhasesArrayType::store_type* featurePhases = nullptr;
  if(applyToSinglePhase)
  {
    featurePhases = featurePhaseArrayPtr->getDataStore();
  }

  std::vector<bool> activeObjects(totalFeatures, true);

  for(size_t i = 1; i < totalFeatures; i++)
  {
    if(!applyToSinglePhase)
    {
      if(numCells.getValue(i) >= minAllowedFeatureSize)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
    else
    {
      if(numCells.getValue(i) >= minAllowedFeatureSize || featurePhases->getValue(i) != phaseNumber)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  if(!good)
  {
    errorReturn = Error{-1, "The minimum size is larger than the largest Feature.  All Features would be removed"};
    return activeObjects;
  }
  for(size_t i = 0; i < totalPoints; i++)
  {
    gnum = featureIdsStoreRef.getValue(i);
    if(!activeObjects[gnum])
    {
      featureIdsStoreRef.setValue(i, -1);
    }
  }
  return activeObjects;
}
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
  params.insert(std::make_unique<NumberParameter<int64>>(k_PhaseNumber_Key, "Phase Index", "Target phase to remove", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Input Image Geometry", "The input image geometry (cell)", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "DataPath to FeatureIds DataArray", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumCellsPath_Key, "Feature Num. Cells Array", "DataPath to NumCells DataArray", DataPath({"NumElements"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases", "DataPath to Feature Phases DataArray", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Link the checkbox to the other parameters
  params.linkParameters(k_ApplySinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplySinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));

  return params;
}

IFilter::UniquePointer RequireMinimumSizeFeaturesFilter::clone() const
{
  return std::make_unique<RequireMinimumSizeFeaturesFilter>();
}

IFilter::PreflightResult RequireMinimumSizeFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel) const
{
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhasesPath_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeomPath_Key);
  auto numCellsPath = args.value<DataPath>(k_NumCellsPath_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplySinglePhase_Key);
  auto minAllowedFeatureSize = args.value<int64>(k_MinAllowedFeaturesSize_Key);

  std::vector<DataPath> dataArrayPaths;

  if(minAllowedFeatureSize < 0)
  {
    std::string ss = fmt::format("The minimum Feature size (%1) must be 0 or positive", minAllowedFeatureSize);
    return {nonstd::make_unexpected(std::vector<Error>{Error{-k_BadMinAllowedFeatureSize, ss}})};
  }

  const FeatureIdsArrayType* featureIdsPtr = dataStructure.getDataAs<FeatureIdsArrayType>(featureIdsPath);
  if(featureIdsPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumCellsPath, "FeatureIds not provided as an Int32 Array."}})};
  }
  const NumCellsArrayType* numCellsPtr = dataStructure.getDataAs<NumCellsArrayType>(numCellsPath);
  if(numCellsPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumCellsPath, "Num Cells not provided as an Int32 Array."}})};
  }
  dataArrayPaths.push_back(numCellsPath);

  if(applyToSinglePhase)
  {
    const PhasesArrayType* featurePhasesPtr = dataStructure.getDataAs<PhasesArrayType>(featurePhasesPath);
    if(featurePhasesPtr != nullptr)
    {
      dataArrayPaths.push_back(featurePhasesPath);
    }
  }

  dataStructure.validateNumberOfTuples(dataArrayPaths);

  DataPath featureGroupDataPath = numCellsPath.getParent();
  const BaseGroup* featureDataGroup = dataStructure.getDataAs<BaseGroup>(featureGroupDataPath);
  if(nullptr == featureDataGroup)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_ParentlessPathError, "The provided NumCells DataPath does not have a parent."}})};
  }

  // Create the preflightResult object
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // This section will warn the user about the removal of NeighborLists
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, numCellsPath, resultOutputActions);
  if(result.outputActions.invalid())
  {
    return result;
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
Result<> RequireMinimumSizeFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhasesPath_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeomPath_Key);
  auto numCellsPath = args.value<DataPath>(k_NumCellsPath_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplySinglePhase_Key);
  auto minAllowedFeatureSize = args.value<int64>(k_MinAllowedFeaturesSize_Key);
  auto phaseNumber = args.value<int64>(k_PhaseNumber_Key);

  PhasesArrayType* featurePhasesArray = applyToSinglePhase ? dataStructure.getDataAs<PhasesArrayType>(featurePhasesPath) : nullptr;

  FeatureIdsArrayType& featureIdsArrayRef = dataStructure.getDataRefAs<FeatureIdsArrayType>(featureIdsPath);
  FeatureIdsArrayType::store_type& featureIdsStoreRef = featureIdsArrayRef.getDataStoreRef();

  NumCellsArrayType& numCellsArrayRef = dataStructure.getDataRefAs<NumCellsArrayType>(numCellsPath);
  NumCellsArrayType::store_type& numCellsStoreRef = numCellsArrayRef.getDataStoreRef();

  if(applyToSinglePhase)
  {
    usize numFeatures = featurePhasesArray->getNumberOfTuples();
    auto featurePhases = featurePhasesArray->getDataStore();

    bool unavailablePhase = true;
    for(size_t i = 0; i < numFeatures; i++)
    {
      if(featurePhases->getValue(i) == phaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      std::string ss = fmt::format("The phase number {} is not available in the supplied Feature phases array with path {}", phaseNumber, featurePhasesPath.toString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{-5555, ss}})};
    }
  }

  Error errorReturn;
  std::vector<bool> activeObjects = remove_smallfeatures(featureIdsArrayRef, numCellsArrayRef, featurePhasesArray, phaseNumber, applyToSinglePhase, minAllowedFeatureSize, errorReturn);
  if(errorReturn.code < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{errorReturn})};
  }

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  assign_badpoints(dataStructure, featureIdsPath, imageGeom.getDimensions(), numCellsArrayRef);

  DataPath cellFeatureGroupPath = numCellsPath.getParent();
  size_t currentFeatureCount = numCellsStoreRef.getNumberOfTuples();

  int32 count = 0;
  for(const auto& value : activeObjects)
  {
    if(value)
    {
      count++;
    }
  }
  std::string message = fmt::format("Feature Count Changed: Previous: {} New: {}", currentFeatureCount, count);
  messageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});

  nx::core::RemoveInactiveObjects(dataStructure, cellFeatureGroupPath, activeObjects, featureIdsArrayRef, currentFeatureCount, messageHandler, shouldCancel);

  return {};
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
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int64>>(args, json, SIMPL::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NumCellsArrayPathKey, k_NumCellsPath_Key));
  // Ignored Array Paths parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
