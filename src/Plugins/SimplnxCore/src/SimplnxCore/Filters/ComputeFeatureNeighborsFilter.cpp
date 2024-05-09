#include "ComputeFeatureNeighborsFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <sstream>

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborsFilter::name() const
{
  return FilterTraits<ComputeFeatureNeighborsFilter>::name;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborsFilter::className() const
{
  return FilterTraits<ComputeFeatureNeighborsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureNeighborsFilter::uuid() const
{
  return FilterTraits<ComputeFeatureNeighborsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureNeighborsFilter::humanName() const
{
  return "Find Feature Neighbors";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureNeighborsFilter::defaultTags() const
{
  return {className(), "Statistics", "Neighbors", "Features"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureNeighborsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreBoundary_Key, "Store Boundary Cells Array", "Whether to store the boundary Cells array", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreSurface_Key, "Store Surface Features Array", "Whether to store the surface Features array", false));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "The geometry in which to identify feature neighbors", DataPath({"DataContainer"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeaturesPath_Key, "Feature Attribute Matrix", "Feature Attribute Matrix of the selected Feature Ids",
                                                                    DataPath({"DataContainer", "CellFeatureData"})));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_BoundaryCellsName_Key, "Boundary Cells",
      "The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6. Only created if Store Boundary Cells Array is checked",
      "BoundaryCells"));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumNeighborsName_Key, "Number of Neighbors", "Number of contiguous neighboring Features for a given Feature", "NumNeighbors"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NeighborListName_Key, "Neighbor List", "List of the contiguous neighboring Features for a given Feature", "NeighborList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SharedSurfaceAreaName_Key, "Shared Surface Area List",
                                                          "List of the shared surface area for each of the contiguous neighboring Features for a given Feature", "SharedSurfaceAreaList"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceFeaturesName_Key, "Surface Features",
                                                          "The name of the surface features data array. Flag equal to 1 if the Feature touches an outer surface of the sample and equal to 0 if it "
                                                          "does not. Only created if Store Surface Features Array is checked",
                                                          "SurfaceFeatures"));

  params.linkParameters(k_StoreBoundary_Key, k_BoundaryCellsName_Key, std::make_any<bool>(true));
  params.linkParameters(k_StoreSurface_Key, k_SurfaceFeaturesName_Key, std::make_any<bool>(true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureNeighborsFilter::clone() const
{
  return std::make_unique<ComputeFeatureNeighborsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureNeighborsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto storeBoundaryCells = args.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = args.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto boundaryCellsName = args.value<std::string>(k_BoundaryCellsName_Key);
  auto numNeighborsName = args.value<std::string>(k_NumNeighborsName_Key);
  auto neighborListName = args.value<std::string>(k_NeighborListName_Key);
  auto sharedSurfaceAreaName = args.value<std::string>(k_SharedSurfaceAreaName_Key);
  auto surfaceFeaturesName = args.value<std::string>(k_SurfaceFeaturesName_Key);
  auto featureAttrMatrixPath = args.value<DataPath>(k_CellFeaturesPath_Key);

  DataPath boundaryCellsPath = featureIdsPath.replaceName(boundaryCellsName);
  DataPath numNeighborsPath = featureAttrMatrixPath.createChildPath(numNeighborsName);
  DataPath neighborListPath = featureAttrMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaPath = featureAttrMatrixPath.createChildPath(sharedSurfaceAreaName);
  DataPath surfaceFeaturesPath = featureAttrMatrixPath.createChildPath(surfaceFeaturesName);

  OutputActions actions;

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
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
  const auto* featureAttrMatrix = dataStructure.getDataAs<AttributeMatrix>(featureAttrMatrixPath);
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
Result<> ComputeFeatureNeighborsFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto storeBoundaryCells = args.value<bool>(k_StoreBoundary_Key);
  auto storeSurfaceFeatures = args.value<bool>(k_StoreSurface_Key);
  auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto boundaryCellsName = args.value<std::string>(k_BoundaryCellsName_Key);
  auto numNeighborsName = args.value<std::string>(k_NumNeighborsName_Key);
  auto neighborListName = args.value<std::string>(k_NeighborListName_Key);
  auto sharedSurfaceAreaName = args.value<std::string>(k_SharedSurfaceAreaName_Key);
  auto surfaceFeaturesName = args.value<std::string>(k_SurfaceFeaturesName_Key);
  auto featureAttrMatrixPath = args.value<DataPath>(k_CellFeaturesPath_Key);

  DataPath boundaryCellsPath = featureIdsPath.replaceName(boundaryCellsName);
  DataPath numNeighborsPath = featureAttrMatrixPath.createChildPath(numNeighborsName);
  DataPath neighborListPath = featureAttrMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaPath = featureAttrMatrixPath.createChildPath(sharedSurfaceAreaName);
  DataPath surfaceFeaturesPath = featureAttrMatrixPath.createChildPath(surfaceFeaturesName);

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);
  auto& neighborList = dataStructure.getDataRefAs<Int32NeighborList>(neighborListPath);
  auto& sharedSurfaceAreaList = dataStructure.getDataRefAs<Float32NeighborList>(sharedSurfaceAreaPath);

  auto* boundaryCellsArray = dataStructure.getDataAs<Int8Array>(boundaryCellsPath);
  auto* surfaceFeaturesArray = dataStructure.getDataAs<BoolArray>(surfaceFeaturesPath);

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

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
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
      messageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, static_cast<int32_t>(progInt)});
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
      messageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, static_cast<int32_t>(progInt)});
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
      messageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, static_cast<int32>(progInt)});
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

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_StoreBoundaryCellsKey = "StoreBoundaryCells";
constexpr StringLiteral k_StoreSurfaceFeaturesKey = "StoreSurfaceFeatures";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
constexpr StringLiteral k_BoundaryCellsArrayNameKey = "BoundaryCellsArrayName";
constexpr StringLiteral k_NumNeighborsArrayNameKey = "NumNeighborsArrayName";
constexpr StringLiteral k_NeighborListArrayNameKey = "NeighborListArrayName";
constexpr StringLiteral k_SharedSurfaceAreaListArrayNameKey = "SharedSurfaceAreaListArrayName";
constexpr StringLiteral k_SurfaceFeaturesArrayNameKey = "SurfaceFeaturesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureNeighborsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureNeighborsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreBoundaryCellsKey, k_StoreBoundary_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreSurfaceFeaturesKey, k_StoreSurface_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixPathKey, k_CellFeaturesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_BoundaryCellsArrayNameKey, k_BoundaryCellsName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NumNeighborsArrayNameKey, k_NumNeighborsName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NeighborListArrayNameKey, k_NeighborListName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SharedSurfaceAreaListArrayNameKey, k_SharedSurfaceAreaName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceFeaturesArrayNameKey, k_SurfaceFeaturesName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
