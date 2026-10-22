#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"
#include <H5Dpublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>
#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @struct ImportReferenceFile
 * @brief Owns a temporary file and its metadata cache entry.
 */
struct ImportReferenceFile
{
  fs::path path;

  ImportReferenceFile(const DataStructure& dataStructure, const std::string& name)
  : path(fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("SelectiveImportReferences_" + name + ".dream3d"))
  {
    DREAM3D::Dream3dPreflightCache::Instance().invalidate(path);
    auto result = DREAM3D::WriteFile(path, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    // A stable timestamp permits cache-hit assertions without a timed wait.
    fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::seconds(10));
  }

  ~ImportReferenceFile()
  {
    DREAM3D::Dream3dPreflightCache::Instance().invalidate(path);
    std::error_code error;
    fs::remove(path, error);
  }
};

template <typename T>
DataObject::IdType CreateReferenceArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId, usize components = 1)
{
  auto store = std::make_unique<DataStore<T>>(std::vector<usize>{1}, std::vector<usize>{components}, T{});
  auto* array = DataArray<T>::Create(dataStructure, name, std::move(store), parentId);
  REQUIRE(array != nullptr);
  return array->getId();
}

/**
 * @struct ReferenceLink
 * @brief Associates a serialized reference getter with its independently named target path.
 */
struct ReferenceLink
{
  DataPath targetPath;
  std::function<DataObject::OptionalId(const IGeometry&)> getId;
};

std::vector<ReferenceLink> PopulateReferences(DataStructure& dataStructure, IGeometry& geometry)
{
  const auto parentId = geometry.getId();
  const DataPath geometryPath({geometry.getName()});
  std::vector<ReferenceLink> links;
  const auto add = [&](const std::string& name, auto getter) { links.push_back({geometryPath.createChildPath(name), getter}); };
  const auto matrix = [&](const std::string& name) {
    auto* attributeMatrix = AttributeMatrix::Create(dataStructure, name, {1}, parentId);
    REQUIRE(attributeMatrix != nullptr);
    return attributeMatrix->getId();
  };
  geometry.setElementSizesId(CreateReferenceArray<float32>(dataStructure, "Sizes", parentId));
  add("Sizes", [](const IGeometry& geom) { return geom.getElementSizesId(); });
  if(auto* grid = dynamic_cast<IGridGeometry*>(&geometry); grid != nullptr)
  {
    grid->setCellData(matrix("Cells"));
    add("Cells", [](const IGeometry& geom) { return dynamic_cast<const IGridGeometry&>(geom).getCellDataId(); });
  }
  if(auto* node = dynamic_cast<INodeGeometry0D*>(&geometry); node != nullptr)
  {
    node->setVertexListId(CreateReferenceArray<float32>(dataStructure, "Vertices", parentId, 3));
    add("Vertices", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry0D&>(geom).getVertexListId(); });
    node->setVertexDataId(matrix("VertexData"));
    add("VertexData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry0D&>(geom).getVertexAttributeMatrixId(); });
  }
  if(auto* node = dynamic_cast<INodeGeometry1D*>(&geometry); node != nullptr)
  {
    node->setEdgeListId(CreateReferenceArray<uint64>(dataStructure, "Edges", parentId, 2));
    add("Edges", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getEdgeListId(); });
    node->setEdgeDataId(matrix("EdgeData"));
    add("EdgeData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getEdgeAttributeMatrixId(); });
    // DynamicListArray has no HDF5 factory. These two targets test ID membership without invoking cache consumers.
    node->setElementContainingVertId(CreateReferenceArray<uint64>(dataStructure, "ContainingVertices", parentId));
    add("ContainingVertices", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getElementContainingVertId(); });
    node->setElementNeighborsId(CreateReferenceArray<uint64>(dataStructure, "Neighbors", parentId));
    add("Neighbors", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getElementNeighborsId(); });
    node->setElementCentroidsId(CreateReferenceArray<float32>(dataStructure, "Centroids", parentId, 3));
    add("Centroids", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getElementCentroidsId(); });
  }
  if(auto* node = dynamic_cast<INodeGeometry2D*>(&geometry); node != nullptr)
  {
    node->setFaceListId(CreateReferenceArray<uint64>(dataStructure, "Faces", parentId, 3));
    add("Faces", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry2D&>(geom).getFaceListId(); });
    node->setFaceDataId(matrix("FaceData"));
    add("FaceData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry2D&>(geom).getFaceAttributeMatrixId(); });
    node->setUnsharedEdgesId(CreateReferenceArray<uint64>(dataStructure, "UnsharedEdges", parentId, 2));
    add("UnsharedEdges", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry2D&>(geom).getUnsharedEdgesId(); });
  }
  if(auto* node = dynamic_cast<INodeGeometry3D*>(&geometry); node != nullptr)
  {
    node->setPolyhedronListId(CreateReferenceArray<uint64>(dataStructure, "Polyhedra", parentId, 4));
    add("Polyhedra", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry3D&>(geom).getPolyhedronListId(); });
    node->setPolyhedraDataId(matrix("PolyhedronData"));
    add("PolyhedronData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry3D&>(geom).getPolyhedraAttributeMatrixId(); });
    node->setUnsharedFacedId(CreateReferenceArray<uint64>(dataStructure, "UnsharedFaces", parentId, 3));
    add("UnsharedFaces", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry3D&>(geom).getUnsharedFacesId(); });
  }
  if(auto* rectGrid = dynamic_cast<RectGridGeom*>(&geometry); rectGrid != nullptr)
  {
    rectGrid->setXBoundsId(CreateReferenceArray<float32>(dataStructure, "XBounds", parentId));
    add("XBounds", [](const IGeometry& geom) { return dynamic_cast<const RectGridGeom&>(geom).getXBoundsId(); });
    rectGrid->setYBoundsId(CreateReferenceArray<float32>(dataStructure, "YBounds", parentId));
    add("YBounds", [](const IGeometry& geom) { return dynamic_cast<const RectGridGeom&>(geom).getYBoundsId(); });
    rectGrid->setZBoundsId(CreateReferenceArray<float32>(dataStructure, "ZBounds", parentId));
    add("ZBounds", [](const IGeometry& geom) { return dynamic_cast<const RectGridGeom&>(geom).getZBoundsId(); });
  }
  return links;
}

class GeometryAliasResolver : public IDataStoreFormatResolver
{
public:
  GeometryAliasResolver() = default;
  ~GeometryAliasResolver() override = default;
  GeometryAliasResolver(const GeometryAliasResolver&) = delete;
  GeometryAliasResolver(GeometryAliasResolver&&) = delete;
  GeometryAliasResolver& operator=(const GeometryAliasResolver&) = delete;
  GeometryAliasResolver& operator=(GeometryAliasResolver&&) = delete;

  mutable std::atomic<usize> Calls{0};

  std::string resolveFormat(const DataStructure& dataStructure, const DataPath& path, [[maybe_unused]] DataType dataType, [[maybe_unused]] uint64 byteSize) const override
  {
    ++Calls;
    const auto geometryPath = path.getParent();
    const auto* geometryPtr = dataStructure.getDataAs<TriangleGeom>(geometryPath);
    const auto* verticesPtr = dataStructure.getDataAs<Float32Array>(geometryPath.createChildPath("Vertices"));
    const auto* facesPtr = dataStructure.getDataAs<DataArray<IGeometry::MeshIndexType>>(geometryPath.createChildPath("Faces"));
    REQUIRE(geometryPtr != nullptr);
    REQUIRE(verticesPtr != nullptr);
    REQUIRE(facesPtr != nullptr);
    CAPTURE(path.toString());
    CHECK(geometryPtr->getVertexListId() == verticesPtr->getId());
    CHECK(geometryPtr->getFaceListId() == facesPtr->getId());
    CHECK(geometryPtr->getVertices() == verticesPtr);
    CHECK(geometryPtr->getFaces() == facesPtr);
    CHECK(verticesPtr->getStoreType() == IDataStore::StoreType::Empty);
    CHECK(facesPtr->getStoreType() == IDataStore::StoreType::Empty);
    return {};
  }
};
} // namespace

