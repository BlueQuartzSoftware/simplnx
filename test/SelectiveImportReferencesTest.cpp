#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

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
} // namespace

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
    if(mode == IDataAction::Mode::Execute)
    {
      REQUIRE(importedEnsemble->getId() == cells->getId());
    }
    REQUIRE_FALSE(imported->getCellDataId().has_value());
    REQUIRE_FALSE(destination.containsData(DataPath({"Image", "CellData"})));
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
  const ImportReferenceFile file(source, "FieldMatrix");
  // The final case retains all references with overlapping source and destination ID ranges.
  for(usize omitted = 0; omitted <= links.size(); ++omitted)
  {
    DYNAMIC_SECTION("type " << static_cast<int>(type) << " mode " << static_cast<int>(mode) << " omitted " << omitted)
    {
      std::vector<DataPath> paths{DataPath({"Geometry"})};
      for(usize field = 0; field < links.size(); ++field)
      {
        if(field != omitted)
        {
          paths.push_back(links[field].targetPath);
        }
      }
      DataStructure destination;
      destination.setNextId(omitted == links.size() ? 3 : 100);
      auto result = ImportH5ObjectPathsAction(file.path, paths).apply(destination, mode);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      const auto* imported = destination.getDataAs<IGeometry>(DataPath({"Geometry"}));
      REQUIRE(imported != nullptr);
      for(usize field = 0; field < links.size(); ++field)
      {
        CAPTURE(links[field].targetPath.toString());
        if(field == omitted)
        {
          REQUIRE_FALSE(links[field].getId(*imported).has_value());
          REQUIRE_FALSE(destination.containsData(links[field].targetPath));
        }
        else
        {
          const auto* target = destination.getData(links[field].targetPath);
          REQUIRE(target != nullptr);
          REQUIRE(links[field].getId(*imported) == target->getId());
          REQUIRE(destination.getData(links[field].getId(*imported)) == target);
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
  auto cached = cache.fetch(file.path);
  SIMPLNX_RESULT_REQUIRE_VALID(cached);
  const auto initialHits = cache.hitCount();

  DataStructure partial;
  auto result = ImportH5ObjectPathsAction(file.path, {DataPath({"First"})}).apply(partial, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto* partialFirst = partial.getDataAs<ImageGeom>(DataPath({"First"}));
  REQUIRE(partialFirst != nullptr);
  REQUIRE_FALSE(partialFirst->getCellDataId().has_value());
  REQUIRE(partial.getAllDataPaths().size() == 1);

  DataStructure full;
  full.setNextId(100);
  result = ImportH5ObjectPathsAction(file.path, {DataPath({"First"}), DataPath({"Second"}), DataPath({"Cells"})}).apply(full, IDataAction::Mode::Preflight);
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
  const auto* unchanged = cached.value().getDataAs<ImageGeom>(DataPath({"First"}));
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
