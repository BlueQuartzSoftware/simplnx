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

namespace complex
{
namespace
{
constexpr int32 k_BadMinAllowedFeatureSize = -5555;
constexpr int32 k_BadNumCellsPath = -5556;
constexpr int32 k_ParentlessPathError = -5557;

void assign_badpoints(DataStructure& data, const DataPath& featureIdsPath, SizeVec3 dimensions, const Int32Array* numCellsPtr)
{
  auto featureIdsPtr = data.getDataAs<Int64Array>(featureIdsPath);

  usize totalPoints = featureIdsPtr->getNumberOfTuples();
  auto featureIds = featureIdsPtr->getDataStore();

  int64 dims[3] = {
      static_cast<int64>(dimensions[0]),
      static_cast<int64>(dimensions[1]),
      static_cast<int64>(dimensions[2]),
  };

  DataStructure tmpDs;
  auto tupleShape = std::vector<usize>{totalPoints};
  auto componentShape = std::vector<usize>{featureIdsPtr->getNumberOfComponents()};
  std::shared_ptr<Int32Array> neighborsPtr(Int32Array::CreateWithStore<DataStore<int32>>(tmpDs, std::string("_INTERNAL_USE_ONLY_Neighbors"), tupleShape, componentShape, {}));
  auto neighbors = neighborsPtr->getDataStore();
  neighbors->fill(-1);

  int32 good = 1;
  int32 current = 0;
  int32 most = 0;
  int64 neighpoint = 0;

  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -dims[0] * dims[1];
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = dims[0] * dims[1];

  usize counter = 1;
  int64 count = 0;
  int64 kstride = 0, jstride = 0;
  int32 featurename = 0, feature = 0;
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
            for(int32 l = 0; l < 6; l++)
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
                    neighbors->setValue(count, neighpoint);
                  }
                }
              }
            }
            for(int32 l = 0; l < 6; l++)
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
    auto parentGroup = data.getDataAs<BaseGroup>(attrMatPath);
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
      neighbor = neighbors->getValue(j);
      if(neighbor >= 0)
      {
        if(featurename < 0 && featureIds->getValue(neighbor) >= 0)
        {
          for(auto& voxelArrayName : voxelArrayNames)
          {
            auto arrayPath = attrMatPath.createChildPath(voxelArrayName);
            auto arr = data.getDataAs<IDataArray>(arrayPath);
            arr->copyTuple(neighbor, j);
          }
        }
      }
    }
  }
}

std::vector<bool> remove_smallfeatures(DataArray<int64>* featureIdsPtr, const DataArray<int32>* numCellsPtr, const DataArray<int32>* featurePhasesPtr, int32 phaseNumber, bool applyToSinglePhase,
                                       int64 minAllowedFeatureSize, Error& errorReturn)
{
  size_t totalPoints = featureIdsPtr->getNumberOfTuples();
  auto featureIds = featureIdsPtr->getDataStore();

  bool good = false;
  int32 gnum;

  size_t totalFeatures = numCellsPtr->getNumberOfTuples();
  auto numCells = numCellsPtr->getDataStore();

  auto featurePhases = featurePhasesPtr->getDataStore();

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
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases Array", "DataPath to Feature Phases DataArray", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumCellsPath_Key, "NumCells Array", "DataPath to NumCells DataArray", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "FeatureIds Array", "DataPath to FeatureIds DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "DataPath to Image Geometry", DataPath{}));

  params.insert(std::make_unique<BoolParameter>(k_ApplySinglePhase_Key, "Apply to Single Phase", "Apply to Single Phase", true));
  params.insert(std::make_unique<NumberParameter<int64>>(k_MinAllowedFeaturesSize_Key, "Minimum Allowed Features Size", "Minimum allowed features size", 0));
  params.insert(std::make_unique<NumberParameter<int64>>(k_PhaseNumber_Key, "Phase Number", "Target Phase", 0));
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
  auto featureIdsPtr = data.getDataAs<Int64Array>(featureIdsPath);
  auto featureIds = featureIdsPtr->getDataStore();

  auto numCellsPtr = data.getDataAs<Int32Array>(numCellsPath);
  auto numCells = numCellsPtr->getDataStore();

  if(numCellsPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumCellsPath, "Num Cells not provided as an Int32 Array."}})};
  }
  dataArrayPaths.push_back(numCellsPath);

  if(applyToSinglePhase)
  {
    auto featurePhasesPtr = data.getDataAs<Int32Array>(featurePhasesPath);
    const AbstractDataStore<int32>* featurePhases = nullptr;
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
  // std::vector<std::string> featureArrayNames = featureAM->getAttributeArrays();
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

Result<> RemoveMinimumSizeFeaturesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhasesPath_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeomPath_Key);
  auto numCellsPath = args.value<DataPath>(k_NumCellsPath_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplySinglePhase_Key);
  auto minAllowedFeatureSize = args.value<int64>(k_MinAllowedFeaturesSize_Key);
  auto phaseNumber = args.value<int64>(k_PhaseNumber_Key);

  auto featurePhasesArray = data.getDataAs<Int32Array>(featurePhasesPath);
  auto featurePhases = featurePhasesArray->getDataStore();

  auto featureIdsPtr = data.getDataAs<Int64Array>(featureIdsPath);
  auto featureIds = featureIdsPtr->getDataStore();

  auto numCellsPtr = data.getDataAs<Int32Array>(numCellsPath);
  auto numCells = numCellsPtr->getDataStore();

  if(applyToSinglePhase)
  {
    usize numFeatures = featurePhasesArray->getNumberOfTuples();
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

  auto imageGeom = data.getDataAs<ImageGeom>(imageGeomPath);
  assign_badpoints(data, featureIdsPath, imageGeom->getDimensions(), numCellsPtr);

  // auto cellFeatureGroupPath = numCellsPath.getParent();
  // auto cellFeatureAttrMat = data.getDataAs<BaseGroup>(cellFeatureGroupPath);
  // cellFeatureAttrMat->removeInactiveObjects(activeObjects, featureIdsArray);

  return {};
}
} // namespace complex