TEST_CASE("Selective import references: geometry aliases use their own child arrays", "[SelectiveImportReferences][GeometryAliases]")
{
  UnitTest::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool bothPlacements = GENERATE(false, true);
  const bool reverse = GENERATE(false, true);
  const bool warm = GENERATE(false, true);
  CAPTURE(static_cast<int>(mode), bothPlacements, reverse, warm);
  const std::array<float32, 9> expectedVertices{0, 0, 0, 1, 0, 0, 0, 1, 0};
  const std::array<IGeometry::MeshIndexType, 3> expectedFaces{0, 1, 2};
  DataStructure source;
  auto* firstPtr = DataGroup::Create(source, "First");
  auto* secondPtr = DataGroup::Create(source, "Second");
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  auto* meshPtr = TriangleGeom::Create(source, "Mesh", firstPtr->getId());
  REQUIRE(meshPtr != nullptr);
  auto vertexStore = std::make_shared<DataStore<float32>>(ShapeType{3}, ShapeType{3}, 0.0F);
  auto faceStore = std::make_shared<DataStore<IGeometry::MeshIndexType>>(ShapeType{1}, ShapeType{3}, IGeometry::MeshIndexType{0});
  const auto writeVertices = vertexStore->copyFromBuffer(0, expectedVertices);
  const auto writeFaces = faceStore->copyFromBuffer(0, expectedFaces);
  SIMPLNX_RESULT_REQUIRE_VALID(writeVertices);
  SIMPLNX_RESULT_REQUIRE_VALID(writeFaces);
  auto* verticesPtr = Float32Array::Create(source, "Vertices", vertexStore, meshPtr->getId());
  auto* facesPtr = DataArray<IGeometry::MeshIndexType>::Create(source, "Faces", faceStore, meshPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  meshPtr->setVertices(*verticesPtr);
  meshPtr->setFaceList(*facesPtr);
  REQUIRE(source.setAdditionalParent(meshPtr->getId(), secondPtr->getId()));
  const ImportReferenceFile file(source, "GeometryAliases");
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  auto handout = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(handout);
  const auto* cachedMeshPtr = handout.value().dataStructure.getDataAs<TriangleGeom>(DataPath({"First", "Mesh"}));
  REQUIRE(cachedMeshPtr != nullptr);
  const auto cachedVertexId = cachedMeshPtr->getVertexListId();
  const auto cachedFaceId = cachedMeshPtr->getFaceListId();
  cache.invalidate(file.path);
  if(warm)
  {
    auto warmResult = cache.fetchNeutralMetadata(file.path);
    SIMPLNX_RESULT_REQUIRE_VALID(warmResult);
  }
  cache.resetStats();
  const std::string selectedParent = reverse ? "First" : "Second";
  std::vector<DataPath> paths{DataPath({selectedParent}), DataPath({selectedParent, "Mesh"}), DataPath({selectedParent, "Mesh", "Vertices"}), DataPath({selectedParent, "Mesh", "Faces"})};
  if(bothPlacements)
  {
    const std::string otherParent = reverse ? "Second" : "First";
    paths.insert(paths.end(), {DataPath({otherParent}), DataPath({otherParent, "Mesh"}), DataPath({otherParent, "Mesh", "Vertices"}), DataPath({otherParent, "Mesh", "Faces"})});
  }
  DataStructure destination;
  const auto resolver = std::make_shared<GeometryAliasResolver>();
  destination.setFormatResolver(resolver);
  const auto result = ImportH5ObjectPathsAction(file.path, paths).apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(resolver->Calls.load() == (bothPlacements ? 4 : 2));
  CHECK(cache.hitCount() == (warm ? 1 : 0));
  CHECK(cache.missCount() == (warm ? 0 : 1));
  for(const auto& parent : {std::string("First"), std::string("Second")})
  {
    if(!bothPlacements && parent != selectedParent)
    {
      CHECK_FALSE(destination.containsData(DataPath({parent})));
      continue;
    }
    const DataPath meshPath({parent, "Mesh"});
    const auto* importedPtr = destination.getDataAs<TriangleGeom>(meshPath);
    const auto* importedVerticesPtr = destination.getDataAs<Float32Array>(meshPath.createChildPath("Vertices"));
    const auto* importedFacesPtr = destination.getDataAs<DataArray<IGeometry::MeshIndexType>>(meshPath.createChildPath("Faces"));
    REQUIRE(importedPtr != nullptr);
    REQUIRE(importedVerticesPtr != nullptr);
    REQUIRE(importedFacesPtr != nullptr);
    CAPTURE(meshPath.toString());
    CHECK(importedPtr->getVertexListId() == importedVerticesPtr->getId());
    CHECK(importedPtr->getFaceListId() == importedFacesPtr->getId());
    CHECK(importedPtr->getVertices() == importedVerticesPtr);
    CHECK(importedPtr->getFaces() == importedFacesPtr);
    CHECK(importedVerticesPtr->getTupleShape() == ShapeType{3});
    CHECK(importedVerticesPtr->getComponentShape() == ShapeType{3});
    CHECK(importedFacesPtr->getTupleShape() == ShapeType{1});
    CHECK(importedFacesPtr->getComponentShape() == ShapeType{3});
    if(mode == IDataAction::Mode::Preflight)
    {
      CHECK(importedVerticesPtr->getIDataStore()->getStoreType() == IDataStore::StoreType::Empty);
      CHECK(importedFacesPtr->getIDataStore()->getStoreType() == IDataStore::StoreType::Empty);
    }
    else
    {
      std::array<float32, 9> actualVertices{};
      std::array<IGeometry::MeshIndexType, 3> actualFaces{};
      const auto readVertices = importedVerticesPtr->getDataStoreRef().copyIntoBuffer(0, actualVertices);
      const auto readFaces = importedFacesPtr->getDataStoreRef().copyIntoBuffer(0, actualFaces);
      SIMPLNX_RESULT_REQUIRE_VALID(readVertices);
      SIMPLNX_RESULT_REQUIRE_VALID(readFaces);
      CHECK(actualVertices == expectedVertices);
      CHECK(actualFaces == expectedFaces);
    }
  }
  if(bothPlacements)
  {
    CHECK(destination.getData(DataPath({"First", "Mesh"})) != destination.getData(DataPath({"Second", "Mesh"})));
    for(const auto* namePtr : {"Vertices", "Faces"})
    {
      const auto* firstArrayPtr = destination.getDataAs<IDataArray>(DataPath({"First", "Mesh", namePtr}));
      const auto* secondArrayPtr = destination.getDataAs<IDataArray>(DataPath({"Second", "Mesh", namePtr}));
      REQUIRE(firstArrayPtr != nullptr);
      REQUIRE(secondArrayPtr != nullptr);
      CHECK(firstArrayPtr != secondArrayPtr);
      CHECK(firstArrayPtr->getId() != secondArrayPtr->getId());
      CHECK(firstArrayPtr->getIDataStore() != secondArrayPtr->getIDataStore());
    }
  }
  CHECK(cachedMeshPtr->getVertexListId() == cachedVertexId);
  CHECK(cachedMeshPtr->getFaceListId() == cachedFaceId);
  CHECK(handout.value().dataStructure.getData(DataPath({"First", "Mesh"})) == handout.value().dataStructure.getData(DataPath({"Second", "Mesh"})));
  CHECK(meshPtr->getVertexListId() == verticesPtr->getId());
  CHECK(meshPtr->getFaceListId() == facesPtr->getId());
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Selective import references: omitted local alias keeps a selected shared target", "[SelectiveImportReferences][GeometryAliases]")
{
  UnitTest::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool selectRoot = GENERATE(false, true);
  DataStructure source;
  auto* geometryPtr = ImageGeom::Create(source, "Geometry");
  auto* aliasPtr = DataGroup::Create(source, "Alias");
  REQUIRE(geometryPtr != nullptr);
  REQUIRE(aliasPtr != nullptr);
  auto* cellsPtr = AttributeMatrix::Create(source, "Cells", {1}, geometryPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geometryPtr->setCellData(cellsPtr->getId());
  REQUIRE(source.setAdditionalParent(geometryPtr->getId(), aliasPtr->getId()));
  const ImportReferenceFile file(source, "PartialGeometryAlias");
  const DataPath rootPath({"Geometry"});
  const DataPath aliasPath({"Alias", "Geometry"});
  const auto selectedCellsPath = (selectRoot ? rootPath : aliasPath).createChildPath("Cells");
  const auto omittedCellsPath = (selectRoot ? aliasPath : rootPath).createChildPath("Cells");
  DataStructure destination;
  const auto result = ImportH5ObjectPathsAction(file.path, {rootPath, DataPath({"Alias"}), aliasPath, selectedCellsPath}).apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* selectedCellsPtr = destination.getDataAs<AttributeMatrix>(selectedCellsPath);
  REQUIRE(selectedCellsPtr != nullptr);
  CHECK_FALSE(destination.containsData(omittedCellsPath));
  CHECK(destination.getAllDataPaths().size() == 4);
  for(const auto& path : {rootPath, aliasPath})
  {
    const auto* importedPtr = destination.getDataAs<ImageGeom>(path);
    REQUIRE(importedPtr != nullptr);
    CHECK(importedPtr->getCellDataId() == selectedCellsPtr->getId());
    CHECK(destination.getData(importedPtr->getCellDataId()) == selectedCellsPtr);
  }
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Selective import references: nested targets preserve explicit alias rules", "[SelectiveImportReferences][GeometryAliases]")
{
  UnitTest::LoadPlugins();
  enum class Scenario
  {
    UniqueNested,
    AmbiguousNested,
    WrongDirectName,
    PrefixSibling,
    DirectChild
  };
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const auto scenario = GENERATE(Scenario::UniqueNested, Scenario::AmbiguousNested, Scenario::WrongDirectName, Scenario::PrefixSibling, Scenario::DirectChild);
  const bool reverse = GENERATE(false, true);
  CAPTURE(static_cast<int>(mode), static_cast<int>(scenario), reverse);
  DataStructure source;
  auto* sizesPtr = Float32Array::Create(source, "Sizes", std::make_unique<DataStore<float32>>(ShapeType{1}, ShapeType{1}, 7.0F));
  auto* geometryPtr = VertexGeom::Create(source, "Mesh");
  REQUIRE(sizesPtr != nullptr);
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setElementSizesId(sizesPtr->getId());
  DataPath expectedPath({"Mesh", "Nested", "Sizes"});
  if(scenario == Scenario::PrefixSibling)
  {
    auto* siblingPtr = DataGroup::Create(source, "MeshExtra");
    REQUIRE(siblingPtr != nullptr);
    REQUIRE(source.setAdditionalParent(sizesPtr->getId(), siblingPtr->getId()));
    expectedPath = DataPath({"Sizes"});
  }
  else
  {
    auto* nestedPtr = DataGroup::Create(source, "Nested", geometryPtr->getId());
    REQUIRE(nestedPtr != nullptr);
    REQUIRE(source.setAdditionalParent(sizesPtr->getId(), nestedPtr->getId()));
    if(scenario == Scenario::AmbiguousNested || scenario == Scenario::DirectChild)
    {
      auto* otherPtr = DataGroup::Create(source, "Other", geometryPtr->getId());
      REQUIRE(otherPtr != nullptr);
      REQUIRE(source.setAdditionalParent(sizesPtr->getId(), otherPtr->getId()));
      expectedPath = DataPath({"Sizes"});
    }
    if(scenario == Scenario::WrongDirectName)
    {
      const auto wrongId = CreateReferenceArray<float32>(source, "Sizes", geometryPtr->getId());
      REQUIRE(wrongId != sizesPtr->getId());
    }
    if(scenario == Scenario::DirectChild)
    {
      REQUIRE(source.setAdditionalParent(sizesPtr->getId(), geometryPtr->getId()));
      expectedPath = DataPath({"Mesh", "Sizes"});
    }
  }
  REQUIRE(source.getData(DataPath({"Sizes"})) == sizesPtr);
  const ImportReferenceFile file(source, "NestedGeometryAliases");
  auto paths = source.getAllDataPaths();
  // Parent-based path enumeration omits the root placement of this shared array.
  paths.emplace_back(std::vector<std::string>{"Sizes"});
  if(reverse)
  {
    std::ranges::reverse(paths);
  }
  DataStructure destination;
  const auto result = ImportH5ObjectPathsAction(file.path, paths).apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* importedPtr = destination.getDataAs<VertexGeom>(DataPath({"Mesh"}));
  const auto* expectedPtr = destination.getDataAs<Float32Array>(expectedPath);
  REQUIRE(importedPtr != nullptr);
  REQUIRE(expectedPtr != nullptr);
  CHECK(importedPtr->getElementSizesId() == expectedPtr->getId());
  CHECK(importedPtr->getElementSizes() == expectedPtr);
  if(mode == IDataAction::Mode::Execute)
  {
    CHECK((*expectedPtr)[0] == 7.0F);
  }
  if(scenario == Scenario::WrongDirectName)
  {
    const auto* wrongPtr = destination.getData(DataPath({"Mesh", "Sizes"}));
    REQUIRE(wrongPtr != nullptr);
    CHECK(importedPtr->getElementSizesId() != wrongPtr->getId());
  }
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Selective import references: many geometry aliases keep unique nested targets", "[SelectiveImportReferences][GeometryAliases]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_AliasCount = 128;
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure source;
  auto* firstParentPtr = DataGroup::Create(source, "Alias_0");
  REQUIRE(firstParentPtr != nullptr);
  auto* geometryPtr = VertexGeom::Create(source, "Mesh", firstParentPtr->getId());
  REQUIRE(geometryPtr != nullptr);
  auto* nestedPtr = DataGroup::Create(source, "Nested", geometryPtr->getId());
  REQUIRE(nestedPtr != nullptr);
  const auto sizesId = CreateReferenceArray<float32>(source, "Sizes", nestedPtr->getId());
  geometryPtr->setElementSizesId(sizesId);
  for(usize index = 1; index < k_AliasCount; ++index)
  {
    auto* parentPtr = DataGroup::Create(source, "Alias_" + std::to_string(index));
    REQUIRE(parentPtr != nullptr);
    REQUIRE(source.setAdditionalParent(geometryPtr->getId(), parentPtr->getId()));
  }
  const ImportReferenceFile file(source, "ManyGeometryAliases");
  DataStructure destination;
  const auto result = ImportH5ObjectPathsAction(file.path, source.getAllDataPaths()).apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(destination.getAllDataPaths().size() == 4 * k_AliasCount);
  std::set<DataObject::IdType> geometryIds;
  std::set<DataObject::IdType> targetIds;
  for(usize index = 0; index < k_AliasCount; ++index)
  {
    const DataPath geometryPath({"Alias_" + std::to_string(index), "Mesh"});
    const auto* importedPtr = destination.getDataAs<VertexGeom>(geometryPath);
    const auto* targetPtr = destination.getDataAs<Float32Array>(geometryPath.createChildPath("Nested").createChildPath("Sizes"));
    REQUIRE(importedPtr != nullptr);
    REQUIRE(targetPtr != nullptr);
    CAPTURE(geometryPath.toString());
    CHECK(importedPtr->getElementSizesId() == targetPtr->getId());
    CHECK(importedPtr->getElementSizes() == targetPtr);
    CHECK(geometryIds.insert(importedPtr->getId()).second);
    CHECK(targetIds.insert(targetPtr->getId()).second);
  }
  UnitTest::CheckArraysInheritTupleDims(destination);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
TEST_CASE("Selective import references: geometry alias publication rolls back and retries", "[SelectiveImportReferences][GeometryAliases]")
{
  UnitTest::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure source;
  auto* firstPtr = DataGroup::Create(source, "First");
  auto* secondPtr = DataGroup::Create(source, "Second");
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  auto* geometryPtr = ImageGeom::Create(source, "Geometry", firstPtr->getId());
  REQUIRE(geometryPtr != nullptr);
  auto* cellsPtr = AttributeMatrix::Create(source, "Cells", {1}, geometryPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geometryPtr->setCellData(cellsPtr->getId());
  REQUIRE(source.setAdditionalParent(geometryPtr->getId(), secondPtr->getId()));
  const ImportReferenceFile file(source, "GeometryAliasRollback");
  DataStructure destination;
  REQUIRE(DataGroup::Create(destination, "First") != nullptr);
  REQUIRE(DataGroup::Create(destination, "Second") != nullptr);
  auto* markerPtr = DataGroup::Create(destination, "Existing");
  REQUIRE(markerPtr != nullptr);
  const auto beforeIds = destination.getAllDataObjectIds();
  const auto beforePaths = destination.getAllDataPaths();
  const auto beforeNextId = destination.getNextId();
  const std::vector<DataPath> selected{DataPath({"First", "Geometry"}), DataPath({"Second", "Geometry"}), DataPath({"First", "Geometry", "Cells"}), DataPath({"Second", "Geometry", "Cells"})};
  const ImportH5ObjectPathsAction action(file.path, selected);
  action.setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFaultType::AfterSecondInsert);
  const auto failed = action.apply(destination, mode);
  REQUIRE(failed.invalid());
  REQUIRE_FALSE(failed.errors().empty());
  CHECK(failed.errors().front().code == -6214);
  CHECK(action.publicationFaultConsumedForTesting());
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getAllDataPaths() == beforePaths);
  CHECK(destination.getNextId() == beforeNextId);
  CHECK(destination.getData(DataPath({"Existing"})) == markerPtr);
  const auto retained = std::dynamic_pointer_cast<ImageGeom>(action.retainedOwnerForTesting());
  REQUIRE(retained != nullptr);
  CHECK(retained->getDataStructure() == nullptr);
  REQUIRE(retained->getCellDataId().has_value());
  CHECK(destination.getData(retained->getCellDataId()) == nullptr);
  const auto retried = ImportH5ObjectPathsAction(file.path, selected).apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(retried);
  for(const auto* parentPtr : {"First", "Second"})
  {
    const auto* importedPtr = destination.getDataAs<ImageGeom>(DataPath({parentPtr, "Geometry"}));
    const auto* importedCellsPtr = destination.getDataAs<AttributeMatrix>(DataPath({parentPtr, "Geometry", "Cells"}));
    REQUIRE(importedPtr != nullptr);
    REQUIRE(importedCellsPtr != nullptr);
    CHECK(importedPtr->getCellDataId() == importedCellsPtr->getId());
    CHECK(destination.getData(importedPtr->getCellDataId()) == importedCellsPtr);
  }
  CHECK(destination.getData(DataPath({"Existing"})) == markerPtr);
  UnitTest::CheckArraysInheritTupleDims(destination);
}
#endif

TEST_CASE("Selective import references: overlapping ElementSizes reset", "[SelectiveImportReferences][DataStructure]")
{
  DataStructure source;
  source.setNextId(1);
  auto* geometry = EdgeGeom::Create(source, "Geometry");
  REQUIRE(geometry != nullptr);
  const auto sizesId = CreateReferenceArray<float32>(source, "Sizes", geometry->getId());
  REQUIRE(sizesId == 2);
  geometry->setElementSizesId(sizesId);
  REQUIRE(DataGroup::Create(source, "Third") != nullptr);
  auto* fourth = DataGroup::Create(source, "Fourth");
  REQUIRE(fourth != nullptr);
  REQUIRE(fourth->getId() == 4);

  source.resetIds(3);
  const auto* sizes = source.getData(DataPath({"Geometry", "Sizes"}));
  REQUIRE(sizes != nullptr);
  REQUIRE(sizes->getId() == 4);
  REQUIRE(geometry->getElementSizesId() == sizes->getId());
  UnitTest::CheckArraysInheritTupleDims(source);
}

TEST_CASE("Selective import references: excluded cell ID cannot alias an ensemble", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    DataStructure source;
    source.setNextId(1);
    auto* geometry = ImageGeom::Create(source, "Image");
    REQUIRE(geometry != nullptr);
    geometry->setDimensions({2, 3, 4});
    auto* cells = AttributeMatrix::Create(source, "CellData", {4, 3, 2}, geometry->getId());
    auto* ensemble = AttributeMatrix::Create(source, "CellEnsembleData", {2}, geometry->getId());
    REQUIRE(cells != nullptr);
    REQUIRE(ensemble != nullptr);
    REQUIRE(geometry->getId() == 1);
    REQUIRE(cells->getId() == 2);
    REQUIRE(ensemble->getId() == 3);
    geometry->setCellData(cells->getId());
    const ImportReferenceFile file(source, "ImageCollision");
    DataStructure destination;
    destination.setNextId(1);
    auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"Image"}), DataPath({"Image", "CellEnsembleData"})}).apply(destination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto* imported = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
    const auto* importedEnsemble = destination.getDataAs<AttributeMatrix>(DataPath({"Image", "CellEnsembleData"}));
    REQUIRE(imported != nullptr);
    REQUIRE(importedEnsemble != nullptr);
    REQUIRE(importedEnsemble->getId() == ensemble->getId());
    REQUIRE_FALSE(imported->getCellDataId().has_value());
    REQUIRE_FALSE(destination.containsData(DataPath({"Image", "CellData"})));
    REQUIRE(destination.getData(cells->getId()) == nullptr);
    const auto importedIds = destination.getAllDataObjectIds();
    REQUIRE(std::find(importedIds.begin(), importedIds.end(), cells->getId()) == importedIds.end());
    const auto importedNextId = destination.getNextId();
    // Reuse the excluded cell ID explicitly so a stale reference would resolve to an unrelated ensemble.
    auto* unrelated = AttributeMatrix::Import(destination, "UnrelatedEnsemble", ShapeType{2}, cells->getId(), imported->getId());
    REQUIRE(unrelated != nullptr);
    REQUIRE(unrelated->getId() == cells->getId());
    REQUIRE(destination.getData(cells->getId()) == unrelated);
    REQUIRE(destination.getData(DataPath({"Image", "UnrelatedEnsemble"})) == unrelated);
    REQUIRE_FALSE(imported->getCellDataId().has_value());
    REQUIRE(destination.getData(DataPath({"Image", "CellEnsembleData"})) == importedEnsemble);
    REQUIRE(destination.getData(importedEnsemble->getId()) == importedEnsemble);
    REQUIRE(importedEnsemble->getId() == ensemble->getId());
    REQUIRE(importedEnsemble->getShape() == ShapeType{2});
    REQUIRE(destination.getNextId() == importedNextId);
    REQUIRE(imported->getDimensions() == geometry->getDimensions());
    auto validation = imported->validate();
    SIMPLNX_RESULT_REQUIRE_VALID(validation);
    UnitTest::CheckArraysInheritTupleDims(destination);
  }
}

