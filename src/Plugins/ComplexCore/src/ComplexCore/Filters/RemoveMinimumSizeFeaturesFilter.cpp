#include "RemoveMinimumSizeFeaturesFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <algorithm>
#include <vector>

namespace complex
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

std::string RemoveMinimumSizeFeaturesFilter::name() const
{
  return FilterTraits<RemoveMinimumSizeFeaturesFilter>::name;
}

std::string RemoveMinimumSizeFeaturesFilter::className() const
{
  return FilterTraits<RemoveMinimumSizeFeaturesFilter>::className;
}

Uuid RemoveMinimumSizeFeaturesFilter::uuid() const
{
  return FilterTraits<RemoveMinimumSizeFeaturesFilter>::uuid;
}

std::string RemoveMinimumSizeFeaturesFilter::humanName() const
{
  return "Remove Minimum Size Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveMinimumSizeFeaturesFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "MinSize", "Feature Removal"};
}

Parameters RemoveMinimumSizeFeaturesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<NumberParameter<int64>>(k_MinAllowedFeaturesSize_Key, "Minimum Allowed Features Size", "Minimum allowed features size", 0));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplySinglePhase_Key, "Apply to Single Phase", "Apply to Single Phase", false));
  params.insert(std::make_unique<NumberParameter<int64>>(k_PhaseNumber_Key, "Phase Index", "Target phase to remove", 0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Input Image Geometry", "The input image geometry (cell)", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "FeatureIds Array", "DataPath to FeatureIds DataArray", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumCellsPath_Key, "Num Cells Array", "DataPath to NumCells DataArray", DataPath({"NumElements"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Phases Array", "DataPath to Feature Phases DataArray", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Link the checkbox to the other parameters
  params.linkParameters(k_ApplySinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplySinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));

  return params;
}

IFilter::UniquePointer RemoveMinimumSizeFeaturesFilter::clone() const
{
  return std::make_unique<RemoveMinimumSizeFeaturesFilter>();
}

IFilter::PreflightResult RemoveMinimumSizeFeaturesFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler,
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

  const FeatureIdsArrayType* featureIdsPtr = data.getDataAs<FeatureIdsArrayType>(featureIdsPath);
  if(featureIdsPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumCellsPath, "FeatureIds not provided as an Int32 Array."}})};
  }
  const NumCellsArrayType* numCellsPtr = data.getDataAs<NumCellsArrayType>(numCellsPath);
  if(numCellsPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumCellsPath, "Num Cells not provided as an Int32 Array."}})};
  }
  dataArrayPaths.push_back(numCellsPath);

  if(applyToSinglePhase)
  {
    const PhasesArrayType* featurePhasesPtr = data.getDataAs<PhasesArrayType>(featurePhasesPath);
    const PhasesArrayType::store_type* featurePhases = nullptr;
    if(featurePhasesPtr != nullptr)
    {
      dataArrayPaths.push_back(featurePhasesPath);
      featurePhases = featurePhasesPtr->getDataStore();
    }
  }

  data.validateNumberOfTuples(dataArrayPaths);

  DataPath featureGroupDataPath = numCellsPath.getParent();
  const BaseGroup* featureDataGroup = data.getDataAs<BaseGroup>(featureGroupDataPath);
  if(nullptr == featureDataGroup)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_ParentlessPathError, "The provided NumCells DataPath does not have a parent."}})};
  }

  // Create the preflightResult object
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Throw a warning to inform the user that the neighbor list arrays could be deleted by this filter
  std::string ss = fmt::format("If this filter modifies the Cell Level Array '{}', all arrays of type NeighborList will be deleted from the feature data group '{}'.  These arrays are:\n",
                               featureIdsPath.toString(), featureGroupDataPath.toString());

  auto result = complex::GetAllChildDataPaths(data, featureGroupDataPath, DataObject::Type::NeighborList);
  if(!result.has_value())
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_FetchChildArrayError, fmt::format("Errors were encountered trying to retrieve the neighbor list children of group '{}'", featureGroupDataPath.toString())}})};
  }
  std::vector<DataPath> featureNeighborListArrays = result.value();
  for(const auto& featureNeighborList : featureNeighborListArrays)
  {
    ss.append("  " + featureNeighborList.toString() + "\n");
    auto action = std::make_unique<DeleteDataAction>(featureNeighborList);
    resultOutputActions.value().actions.emplace_back(std::move(action));
  }

  // Inform users that the following arrays are going to be modified in place
  // Feature Data is going to be modified
  complex::AppendDataModifiedActions(data, resultOutputActions.value().modifiedActions, featureGroupDataPath, {});

  resultOutputActions.warnings().push_back(Warning{k_NeighborListRemoval, ss});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
Result<> RemoveMinimumSizeFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
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
  messageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, message});

  complex::RemoveInactiveObjects(dataStructure, cellFeatureGroupPath, activeObjects, featureIdsArrayRef, currentFeatureCount);

  return {};
}
} // namespace complex
