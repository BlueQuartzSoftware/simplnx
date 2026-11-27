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
    dream3d::Dream3dPreflightCache::Instance().invalidate(path);
    auto result = dream3d::WriteFile(path, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    // A stable timestamp permits cache-hit assertions without a timed wait.
    fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::seconds(10));
  }

  ~ImportReferenceFile()
  {
    dream3d::Dream3dPreflightCache::Instance().invalidate(path);
    std::error_code error;
    fs::remove(path, error);
  }
};

template <typename T>
DataObject::IdType CreateReferenceArray(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId, usize components = 1)
{
  auto store = std::make_unique<DataStore<T>>(std::vector<usize>{1}, std::vector<usize>{components}, T{});
  auto* arrayPtr = DataArray<T>::Create(dataStructure, name, std::move(store), parentId);
  REQUIRE(arrayPtr != nullptr);
  return arrayPtr->getId();
}

/**
 * @struct ReferenceLink
 * @brief Associates a serialized reference getter with its independently named target path.
 */
struct ReferenceLink
{
  DataPath target_path;
  std::function<DataObject::OptionalIdType(const IGeometry&)> get_id;
};

std::vector<ReferenceLink> PopulateReferences(DataStructure& dataStructure, IGeometry& geometry)
{
  const auto parentId = geometry.getId();
  const DataPath geometryPath({geometry.getName()});
  std::vector<ReferenceLink> links;
  const auto add = [&](const std::string& name, auto getter) { links.push_back({geometryPath.createChildPath(name), getter}); };
  const auto matrix = [&](const std::string& name) {
    auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, name, {1}, parentId);
    REQUIRE(attributeMatrixPtr != nullptr);
    return attributeMatrixPtr->getId();
  };
  geometry.setElementSizesId(CreateReferenceArray<float32>(dataStructure, "Sizes", parentId));
  add("Sizes", [](const IGeometry& geom) { return geom.getElementSizesId(); });
  if(auto* gridPtr = dynamic_cast<IGridGeometry*>(&geometry); gridPtr != nullptr)
  {
    gridPtr->setCellData(matrix("Cells"));
    add("Cells", [](const IGeometry& geom) { return dynamic_cast<const IGridGeometry&>(geom).getCellDataId(); });
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry0D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setVertexListId(CreateReferenceArray<float32>(dataStructure, "Vertices", parentId, 3));
    add("Vertices", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry0D&>(geom).getVertexListId(); });
    nodePtr->setVertexDataId(matrix("VertexData"));
    add("VertexData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry0D&>(geom).getVertexAttributeMatrixId(); });
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry1D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setEdgeListId(CreateReferenceArray<uint64>(dataStructure, "Edges", parentId, 2));
    add("Edges", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getEdgeListId(); });
    nodePtr->setEdgeDataId(matrix("EdgeData"));
    add("EdgeData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getEdgeAttributeMatrixId(); });
    // DynamicListArray has no HDF5 factory. These two targets test ID membership without invoking cache consumers.
    nodePtr->setElementContainingVertId(CreateReferenceArray<uint64>(dataStructure, "ContainingVertices", parentId));
    add("ContainingVertices", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getElementContainingVertId(); });
    nodePtr->setElementNeighborsId(CreateReferenceArray<uint64>(dataStructure, "Neighbors", parentId));
    add("Neighbors", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getElementNeighborsId(); });
    nodePtr->setElementCentroidsId(CreateReferenceArray<float32>(dataStructure, "Centroids", parentId, 3));
    add("Centroids", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry1D&>(geom).getElementCentroidsId(); });
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry2D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setFaceListId(CreateReferenceArray<uint64>(dataStructure, "Faces", parentId, 3));
    add("Faces", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry2D&>(geom).getFaceListId(); });
    nodePtr->setFaceDataId(matrix("FaceData"));
    add("FaceData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry2D&>(geom).getFaceAttributeMatrixId(); });
    nodePtr->setUnsharedEdgesId(CreateReferenceArray<uint64>(dataStructure, "UnsharedEdges", parentId, 2));
    add("UnsharedEdges", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry2D&>(geom).getUnsharedEdgesId(); });
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry3D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setPolyhedronListId(CreateReferenceArray<uint64>(dataStructure, "Polyhedra", parentId, 4));
    add("Polyhedra", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry3D&>(geom).getPolyhedronListId(); });
    nodePtr->setPolyhedraDataId(matrix("PolyhedronData"));
    add("PolyhedronData", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry3D&>(geom).getPolyhedraAttributeMatrixId(); });
    nodePtr->setUnsharedFacedId(CreateReferenceArray<uint64>(dataStructure, "UnsharedFaces", parentId, 3));
    add("UnsharedFaces", [](const IGeometry& geom) { return dynamic_cast<const INodeGeometry3D&>(geom).getUnsharedFacesId(); });
  }
  if(auto* rectGridPtr = dynamic_cast<RectGridGeom*>(&geometry); rectGridPtr != nullptr)
  {
    const auto makeBounds = [&](const std::string& name) {
      auto store = std::make_unique<DataStore<float32>>(ShapeType{2}, ShapeType{1}, 0.0f);
      (*store)[1] = 1.0f;
      auto* arrayPtr = Float32ArrayType::Create(dataStructure, name, std::move(store), parentId);
      REQUIRE(arrayPtr != nullptr);
      return arrayPtr;
    };
    auto* xBoundsPtr = makeBounds("XBounds");
    auto* yBoundsPtr = makeBounds("YBounds");
    auto* zBoundsPtr = makeBounds("ZBounds");
    const auto result = rectGridPtr->setBounds(xBoundsPtr, yBoundsPtr, zBoundsPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    add("XBounds", [](const IGeometry& geom) { return dynamic_cast<const RectGridGeom&>(geom).getXBoundsId(); });
    add("YBounds", [](const IGeometry& geom) { return dynamic_cast<const RectGridGeom&>(geom).getYBoundsId(); });
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

  mutable std::atomic<usize> calls{0};

  std::string resolveFormat(const DataStructure& dataStructure, const DataPath& path, [[maybe_unused]] DataType dataType, [[maybe_unused]] uint64 byteSize) const override
  {
    ++calls;
    const auto geometryPath = path.getParent();
    const auto* geometryPtr = dataStructure.getDataAs<TriangleGeom>(geometryPath);
    const auto* verticesPtr = dataStructure.getDataAs<Float32ArrayType>(geometryPath.createChildPath("Vertices"));
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
  unit_test::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
  auto* verticesPtr = Float32ArrayType::Create(source, "Vertices", vertexStore, meshPtr->getId());
  auto* facesPtr = DataArray<IGeometry::MeshIndexType>::Create(source, "Faces", faceStore, meshPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  meshPtr->setVertices(*verticesPtr);
  meshPtr->setFaceList(*facesPtr);
  REQUIRE(source.setAdditionalParent(meshPtr->getId(), secondPtr->getId()));
  const ImportReferenceFile file(source, "GeometryAliases");
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  auto handout = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(handout);
  const auto* cachedMeshPtr = handout.value().data_structure.getDataAs<TriangleGeom>(DataPath({"First", "Mesh"}));
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
  CHECK(resolver->calls.load() == (bothPlacements ? 4 : 2));
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
    const auto* importedVerticesPtr = destination.getDataAs<Float32ArrayType>(meshPath.createChildPath("Vertices"));
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
  CHECK(handout.value().data_structure.getData(DataPath({"First", "Mesh"})) == handout.value().data_structure.getData(DataPath({"Second", "Mesh"})));
  CHECK(meshPtr->getVertexListId() == verticesPtr->getId());
  CHECK(meshPtr->getFaceListId() == facesPtr->getId());
  unit_test::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Selective import references: omitted local alias keeps a selected shared target", "[SelectiveImportReferences][GeometryAliases]")
{
  unit_test::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
  unit_test::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Selective import references: nested targets preserve explicit alias rules", "[SelectiveImportReferences][GeometryAliases]")
{
  unit_test::LoadPlugins();
  enum class Scenario
  {
    UniqueNested,
    AmbiguousNested,
    WrongDirectName,
    PrefixSibling,
    DirectChild
  };
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const auto scenario = GENERATE(Scenario::UniqueNested, Scenario::AmbiguousNested, Scenario::WrongDirectName, Scenario::PrefixSibling, Scenario::DirectChild);
  const bool reverse = GENERATE(false, true);
  CAPTURE(static_cast<int>(mode), static_cast<int>(scenario), reverse);
  DataStructure source;
  auto* sizesPtr = Float32ArrayType::Create(source, "Sizes", std::make_unique<DataStore<float32>>(ShapeType{1}, ShapeType{1}, 7.0F));
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
  const auto* expectedPtr = destination.getDataAs<Float32ArrayType>(expectedPath);
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
  unit_test::CheckArraysInheritTupleDims(destination);
}

TEST_CASE("Selective import references: many geometry aliases keep unique nested targets", "[SelectiveImportReferences][GeometryAliases]")
{
  unit_test::LoadPlugins();
  constexpr usize k_AliasCount = 128;
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
    const auto* targetPtr = destination.getDataAs<Float32ArrayType>(geometryPath.createChildPath("Nested").createChildPath("Sizes"));
    REQUIRE(importedPtr != nullptr);
    REQUIRE(targetPtr != nullptr);
    CAPTURE(geometryPath.toString());
    CHECK(importedPtr->getElementSizesId() == targetPtr->getId());
    CHECK(importedPtr->getElementSizes() == targetPtr);
    CHECK(geometryIds.insert(importedPtr->getId()).second);
    CHECK(targetIds.insert(targetPtr->getId()).second);
  }
  unit_test::CheckArraysInheritTupleDims(destination);
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
TEST_CASE("Selective import references: geometry alias publication rolls back and retries", "[SelectiveImportReferences][GeometryAliases]")
{
  unit_test::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
  unit_test::CheckArraysInheritTupleDims(destination);
}
#endif

TEST_CASE("Selective import references: overlapping ElementSizes reset", "[SelectiveImportReferences][DataStructure]")
{
  DataStructure source;
  source.setNextId(1);
  auto* geometryPtr = EdgeGeom::Create(source, "Geometry");
  REQUIRE(geometryPtr != nullptr);
  const auto sizesId = CreateReferenceArray<float32>(source, "Sizes", geometryPtr->getId());
  REQUIRE(sizesId == 2);
  geometryPtr->setElementSizesId(sizesId);
  REQUIRE(DataGroup::Create(source, "Third") != nullptr);
  auto* fourthPtr = DataGroup::Create(source, "Fourth");
  REQUIRE(fourthPtr != nullptr);
  REQUIRE(fourthPtr->getId() == 4);

  source.resetIds(3);
  const auto* sizesPtr = source.getData(DataPath({"Geometry", "Sizes"}));
  REQUIRE(sizesPtr != nullptr);
  REQUIRE(sizesPtr->getId() == 4);
  REQUIRE(geometryPtr->getElementSizesId() == sizesPtr->getId());
  unit_test::CheckArraysInheritTupleDims(source);
}

TEST_CASE("Selective import references: excluded cell ID cannot alias an ensemble", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    DataStructure source;
    source.setNextId(1);
    auto* geometryPtr = ImageGeom::Create(source, "Image");
    REQUIRE(geometryPtr != nullptr);
    geometryPtr->setDimensions({2, 3, 4});
    auto* cellsPtr = AttributeMatrix::Create(source, "CellData", {4, 3, 2}, geometryPtr->getId());
    auto* ensemblePtr = AttributeMatrix::Create(source, "CellEnsembleData", {2}, geometryPtr->getId());
    REQUIRE(cellsPtr != nullptr);
    REQUIRE(ensemblePtr != nullptr);
    REQUIRE(geometryPtr->getId() == 1);
    REQUIRE(cellsPtr->getId() == 2);
    REQUIRE(ensemblePtr->getId() == 3);
    geometryPtr->setCellData(cellsPtr->getId());
    const ImportReferenceFile file(source, "ImageCollision");
    DataStructure destination;
    destination.setNextId(1);
    auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"Image"}), DataPath({"Image", "CellEnsembleData"})}).apply(destination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto* importedPtr = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
    const auto* importedEnsemblePtr = destination.getDataAs<AttributeMatrix>(DataPath({"Image", "CellEnsembleData"}));
    REQUIRE(importedPtr != nullptr);
    REQUIRE(importedEnsemblePtr != nullptr);
    REQUIRE(importedEnsemblePtr->getId() == ensemblePtr->getId());
    REQUIRE_FALSE(importedPtr->getCellDataId().has_value());
    REQUIRE_FALSE(destination.containsData(DataPath({"Image", "CellData"})));
    REQUIRE(destination.getData(cellsPtr->getId()) == nullptr);
    const auto importedIds = destination.getAllDataObjectIds();
    REQUIRE(std::find(importedIds.begin(), importedIds.end(), cellsPtr->getId()) == importedIds.end());
    const auto importedNextId = destination.getNextId();
    // Reuse the excluded cell ID explicitly so a stale reference would resolve to an unrelated ensemble.
    auto* unrelatedPtr = AttributeMatrix::Import(destination, "UnrelatedEnsemble", ShapeType{2}, cellsPtr->getId(), importedPtr->getId());
    REQUIRE(unrelatedPtr != nullptr);
    REQUIRE(unrelatedPtr->getId() == cellsPtr->getId());
    REQUIRE(destination.getData(cellsPtr->getId()) == unrelatedPtr);
    REQUIRE(destination.getData(DataPath({"Image", "UnrelatedEnsemble"})) == unrelatedPtr);
    REQUIRE_FALSE(importedPtr->getCellDataId().has_value());
    REQUIRE(destination.getData(DataPath({"Image", "CellEnsembleData"})) == importedEnsemblePtr);
    REQUIRE(destination.getData(importedEnsemblePtr->getId()) == importedEnsemblePtr);
    REQUIRE(importedEnsemblePtr->getId() == ensemblePtr->getId());
    REQUIRE(importedEnsemblePtr->getShape() == ShapeType{2});
    REQUIRE(destination.getNextId() == importedNextId);
    REQUIRE(importedPtr->getDimensions() == geometryPtr->getDimensions());
    auto validation = importedPtr->validate();
    SIMPLNX_RESULT_REQUIRE_VALID(validation);
    unit_test::CheckArraysInheritTupleDims(destination);
  }
}

TEST_CASE("Selective import references: field matrix and full selection", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool aliases = GENERATE(false, true);
  const auto type = GENERATE(IGeometry::Type::Image, IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Tetrahedral, IGeometry::Type::RectGrid);
  DataStructure source;
  IGeometry* geometryPtr = nullptr;
  switch(type)
  {
  case IGeometry::Type::Image:
    geometryPtr = ImageGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Vertex:
    geometryPtr = VertexGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Edge:
    geometryPtr = EdgeGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Triangle:
    geometryPtr = TriangleGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::Tetrahedral:
    geometryPtr = TetrahedralGeom::Create(source, "Geometry");
    break;
  case IGeometry::Type::RectGrid:
    geometryPtr = RectGridGeom::Create(source, "Geometry");
    break;
  default:
    FAIL("Unexpected fixture geometry");
  }
  REQUIRE(geometryPtr != nullptr);
  const auto links = PopulateReferences(source, *geometryPtr);
  std::vector<DataPath> geometryPaths{DataPath({"Geometry"})};
  if(aliases)
  {
    auto* aliasParentPtr = DataGroup::Create(source, "Alias");
    REQUIRE(aliasParentPtr != nullptr);
    REQUIRE(source.setAdditionalParent(geometryPtr->getId(), aliasParentPtr->getId()));
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
            paths.push_back(geometryPath.createChildPath(links[field].target_path.getTargetName()));
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
          const auto targetPath = geometryPath.createChildPath(links[field].target_path.getTargetName());
          CAPTURE(targetPath.toString());
          if(field == omitted)
          {
            REQUIRE_FALSE(links[field].get_id(*importedPtr).has_value());
            REQUIRE_FALSE(destination.containsData(targetPath));
          }
          else
          {
            const auto* targetPtr = destination.getData(targetPath);
            REQUIRE(targetPtr != nullptr);
            REQUIRE(links[field].get_id(*importedPtr) == targetPtr->getId());
            REQUIRE(destination.getData(links[field].get_id(*importedPtr)) == targetPtr);
          }
        }
      }
      unit_test::CheckArraysInheritTupleDims(destination);
    }
  }
}