TEST_CASE("Selective import references: field matrix and full selection", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool aliases = GENERATE(false, true);
  const auto type = GENERATE(IGeometry::Type::Image, IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Tetrahedral, IGeometry::Type::RectGrid);
  DataStructure source;
  IGeometry* geometry = nullptr;
  switch(type)
  {
  case IGeometry::Type::Image:
    geometry = ImageGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Vertex:
    geometry = VertexGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Edge:
    geometry = EdgeGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Triangle:
    geometry = TriangleGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Tetrahedral:
    geometry = TetrahedralGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::RectGrid:
    geometry = RectGridGeom::Create(source, "Geometry");
    break;
  default:
    FAIL("Unexpected fixture geometry");
  }
  REQUIRE(geometry != nullptr);
  const auto links = PopulateReferences(source, *geometry);
  std::vector<DataPath> geometryPaths{DataPath({"Geometry"})};
  if(aliases)
  {
    auto* aliasParentPtr = DataGroup::Create(source, "Alias");
    REQUIRE(aliasParentPtr != nullptr);
    REQUIRE(source.setAdditionalParent(geometry->getId(), aliasParentPtr->getId()));
    geometryPaths.emplace_back(std::vector<std::string>{"Alias", "Geometry"});
  }
  const ImportReferenceFile file(source, "FieldMatrix");
  // The final case retains all references with overlapping source and destination ID ranges.
  for(usize omitted = 0; omitted <= links.size(); ++omitted)
  {
    DYNAMIC_SECTION("type " << static_cast<int>(type) << " mode " << static_cast<int>(mode) << " omitted " << omitted << " aliases " << aliases)
    {
      auto paths = geometryPaths;
      if(aliases)
      {
        paths.emplace_back(std::vector<std::string>{"Alias"});
      }
      for(const auto& geometryPath : geometryPaths)
      {
        for(usize field = 0; field < links.size(); ++field)
        {
          if(field != omitted)
          {
            paths.push_back(geometryPath.createChildPath(links[field].targetPath.getTargetName()));
          }
        }
      }
      DataStructure destination;
      destination.setNextId(omitted == links.size() ? 3 : 100);
      auto result = ImportH5ObjectPathsAction(file.path, paths).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      for(const auto& geometryPath : geometryPaths)
      {
        const auto* importedPtr = destination.getDataAs<IGeometry>(geometryPath);
        REQUIRE(importedPtr != nullptr);
        for(usize field = 0; field < links.size(); ++field)
        {
          const auto targetPath = geometryPath.createChildPath(links[field].targetPath.getTargetName());
          CAPTURE(targetPath.toString());
          if(field == omitted)
          {
            REQUIRE_FALSE(links[field].getId(*importedPtr).has_value());
            REQUIRE_FALSE(destination.containsData(targetPath));
          }
          else
          {
            const auto* targetPtr = destination.getData(targetPath);
            REQUIRE(targetPtr != nullptr);
            REQUIRE(links[field].getId(*importedPtr) == targetPtr->getId());
            REQUIRE(destination.getData(links[field].getId(*importedPtr)) == targetPtr);
          }
        }
      }
      UnitTest::CheckArraysInheritTupleDims(destination);
    }
  }
}

