#include <catch2/catch.hpp>

#include "ComplexCore/Filters/FindNeighbors.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
const std::string k_ImageGeomName = "Image Geom";
const std::string k_FeatureIdsName = "Feature IDs";
const std::string k_BoundaryCellsName = "Boundary Cells";
const std::string k_NumNeighborsName = "Num Neighbors";
const std::string k_NeighborListName = "Neighbor List";
const std::string k_SharedSurfaceAreaName = "Shared Surface Area";
const std::string k_SurfaceFeaturesName = "Surface Features";
const std::vector<std::string> k_CellFeatureNames{"Array 1", "Array 2", "Array 3"};

DataStructure createTestData()
{
  DataStructure data;
  auto* imageGeom = ImageGeom::Create(data, k_ImageGeomName);
  imageGeom->setDimensions({5, 5, 5});
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});

  IDataStore::ShapeType tupleShape{5, 5, 5};
  IDataStore::ShapeType compDims{1};

  {
    auto* featureIdsArray = Int32Array::CreateWithStore<Int32DataStore>(data, k_FeatureIdsName, tupleShape, compDims, imageGeom->getId());
    auto& featureIds = featureIdsArray->getDataStoreRef();
    for(usize i = 0; i < featureIds.getSize(); ++i)
    {
      featureIds[i] = i / 3;
    }
  }

  for(const auto& cellFeatureName : k_CellFeatureNames)
  {
    auto* featureArray = Int32Array::CreateWithStore<Int32DataStore>(data, cellFeatureName, tupleShape, compDims, imageGeom->getId());
    featureArray->fill(5);
  }

  return data;
}
} // namespace

TEST_CASE("FindNeighbors(Instantiate)", "[ComplexCore][FindNeighbors]")
{
  static constexpr uint64 k_NComp = 3;
  static constexpr uint64 k_NumTuples = 25;
  const static DynamicTableData::TableDataType k_TupleDims = {{static_cast<double>(k_NumTuples)}};

  static const DataPath k_ImageGeom({k_ImageGeomName});
  static const DataPath k_FeatureIds({k_ImageGeomName, k_FeatureIdsName});
  static const DataPath k_BoundaryCells({k_ImageGeomName, k_BoundaryCellsName});
  static const DataPath k_SurfaceFeatures({k_ImageGeomName, k_SurfaceFeaturesName});
  static const DataPath k_NumNeighbors({k_ImageGeomName, k_NumNeighborsName});
  static const DataPath k_NeighborList({k_ImageGeomName, k_NeighborListName});
  static const DataPath k_SharedSurfaceArea({k_ImageGeomName, k_SharedSurfaceAreaName});

  std::vector<DataPath> k_CellFeatures;
  for(const auto& cellFeature : k_CellFeatureNames)
  {
    k_CellFeatures.push_back(DataPath({k_ImageGeomName, cellFeature}));
  }

  FindNeighbors filter;
  DataStructure ds = createTestData();
  Arguments args;

  args.insert(FindNeighbors::k_StoreBoundary_Key, std::make_any<bool>(false));
  args.insert(FindNeighbors::k_StoreSurface_Key, std::make_any<bool>(false));
  args.insert(FindNeighbors::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeom));
  args.insert(FindNeighbors::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIds));
  args.insert(FindNeighbors::k_CellFeatures_Key, std::make_any<std::vector<DataPath>>(k_CellFeatures));
  args.insert(FindNeighbors::k_BoundaryCells_Key, std::make_any<DataPath>(k_BoundaryCells));
  args.insert(FindNeighbors::k_NumNeighbors_Key, std::make_any<DataPath>(k_NumNeighbors));
  args.insert(FindNeighbors::k_NeighborList_Key, std::make_any<DataPath>(k_NeighborList));
  args.insert(FindNeighbors::k_SharedSurfaceArea_Key, std::make_any<DataPath>(k_SharedSurfaceArea));
  args.insert(FindNeighbors::k_SurfaceFeatures_Key, std::make_any<DataPath>(k_SurfaceFeatures));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}