TEST_CASE("Selective import references: loader ancestors are not selected targets", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    DataStructure source;
    auto* geometryPtr = ImageGeom::Create(source, "Image");
    auto* cellsPtr = AttributeMatrix::Create(source, "Cells", {1});
    REQUIRE(geometryPtr != nullptr);
    REQUIRE(cellsPtr != nullptr);
    geometryPtr->setCellData(cellsPtr->getId());
    CreateReferenceArray<float32>(source, "SelectedChild", cellsPtr->getId());
    const ImportReferenceFile file(source, "Ancestor");
    const std::vector<DataPath> paths{DataPath({"Image"}), DataPath({"Cells", "SelectedChild"})};
    auto loaded = dream3d::LoadDataStructureArrays(file.path, paths);
    SIMPLNX_RESULT_REQUIRE_VALID(loaded);
    REQUIRE(loaded.value().containsData(DataPath({"Cells"})));

    DataStructure destination;
    destination.setNextId(100);
    auto* existingCellsPtr = AttributeMatrix::Create(destination, "Cells", {1});
    REQUIRE(existingCellsPtr != nullptr);
    auto result = ImportH5ObjectPathsAction(file.path, paths).apply(destination, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    const auto* importedPtr = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
    REQUIRE(importedPtr != nullptr);
    REQUIRE_FALSE(importedPtr->getCellDataId().has_value());
    REQUIRE(destination.getData(DataPath({"Cells"})) == existingCellsPtr);
    REQUIRE(destination.containsData(DataPath({"Cells", "SelectedChild"})));
    REQUIRE(destination.getAllDataPaths().size() == 3);
    unit_test::CheckArraysInheritTupleDims(destination);
  }
}