TEST_CASE("Selective import references: loader ancestors are not selected targets", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    DataStructure source;
    auto* geometry = ImageGeom::Create(source, "Image");
    auto* cells = AttributeMatrix::Create(source, "Cells", {1});
    REQUIRE(geometry != nullptr);
    REQUIRE(cells != nullptr);
    geometry->setCellData(cells->getId());
    CreateReferenceArray<float32>(source, "SelectedChild", cells->getId());
    const ImportReferenceFile file(source, "Ancestor");
    const std::vector<DataPath> paths{DataPath({"Image"}), DataPath({"Cells", "SelectedChild"})};
    auto loaded = DREAM3D::LoadDataStructureArrays(file.path, paths);
    SIMPLNX_RESULT_REQUIRE_VALID(loaded);
    REQUIRE(loaded.value().containsData(DataPath({"Cells"})));

    DataStructure destination;
    destination.setNextId(100);
    auto* existingCells = AttributeMatrix::Create(destination, "Cells", {1});
    REQUIRE(existingCells != nullptr);
    auto result = ImportH5ObjectPathsAction(file.path, paths).apply(destination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto* imported = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
    REQUIRE(imported != nullptr);
    REQUIRE_FALSE(imported->getCellDataId().has_value());
    REQUIRE(destination.getData(DataPath({"Cells"})) == existingCells);
    REQUIRE(destination.containsData(DataPath({"Cells", "SelectedChild"})));
    REQUIRE(destination.getAllDataPaths().size() == 3);
    UnitTest::CheckArraysInheritTupleDims(destination);
  }
}

