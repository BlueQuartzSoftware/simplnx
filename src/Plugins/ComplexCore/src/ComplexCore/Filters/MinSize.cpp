#include "MinSize.hpp"

#include <vector>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{
namespace
{
constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_Phiases_path";
constexpr StringLiteral k_NumCellsPath_Key = "num_cells_path";
constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
constexpr StringLiteral k_ImageGeomPath_Key = "image_geom_path";
constexpr StringLiteral k_ApplySinglePhase_Key = "apply_single_phase";
constexpr StringLiteral k_MinAllowedFeaturesSize_Key = "min_allowed_features_size";
constexpr StringLiteral k_PhaseNumber_Key = "phase_number";

constexpr int32 k_BadMinAllowedFeatureSize = -5555;

void assign_badpoints(DataArray<int64>* featureIdsPtr, SizeVec3 dimensions, const DataArray<int32>* numCellsPtr)
{
  size_t totalPoints = featureIdsPtr->getNumberOfTuples();
  auto featureIds = featureIdsPtr->getDataStore();

  int64 dims[3] = {
      static_cast<int64_t>(dimensions[0]),
      static_cast<int64_t>(dimensions[1]),
      static_cast<int64_t>(dimensions[2]),
  };

  auto neighborsPtr = std::make_shared<Int32Array>(totalPoints, std::string("_INTERNAL_USE_ONLY_Neighbors"), true);
  auto neighbors = neighborsPtr->getDataStore();
  neighbors->fill(-1);

  int32_t good = 1;
  int32_t current = 0;
  int32_t most = 0;
  int64_t neighpoint = 0;

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -dims[0] * dims[1];
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = dims[0] * dims[1];

  size_t counter = 1;
  int64_t count = 0;
  int64_t kstride = 0, jstride = 0;
  int32_t featurename = 0, feature = 0;
  int32_t neighbor = 0;
  std::vector<int32> n(numCellsPtr->getNumberOfTuples(), 0);
  while(counter != 0)
  {
    counter = 0;
    for(int64_t k = 0; k < dims[2]; k++)
    {
      kstride = dims[0] * dims[1] * k;
      for(int64_t j = 0; j < dims[1]; j++)
      {
        jstride = dims[0] * j;
        for(int64_t i = 0; i < dims[0]; i++)
        {
          count = kstride + jstride + i;
          featurename = featureIds->getValue(count);
          if(featurename < 0)
          {
            counter++;
            most = 0;
            for(int32_t l = 0; l < 6; l++)
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
            for(int32_t l = 0; l < 6; l++)
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
    std::string attrMatName = featureIdsArrayPath.getParentPath();
    std::vector<std::string> voxelArrayNames = m->getAttributeMatrix(attrMatName)->getAttributeArrayNames();
    for(const auto& dataArrayPath : ignoredDataArrayPaths)
    {
      voxelArrayNames.erase(std::remove(voxelArrayNames.begin(), voxelArrayNames.end(), dataArrayPath.getDataArrayName()));
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
            IDataArray::Pointer p = m->getAttributeMatrix(attrMatName)->getAttributeArray(voxelArrayName);
            p->copyTuple(neighbor, j);
          }
        }
      }
    }
  }
}

std::vector<bool> remove_smallfeatures(DataArray<int64>* featureIdsPtr, const DataArray<int32>* numCellsPtr, const DataArray<int32>* featurePhasesPtr, int32 phaseNumber, bool applyToSinglePhase,
                                       int64 minAllowedFeatureSize, Error& errorReturn = {})
{
  size_t totalPoints = featureIdsPtr->getNumberOfTuples();
  auto featureIds = featureIdsPtr->getDataStore();

  bool good = false;
  int32_t gnum;

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

std::string MinSize::name() const
{
  return FilterTraits<MinSize>::name;
}

std::string MinSize::className() const
{
  return FilterTraits<MinSize>::className;
}

Uuid MinSize::uuid() const
{
  return FilterTraits<MinSize>::uuid;
}

std::string MinSize::humanName() const
{
  return "Minimum Size";
}

Parameters MinSize::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_FeaturePhasesPath_Key, "Feature Phases Array", "DataPath to Feature Phases DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_NumCellsPath_Key, "NumCells Array", "DataPath to NumCells DataArray", 0));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_FeatureIdsPath_Key, "FeatureIds Array", "DataPath to FeatureIds DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "DataPath to Image Geometry", DataPath{}));

  params.insert(std::make_unique<BoolParameter>(k_ImageGeomPath_Key, "Apply to Single Phase", "Apply to Single Phase", true));
  params.insert(std::make_unique<NumberParameter<int64>>(k_MinAllowedFeaturesSize_Key, "Minimum Allowed Features Size", "Minimum allowed features size", 0));
  params.insert(std::make_unique<NumberParameter<int64>>(k_PhaseNumber_Key, "Phase Number", "Target Phase", 0));
  return params;
}

IFilter::UniquePointer MinSize::clone() const
{
  return std::make_unique<MinSize>();
}

IFilter::PreflightResult MinSize::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
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
  auto featureIdsPtr = data.getDataAs<DataArray<int32>>(featureIdsPath);
  auto featureIds = featureIdsPtr->getDataStore();

  auto numCellsPtr = data.getDataAs<DataArray<int32>>(numCellsPath);
  auto numCells = numCellsPtr->getDataStore();

  if(numCellsPtr == nullptr)
  {
    
  }
  dataArrayPaths.push_back(numCellsPath);

  if(applyToSinglePhase)
  {
    auto featurePhasesPtr = data.getDataAs<DataArray<int32>>(featurePhasesPath);
    if(featurePhasesPtr != nullptr)
    {
      dataArrayPaths.push_back(featurePhasesPath);
      featurePhases = featurePhasesPtr->getDataStore();
    }
  }

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);

  // Throw a warning to inform the user that the neighbor list arrays could be deleted by this filter
  std::string featureIdsPath = featureIdsPath.getDataContainerName() + "/" + featureIdsPath.getAttributeMatrixName() + "/" + getFeatureIdsPath.getDataArrayName();
  int err = -1;
  AttributeMatrix::Pointer featureAM = getDataContainerArray()->getPrereqAttributeMatrixFromPath(this, getNumCellsArrayPath(), err);
  if(nullptr == featureAM.get())
  {
    return;
  }

  std::string ss = fmt::format("If this filter modifies the Cell Level Array '{}', all arrays of type NeighborList will be deleted.  These arrays are:\n", featureIdsPath);
  std::vector<std::string> featureArrayNames = featureAM->getAttributeArrayNames();
  for(const auto& featureArrayName : featureArrayNames)
  {
    IDataArray::Pointer arr = featureAM->getAttributeArray(featureArrayName);
    std::string type = arr->getTypeAsString();
    if(type.compare("NeighborList<T>") == 0)
    {
      ss.append("\n" + numCellsArrayPath.getDataContainerName() + "/" + getNumCellsArrayPath().getAttributeMatrixName() + "/" + arr->getName());
    }
  }

  setWarningCondition(-5556, ss);

  auto action = std::make_unique<CreateArrayAction>(dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> MinSize::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
