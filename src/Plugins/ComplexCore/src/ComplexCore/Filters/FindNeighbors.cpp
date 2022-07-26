#include "FindNeighbors.hpp"

#include <sstream>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

namespace complex
{
std::string FindNeighbors::name() const
{
  return FilterTraits<FindNeighbors>::name;
}

std::string FindNeighbors::className() const
{
  return FilterTraits<FindNeighbors>::className;
}

Uuid FindNeighbors::uuid() const
{
  return FilterTraits<FindNeighbors>::uuid;
}

std::string FindNeighbors::humanName() const
{
  return "Find Feature Neighbors";
}

Parameters FindNeighbors::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Parameters"});

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreBoundary_Key, "Store Boundary Cells Array", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreSurface_Key, "Store Surface Features Array", "", false));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature Ids", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatures_Key, "Cell Feature AttributeMatrix", "", DataPath({"Data Container", "Feature Data"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_BoundaryCells_Key, "Boundary Cells", "", DataPath({"BoundaryCells"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumNeighbors_Key, "Number of Neighbors", "", DataPath({"NumNeighbors2"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborList_Key, "Neighbor List", "", DataPath({"NeighborList2"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedSurfaceArea_Key, "Shared Surface Area List", "", DataPath({"SharedSurfaceAreaList2"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceFeatures_Key, "Surface Features", "", DataPath({"SurfaceFeatures"})));

  params.linkParameters(k_StoreBoundary_Key, k_BoundaryCells_Key, std::make_any<bool>(true));
  params.linkParameters(k_StoreSurface_Key, k_SurfaceFeatures_Key, std::make_any<bool>(true));
  return params;
}

IFilter::UniquePointer FindNeighbors::clone() const
{
  return std::make_unique<FindNeighbors>();
}

IFilter::PreflightResult FindNeighbors::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto storeBoundaryCells = args.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = args.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIds_Key);
  auto boundaryCellsPath = args.value<DataPath>(k_BoundaryCells_Key);
  auto numNeighborsPath = args.value<DataPath>(k_NumNeighbors_Key);
  auto neighborListPath = args.value<DataPath>(k_NeighborList_Key);
  auto sharedSurfaceAreaPath = args.value<DataPath>(k_SharedSurfaceArea_Key);
  auto surfaceFeaturesPath = args.value<DataPath>(k_SurfaceFeatures_Key);
  auto featureAttrMatrixPath = args.value<DataPath>(k_CellFeatures_Key);

  OutputActions actions;

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto tupleCount = featureIdsArray.getNumberOfTuples();
  std::vector<usize> tupleShape{tupleCount};

  const std::vector<usize> cDims{1};

  if(storeBoundaryCells)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int8, tupleShape, cDims, boundaryCellsPath);
    actions.actions.push_back(std::move(action));
  }

  // Feature Data:
  // Validating the Feature Attribute Matrix and trying to find a child of the Group
  // that is an IDataArray subclass, so we can get the proper tuple shape
  const auto* featureAttrMatrix = data.getDataAs<DataGroup>(featureAttrMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12600, "Feature Attribute Matrix Path is NOT a DataGroup"}})};
  }
  const auto& featureAttrMatrixChildren = featureAttrMatrix->getDataMap();
  bool childDataArrayFound = false;
  for(const auto& child : featureAttrMatrixChildren)
  {
    if(child.second->getDataObjectType() == DataObject::Type::DataArray)
    {
      const auto* childDataArray = dynamic_cast<IDataArray*>(child.second.get());
      tupleShape = childDataArray->getIDataStore()->getTupleShape();
      tupleCount = childDataArray->getNumberOfTuples();
      childDataArrayFound = true;
      break;
    }
  }
  // We must find a child IDataArray subclass to get the tuple shape correct.
  if(!childDataArrayFound)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12601, "Feature Attribute Matrix does not have a child IDataArray"}})};
  }

  // Create the NumNeighbors Output Data Array
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, cDims, numNeighborsPath);
    actions.actions.push_back(std::move(action));
  }

  if(storeSurfaceFeatures)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::boolean, tupleShape, cDims, surfaceFeaturesPath);
    actions.actions.push_back(std::move(action));
  }

  // Do this whole block FIRST otherwise the side effect is that a call to m->getNumCellFeatureTuples will = 0
  // because we are just creating an empty NeighborList object.
  // Now we are going to get a "Pointer" to the NeighborList object out of the DataContainer
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::int32, tupleCount, neighborListPath);
    actions.actions.push_back(std::move(action));
  }

  // And we do the same for the SharedSurfaceArea list
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupleCount, sharedSurfaceAreaPath);
    actions.actions.push_back(std::move(action));
  }

  return {std::move(actions)};
}