TEST_CASE("Selective import references: cached selections and shared targets stay isolated", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure source;
  auto* firstPtr = ImageGeom::Create(source, "First");
  auto* secondPtr = ImageGeom::Create(source, "Second");
  auto* cellsPtr = AttributeMatrix::Create(source, "Cells", {1});
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  REQUIRE(cellsPtr != nullptr);
  firstPtr->setCellData(cellsPtr->getId());
  secondPtr->setCellData(cellsPtr->getId());
  const ImportReferenceFile file(source, "Cache");
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  auto cached = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(cached);
  const auto initialHits = cache.hitCount();

  DataStructure partial;
  auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"First"})}).apply(partial, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* partialFirstPtr = partial.getDataAs<ImageGeom>(DataPath({"First"}));
  REQUIRE(partialFirstPtr != nullptr);
  REQUIRE_FALSE(partialFirstPtr->getCellDataId().has_value());
  REQUIRE(partial.getAllDataPaths().size() == 1);

  DataStructure full;
  full.setNextId(100);
  result = ImportH5ObjectPathsAction(file.path, {DataPath({"First"}), DataPath({"Second"}), DataPath({"Cells"})}).apply(full, mode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(cache.hitCount() >= initialHits + 2);
  const auto* importedFirstPtr = full.getDataAs<ImageGeom>(DataPath({"First"}));
  const auto* importedSecondPtr = full.getDataAs<ImageGeom>(DataPath({"Second"}));
  const auto* importedCellsPtr = full.getDataAs<AttributeMatrix>(DataPath({"Cells"}));
  REQUIRE(importedFirstPtr != nullptr);
  REQUIRE(importedSecondPtr != nullptr);
  REQUIRE(importedCellsPtr != nullptr);
  REQUIRE(importedFirstPtr->getCellDataId() == importedCellsPtr->getId());
  REQUIRE(importedSecondPtr->getCellDataId() == importedCellsPtr->getId());
  REQUIRE(full.getData(importedFirstPtr->getCellDataId()) == importedCellsPtr);
  REQUIRE(full.getData(importedSecondPtr->getCellDataId()) == importedCellsPtr);
  const auto* unchangedPtr = cached.value().data_structure.getDataAs<ImageGeom>(DataPath({"First"}));
  REQUIRE(unchangedPtr != nullptr);
  REQUIRE(unchangedPtr->getCellDataId() == cellsPtr->getId());
  unit_test::CheckArraysInheritTupleDims(partial);
  unit_test::CheckArraysInheritTupleDims(full);
}

TEST_CASE("Selective import references: retained malformed links preserve validation", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DYNAMIC_SECTION("mode " << static_cast<int>(mode))
  {
    SECTION("Retained cell matrix keeps the existing tuple-count error")
    {
      DataStructure source;
      auto* geometryPtr = ImageGeom::Create(source, "Image");
      REQUIRE(geometryPtr != nullptr);
      geometryPtr->setDimensions({2, 3, 4});
      auto* cellsPtr = AttributeMatrix::Create(source, "Cells", {2}, geometryPtr->getId());
      REQUIRE(cellsPtr != nullptr);
      geometryPtr->setCellData(cellsPtr->getId());
      const ImportReferenceFile file(source, "MalformedCells");
      DataStructure destination;
      destination.setNextId(100);
      auto result = ImportH5ObjectPathsAction(file.path, source.getAllDataPaths()).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* importedPtr = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
      const auto* importedCellsPtr = destination.getData(DataPath({"Image", "Cells"}));
      REQUIRE(importedPtr != nullptr);
      REQUIRE(importedCellsPtr != nullptr);
      REQUIRE(importedPtr->getCellDataId() == importedCellsPtr->getId());
      const auto validation = importedPtr->validate();
      REQUIRE(validation.invalid());
      REQUIRE(validation.errors().front().code == -4501);
      unit_test::CheckArraysInheritTupleDims(destination);
    }
    SECTION("Selected wrong-type target remains referenced")
    {
      DataStructure source;
      auto* geometryPtr = ImageGeom::Create(source, "Image");
      REQUIRE(geometryPtr != nullptr);
      auto* wrongTypePtr = DataGroup::Create(source, "WrongType", geometryPtr->getId());
      REQUIRE(wrongTypePtr != nullptr);
      geometryPtr->setCellData(wrongTypePtr->getId());
      const ImportReferenceFile file(source, "WrongType");
      DataStructure destination;
      destination.setNextId(100);
      auto result = ImportH5ObjectPathsAction(file.path, source.getAllDataPaths()).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* importedPtr = destination.getDataAs<ImageGeom>(DataPath({"Image"}));
      const auto* importedTargetPtr = destination.getData(DataPath({"Image", "WrongType"}));
      REQUIRE(importedPtr != nullptr);
      REQUIRE(importedTargetPtr != nullptr);
      REQUIRE(importedPtr->getCellDataId() == importedTargetPtr->getId());
      REQUIRE(dynamic_cast<const AttributeMatrix*>(importedTargetPtr) == nullptr);
      unit_test::CheckArraysInheritTupleDims(destination);
    }
    SECTION("Omitted vertices do not suppress vertex-matrix validation")
    {
      DataStructure source;
      auto* geometryPtr = VertexGeom::Create(source, "Vertex");
      REQUIRE(geometryPtr != nullptr);
      geometryPtr->setVertexListId(CreateReferenceArray<float32>(source, "Vertices", geometryPtr->getId(), 3));
      auto* vertexDataPtr = AttributeMatrix::Create(source, "VertexData", {1}, geometryPtr->getId());
      REQUIRE(vertexDataPtr != nullptr);
      geometryPtr->setVertexDataId(vertexDataPtr->getId());
      const ImportReferenceFile file(source, "MissingVertices");
      DataStructure destination;
      auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"Vertex"}), DataPath({"Vertex", "VertexData"})}).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* importedPtr = destination.getDataAs<VertexGeom>(DataPath({"Vertex"}));
      const auto* importedDataPtr = destination.getData(DataPath({"Vertex", "VertexData"}));
      REQUIRE(importedPtr != nullptr);
      REQUIRE(importedDataPtr != nullptr);
      REQUIRE_FALSE(importedPtr->getVertexListId().has_value());
      REQUIRE(importedPtr->getVertexAttributeMatrixId() == importedDataPtr->getId());
      const auto validation = importedPtr->validate();
      REQUIRE(validation.invalid());
      REQUIRE(validation.errors().front().code == -4500);
      unit_test::CheckArraysInheritTupleDims(destination);
    }
  }
}