TEST_CASE("Selective import references: cached selections and shared targets stay isolated", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure source;
  auto* first = ImageGeom::Create(source, "First");
  auto* second = ImageGeom::Create(source, "Second");
  auto* cells = AttributeMatrix::Create(source, "Cells", {1});
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  REQUIRE(cells != nullptr);
  first->setCellData(cells->getId());
  second->setCellData(cells->getId());
  const ImportReferenceFile file(source, "Cache");
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  auto cached = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(cached);
  const auto initialHits = cache.hitCount();

  DataStructure partial;
  auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"First"})}).apply(partial, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* partialFirst = partial.getDataAs<ImageGeom>(DataPath({"First"}));
  REQUIRE(partialFirst != nullptr);
  REQUIRE_FALSE(partialFirst->getCellDataId().has_value());
  REQUIRE(partial.getAllDataPaths().size() == 1);

  DataStructure full;
  full.setNextId(100);
  result = ImportH5ObjectPathsAction(file.path, {DataPath({"First"}), DataPath({"Second"}), DataPath({"Cells"})}).apply(full, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(cache.hitCount() >= initialHits + 2);
  const auto* importedFirst = full.getDataAs<ImageGeom>(DataPath({"First"}));
  const auto* importedSecond = full.getDataAs<ImageGeom>(DataPath({"Second"}));
  const auto* importedCells = full.getDataAs<AttributeMatrix>(DataPath({"Cells"}));
  REQUIRE(importedFirst != nullptr);
  REQUIRE(importedSecond != nullptr);
  REQUIRE(importedCells != nullptr);
  REQUIRE(importedFirst->getCellDataId() == importedCells->getId());
  REQUIRE(importedSecond->getCellDataId() == importedCells->getId());
  REQUIRE(full.getData(importedFirst->getCellDataId()) == importedCells);
  REQUIRE(full.getData(importedSecond->getCellDataId()) == importedCells);
  const auto* unchanged = cached.value().dataStructure.getDataAs<ImageGeom>(DataPath({"First"}));
  REQUIRE(unchanged != nullptr);
  REQUIRE(unchanged->getCellDataId() == cells->getId());
  UnitTest::CheckArraysInheritTupleDims(partial);
  UnitTest::CheckArraysInheritTupleDims(full);
}

TEST_CASE("Selective import references: retained malformed links preserve validation", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    SECTION("Retained cell matrix keeps the existing tuple-count error")
    {
      DataStructure source;
      auto* geometry = ImageGeom::Create(source, "Image");
      REQUIRE(geometry != nullptr);
      geometry->setDimensions({2, 3, 4});
      auto* cells = AttributeMatrix::Create(source, "Cells", {2}, geometry->getId());
      REQUIRE(cells != nullptr);
      geometry->setCellData(cells->getId());
      const ImportReferenceFile file(source, "MalformedCells");
      DataStructure destination;
      destination.setNextId(100);
      auto result = ImportH5ObjectPathsAction(file.path, source.getAllDataPaths()).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* imported = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
      const auto* importedCells = destination.getData(DataPath({"Image", "Cells"}));
      REQUIRE(imported != nullptr);
      REQUIRE(importedCells != nullptr);
      REQUIRE(imported->getCellDataId() == importedCells->getId());
      const auto validation = imported->validate();
      REQUIRE(validation.invalid());
      REQUIRE(validation.errors().front().code == -4501);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }
    SECTION("Selected wrong-type target remains referenced")
    {
      DataStructure source;
      auto* geometry = ImageGeom::Create(source, "Image");
      REQUIRE(geometry != nullptr);
      auto* wrongType = DataGroup::Create(source, "WrongType", geometry->getId());
      REQUIRE(wrongType != nullptr);
      geometry->setCellData(wrongType->getId());
      const ImportReferenceFile file(source, "WrongType");
      DataStructure destination;
      destination.setNextId(100);
      auto result = ImportH5ObjectPathsAction(file.path, source.getAllDataPaths()).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* imported = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
      const auto* importedTarget = destination.getData(DataPath({"Image", "WrongType"}));
      REQUIRE(imported != nullptr);
      REQUIRE(importedTarget != nullptr);
      REQUIRE(imported->getCellDataId() == importedTarget->getId());
      REQUIRE(dynamic_cast<const AttributeMatrix*>(importedTarget) == nullptr);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }
    SECTION("Omitted vertices do not suppress vertex-matrix validation")
    {
      DataStructure source;
      auto* geometry = VertexGeom::Create(source, "Vertex");
      REQUIRE(geometry != nullptr);
      geometry->setVertexListId(CreateReferenceArray<float32>(source, "Vertices", geometry->getId(), 3));
      auto* vertexData = AttributeMatrix::Create(source, "VertexData", {1}, geometry->getId());
      REQUIRE(vertexData != nullptr);
      geometry->setVertexDataId(vertexData->getId());
      const ImportReferenceFile file(source, "MissingVertices");
      DataStructure destination;
      auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"Vertex"}), DataPath({"Vertex", "VertexData"})}).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* imported = destination.getDataAs<VertexGeom>(DataPath({"Vertex"}));
      const auto* importedData = destination.getData(DataPath({"Vertex", "VertexData"}));
      REQUIRE(imported != nullptr);
      REQUIRE(importedData != nullptr);
      REQUIRE_FALSE(imported->getVertexListId().has_value());
      REQUIRE(imported->getVertexAttributeMatrixId() == importedData->getId());
      const auto validation = imported->validate();
      REQUIRE(validation.invalid());
      REQUIRE(validation.errors().front().code == -4500);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }
  }
}

TEST_CASE("Selective import references: selection errors and group shells stay compatible", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure source;
  auto* group = DataGroup::Create(source, "Group");
  REQUIRE(group != nullptr);
  CreateReferenceArray<float32>(source, "Child", group->getId());
  const ImportReferenceFile file(source, "Compatibility");
  const DataPath parentPath({"Group"});
  const DataPath childPath({"Group", "Child"});
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    DataStructure destination;
    SECTION("Duplicate selection")
    {
      auto result = ImportH5ObjectPathsAction(file.path, {parentPath, parentPath}).apply(destination, mode);
      REQUIRE(result.invalid());
      REQUIRE(result.errors().front().code == -6203);
    }
    SECTION("Empty selection")
    {
      auto result = ImportH5ObjectPathsAction(file.path, {}).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      REQUIRE(destination.getAllDataPaths().empty());
    }
    SECTION("Missing parent")
    {
      auto result = ImportH5ObjectPathsAction(file.path, {childPath}).apply(destination, mode);
      REQUIRE(result.invalid());
      REQUIRE(result.errors().front().code == -6202);
    }
    SECTION("Existing target")
    {
      REQUIRE(DataGroup::Create(destination, "Group") != nullptr);
      auto result = ImportH5ObjectPathsAction(file.path, {parentPath}).apply(destination, mode);
      REQUIRE(result.invalid());
      REQUIRE(result.errors().front().code == -6203);
    }
    SECTION("Group shell")
    {
      auto result = ImportH5ObjectPathsAction(file.path, {parentPath}).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      REQUIRE(destination.containsData(parentPath));
      REQUIRE_FALSE(destination.containsData(childPath));
    }
    UnitTest::CheckArraysInheritTupleDims(destination);
  }
}

namespace
{
constexpr std::array<int32, 4> k_AliasValues{7, -3, 0, 41};
constexpr std::array<int32, 2> k_LaterAliasValues{901, -112};

struct AliasPlanRecord
{
  DataPath path;
  DataType type;
  uint64 bytes = 0;
};

class AliasPathResolver : public IDataStoreFormatResolver
{
public:
  AliasPathResolver() = default;
  ~AliasPathResolver() override = default;
  AliasPathResolver(const AliasPathResolver&) = default;
  AliasPathResolver(AliasPathResolver&&) = default;
  AliasPathResolver& operator=(const AliasPathResolver&) = default;
  AliasPathResolver& operator=(AliasPathResolver&&) = default;

  mutable std::vector<AliasPlanRecord> records;

  std::string resolveFormat(const DataStructure&, const DataPath& path, DataType type, uint64 bytes) const override
  {
    records.push_back({path, type, bytes});
    return {};
  }
};

template <usize Count>
Int32Array* CreateAliasArray(DataStructure& structure, const std::string& name, DataObject::IdType parent, const std::array<int32, Count>& values)
{
  auto store = std::make_shared<DataStore<int32>>(ShapeType{Count}, ShapeType{1}, int32{0});
  for(usize valueIdx = 0; valueIdx < Count; ++valueIdx)
  {
    store->setValue(valueIdx, values[valueIdx]);
  }
  auto* array = Int32Array::Create(structure, name, std::move(store), parent);
  REQUIRE(array != nullptr);
  return array;
}

void RequireAliasParents(const DataObject& object, std::vector<DataObject::IdType> expected)
{
  const auto parents = object.getParentIds();
  std::vector<DataObject::IdType> actual(parents.begin(), parents.end());
  std::sort(actual.begin(), actual.end());
  std::sort(expected.begin(), expected.end());
  REQUIRE(actual == expected);
}

/**
 * @brief Validates the closed source through a fixed-size independent HDF5 read.
 * @tparam Count Fixes the permitted value count and buffer size.
 * @param file Supplies an independently reopened source.
 * @param path Identifies the numeric dataset below DataStructure.
 * @param expected Supplies the literal value oracle.
 */
template <usize Count>
void RequireAliasFileValues(const HDF5::FileIO& file, const DataPath& path, const std::array<int32, Count>& expected)
{
  const auto fileId = file.getId();
  const std::lock_guard lock(HDF5::Support::ApiLock());
  const auto datasetPath = "/DataStructure/" + path.toString();
  const hid_t dataset = H5Dopen2(fileId, datasetPath.c_str(), H5P_DEFAULT);
  REQUIRE(dataset >= 0);
  const auto datasetGuard = MakeScopeGuard([dataset]() noexcept { H5Dclose(dataset); });
  const hid_t space = H5Dget_space(dataset);
  REQUIRE(space >= 0);
  const auto spaceGuard = MakeScopeGuard([space]() noexcept { H5Sclose(space); });
  REQUIRE(H5Sget_simple_extent_ndims(space) == 2);
  REQUIRE(H5Sget_simple_extent_npoints(space) == static_cast<hssize_t>(Count));
  std::array<hsize_t, 2> shape{};
  REQUIRE(H5Sget_simple_extent_dims(space, shape.data(), nullptr) == 2);
  REQUIRE(shape == std::array<hsize_t, 2>{Count, 1});
  const hid_t type = H5Dget_type(dataset);
  REQUIRE(type >= 0);
  const auto typeGuard = MakeScopeGuard([type]() noexcept { H5Tclose(type); });
  REQUIRE(H5Tequal(type, H5T_NATIVE_INT32) > 0);
  std::array<int32, Count> actual{};
  REQUIRE(H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, actual.data()) >= 0);
  REQUIRE(actual == expected);
}