Result<> FindNeighbors::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto storeBoundaryCells = args.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = args.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIds_Key);
  auto boundaryCellsPath = args.value<DataPath>(k_BoundaryCells_Key);
  auto numNeighborsPath = args.value<DataPath>(k_NumNeighbors_Key);
  auto neighborListPath = args.value<DataPath>(k_NeighborList_Key);
  auto sharedSurfaceAreaPath = args.value<DataPath>(k_SharedSurfaceArea_Key);
  auto surfaceFeaturesPath = args.value<DataPath>(k_SurfaceFeatures_Key);

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);
  auto& neighborList = data.getDataRefAs<Int32NeighborListType>(neighborListPath);
  auto& sharedSurfaceAreaList = data.getDataRefAs<FloatNeighborListType>(sharedSurfaceAreaPath);

  auto* boundaryCellsArray = data.getDataAs<Int32Array>(boundaryCellsPath);
  auto* surfaceFeaturesArray = data.getDataAs<Int32Array>(surfaceFeaturesPath);

  auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& numNeighbors = numNeighborsArray.getDataStoreRef();

  usize totalPoints = featureIdsArray.getNumberOfTuples();
  usize totalFeatures = numNeighborsArray.getNumberOfTuples();

  /* Ensure that we will be able to work with the user selected featureId Array */
  const auto [minFeatureId, maxFeatureId] = std::minmax_element(featureIdsArray.begin(), featureIdsArray.end());
  if(static_cast<usize>(*maxFeatureId) >= totalFeatures)
  {
    std::stringstream out;
    out << "Data Array " << featureIdsArray.getName() << " has a maximum value of " << *maxFeatureId << " which is greater than the "
        << " number of features from array " << numNeighborsArray.getName() << " which has " << totalFeatures << ". Did you select the "
        << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  auto& imageGeom = data.getDataRefAs<ImageGeom>(imageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();
  const auto imageGeomNumX = imageGeom.getNumXPoints();
  const auto imageGeomNumY = imageGeom.getNumYPoints();
  const auto imageGeomNumZ = imageGeom.getNumZPoints();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::array<int64, 6> neighPoints = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};

  int64 column = 0;
  int64 row = 0;
  int64 plane = 0;
  int32 feature = 0;
  int32 nnum = 0;
  uint8 onsurf = 0;
  bool good = false;
  int64 neighbor = 0;

  std::vector<std::vector<int32>> neighborlist(totalFeatures);
  std::vector<std::vector<float>> neighborsurfacearealist(totalFeatures);

  int32 nListSize = 100;

  float progInt = 0.0F;
  auto start = std::chrono::steady_clock::now();
  // Initialize the neighbor lists
  for(usize i = 1; i < totalFeatures; i++)
  {
    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      progInt = static_cast<float>(i) / static_cast<float>(totalFeatures) * 100.0f;
      std::string message = fmt::format("Initializing Neighbor Lists || {:2.0f}% Complete", progInt);
      messageHandler(complex::IFilter::ProgressMessage{complex::IFilter::Message::Type::Info, message, static_cast<int32_t>(progInt)});
      start = std::chrono::steady_clock::now();
    }

    if(shouldCancel)
    {
      return {};
    }

    numNeighbors[i] = 0;
    neighborlist[i].resize(nListSize);
    neighborsurfacearealist[i].assign(nListSize, -1.0f);
    if(storeSurfaceFeatures)
    {
      auto& surfaceFeatures = surfaceFeaturesArray->getDataStoreRef();
      surfaceFeatures[i] = 0;
    }
  }

  progInt = 0.0F;
  start = std::chrono::steady_clock::now();
  // Loop over all points to generate the neighbor lists
  for(usize j = 0; j < totalPoints; j++)
  {
    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      progInt = static_cast<float>(j) / static_cast<float>(totalPoints) * 100.0f;
      std::string message = fmt::format("Determining Neighbor Lists || {:2.0f}% Complete", progInt);
      messageHandler(complex::IFilter::ProgressMessage{complex::IFilter::Message::Type::Info, message, static_cast<int32_t>(progInt)});
      start = std::chrono::steady_clock::now();
    }

    if(shouldCancel)
    {
      return {};
    }

    onsurf = 0;
    feature = featureIds[j];
    if(feature > 0 && feature < neighborlist.size())
    {
      column = static_cast<int64>(j % imageGeomNumX);
      row = static_cast<int64>((j / imageGeomNumX) % imageGeomNumY);
      plane = static_cast<int64>(j / (imageGeomNumX * imageGeomNumY));
      if(storeSurfaceFeatures)
      {
        auto& surfaceFeatures = surfaceFeaturesArray->getDataStoreRef();

        if((column == 0 || column == static_cast<int64>((imageGeomNumX - 1)) || row == 0 || row == static_cast<int64>((imageGeomNumY)-1) || plane == 0 ||
            plane == static_cast<int64>((imageGeomNumZ - 1))) &&
           imageGeomNumZ != 1)
        {
          surfaceFeatures[feature] = 1;
        }
        if((column == 0 || column == static_cast<int64>((imageGeomNumX - 1)) || row == 0 || row == static_cast<int64>((imageGeomNumY - 1))) && imageGeomNumZ == 1)
        {
          surfaceFeatures[feature] = 1;
        }
      }
      for(size_t k = 0; k < 6; k++)
      {
        good = true;
        neighbor = static_cast<int64>(j + neighPoints[k]);
        if(k == 0 && plane == 0)
        {
          good = false;
        }
        if(k == 5 && plane == (imageGeomNumZ - 1))
        {
          good = false;
        }
        if(k == 1 && row == 0)
        {
          good = false;
        }
        if(k == 4 && row == (imageGeomNumY - 1))
        {
          good = false;
        }
        if(k == 2 && column == 0)
        {
          good = false;
        }
        if(k == 3 && column == (imageGeomNumX - 1))
        {
          good = false;
        }
        if(good && featureIds[neighbor] != feature && featureIds[neighbor] > 0)
        {
          onsurf++;
          nnum = numNeighbors[feature];
          neighborlist[feature].push_back(featureIds[neighbor]);
          nnum++;
          numNeighbors[feature] = nnum;
        }
      }
    }
    if(storeBoundaryCells)
    {
      auto& boundaryCells = boundaryCellsArray->getDataStoreRef();
      boundaryCells[j] = static_cast<int32>(onsurf);
    }
  }

  FloatVec3 spacing = imageGeom.getSpacing();

  progInt = 0;
  start = std::chrono::steady_clock::now();
  // We do this to create new set of NeighborList objects
  for(usize i = 1; i < totalFeatures; i++)
  {
    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      progInt = static_cast<float>(i) / static_cast<float>(totalFeatures) * 100.0f;
      std::string message = fmt::format("Calculating Surface Areas || {:2.0f}% Complete", progInt);
      messageHandler(complex::IFilter::ProgressMessage{complex::IFilter::Message::Type::Info, message, static_cast<int32>(progInt)});
      start = std::chrono::steady_clock::now();
    }
    if(shouldCancel)
    {
      return {};
    }

    std::map<int32, int32> neighToCount;
    auto numneighs = static_cast<int32>(neighborlist[i].size());

    // this increments the voxel counts for each feature
    for(int32 j = 0; j < numneighs; j++)
    {
      neighToCount[neighborlist[i][j]]++;
    }

    std::map<int32, int32>::iterator neighborIter = neighToCount.find(0);
    neighToCount.erase(neighborIter);
    neighborIter = neighToCount.find(-1);
    if(neighborIter != neighToCount.end())
    {
      neighToCount.erase(neighborIter);
    }
    // Resize the features neighbor list to zero
    neighborlist[i].resize(0);
    neighborsurfacearealist[i].resize(0);

    for(const auto [neigh, number] : neighToCount)
    {
      float area = static_cast<float>(number) * spacing[0] * spacing[1];

      // Push the neighbor feature id back onto the list, so we stay synced up
      neighborlist[i].push_back(neigh);
      neighborsurfacearealist[i].push_back(area);
    }
    numNeighbors[i] = static_cast<int32>(neighborlist[i].size());

    // Set the vector for each list into the NeighborList Object
    NeighborList<int32>::SharedVectorType sharedNeiLst(new std::vector<int32>);
    sharedNeiLst->assign(neighborlist[i].begin(), neighborlist[i].end());
    neighborList.setList(static_cast<int32>(i), sharedNeiLst);

    NeighborList<float32>::SharedVectorType sharedSAL(new std::vector<float32>);
    sharedSAL->assign(neighborsurfacearealist[i].begin(), neighborsurfacearealist[i].end());
    sharedSurfaceAreaList.setList(static_cast<int32>(i), sharedSAL);
  }

  return {};
}
} // namespace complex