TEST_CASE("Selective import references: selection errors and group shells stay compatible", "[SelectiveImportReferences]")
{
  const auto application = Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  DataStructure source;
  auto* groupPtr = DataGroup::Create(source, "Group");
  REQUIRE(groupPtr != nullptr);
  CreateReferenceArray<float32>(source, "Child", groupPtr->getId());
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
    unit_test::CheckArraysInheritTupleDims(destination);
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

  std::string resolveFormat(const DataStructure& /*dataStructure*/, const DataPath& path, DataType type, uint64 bytes) const override
  {
    records.push_back({path, type, bytes});
    return {};
  }
};

template <usize Count>
Int32ArrayType* CreateAliasArray(DataStructure& structure, const std::string& name, DataObject::IdType parent, const std::array<int32, Count>& values)
{
  auto store = std::make_shared<DataStore<int32>>(ShapeType{Count}, ShapeType{1}, int32{0});
  for(usize valueIdx = 0; valueIdx < Count; ++valueIdx)
  {
    store->setValue(valueIdx, values[valueIdx]);
  }
  auto* arrayPtr = Int32ArrayType::Create(structure, name, std::move(store), parent);
  REQUIRE(arrayPtr != nullptr);
  return arrayPtr;
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
void RequireAliasFileValues(const hdf5::FileIO& file, const DataPath& path, const std::array<int32, Count>& expected)
{
  const auto fileId = file.getId();
  const std::lock_guard lock(hdf5::Support::ApiLock());
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
const Int32ArrayType* RequireAliasOutput(const DataStructure& structure, const DataPath& path, IDataAction::Mode mode, const std::array<int32, Count>& values)
{
  const auto* arrayPtr = structure.getDataAs<Int32ArrayType>(path);
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(structure.getData(arrayPtr->getId()) == arrayPtr);
  REQUIRE(arrayPtr->getTupleShape() == ShapeType{Count});
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  REQUIRE(arrayPtr->size() == Count);
  REQUIRE(arrayPtr->getIDataStore() != nullptr);
  CHECK(arrayPtr->getDataFormat().empty());
  CHECK(arrayPtr->getIDataStore()->getPlannedStoreType() == IDataStore::StoreType::InMemory);
  if(mode == IDataAction::Mode::Preflight)
  {
    CHECK(arrayPtr->getStoreType() == IDataStore::StoreType::Empty);
  }
  else
  {
    REQUIRE(arrayPtr->getStoreType() == IDataStore::StoreType::InMemory);
    for(usize valueIdx = 0; valueIdx < Count; ++valueIdx)
    {
      CHECK((*arrayPtr)[valueIdx] == values[valueIdx]);
    }
  }
  return arrayPtr;
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
    const auto* objectPtr = structure.getData(path);
    REQUIRE(objectPtr != nullptr);
    CHECK(structure.getData(objectPtr->getId()) == objectPtr);
    CHECK(seen.insert(objectPtr->getId()).second);
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
  auto* firstPtr = DataGroup::Create(source, "First");
  auto* secondPtr = DataGroup::Create(source, "Second");
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  auto* sharedPtr = CreateAliasArray(source, "Shared", firstPtr->getId(), k_AliasValues);
  REQUIRE(source.setAdditionalParent(sharedPtr->getId(), secondPtr->getId()));
  // The deeper distinct object must keep its reserved source ID when an earlier alias is split.
  auto* laterGroupPtr = DataGroup::Create(source, "LaterGroup", secondPtr->getId());
  REQUIRE(laterGroupPtr != nullptr);
  auto* laterPtr = CreateAliasArray(source, "Later", laterGroupPtr->getId(), k_LaterAliasValues);
  REQUIRE(firstPtr->getId() == 1);
  REQUIRE(secondPtr->getId() == 2);
  REQUIRE(sharedPtr->getId() == 3);
  REQUIRE(laterGroupPtr->getId() == 4);
  REQUIRE(laterPtr->getId() == 5);
  REQUIRE(source.getData(firstArrayPath) == source.getData(secondArrayPath));
  RequireAliasParents(*sharedPtr, {firstPtr->getId(), secondPtr->getId()});
  const ImportReferenceFile file(source, "SharedArrayAliases");
  {
    const auto reader = hdf5::FileIO::ReadFile(file.path);
    REQUIRE(reader.isValid());
    RequireAliasFileValues(reader, firstArrayPath, k_AliasValues);
    RequireAliasFileValues(reader, secondArrayPath, k_AliasValues);
    RequireAliasFileValues(reader, laterArrayPath, k_LaterAliasValues);
  }
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  auto metadata = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  REQUIRE(metadata.value().file_version == dream3d::k_CurrentFileVersion);
  const auto& sourceHandout = metadata.value().data_structure;
  const auto* sourceFirstPtr = sourceHandout.getDataAs<Int32ArrayType>(firstArrayPath);
  const auto* sourceSecondPtr = sourceHandout.getDataAs<Int32ArrayType>(secondArrayPath);
  REQUIRE(sourceFirstPtr != nullptr);
  REQUIRE(sourceFirstPtr == sourceSecondPtr);
  REQUIRE(sourceFirstPtr->getId() == sharedPtr->getId());
  REQUIRE(sourceHandout.getData(sourceFirstPtr->getId()) == sourceFirstPtr);
  RequireAliasParents(*sourceFirstPtr, {firstPtr->getId(), secondPtr->getId()});
  const auto sourceNext = sourceHandout.getNextId();
  REQUIRE(sourceNext == 6);
  REQUIRE(sourceHandout.getAllDataObjectIds().size() == 5);
  REQUIRE(sourceHandout.getAllDataPaths().size() == 6);
  const auto* sourceLaterPtr = sourceHandout.getDataAs<Int32ArrayType>(laterArrayPath);
  REQUIRE(sourceLaterPtr != nullptr);
  REQUIRE(sourceLaterPtr->getId() == laterPtr->getId());
  REQUIRE(sourceHandout.getData(sourceLaterPtr->getId()) == sourceLaterPtr);
  // Evaluate to bool before Catch2 tries to stream libc++'s 128-bit file-clock duration.
  REQUIRE((fs::file_time_type::clock::now() - fs::last_write_time(file.path) >= dream3d::Dream3dPreflightCache::k_MtimeTrustWindow));
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
  const auto* firstCopyPtr = RequireAliasOutput(destination, firstArrayPath, mode, k_AliasValues);
  const auto* secondCopyPtr = RequireAliasOutput(destination, secondArrayPath, mode, k_AliasValues);
  const auto* laterCopyPtr = RequireAliasOutput(destination, laterArrayPath, mode, k_LaterAliasValues);
  CHECK(firstCopyPtr != secondCopyPtr);
  CHECK(firstCopyPtr->getId() != secondCopyPtr->getId());
  CHECK(firstCopyPtr->getIDataStore() != secondCopyPtr->getIDataStore());
  CHECK(std::min(firstCopyPtr->getId(), secondCopyPtr->getId()) == sharedPtr->getId());
  CHECK(std::max(firstCopyPtr->getId(), secondCopyPtr->getId()) == sourceNext);
  CHECK(laterCopyPtr->getId() == laterPtr->getId());
  REQUIRE(destination.getData(laterGroupPath) != nullptr);
  CHECK(destination.getData(laterGroupPath)->getId() == laterGroupPtr->getId());
  CHECK(destination.getNextId() == sourceNext + 1);
  RequireAliasParents(*firstCopyPtr, {destination.getData(firstPath)->getId()});
  RequireAliasParents(*secondCopyPtr, {destination.getData(secondPath)->getId()});
  RequireAliasParents(*laterCopyPtr, {destination.getData(laterGroupPath)->getId()});
  RequireAliasPlans(*resolver, {{firstArrayPath, 16}, {secondArrayPath, 16}, {laterArrayPath, 8}});
  CHECK(sourceHandout.getData(firstArrayPath) == sourceHandout.getData(secondArrayPath));
  CHECK(sourceHandout.getData(firstArrayPath)->getId() == sharedPtr->getId());
  CHECK(sourceHandout.getNextId() == sourceNext);
  if(warm)
  {
    const auto* warmArrayPtr = RequireAliasOutput(warmDestination, firstArrayPath, mode, k_AliasValues);
    CHECK(warmArrayPtr != firstCopyPtr);
    CHECK(warmArrayPtr->getIDataStore() != firstCopyPtr->getIDataStore());
  }
  unit_test::CheckArraysInheritTupleDims(destination);
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
  auto* firstPtr = DataGroup::Create(source, "First");
  auto* secondPtr = DataGroup::Create(source, "Second");
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  auto* sharedGroupPtr = DataGroup::Create(source, "SharedGroup", firstPtr->getId());
  REQUIRE(sharedGroupPtr != nullptr);
  REQUIRE(source.setAdditionalParent(sharedGroupPtr->getId(), secondPtr->getId()));
  auto* childPtr = CreateAliasArray(source, "Child", sharedGroupPtr->getId(), k_AliasValues);
  REQUIRE(firstPtr->getId() == 1);
  REQUIRE(secondPtr->getId() == 2);
  REQUIRE(sharedGroupPtr->getId() == 3);
  REQUIRE(childPtr->getId() == 4);
  REQUIRE(source.getData(firstGroupPath) == source.getData(secondGroupPath));
  REQUIRE(source.getData(firstChildPath) == source.getData(secondChildPath));
  RequireAliasParents(*sharedGroupPtr, {firstPtr->getId(), secondPtr->getId()});
  // The child has one direct parent and inherits two paths through that parent's placements.
  RequireAliasParents(*childPtr, {sharedGroupPtr->getId()});
  const ImportReferenceFile file(source, "SharedGroupAliases");
  {
    const auto reader = hdf5::FileIO::ReadFile(file.path);
    REQUIRE(reader.isValid());
    RequireAliasFileValues(reader, firstChildPath, k_AliasValues);
    RequireAliasFileValues(reader, secondChildPath, k_AliasValues);
  }
  auto& cache = dream3d::Dream3dPreflightCache::Instance();
  auto metadata = cache.fetchNeutralMetadata(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  const auto& sourceHandout = metadata.value().data_structure;
  const auto* sourceGroupPtr = sourceHandout.getDataAs<DataGroup>(firstGroupPath);
  const auto* sourceChildPtr = sourceHandout.getDataAs<Int32ArrayType>(firstChildPath);
  REQUIRE(sourceGroupPtr != nullptr);
  REQUIRE(sourceChildPtr != nullptr);
  REQUIRE(sourceGroupPtr == sourceHandout.getData(secondGroupPath));
  REQUIRE(sourceChildPtr == sourceHandout.getData(secondChildPath));
  REQUIRE(sourceGroupPtr->getId() == sharedGroupPtr->getId());
  REQUIRE(sourceChildPtr->getId() == childPtr->getId());
  RequireAliasParents(*sourceGroupPtr, {firstPtr->getId(), secondPtr->getId()});
  RequireAliasParents(*sourceChildPtr, {sourceGroupPtr->getId()});
  const auto sourceNext = sourceHandout.getNextId();
  REQUIRE(sourceNext == 5);
  REQUIRE(sourceHandout.getAllDataObjectIds().size() == 4);
  REQUIRE(sourceHandout.getAllDataPaths().size() == 6);
  // Evaluate to bool before Catch2 tries to stream libc++'s 128-bit file-clock duration.
  REQUIRE((fs::file_time_type::clock::now() - fs::last_write_time(file.path) >= dream3d::Dream3dPreflightCache::k_MtimeTrustWindow));
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
  const auto* firstGroupPtr = destination.getDataAs<DataGroup>(firstGroupPath);
  const auto* secondGroupPtr = destination.getDataAs<DataGroup>(secondGroupPath);
  REQUIRE(firstGroupPtr != nullptr);
  REQUIRE(secondGroupPtr != nullptr);
  CHECK(firstGroupPtr != secondGroupPtr);
  CHECK(firstGroupPtr->getId() != secondGroupPtr->getId());
  CHECK(std::min(firstGroupPtr->getId(), secondGroupPtr->getId()) == sharedGroupPtr->getId());
  CHECK(std::max(firstGroupPtr->getId(), secondGroupPtr->getId()) == sourceNext);
  CHECK(firstGroupPtr->getSize() == 1);
  CHECK(secondGroupPtr->getSize() == 1);
  const auto* firstChildPtr = RequireAliasOutput(destination, firstChildPath, mode, k_AliasValues);
  const auto* secondChildPtr = RequireAliasOutput(destination, secondChildPath, mode, k_AliasValues);
  CHECK(firstChildPtr != secondChildPtr);
  CHECK(firstChildPtr->getId() != secondChildPtr->getId());
  CHECK(firstChildPtr->getIDataStore() != secondChildPtr->getIDataStore());
  CHECK(std::min(firstChildPtr->getId(), secondChildPtr->getId()) == childPtr->getId());
  CHECK(std::max(firstChildPtr->getId(), secondChildPtr->getId()) == sourceNext + 1);
  CHECK(destination.getNextId() == sourceNext + 2);
  RequireAliasParents(*firstGroupPtr, {destination.getData(firstPath)->getId()});
  RequireAliasParents(*secondGroupPtr, {destination.getData(secondPath)->getId()});
  RequireAliasParents(*firstChildPtr, {firstGroupPtr->getId()});
  RequireAliasParents(*secondChildPtr, {secondGroupPtr->getId()});
  RequireAliasPlans(*resolver, {{firstChildPath, 16}, {secondChildPath, 16}});
  CHECK(sourceHandout.getData(firstGroupPath) == sourceHandout.getData(secondGroupPath));
  CHECK(sourceHandout.getData(firstChildPath) == sourceHandout.getData(secondChildPath));
  CHECK(sourceHandout.getData(firstGroupPath)->getId() == sharedGroupPtr->getId());
  CHECK(sourceHandout.getData(firstChildPath)->getId() == childPtr->getId());
  if(warm)
  {
    const auto* warmChildPtr = RequireAliasOutput(warmDestination, firstChildPath, mode, k_AliasValues);
    CHECK(warmChildPtr != firstChildPtr);
    CHECK(warmChildPtr->getIDataStore() != firstChildPtr->getIDataStore());
  }
  unit_test::CheckArraysInheritTupleDims(destination);
}

namespace
{
DataStructure MakeAliasControlSource()
{
  DataStructure source;
  source.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* firstPtr = DataGroup::Create(source, "First");
  auto* secondPtr = DataGroup::Create(source, "Second");
  REQUIRE(firstPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  auto* sharedPtr = CreateAliasArray(source, "Shared", firstPtr->getId(), k_AliasValues);
  REQUIRE(source.setAdditionalParent(sharedPtr->getId(), secondPtr->getId()));
  REQUIRE(source.getAllDataObjectIds().size() == 3);
  REQUIRE(source.getNextId() == 4);
  REQUIRE(source.getData(DataPath({"First", "Shared"})) == source.getData(DataPath({"Second", "Shared"})));
  RequireAliasParents(*sharedPtr, {firstPtr->getId(), secondPtr->getId()});
  return source;
}

const std::vector<DataPath> k_AliasControlSelection{DataPath({"First"}), DataPath({"Second"}), DataPath({"First", "Shared"}), DataPath({"Second", "Shared"})};

void RequireAliasControlFile(const ImportReferenceFile& file)
{
  const auto reader = hdf5::FileIO::ReadFile(file.path);
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
  constexpr auto k_MaximumId = std::numeric_limits<DataObject::IdType>::max();
  const auto startId = k_MaximumId - (fits ? 4 : 3);
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
    const auto* firstPtr = RequireAliasOutput(destination, DataPath({"First", "Shared"}), mode, k_AliasValues);
    const auto* secondPtr = RequireAliasOutput(destination, DataPath({"Second", "Shared"}), mode, k_AliasValues);
    CHECK(firstPtr != secondPtr);
    CHECK(firstPtr->getIDataStore() != secondPtr->getIDataStore());
    CHECK(std::min(firstPtr->getId(), secondPtr->getId()) == k_MaximumId - 2);
    CHECK(std::max(firstPtr->getId(), secondPtr->getId()) == k_MaximumId - 1);
    CHECK(destination.getNextId() == k_MaximumId);
    RequireAliasPlans(*resolver, {{DataPath({"First", "Shared"}), 16}, {DataPath({"Second", "Shared"}), 16}});
  }
  unit_test::CheckArraysInheritTupleDims(destination);
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
    auto* retainedParentPtr = DataGroup::Import(destination, "RetainedParent", 100);
    REQUIRE(retainedParentPtr != nullptr);
    destination.setNextId(1);
    DataStructure retentionSource;
    auto* sourceParentPtr = DataGroup::Create(retentionSource, "RetainedParent");
    REQUIRE(sourceParentPtr != nullptr);
    REQUIRE(DataGroup::Create(retentionSource, "Unselected") != nullptr);
    REQUIRE(DataGroup::Create(retentionSource, "Earlier") != nullptr);
    auto* retainedPtr = DataGroup::Create(retentionSource, "RetainedCloneId", sourceParentPtr->getId());
    REQUIRE(retainedPtr != nullptr);
    REQUIRE(retainedPtr->getId() == cloneId);
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
    REQUIRE(destination.getData(DataPath({"RetainedParent"})) == retainedParentPtr);
    const auto entry = retainedParentPtr->getDataMap().find(cloneId);
    REQUIRE(entry != retainedParentPtr->getDataMap().end());
    REQUIRE(entry->second.get() == destination.getData(occupiedPath));
  }
  else
  {
    auto* occupiedPtr = DataGroup::Import(destination, "OccupiedCloneId", cloneId);
    REQUIRE(occupiedPtr != nullptr);
    REQUIRE(occupiedPtr->getId() == cloneId);
    if(occupantKind == 1)
    {
      REQUIRE(destination.removeData(cloneId));
      REQUIRE(destination.getData(cloneId) == nullptr);
      REQUIRE(destination.getAllDataObjectIds() == std::vector<DataObject::IdType>{cloneId});
    }
  }
  const auto* originalOwnerPtr = destination.getData(occupiedPath);
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
  CHECK(destination.getData(occupiedPath) == originalOwnerPtr);
  CHECK(destination.getAllDataObjectIds() == beforeIds);
  CHECK(destination.getAllDataPaths() == beforePaths);
  CHECK(destination.getNextId() == 1);
  if(retainingAction != nullptr)
  {
    const auto cleaned = retainingAction->cleanupFailedImportForTesting(destination);
    SIMPLNX_RESULT_REQUIRE_VALID(cleaned);
    CHECK(destination.getData(occupiedPath) == nullptr);
    const auto* parentPtr = destination.getDataAs<DataGroup>(DataPath({"RetainedParent"}));
    REQUIRE(parentPtr != nullptr);
    CHECK(parentPtr->empty());
    CHECK(destination.getData(100) == parentPtr);
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
  auto* firstParentPtr = DataGroup::Create(destination, "First");
  auto* secondParentPtr = DataGroup::Create(destination, "Second");
  REQUIRE(firstParentPtr != nullptr);
  REQUIRE(secondParentPtr != nullptr);
  auto* markerPtr = CreateAliasArray(destination, "Existing", firstParentPtr->getId(), k_LaterAliasValues);
  const auto* markerStorePtr = markerPtr->getIDataStore();
  const auto beforeIds = destination.getAllDataObjectIds();
  const auto beforePaths = destination.getAllDataPaths();
  const auto beforeNext = destination.getNextId();
  const auto originalCopyId = beforeNext + 2;
  const auto cloneId = beforeNext + 3;
  const auto resolver = std::make_shared<AliasPathResolver>();
  destination.setFormatResolver(resolver);
  std::vector<DataObject::IdType> removals;
  nod::scoped_connection connection(destination.getSignal().connect([&](DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>& message) {
    if(message->getMsgType() == DataRemovedMessage::k_MsgType)
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
  CHECK(destination.getData(firstPath) == firstParentPtr);
  CHECK(destination.getData(secondPath) == secondParentPtr);
  CHECK(firstParentPtr->getSize() == 1);
  CHECK(secondParentPtr->empty());
  CHECK(destination.getData(markerPath) == markerPtr);
  CHECK(markerPtr->getIDataStore() == markerStorePtr);
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
  const auto* firstCopyPtr = RequireAliasOutput(destination, firstArrayPath, mode, k_AliasValues);
  const auto* secondCopyPtr = RequireAliasOutput(destination, secondArrayPath, mode, k_AliasValues);
  CHECK(firstCopyPtr != secondCopyPtr);
  CHECK(firstCopyPtr->getIDataStore() != secondCopyPtr->getIDataStore());
  CHECK(std::min(firstCopyPtr->getId(), secondCopyPtr->getId()) == originalCopyId);
  CHECK(std::max(firstCopyPtr->getId(), secondCopyPtr->getId()) == cloneId);
  CHECK(destination.getData(cloneId) != retained.get());
  RequireAliasParents(*firstCopyPtr, {firstParentPtr->getId()});
  RequireAliasParents(*secondCopyPtr, {secondParentPtr->getId()});
  RequireAliasPlans(*resolver, {{firstArrayPath, 16}, {secondArrayPath, 16}});
  const auto removalCount = removals.size();
  retained.reset();
  action.setPublicationFaultForTesting(ImportH5ObjectPathsAction::PublicationFaultType::None);
  CHECK(removals.size() == removalCount);
  const auto nextId = destination.getNextId();
  REQUIRE(nextId == cloneId + 1);
  auto* nextPtr = DataGroup::Create(destination, "AfterAliasRetry");
  REQUIRE(nextPtr != nullptr);
  CHECK(nextPtr->getId() == nextId);
  CHECK(destination.getData(nextId) == nextPtr);
  CHECK(destination.getData(DataPath({"AfterAliasRetry"})) == nextPtr);
  CHECK(destination.getData(markerPath) == markerPtr);
  CHECK(markerPtr->getIDataStore() == markerStorePtr);
  unit_test::CheckArraysInheritTupleDims(destination);
}
#endif
