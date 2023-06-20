#include "FindNeighbors.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <sstream>

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNeighbors::name() const
{
  return FilterTraits<FindNeighbors>::name;
}

//------------------------------------------------------------------------------
std::string FindNeighbors::className() const
{
  return FilterTraits<FindNeighbors>::className;
}

//------------------------------------------------------------------------------
Uuid FindNeighbors::uuid() const
{
  return FilterTraits<FindNeighbors>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNeighbors::humanName() const
{
  return "Find Feature Neighbors";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNeighbors::defaultTags() const
{
  return {"Statistics", "Neighbors", "Features"};
}

//------------------------------------------------------------------------------
Parameters FindNeighbors::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreBoundary_Key, "Store Boundary Cells Array", "Whether to store the boundary Cells array", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreSurface_Key, "Store Surface Features Array", "Whether to store the surface Features array", false));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geometry", "The geometry in which to identify feature neighbors", DataPath({"DataContainer"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIds_Key, "Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatures_Key, "Cell Feature AttributeMatrix", "Feature Attribute Matrix of the selected Feature Ids",
                                                                    DataPath({"DataContainer", "CellFeatureData"})));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_BoundaryCells_Key, "Boundary Cells",
      "The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6. Only created if Store Boundary Cells Array is checked",
      "BoundaryCells"));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumNeighbors_Key, "Number of Neighbors", "Number of contiguous neighboring Features for a given Feature", "NumNeighbors2"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborList_Key, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", "NeighborList2"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SharedSurfaceArea_Key, "Shared Surface Area List",
                                                          "List of the shared surface area for each of the contiguous neighboring Features for a given Feature", "SharedSurfaceAreaList2"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceFeatures_Key, "Surface Features",
                                                          "The name of the surface features data array. Flag equal to 1 if the Feature touches an outer surface of the sample and equal to 0 if it "
                                                          "does not. Only created if Store Surface Features Array is checked",
                                                          "SurfaceFeatures"));

  params.linkParameters(k_StoreBoundary_Key, k_BoundaryCells_Key, std::make_any<bool>(true));
  params.linkParameters(k_StoreSurface_Key, k_SurfaceFeatures_Key, std::make_any<bool>(true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNeighbors::clone() const
{
  return std::make_unique<FindNeighbors>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNeighbors::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto storeBoundaryCells = args.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = args.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIds_Key);
  auto boundaryCellsName = args.value<std::string>(k_BoundaryCells_Key);
  auto numNeighborsName = args.value<std::string>(k_NumNeighbors_Key);
  auto neighborListName = args.value<std::string>(k_NeighborList_Key);
  auto sharedSurfaceAreaName = args.value<std::string>(k_SharedSurfaceArea_Key);
  auto surfaceFeaturesName = args.value<std::string>(k_SurfaceFeatures_Key);
  auto featureAttrMatrixPath = args.value<DataPath>(k_CellFeatures_Key);

  DataPath boundaryCellsPath = featureIdsPath.getParent().createChildPath(boundaryCellsName);
  DataPath numNeighborsPath = featureAttrMatrixPath.createChildPath(numNeighborsName);
  DataPath neighborListPath = featureAttrMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaPath = featureAttrMatrixPath.createChildPath(sharedSurfaceAreaName);
  DataPath surfaceFeaturesPath = featureAttrMatrixPath.createChildPath(surfaceFeaturesName);

  OutputActions actions;

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  std::vector<usize> tupleShape = featureIdsArray.getIDataStore()->getTupleShape();

  const std::vector<usize> cDims{1};

  // Create output Cell Data Arrays (if the user requested it)
  if(storeBoundaryCells)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int8, tupleShape, cDims, boundaryCellsPath);
    actions.appendAction(std::move(action));
  }

  // Feature Data:
  // Validating the Feature Attribute Matrix and trying to find a child of the Group
  // that is an IDataArray subclass, so we can get the proper tuple shape
  const auto* featureAttrMatrix = data.getDataAs<AttributeMatrix>(featureAttrMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12600, "Cell Feature AttributeMatrix Path is NOT an AttributeMatrix"}})};
  }
  tupleShape = featureAttrMatrix->getShape();
  auto tupleCount = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());

  // Create the NumNeighbors Output Data Array in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, cDims, numNeighborsPath);
    actions.appendAction(std::move(action));
  }
  // Create the NeighborList Output NeighborList in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::int32, tupleCount, neighborListPath);
    actions.appendAction(std::move(action));
  }
  // And we do the same for the SharedSurfaceArea list in the Feature Attribute Matrix
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupleCount, sharedSurfaceAreaPath);
    actions.appendAction(std::move(action));
  }
  // Create the SurfaceFeatures Output Data Array in the Feature Attribute Matrix
  if(storeSurfaceFeatures)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::boolean, tupleShape, cDims, surfaceFeaturesPath);
    actions.appendAction(std::move(action));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindNeighbors::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto storeBoundaryCells = args.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = args.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIds_Key);
  auto boundaryCellsName = args.value<std::string>(k_BoundaryCells_Key);
  auto numNeighborsName = args.value<std::string>(k_NumNeighbors_Key);
  auto neighborListName = args.value<std::string>(k_NeighborList_Key);
  auto sharedSurfaceAreaName = args.value<std::string>(k_SharedSurfaceArea_Key);
  auto surfaceFeaturesName = args.value<std::string>(k_SurfaceFeatures_Key);
  auto featureAttrMatrixPath = args.value<DataPath>(k_CellFeatures_Key);

  DataPath boundaryCellsPath = featureIdsPath.getParent().createChildPath(boundaryCellsName);
  DataPath numNeighborsPath = featureAttrMatrixPath.createChildPath(numNeighborsName);
  DataPath neighborListPath = featureAttrMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaPath = featureAttrMatrixPath.createChildPath(sharedSurfaceAreaName);
  DataPath surfaceFeaturesPath = featureAttrMatrixPath.createChildPath(surfaceFeaturesName);

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);
  auto& neighborList = data.getDataRefAs<Int32NeighborList>(neighborListPath);
  auto& sharedSurfaceAreaList = data.getDataRefAs<Float32NeighborList>(sharedSurfaceAreaPath);

  auto* boundaryCellsArray = data.getDataAs<Int8Array>(boundaryCellsPath);
  auto* surfaceFeaturesArray = data.getDataAs<BoolArray>(surfaceFeaturesPath);

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
  const auto imageGeomNumX = imageGeom.getNumXCells();
  const auto imageGeomNumY = imageGeom.getNumYCells();
  const auto imageGeomNumZ = imageGeom.getNumZCells();

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

      // Push the neighbor feature identifier back onto the list, so we stay synced up
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
