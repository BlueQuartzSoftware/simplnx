#include "RemoveMinimumSizeFeaturesFilter.hpp"

#include <algorithm>
#include <set>
#include <vector>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"

namespace complex
{

using FeatureIdsArrayType = Int32Array;
using NumCellsArrayType = Int32Array;
using PhasesArrayType = Int32Array;
using NeighborsArrayType = Int32Array;

namespace
{
constexpr int32 k_BadMinAllowedFeatureSize = -5555;
constexpr int32 k_BadNumCellsPath = -5556;
constexpr int32 k_ParentlessPathError = -5557;

void assign_badpoints(DataStructure& dataStructure, const DataPath& featureIdsPath, SizeVec3 dimensions, const NumCellsArrayType* numCellsPtr)
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
  std::vector<int32> n(numCellsPtr->getNumberOfTuples(), 0);

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
    auto attrMatPath = featureIdsPath.getParent();
    auto parentGroup = dataStructure.getDataAs<BaseGroup>(attrMatPath);
    std::vector<std::string> voxelArrayNames;
    for(const auto& [id, sharedChild] : *parentGroup)
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
            auto arr = dataStructure.getDataAs<IDataArray>(arrayPath);
            arr->copyTuple(neighbor, j);
          }
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
std::vector<bool> remove_smallfeatures(FeatureIdsArrayType* featureIdsPtr, const NumCellsArrayType* numCellsPtr, const PhasesArrayType* featurePhasesPtr, int32_t phaseNumber, bool applyToSinglePhase,
                                       int64 minAllowedFeatureSize, Error& errorReturn)
{
  size_t totalPoints = featureIdsPtr->getNumberOfTuples();
  FeatureIdsArrayType::store_type* featureIds = featureIdsPtr->getDataStore();

  bool good = false;
  int32 gnum;

  size_t totalFeatures = numCellsPtr->getNumberOfTuples();
  const NumCellsArrayType::store_type* numCells = numCellsPtr->getDataStore();

  const PhasesArrayType::store_type* featurePhases = nullptr;
  if(applyToSinglePhase)
  {
    featurePhases = featurePhasesPtr->getDataStore();
  }

  std::vector<bool> activeObjects(totalFeatures, true);

  for(size_t i = 1; i < totalFeatures; i++)
  {
    if(!applyToSinglePhase)
    {
      if(numCells->getValue(i) >= minAllowedFeatureSize)
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
      if(numCells->getValue(i) >= minAllowedFeatureSize || featurePhases->getValue(i) != phaseNumber)
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
    gnum = featureIds->getValue(i);
    if(!activeObjects[gnum])
    {
      featureIds->setValue(i, -1);
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
  return {"#Processing", "#Cleanup", "#MinSize"};
}

Parameters RemoveMinimumSizeFeaturesFilter::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<BoolParameter>(k_ApplySinglePhase_Key, "Apply to Single Phase", "Apply to Single Phase", true));
  params.insert(std::make_unique<NumberParameter<int64>>(k_MinAllowedFeaturesSize_Key, "Minimum Allowed Features Size", "Minimum allowed features size", 0));
  params.insert(std::make_unique<NumberParameter<int64>>(k_PhaseNumber_Key, "Phase Number", "Target Phase", 0));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "[Feature] Phases Array", "DataPath to Feature Phases DataArray", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumCellsPath_Key, "[Feature] Num Cells Array", "DataPath to NumCells DataArray", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "[Cell] FeatureIds Array", "DataPath to FeatureIds DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "DataPath to Image Geometry", DataPath{}));

  return params;
}

IFilter::UniquePointer RemoveMinimumSizeFeaturesFilter::clone() const
{
  return std::make_unique<RemoveMinimumSizeFeaturesFilter>();
}

IFilter::PreflightResult RemoveMinimumSizeFeaturesFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
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

  auto imageGeom = data.getDataAs<ImageGeom>(imageGeomPath);

  std::vector<size_t> cDims(1, 1);
  const FeatureIdsArrayType* featureIdsPtr = data.getDataAs<FeatureIdsArrayType>(featureIdsPath);
  const FeatureIdsArrayType::store_type* featureIds = featureIdsPtr->getDataStore();

  const NumCellsArrayType* numCellsPtr = data.getDataAs<NumCellsArrayType>(numCellsPath);
  const NumCellsArrayType::store_type* numCells = numCellsPtr->getDataStore();

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

  // Throw a warning to inform the user that the neighbor list arrays could be deleted by this filter
  DataPath parentPath = numCellsPath.getParent();
  auto featureAM = data.getDataAs<BaseGroup>(parentPath);
  if(nullptr == featureAM)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_ParentlessPathError, "The provided NumCells DataPath does not have a parent."}})};
  }

  std::string ss = fmt::format("If this filter modifies the Cell Level Array '{}', all arrays of type NeighborList will be deleted.  These arrays are:\n", featureIdsPath.toString());
  for(auto& [id, sharedChild] : (*featureAM))
  {
    if(sharedChild->getTypeName() != "NeighborList<T>")
    {
      continue;
    }
    DataPath childPath = parentPath.createChildPath(sharedChild->getName());
    ss.append("\n" + childPath.toString());
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> RemoveMinimumSizeFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhasesPath_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeomPath_Key);
  auto numCellsPath = args.value<DataPath>(k_NumCellsPath_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplySinglePhase_Key);
  auto minAllowedFeatureSize = args.value<int64>(k_MinAllowedFeaturesSize_Key);
  auto phaseNumber = args.value<int64>(k_PhaseNumber_Key);

  PhasesArrayType* featurePhasesArray = nullptr;
  if(applyToSinglePhase)
  {
    featurePhasesArray = dataStructure.getDataAs<PhasesArrayType>(featurePhasesPath);
  }

  FeatureIdsArrayType* featureIdsPtr = dataStructure.getDataAs<FeatureIdsArrayType>(featureIdsPath);
  FeatureIdsArrayType::store_type* featureIds = featureIdsPtr->getDataStore();

  NumCellsArrayType* numCellsPtr = dataStructure.getDataAs<NumCellsArrayType>(numCellsPath);
  NumCellsArrayType::store_type* numCells = numCellsPtr->getDataStore();

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
  std::vector<bool> activeObjects = remove_smallfeatures(featureIdsPtr, numCellsPtr, featurePhasesArray, phaseNumber, applyToSinglePhase, minAllowedFeatureSize, errorReturn);
  if(errorReturn.code < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{errorReturn})};
  }

  auto imageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  assign_badpoints(dataStructure, featureIdsPath, imageGeom->getDimensions(), numCellsPtr);

  DataPath cellFeatureGroupPath = numCellsPath.getParent();
  size_t currentFeatureCount = numCells->getNumberOfTuples();
  complex::RemoveInactiveObjects(dataStructure, cellFeatureGroupPath, activeObjects, featureIdsPtr, currentFeatureCount);

  return {};
}
} // namespace complex