template <usize Count>
const Int32Array* RequireAliasOutput(const DataStructure& structure, const DataPath& path, IDataAction::Mode mode, const std::array<int32, Count>& values)
{
  const auto* array = structure.getDataAs<Int32Array>(path);
  REQUIRE(array != nullptr);
  REQUIRE(structure.getData(array->getId()) == array);
  REQUIRE(array->getTupleShape() == ShapeType{Count});
  REQUIRE(array->getComponentShape() == ShapeType{1});
  REQUIRE(array->size() == Count);
  REQUIRE(array->getIDataStore() != nullptr);
  CHECK(array->getDataFormat().empty());
  CHECK(array->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  if(mode == IDataAction::Mode::Preflight)
  {
    CHECK(array->getStoreType() == IDataStore::StoreType::Empty);
  }
  else
  {
    REQUIRE(array->getStoreType() == IDataStore::StoreType::InMemory);
    for(usize valueIdx = 0; valueIdx < Count; ++valueIdx)
    {
      CHECK((*array)[valueIdx] == values[valueIdx]);
    }
  }
  return array;
}

void RequireAliasPlans(const AliasPathResolver& resolver, const std::vector<std::pair<DataPath, uint64>>& expected)
{
  REQUIRE(resolver.records.size() == expected.size());
  for(const auto& expectedRecord : expected)
  {
    CAPTURE(expectedRecord.first.toString());
    CHECK(std::count_if(resolver.records.begin(), resolver.records.end(), [&](const AliasPlanRecord& record) { return record.path == expectedRecord.first; }) == 1);
  }
  for(const auto& record : resolver.records)
  {
    const auto expectedRecord = std::find_if(expected.begin(), expected.end(), [&](const auto& item) { return item.first == record.path; });
    REQUIRE(expectedRecord != expected.end());
    CHECK(record.type == DataType::int32);
    CHECK(record.bytes == expectedRecord->second);
  }
}

void RequireAliasPathIds(const DataStructure& structure, const std::vector<DataPath>& expectedPaths)
{
  REQUIRE(structure.getAllDataPaths().size() == expectedPaths.size());
  const auto ids = structure.getAllDataObjectIds();
  REQUIRE(ids.size() == expectedPaths.size());
  std::set<DataObject::IdType> seen;
  for(const auto& path : expectedPaths)
  {
    const auto* object = structure.getData(path);
    REQUIRE(object != nullptr);
    CHECK(structure.getData(object->getId()) == object);
    CHECK(seen.insert(object->getId()).second);
  }
}
} // namespace

