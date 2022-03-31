#include "MinNeighbors.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{
namespace
{
constexpr int64 k_TupleCountInvalidError = -250;
constexpr int64 k_MissingFeaturePhasesError = -251;

void assignBadPoints(DataStructure& data, const Arguments& args)
{
  auto imageGeomPath = args.value<DataPath>(MinNeighbors::k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(MinNeighbors::k_FeatureIds_Key);
  auto featurePhasesPath = args.value<DataPath>(MinNeighbors::k_FeaturePhases_Key);
  auto numNeighborsPath = args.value<DataPath>(MinNeighbors::k_NumNeighbors_Key);
  auto voxelArrayPaths = args.value<std::vector<DataPath>>(MinNeighbors::k_VoxelArrays_Key);

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& featurePhasesArray = data.getDataRefAs<Int32Array>(featurePhasesPath);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);

  auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& featurePhases = featurePhasesArray.getDataStoreRef();

  std::vector<std::reference_wrapper<IDataArray>> voxelArrays;
  for(const auto& arrayPath : voxelArrayPaths)
  {
    voxelArrays.push_back(data.getDataRefAs<IDataArray>(arrayPath));
  }

  usize totalPoints = featureIdsArray.getNumberOfTuples();
  SizeVec3 udims = data.getDataRefAs<ImageGeom>(imageGeomPath).getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  Int32DataStore neighbors(IDataStore::ShapeType{totalPoints}, IDataStore::ShapeType{1});
  neighbors.fill(-1);

  int32 good = 1;
  int32 current = 0;
  int32 most = 0;
  int64 neighpoint = 0;
  usize numfeatures = numNeighborsArray.getNumberOfTuples();

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
  std::vector<int32> n(numfeatures + 1, 0);
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
          featurename = featureIds[count];
          if(featurename < 0)
          {
            counter++;
            current = 0;
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
                feature = featureIds[neighpoint];
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
                feature = featureIds[neighpoint];
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
    for(usize j = 0; j < totalPoints; j++)
    {
      featurename = featureIds[j];
      neighbor = neighbors[j];
      if(featurename < 0 && neighbor >= 0 && featureIds[neighbor] >= 0)
      {
        for(const auto& voxelArray : voxelArrays)
        {
          voxelArray.get().copyTuple(neighbor, j);
        }
      }
    }
  }
}

nonstd::expected<std::vector<bool>, Error> mergeContainedFeatures(DataStructure& data, const Arguments& args)
{
  auto imageGeomPath = args.value<DataPath>(MinNeighbors::k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(MinNeighbors::k_FeatureIds_Key);
  auto featurePhasesPath = args.value<DataPath>(MinNeighbors::k_FeaturePhases_Key);
  auto numNeighborsPath = args.value<DataPath>(MinNeighbors::k_NumNeighbors_Key);
  auto minNumNeighbors = args.value<uint64>(MinNeighbors::k_MinNumNeighbors_Key);

  auto applyToSinglePhase = args.value<bool>(MinNeighbors::k_ApplyToSinglePhase_Key);
  auto phaseNumber = args.value<uint64>(MinNeighbors::k_PhaseNumber_Key);

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& featurePhasesArray = data.getDataRefAs<Int32Array>(featurePhasesPath);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);

  auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& featurePhases = featurePhasesArray.getDataStoreRef();
  auto& numNeighbors = numNeighborsArray.getDataStoreRef();

  bool good = false;

  usize totalPoints = data.getDataRefAs<ImageGeom>(imageGeomPath).getNumberOfElements();
  usize totalFeatures = numNeighborsArray.getNumberOfTuples();

  std::vector<bool> activeObjects(totalFeatures, true);

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(!applyToSinglePhase)
    {
      if(numNeighbors[i] >= minNumNeighbors)
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
      if(numNeighbors[i] >= minNumNeighbors || featurePhases[i] != phaseNumber)
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
    return nonstd::make_unexpected<Error>({-1, "The minimum number of neighbors is larger than the Feature with the most neighbors.  All Features would be removed"});
  }
  for(usize i = 0; i < totalPoints; i++)
  {
    int32 featureId = featureIds[i];
    if(!activeObjects[featureId])
    {
      featureIds[i] = -1;
    }
  }
  return activeObjects;
}
} // namespace