TEST_CASE("C8 distinct paths to a shared array retain independent destination copies", "[C8][SelectedImportAliases][SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool warm = GENERATE(false, true);
  CAPTURE(static_cast<int>(mode), warm);
  const auto memory = std::make_shared<InMemoryFormatResolver>();
  const DataPath firstPath({"First"});
  const DataPath secondPath({"Second"});
  const DataPath firstArrayPath({"First", "Shared"});
  const DataPath secondArrayPath({"Second", "Shared"});
  const DataPath laterGroupPath({"Second", "LaterGroup"});
  const DataPath laterArrayPath({"Second", "LaterGroup", "Later"});
  DataStructure source;
  source.setFormatResolver(memory);
  auto* first = DataGroup::Create(source, "First");
  auto* second = DataGroup::Create(source, "Second");
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  auto* shared = CreateAliasArray(source, "Shared", first->getId(), k_AliasValues);
  REQUIRE(source.setAdditionalParent(shared->getId(), second->getId()));
  // The deeper distinct object must keep its reserved source ID when an earlier alias is split.
  auto* laterGroup = DataGroup::Create(source, "LaterGroup", second->getId());
  REQUIRE(laterGroup != nullptr);
  auto* later = CreateAliasArray(source, "Later", laterGroup->getId(), k_LaterAliasValues);
  REQUIRE(first->getId() == 1);
  REQUIRE(second->getId() == 2);
  REQUIRE(shared->getId() == 3);
  REQUIRE(laterGroup->getId() == 4);
  REQUIRE(later->getId() == 5);
  REQUIRE(source.getData(firstArrayPath) == source.getData(secondArrayPath));
  RequireAliasParents(*shared, {first->getId(), second->getId()});
  const ImportReferenceFile file(source, "SharedArrayAliases");
  {
    const auto reader = HDF5::FileIO::ReadFile(file.path);
    REQUIRE(reader.isValid());
    RequireAliasFileValues(reader, firstArrayPath, k_AliasValues);
    RequireAliasFileValues(reader, secondArrayPath, k_AliasValues);
    RequireAliasFileValues(reader, laterArrayPath, k_LaterAliasValues);
  }
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  auto metadata = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  REQUIRE(metadata.value().fileVersion == DREAM3D::k_CurrentFileVersion);
  const auto& sourceHandout = metadata.value().dataStructure;
  const auto* sourceFirst = sourceHandout.getDataAs<Int32Array>(firstArrayPath);
  const auto* sourceSecond = sourceHandout.getDataAs<Int32Array>(secondArrayPath);
  REQUIRE(sourceFirst != nullptr);
  REQUIRE(sourceFirst == sourceSecond);
  REQUIRE(sourceFirst->getId() == shared->getId());
  REQUIRE(sourceHandout.getData(sourceFirst->getId()) == sourceFirst);
  RequireAliasParents(*sourceFirst, {first->getId(), second->getId()});
  const auto sourceNext = sourceHandout.getNextId();
  REQUIRE(sourceNext == 6);
  REQUIRE(sourceHandout.getAllDataObjectIds().size() == 5);
  REQUIRE(sourceHandout.getAllDataPaths().size() == 6);
  const auto* sourceLater = sourceHandout.getDataAs<Int32Array>(laterArrayPath);
  REQUIRE(sourceLater != nullptr);
  REQUIRE(sourceLater->getId() == later->getId());
  REQUIRE(sourceHandout.getData(sourceLater->getId()) == sourceLater);
  // Evaluate to bool before Catch2 tries to stream libc++'s 128-bit file-clock duration.
  REQUIRE((fs::file_time_type::clock::now() - fs::last_write_time(file.path) >= DREAM3D::Dream3dPreflightCache::k_MtimeTrustWindow));
  cache.invalidate(file.path);
  cache.resetStats();
  DataStructure warmDestination;
  warmDestination.setFormatResolver(memory);
  if(warm)
  {
    // A valid single-placement action warms the same metadata mode before the alias regression is measured.
    const auto warmResult = ImportH5ObjectPathsAction(file.path, {firstPath, firstArrayPath}).apply(warmDestination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(warmResult);
    RequireAliasOutput(warmDestination, firstArrayPath, mode, k_AliasValues);
    REQUIRE(warmDestination.getAllDataPaths().size() == 2);
    REQUIRE(cache.missCount() == 1);
    REQUIRE(cache.hitCount() == 0);
    cache.resetStats();
  }
  REQUIRE(cache.missCount() == 0);
  REQUIRE(cache.hitCount() == 0);
  DataStructure destination;
  const auto resolver = std::make_shared<AliasPathResolver>();
  destination.setFormatResolver(resolver);
  const auto initialNextId = destination.getNextId();
  const std::vector<DataPath> selection{firstPath, secondPath, firstArrayPath, secondArrayPath, laterGroupPath, laterArrayPath};
  const auto result = ImportH5ObjectPathsAction(file.path, selection).apply(destination, mode);
  CHECK(cache.missCount() == (warm ? 0 : 1));
  CHECK(cache.hitCount() == (warm ? 1 : 0));
  if(result.invalid())
  {
    CHECK(destination.getAllDataPaths().empty());
    CHECK(destination.getAllDataObjectIds().empty());
    CHECK(destination.getNextId() == initialNextId);
  }
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  RequireAliasPathIds(destination, selection);
  const auto* firstCopy = RequireAliasOutput(destination, firstArrayPath, mode, k_AliasValues);
  const auto* secondCopy = RequireAliasOutput(destination, secondArrayPath, mode, k_AliasValues);
  const auto* laterCopy = RequireAliasOutput(destination, laterArrayPath, mode, k_LaterAliasValues);
  CHECK(firstCopy != secondCopy);
  CHECK(firstCopy->getId() != secondCopy->getId());
  CHECK(firstCopy->getIDataStore() != secondCopy->getIDataStore());
  CHECK(std::min(firstCopy->getId(), secondCopy->getId()) == shared->getId());
  CHECK(std::max(firstCopy->getId(), secondCopy->getId()) == sourceNext);
  CHECK(laterCopy->getId() == later->getId());
  REQUIRE(destination.getData(laterGroupPath) != nullptr);
  CHECK(destination.getData(laterGroupPath)->getId() == laterGroup->getId());
  CHECK(destination.getNextId() == sourceNext + 1);
  RequireAliasParents(*firstCopy, {destination.getData(firstPath)->getId()});
  RequireAliasParents(*secondCopy, {destination.getData(secondPath)->getId()});
  RequireAliasParents(*laterCopy, {destination.getData(laterGroupPath)->getId()});
  RequireAliasPlans(*resolver, {{firstArrayPath, 16}, {secondArrayPath, 16}, {laterArrayPath, 8}});
  CHECK(sourceHandout.getData(firstArrayPath) == sourceHandout.getData(secondArrayPath));
  CHECK(sourceHandout.getData(firstArrayPath)->getId() == shared->getId());
  CHECK(sourceHandout.getNextId() == sourceNext);
  if(warm)
  {
    const auto* warmArray = RequireAliasOutput(warmDestination, firstArrayPath, mode, k_AliasValues);
    CHECK(warmArray != firstCopy);
    CHECK(warmArray->getIDataStore() != firstCopy->getIDataStore());
  }
  UnitTest::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("C8 shared group paths retain independent destination groups and children", "[C8][SelectedImportAliases][SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool warm = GENERATE(false, true);
  CAPTURE(static_cast<int>(mode), warm);
  const auto memory = std::make_shared<InMemoryFormatResolver>();
  const DataPath firstPath({"First"});
  const DataPath secondPath({"Second"});
  const DataPath firstGroupPath({"First", "SharedGroup"});
  const DataPath secondGroupPath({"Second", "SharedGroup"});
  const DataPath firstChildPath({"First", "SharedGroup", "Child"});
  const DataPath secondChildPath({"Second", "SharedGroup", "Child"});
  DataStructure source;
  source.setFormatResolver(memory);
  auto* first = DataGroup::Create(source, "First");
  auto* second = DataGroup::Create(source, "Second");
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  auto* sharedGroup = DataGroup::Create(source, "SharedGroup", first->getId());
  REQUIRE(sharedGroup != nullptr);
  REQUIRE(source.setAdditionalParent(sharedGroup->getId(), second->getId()));
  auto* child = CreateAliasArray(source, "Child", sharedGroup->getId(), k_AliasValues);
  REQUIRE(first->getId() == 1);
  REQUIRE(second->getId() == 2);
  REQUIRE(sharedGroup->getId() == 3);
  REQUIRE(child->getId() == 4);
  REQUIRE(source.getData(firstGroupPath) == source.getData(secondGroupPath));
  REQUIRE(source.getData(firstChildPath) == source.getData(secondChildPath));
  RequireAliasParents(*sharedGroup, {first->getId(), second->getId()});
  // The child has one direct parent and inherits two paths through that parent's placements.
  RequireAliasParents(*child, {sharedGroup->getId()});
  const ImportReferenceFile file(source, "SharedGroupAliases");
  {
    const auto reader = HDF5::FileIO::ReadFile(file.path);
    REQUIRE(reader.isValid());
    RequireAliasFileValues(reader, firstChildPath, k_AliasValues);
    RequireAliasFileValues(reader, secondChildPath, k_AliasValues);
  }
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  auto metadata = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  const auto& sourceHandout = metadata.value().dataStructure;
  const auto* sourceGroup = sourceHandout.getDataAs<DataGroup>(firstGroupPath);
  const auto* sourceChild = sourceHandout.getDataAs<Int32Array>(firstChildPath);
  REQUIRE(sourceGroup != nullptr);
  REQUIRE(sourceChild != nullptr);
  REQUIRE(sourceGroup == sourceHandout.getData(secondGroupPath));
  REQUIRE(sourceChild == sourceHandout.getData(secondChildPath));
  REQUIRE(sourceGroup->getId() == sharedGroup->getId());
  REQUIRE(sourceChild->getId() == child->getId());
  RequireAliasParents(*sourceGroup, {first->getId(), second->getId()});
  RequireAliasParents(*sourceChild, {sourceGroup->getId()});
  const auto sourceNext = sourceHandout.getNextId();
  REQUIRE(sourceNext == 5);
  REQUIRE(sourceHandout.getAllDataObjectIds().size() == 4);
  REQUIRE(sourceHandout.getAllDataPaths().size() == 6);
  // Evaluate to bool before Catch2 tries to stream libc++'s 128-bit file-clock duration.
  REQUIRE((fs::file_time_type::clock::now() - fs::last_write_time(file.path) >= DREAM3D::Dream3dPreflightCache::k_MtimeTrustWindow));
  cache.invalidate(file.path);
  cache.resetStats();
  DataStructure warmDestination;
  warmDestination.setFormatResolver(memory);
  if(warm)
  {
    const auto warmResult = ImportH5ObjectPathsAction(file.path, {firstPath, firstGroupPath, firstChildPath}).apply(warmDestination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(warmResult);
    RequireAliasOutput(warmDestination, firstChildPath, mode, k_AliasValues);
    REQUIRE(warmDestination.getAllDataPaths().size() == 3);
    REQUIRE(cache.missCount() == 1);
    REQUIRE(cache.hitCount() == 0);
    cache.resetStats();
  }
  REQUIRE(cache.missCount() == 0);
  REQUIRE(cache.hitCount() == 0);
  DataStructure destination;
  const auto resolver = std::make_shared<AliasPathResolver>();
  destination.setFormatResolver(resolver);
  const auto initialNextId = destination.getNextId();
  const std::vector<DataPath> selection{firstPath, secondPath, firstGroupPath, secondGroupPath, firstChildPath, secondChildPath};
  const auto result = ImportH5ObjectPathsAction(file.path, selection).apply(destination, mode);
  CHECK(cache.missCount() == (warm ? 0 : 1));
  CHECK(cache.hitCount() == (warm ? 1 : 0));
  if(result.invalid())
  {
    CHECK(destination.getAllDataPaths().empty());
    CHECK(destination.getAllDataObjectIds().empty());
    CHECK(destination.getNextId() == initialNextId);
  }
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  RequireAliasPathIds(destination, selection);
  const auto* firstGroup = destination.getDataAs<DataGroup>(firstGroupPath);
  const auto* secondGroup = destination.getDataAs<DataGroup>(secondGroupPath);
  REQUIRE(firstGroup != nullptr);
  REQUIRE(secondGroup != nullptr);
  CHECK(firstGroup != secondGroup);
  CHECK(firstGroup->getId() != secondGroup->getId());
  CHECK(std::min(firstGroup->getId(), secondGroup->getId()) == sharedGroup->getId());
  CHECK(std::max(firstGroup->getId(), secondGroup->getId()) == sourceNext);
  CHECK(firstGroup->getSize() == 1);
  CHECK(secondGroup->getSize() == 1);
  const auto* firstChild = RequireAliasOutput(destination, firstChildPath, mode, k_AliasValues);
  const auto* secondChild = RequireAliasOutput(destination, secondChildPath, mode, k_AliasValues);
  CHECK(firstChild != secondChild);
  CHECK(firstChild->getId() != secondChild->getId());
  CHECK(firstChild->getIDataStore() != secondChild->getIDataStore());
  CHECK(std::min(firstChild->getId(), secondChild->getId()) == child->getId());
  CHECK(std::max(firstChild->getId(), secondChild->getId()) == sourceNext + 1);
  CHECK(destination.getNextId() == sourceNext + 2);
  RequireAliasParents(*firstGroup, {destination.getData(firstPath)->getId()});
  RequireAliasParents(*secondGroup, {destination.getData(secondPath)->getId()});
  RequireAliasParents(*firstChild, {firstGroup->getId()});
  RequireAliasParents(*secondChild, {secondGroup->getId()});
  RequireAliasPlans(*resolver, {{firstChildPath, 16}, {secondChildPath, 16}});
  CHECK(sourceHandout.getData(firstGroupPath) == sourceHandout.getData(secondGroupPath));
  CHECK(sourceHandout.getData(firstChildPath) == sourceHandout.getData(secondChildPath));
  CHECK(sourceHandout.getData(firstGroupPath)->getId() == sharedGroup->getId());
  CHECK(sourceHandout.getData(firstChildPath)->getId() == child->getId());
  if(warm)
  {
    const auto* warmChild = RequireAliasOutput(warmDestination, firstChildPath, mode, k_AliasValues);
    CHECK(warmChild != firstChild);
    CHECK(warmChild->getIDataStore() != firstChild->getIDataStore());
  }
  UnitTest::CheckArraysInheritTupleDims(destination);
}

namespace
{
DataStructure MakeAliasControlSource()
{
  DataStructure source;
  source.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* first = DataGroup::Create(source, "First");
  auto* second = DataGroup::Create(source, "Second");
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  auto* shared = CreateAliasArray(source, "Shared", first->getId(), k_AliasValues);
  REQUIRE(source.setAdditionalParent(shared->getId(), second->getId()));
  REQUIRE(source.getAllDataObjectIds().size() == 3);
  REQUIRE(source.getNextId() == 4);
  REQUIRE(source.getData(DataPath({"First", "Shared"})) == source.getData(DataPath({"Second", "Shared"})));
  RequireAliasParents(*shared, {first->getId(), second->getId()});
  return source;
}

const std::vector<DataPath> k_AliasControlSelection{DataPath({"First"}), DataPath({"Second"}), DataPath({"First", "Shared"}), DataPath({"Second", "Shared"})};

void RequireAliasControlFile(const ImportReferenceFile& file)
{
  const auto reader = HDF5::FileIO::ReadFile(file.path);
  REQUIRE(reader.isValid());
  RequireAliasFileValues(reader, DataPath({"First", "Shared"}), k_AliasValues);
  RequireAliasFileValues(reader, DataPath({"Second", "Shared"}), k_AliasValues);
}
} // namespace

TEST_CASE("C8 alias identifier capacity includes every extra placement", "[C8][SelectedImportAliases]")
{
  const auto application = Application::GetOrCreateInstance();
  const bool fits = GENERATE(false, true);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(fits, static_cast<int>(mode));
  auto source = MakeAliasControlSource();
  const ImportReferenceFile file(source, "AliasIdentifierCapacity");
  RequireAliasControlFile(file);
  constexpr auto maximumId = std::numeric_limits<DataObject::IdType>::max();
  const auto startId = maximumId - (fits ? 4 : 3);
  DataStructure destination;
  destination.setNextId(startId);
  const auto resolver = std::make_shared<AliasPathResolver>();
  destination.setFormatResolver(resolver);
  const auto result = ImportH5ObjectPathsAction(file.path, k_AliasControlSelection).apply(destination, mode);
  if(!fits)
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -6213);
    CHECK(result.errors()[0].message.find("additional placement count 1") != std::string::npos);
    CHECK(result.errors()[0].message.find(std::to_string(startId)) != std::string::npos);
    CHECK(destination.getNextId() == startId);
    CHECK(destination.getAllDataPaths().empty());
    CHECK(destination.getAllDataObjectIds().empty());
    CHECK(resolver->records.empty());
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    RequireAliasPathIds(destination, k_AliasControlSelection);
    const auto* first = RequireAliasOutput(destination, DataPath({"First", "Shared"}), mode, k_AliasValues);
    const auto* second = RequireAliasOutput(destination, DataPath({"Second", "Shared"}), mode, k_AliasValues);
    CHECK(first != second);
    CHECK(first->getIDataStore() != second->getIDataStore());
    CHECK(std::min(first->getId(), second->getId()) == maximumId - 2);
    CHECK(std::max(first->getId(), second->getId()) == maximumId - 1);
    CHECK(destination.getNextId() == maximumId);
    RequireAliasPlans(*resolver, {{DataPath({"First", "Shared"}), 16}, {DataPath({"Second", "Shared"}), 16}});
  }
  UnitTest::CheckArraysInheritTupleDims(destination);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
TEST_CASE("C8 alias clone IDs preserve live expired and hierarchy-only occupants", "[C8][SelectedImportAliases]")
{
  const auto application = Application::GetOrCreateInstance();
  const int occupantKind = GENERATE(0, 1, 2);
  CAPTURE(occupantKind);
  auto source = MakeAliasControlSource();
  const ImportReferenceFile file(source, "AliasOccupiedCloneId");
  RequireAliasControlFile(file);
  const auto cloneId = source.getNextId();
  REQUIRE(cloneId == 4);
  DataStructure destination;
  std::unique_ptr<ImportReferenceFile> retainedFile;
  std::unique_ptr<ImportH5ObjectPathsAction> retainingAction;
  const DataPath occupiedPath = occupantKind == 2 ? DataPath({"RetainedParent", "RetainedCloneId"}) : DataPath({"OccupiedCloneId"});
  if(occupantKind == 2)
  {
    // The existing high-ID parent keeps the retained child outside the registry without relying on sibling order.
    auto* retainedParent = DataGroup::Import(destination, "RetainedParent", 100);
    REQUIRE(retainedParent != nullptr);
    destination.setNextId(1);
    DataStructure retentionSource;
    auto* sourceParent = DataGroup::Create(retentionSource, "RetainedParent");
    REQUIRE(sourceParent != nullptr);
    REQUIRE(DataGroup::Create(retentionSource, "Unselected") != nullptr);
    REQUIRE(DataGroup::Create(retentionSource, "Earlier") != nullptr);
    auto* retained = DataGroup::Create(retentionSource, "RetainedCloneId", sourceParent->getId());
    REQUIRE(retained != nullptr);
    REQUIRE(retained->getId() == cloneId);
    retainedFile = std::make_unique<ImportReferenceFile>(retentionSource, "AliasHierarchyOnlyId");
    retainingAction = std::make_unique<ImportH5ObjectPathsAction>(retainedFile->path, std::vector<DataPath>{DataPath({"Earlier"}), occupiedPath});
    retainingAction->setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFaultType::AfterHierarchy, true);
    const auto retainedResult = retainingAction->apply(destination, IDataAction::Mode::Preflight);
    REQUIRE(retainedResult.invalid());
    REQUIRE(retainingAction->publicationFaultConsumedForTesting());
    REQUIRE(retainingAction->cleanupFaultConsumedForTesting());
    REQUIRE(destination.getData(occupiedPath) != nullptr);
    REQUIRE(destination.getData(occupiedPath)->getId() == cloneId);
    REQUIRE(destination.getData(cloneId) == nullptr);
    REQUIRE(destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{100});
    REQUIRE(destination.getData(DataPath({"RetainedParent"})) == retainedParent);
    const auto entry = retainedParent->getDataMap().find(cloneId);
    REQUIRE(entry != retainedParent->getDataMap().end());
    REQUIRE(entry->second.get() == destination.getData(occupiedPath));
  }
  else
  {
    auto* occupied = DataGroup::Import(destination, "OccupiedCloneId", cloneId);
    REQUIRE(occupied != nullptr);
    REQUIRE(occupied->getId() == cloneId);
    if(occupantKind == 1)
    {
      REQUIRE(destination.removeData(cloneId));
      REQUIRE(destination.getData(cloneId) == nullptr);
      REQUIRE(destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{cloneId});
    }
  }
  const auto* originalOwner = destination.getData(occupiedPath);
  const auto beforeIds = destination.getAllDataObjectIds();
  const auto beforePaths = destination.getAllDataPaths();
  destination.setNextId(1);
  const auto resolver = std::make_shared<AliasPathResolver>();
  destination.setFormatResolver(resolver);
  const auto result = ImportH5ObjectPathsAction(file.path, k_AliasControlSelection).apply(destination, IDataAction::Mode::Preflight);
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -6213);
  CHECK(result.errors()[0].message.find("identifier 4 conflicts with an existing or selected object") != std::string::npos);
  CHECK(resolver->records.empty());
  CHECK(destination.getData(occupiedPath) == originalOwner);
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getAllDataPaths() == beforePaths);
  CHECK(destination.getNextId() == 1);
  if(retainingAction != nullptr)
  {
    const auto cleaned = retainingAction->cleanupFailedImportForTesting(destination);
    SIMPLNX_RESULT_REQUIRE_VALID(cleaned);
    CHECK(destination.getData(occupiedPath) == nullptr);
    const auto* parent = destination.getDataAs<DataGroup>(DataPath({"RetainedParent"}));
    REQUIRE(parent != nullptr);
    CHECK(parent->empty());
    CHECK(destination.getData(100) == parent);
  }
}

TEST_CASE("C8 alias publication rollback removes both copies and restores identifiers", "[C8][SelectedImportAliases][SelectedImportPublication]")
{
  const auto application = Application::GetOrCreateInstance();
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  CAPTURE(static_cast<int>(mode));
  auto source = MakeAliasControlSource();
  const ImportReferenceFile file(source, "AliasPublicationRollback");
  RequireAliasControlFile(file);
  const DataPath firstPath({"First"});
  const DataPath secondPath({"Second"});
  const DataPath firstArrayPath({"First", "Shared"});
  const DataPath secondArrayPath({"Second", "Shared"});
  const DataPath markerPath({"First", "Existing"});
  DataStructure destination;
  auto* firstParent = DataGroup::Create(destination, "First");
  auto* secondParent = DataGroup::Create(destination, "Second");
  REQUIRE(firstParent != nullptr);
  REQUIRE(secondParent != nullptr);
  auto* marker = CreateAliasArray(destination, "Existing", firstParent->getId(), k_LaterAliasValues);
  const auto* markerStore = marker->getIDataStore();
  const auto beforeIds = destination.getAllDataObjectIds();
  const auto beforePaths = destination.getAllDataPaths();
  const auto beforeNext = destination.getNextId();
  const auto originalCopyId = beforeNext + 2;
  const auto cloneId = beforeNext + 3;
  const auto resolver = std::make_shared<AliasPathResolver>();
  destination.setFormatResolver(resolver);
  std::vector<DataObject::IdType> removals;
  nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() == DataRemovedMessage::MsgType)
    {
      removals.push_back(std::static_pointer_cast<DataRemovedMessage>(message)->getId());
    }
  }));
  const std::vector<DataPath> leaves{firstArrayPath, secondArrayPath};
  ImportH5ObjectPathsAction action(file.path, leaves);
  action.setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFaultType::AfterSecondInsert);
  const auto result = action.apply(destination, mode);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors()[0].code == -6214);
  REQUIRE(action.publicationFaultConsumedForTesting());
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getAllDataPaths() == beforePaths);
  CHECK(destination.getNextId() == beforeNext);
  CHECK(destination.getData(firstPath) == firstParent);
  CHECK(destination.getData(secondPath) == secondParent);
  CHECK(firstParent->getSize() == 1);
  CHECK(secondParent->empty());
  CHECK(destination.getData(markerPath) == marker);
  CHECK(marker->getIDataStore() == markerStore);
  RequireAliasOutput(destination, markerPath, IDataAction::Mode::Execute, k_LaterAliasValues);
  RequireAliasPlans(*resolver, {{firstArrayPath, 16}, {secondArrayPath, 16}});
  std::sort(removals.begin(), removals.end());
  CHECK(removals == std::vector<DataObject::IdType>{originalCopyId, cloneId});
  auto retained = action.retainedOwnerForTesting();
  REQUIRE(retained != nullptr);
  CHECK(retained->getId() == cloneId);
  CHECK(retained->getDataStructure() == nullptr);
  resolver->records.clear();
  const ImportH5ObjectPathsAction retry(file.path, leaves);
  const auto retried = retry.apply(destination, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(retried);
  const auto* firstCopy = RequireAliasOutput(destination, firstArrayPath, mode, k_AliasValues);
  const auto* secondCopy = RequireAliasOutput(destination, secondArrayPath, mode, k_AliasValues);
  CHECK(firstCopy != secondCopy);
  CHECK(firstCopy->getIDataStore() != secondCopy->getIDataStore());
  CHECK(std::min(firstCopy->getId(), secondCopy->getId()) == originalCopyId);
  CHECK(std::max(firstCopy->getId(), secondCopy->getId()) == cloneId);
  CHECK(destination.getData(cloneId) != retained.get());
  RequireAliasParents(*firstCopy, {firstParent->getId()});
  RequireAliasParents(*secondCopy, {secondParent->getId()});
  RequireAliasPlans(*resolver, {{firstArrayPath, 16}, {secondArrayPath, 16}});
  const auto removalCount = removals.size();
  retained.reset();
  action.setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFaultType::None);
  CHECK(removals.size() == removalCount);
  const auto nextId = destination.getNextId();
  REQUIRE(nextId == cloneId + 1);
  auto* next = DataGroup::Create(destination, "AfterAliasRetry");
  REQUIRE(next != nullptr);
  CHECK(next->getId() == nextId);
  CHECK(destination.getData(nextId) == next);
  CHECK(destination.getData(DataPath({"AfterAliasRetry"})) == next);
  CHECK(destination.getData(markerPath) == marker);
  CHECK(marker->getIDataStore() == markerStore);
  UnitTest::CheckArraysInheritTupleDims(destination);
}
#endif