std::string MinNeighbors::name() const
{
  return FilterTraits<MinNeighbors>::name;
}

std::string MinNeighbors::className() const
{
  return FilterTraits<MinNeighbors>::className;
}

Uuid MinNeighbors::uuid() const
{
  return FilterTraits<MinNeighbors>::uuid;
}

std::string MinNeighbors::humanName() const
{
  return "Minimum Number of Neighbors";
}

Parameters MinNeighbors::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geom", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<UInt64Parameter>(k_MinNumNeighbors_Key, "Minimum Number Neighbors", "", 0));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSinglePhase_Key, "Apply to Single Phase Only", "", true));
  params.insert(std::make_unique<UInt64Parameter>(k_PhaseNumber_Key, "Phase Index", "", 0));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature IDs", "", DataPath{}, false, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhases_Key, "Feature Phases", "", DataPath{}, true, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumNeighbors_Key, "Number of Neighbors", "", DataPath{}, false, ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_VoxelArrays_Key, "Voxel Arrays", "", std::vector<DataPath>{}));

  params.linkParameters(k_ApplyToSinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplyToSinglePhase_Key, k_FeaturePhases_Key, std::make_any<bool>(false));
  return params;
}

IFilter::UniquePointer MinNeighbors::clone() const
{
  return std::make_unique<MinNeighbors>();
}

IFilter::PreflightResult MinNeighbors::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplyToSinglePhase_Key);
  auto phaseNumber = args.value<uint64>(k_PhaseNumber_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIds_Key);
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhases_Key);
  auto numNeighborsPath = args.value<DataPath>(k_NumNeighbors_Key);
  auto minNumNeighbors = args.value<uint64>(k_MinNumNeighbors_Key);

  std::vector<DataPath> dataArrayPaths;

  std::vector<usize> cDims = {1};
  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);

  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);
  dataArrayPaths.push_back(numNeighborsPath);

  if(applyToSinglePhase)
  {
    auto* featurePhasesArray = data.getDataAs<Int32Array>(featurePhasesPath);
    if(featurePhasesArray == nullptr)
    {
      std::string ss = fmt::format("Could not find Feature Phases array at path '{}'", featurePhasesPath.toString());
      return {MakeErrorResult<OutputActions>(k_MissingFeaturePhasesError, ss)};
    }

    dataArrayPaths.push_back(featurePhasesPath);
  }

  if(!data.validateNumberOfTuples(dataArrayPaths))
  {
    std::string ss = fmt::format("DataArrays do not have matching tuple count");
    return {MakeErrorResult<OutputActions>(k_TupleCountInvalidError, ss)};
  }

  OutputActions actions;
  return {std::move(actions)};
}

Result<> MinNeighbors::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhases_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplyToSinglePhase_Key);
  auto phaseNumber = args.value<uint64>(k_PhaseNumber_Key);

  // If running on a single phase, validate that the user has not entered a phase number
  // that is not in the system ; the filter would not crash otherwise, but the user should
  // be notified of unanticipated behavior ; this cannot be done in the dataCheck since
  // we don't have acces to the data yet
  if(applyToSinglePhase)
  {
    auto& featurePhasesArray = data.getDataRefAs<Int32Array>(featurePhasesPath);
    auto& featurePhases = featurePhasesArray.getDataStoreRef();

    usize numFeatures = featurePhasesArray.getNumberOfTuples();
    bool unavailablePhase = true;
    for(usize i = 0; i < numFeatures; i++)
    {
      if(featurePhases[i] == phaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      std::string ss = fmt::format("The phase number ({}) is not available in the supplied Feature phases array with path ({})", phaseNumber, featurePhasesPath.toString());
      return MakeErrorResult(-5555, ss);
    }
  }

  auto activeObjects = mergeContainedFeatures(data, args);
  if(!activeObjects.has_value())
  {
    return {nonstd::make_unexpected(std::vector<Error>{activeObjects.error()})};
  }
  assignBadPoints(data, args);

  return {};
}
} // namespace complex
