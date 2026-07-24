#include "SimplnxCore/Filters/CreateDataArrayFilter.hpp"
#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/Filters/WriteDREAM3DFilter.hpp"

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/HDF5DatasetProbe.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
namespace Constants
{
const fs::path k_DataDir = "test/data";
const fs::path k_Dream3dFilename = "newFile.dream3d";
const fs::path k_ExportFilename1 = "export.dream3d";
const fs::path k_ExportFilename2 = "export2.dream3d";
const fs::path k_MultiExportFilename1 = "multi_export1.dream3d";
const fs::path k_MultiExportFilename2 = "multi_export2.dream3d";
const fs::path k_MultiExportFilename3 = "multi_export3.dream3d";

constexpr StringLiteral k_CellData = "Cell Data";
constexpr StringLiteral k_DataContainer = "Data Container";
constexpr StringLiteral k_EdgeGeom = "EdgeGeom";
constexpr StringLiteral k_ImageGeom = "ImageGeom";
constexpr StringLiteral k_RectGridGeom = "RectGrid";
constexpr StringLiteral k_HexGeom = "HexahedralGeom";
constexpr StringLiteral k_QuadGeom = "QuadGeom";
constexpr StringLiteral k_TetrahedralGeom = "TetrahedralGeom";
constexpr StringLiteral k_TriangleGeom = "TriangleGeom";
constexpr StringLiteral k_VertexGeom = "VertexGeom";
constexpr StringLiteral k_XBounds = "X Bounds";
constexpr StringLiteral k_YBounds = "Y Bounds";
constexpr StringLiteral k_ZBounds = "Z Bounds";

constexpr StringLiteral k_DynamicListArray = "DynamicList";
constexpr StringLiteral k_NeighborList = "NeighborList";
constexpr StringLiteral k_StringArray = "String Array";
constexpr StringLiteral k_VertexList = "Vertices";
constexpr StringLiteral k_Edges = "Edges";
constexpr StringLiteral k_Faces = "Faces";
constexpr StringLiteral k_Polyhedra = "Polyhedra";
constexpr StringLiteral k_HexaArray = "Hexahedra Array";
constexpr StringLiteral k_TetraArray = "Tetrahedra Array";
constexpr StringLiteral k_QuadArray = "Quad Array";
constexpr StringLiteral k_Int8 = "Int 8";
constexpr StringLiteral k_Int16 = "Int 16";
constexpr StringLiteral k_Int32 = "Int 32";
constexpr StringLiteral k_Int64 = "Int 64";
constexpr StringLiteral k_UInt8 = "UInt 8";
constexpr StringLiteral k_UInt16 = "UInt 16";
constexpr StringLiteral k_UInt32 = "UInt 32";
constexpr StringLiteral k_UInt64 = "UInt 64";
constexpr StringLiteral k_Float32 = "Float 32";
constexpr StringLiteral k_Float64 = "Float 64";
constexpr int64 k_ScalarValue = 3;
const ShapeType k_TupleShape{3, 2, 1};
const SizeVec3 k_ImageShape{1, 2, 3};
constexpr int16 k_ListCount = 6;
} // namespace Constants

std::mutex m_DataMutex;

namespace DataNames
{
constexpr StringLiteral k_Group1Name = "Top-Level";
constexpr StringLiteral k_Group2Name = "Second-Level";
constexpr StringLiteral k_Group3Name = "Third-Level";
constexpr StringLiteral k_AttributeMatrixName = "AttributeMatrix";
constexpr StringLiteral k_ArrayName = "Test-Array";
constexpr StringLiteral k_Array2Name = "Test-Array2";

constexpr StringLiteral k_CreateDataFilterName = "Create Data Group";
constexpr StringLiteral k_ExportD3DFilterName = "Write DREAM3D-NX File";
} // namespace DataNames

const FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_CreateDataArrayHandle(Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExportD3DHandle(Uuid::FromString("b3a95784-2ced-41ec-8d3d-0242ac130003").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ImportD3DHandle(Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());

fs::path GetDataDir(const Application& app)
{
  return std::filesystem::path(unit_test::k_BinaryTestOutputDir.view());
}

fs::path GetIODataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_Dream3dFilename;
}

fs::path GetXdmfPath()
{
  fs::path filePath = GetIODataPath();
  filePath.replace_extension(".xdmf");
  return filePath;
}

fs::path GetExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename1;
}

fs::path GetReExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename2;
}

fs::path GetMultiExportDataPath1()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename1;
}

fs::path GetMultiExportDataPath2()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename2;
}

fs::path GetReMultiExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename3;
}

/**
 * @brief Creates and sets arrays for a 2D geometry of type T.
 * @param dataStructure
 * @param name
 * @param vertexArray
 * @param edgeList
 * @param faceList
 * @return T*
 */
template <typename T>
T* Create2DGeom(DataStructure& dataStructure, const std::string& name, const IGeometry::SharedVertexList& vertexArray, const IGeometry::SharedEdgeList& edgeList,
                const IGeometry::SharedFaceList& faceList)
{
  auto* geom = T::Create(dataStructure, name);
  geom->setVertices(vertexArray);
  geom->setEdgeList(edgeList);
  geom->setFaceList(faceList);
  return geom;
}

/**
 * @brief Creates and sets arrays for a 3D geometry of type T.
 * @param dataStructure
 * @param name
 * @param vertexArray
 * @param edgeList
 * @param faceList
 * @param polyArray
 * @return T*
 */
template <typename T>
T* Create3DGeom(DataStructure& dataStructure, const std::string& name, const IGeometry::SharedVertexList& vertexArray, const IGeometry::SharedEdgeList& edgeList,
                const IGeometry::SharedFaceList& faceList, const IGeometry::SharedHexList& polyArray)
{
  auto* geom = T::Create(dataStructure, name);
  geom->setVertices(vertexArray);
  geom->setEdgeList(edgeList);
  geom->setFaceList(faceList);
  geom->setPolyhedraList(polyArray);
  return geom;
}

/**
 * @brief Fill the data store with values, but do not use values greater than the number of tuples.
 * @param dataStore
 */
template <typename T>
void FillDataStore(AbstractDataStore<T>& dataStore)
{
  const auto numTuples = dataStore.getNumberOfTuples();
  const auto numComponents = dataStore.getNumberOfComponents();

  for(usize i = 0; i < numTuples; i++)
  {
    const usize offset = i * numComponents;
    for(usize j = 0; j < numComponents; j++)
    {
      usize value = (i + j) % numTuples;
      dataStore[offset + j] = static_cast<T>(value);
    }
  }
}

template <typename T>
void CheckDataStore(const AbstractDataStore<T>& dataStore, usize requiredComponents)
{
  const auto numTuples = dataStore.getNumberOfTuples();
  const auto numComponents = dataStore.getNumberOfComponents();
  REQUIRE(numComponents == requiredComponents);

  for(usize i = 0; i < numTuples; i++)
  {
    const usize offset = i * numComponents;
    for(usize j = 0; j < numComponents; j++)
    {
      usize value = (i + j) % numTuples;
      REQUIRE(dataStore[offset + j] == static_cast<T>(value));
    }
  }
}

void CheckGeom0D(const INodeGeometry0D* geom, DataObject::IdType vertexId)
{
  REQUIRE(geom != nullptr);
  REQUIRE(geom->getVertexListId() == vertexId);
}

void CheckGeom1D(const INodeGeometry1D* geom, DataObject::IdType vertexId, DataObject::IdType edgeId)
{
  REQUIRE(geom != nullptr);
  REQUIRE(geom->getEdgeListId() == edgeId);
  CheckGeom0D(geom, vertexId);
}

void CheckGeom2D(const INodeGeometry2D* geom, DataObject::IdType vertexId, DataObject::IdType edgeId, DataObject::IdType faceId)
{
  REQUIRE(geom != nullptr);
  REQUIRE(geom->getFaceListId() == faceId);
  CheckGeom1D(geom, vertexId, edgeId);
}

void CheckGeom3D(const INodeGeometry3D* geom, DataObject::IdType vertexId, DataObject::IdType edgeId, DataObject::IdType faceId, DataObject::IdType polyhedraId)
{
  REQUIRE(geom != nullptr);
  REQUIRE(geom->getPolyhedronListId().has_value());
  REQUIRE(geom->getPolyhedronListId().value() == polyhedraId);
  CheckGeom2D(geom, vertexId, edgeId, faceId);
}

template <typename T>
void CheckScalarData(const DataStructure& dataStructure, const DataPath& path)
{
  auto* scalarData = dataStructure.getDataAs<ScalarData<T>>(path);
  REQUIRE(scalarData != nullptr);
  REQUIRE(scalarData->getValue() == Approx(static_cast<T>(Constants::k_ScalarValue)));
}

void CheckNeighborListStore(const AbstractListStore<int16>& store)
{
  REQUIRE(store.getNumberOfTuples() == Constants::k_ListCount);
  for(usize i = 0; i < Constants::k_ListCount; i++)
  {
    auto list = store.getList(i);
    REQUIRE(list.size() == 2);
    REQUIRE(list[0] == 1);
    REQUIRE(list[1] == 2);
  }
}

void CheckTestDataStructure(const DataStructure& dataStructure)
{
  DataPath dataGroupPath({Constants::k_DataContainer});
  REQUIRE(dataStructure.getDataAs<DataGroup>(dataGroupPath) != nullptr);

  const auto* neighborList = dataStructure.getDataAs<Int16NeighborList>(dataGroupPath.createChildPath(Constants::k_NeighborList));
  REQUIRE(neighborList != nullptr);
  const auto storePtr = neighborList->getStore();
  REQUIRE(storePtr != nullptr);
  CheckNeighborListStore(*storePtr);

  const auto* vertexArray = dataStructure.getDataAs<Float32Array>(dataGroupPath.createChildPath(Constants::k_VertexList));
  REQUIRE(vertexArray != nullptr);
  const auto& vertices = vertexArray->getDataStoreRef();
  CheckDataStore<float32>(vertices, 3);

  const auto* edgeArray = dataStructure.getDataAs<UInt64Array>(dataGroupPath.createChildPath(Constants::k_Edges));
  REQUIRE(edgeArray != nullptr);
  const auto& edges = edgeArray->getDataStoreRef();
  CheckDataStore<uint64>(edges, 2);

  const auto* faceArray = dataStructure.getDataAs<UInt64Array>(dataGroupPath.createChildPath(Constants::k_Faces));
  REQUIRE(faceArray != nullptr);
  const auto& faces = faceArray->getDataStoreRef();
  CheckDataStore<uint64>(faces, 3);

  const auto* quadArray = dataStructure.getDataAs<UInt64Array>(dataGroupPath.createChildPath(Constants::k_QuadArray));
  REQUIRE(quadArray != nullptr);
  const auto& quads = quadArray->getDataStoreRef();
  CheckDataStore<uint64>(quads, 4);

  const auto* hexaArray = dataStructure.getDataAs<UInt64Array>(dataGroupPath.createChildPath(Constants::k_HexaArray));
  REQUIRE(hexaArray != nullptr);
  const auto& hexa = hexaArray->getDataStoreRef();
  CheckDataStore<uint64>(hexa, 8);

  const auto* tetraArray = dataStructure.getDataAs<UInt64Array>(dataGroupPath.createChildPath(Constants::k_TetraArray));
  REQUIRE(tetraArray != nullptr);
  const auto& tetra = tetraArray->getDataStoreRef();
  CheckDataStore<uint64>(tetra, 4);

  const auto* polyArray = dataStructure.getDataAs<UInt64Array>(dataGroupPath.createChildPath(Constants::k_Polyhedra));
  REQUIRE(polyArray != nullptr);
  const auto& polyhedra = polyArray->getDataStoreRef();
  CheckDataStore<uint64>(polyhedra, 4);

  const auto* stringArray = dataStructure.getDataAs<StringArray>(dataGroupPath.createChildPath(Constants::k_StringArray));
  REQUIRE(stringArray != nullptr);
  auto stringCount = stringArray->getNumberOfTuples();
  REQUIRE(stringCount == 6);
  REQUIRE(stringArray->at(0) == "1");
  REQUIRE(stringArray->at(1) == "2");
  REQUIRE(stringArray->at(2) == "3");
  REQUIRE(stringArray->at(3) == "4");
  REQUIRE(stringArray->at(4) == "5");
  REQUIRE(stringArray->at(5) == "6");

  CheckScalarData<int8>(dataStructure, dataGroupPath.createChildPath(Constants::k_Int8));
  CheckScalarData<int16>(dataStructure, dataGroupPath.createChildPath(Constants::k_Int16));
  CheckScalarData<int32>(dataStructure, dataGroupPath.createChildPath(Constants::k_Int32));
  CheckScalarData<int64>(dataStructure, dataGroupPath.createChildPath(Constants::k_Int64));
  CheckScalarData<uint8>(dataStructure, dataGroupPath.createChildPath(Constants::k_UInt8));
  CheckScalarData<uint16>(dataStructure, dataGroupPath.createChildPath(Constants::k_UInt16));
  CheckScalarData<uint32>(dataStructure, dataGroupPath.createChildPath(Constants::k_UInt32));
  CheckScalarData<uint64>(dataStructure, dataGroupPath.createChildPath(Constants::k_UInt64));
  CheckScalarData<float32>(dataStructure, dataGroupPath.createChildPath(Constants::k_Float32));
  CheckScalarData<float64>(dataStructure, dataGroupPath.createChildPath(Constants::k_Float64));

  const auto* vertexGeom = dataStructure.getDataAs<VertexGeom>(DataPath({Constants::k_VertexGeom}));
  CheckGeom0D(vertexGeom, vertexArray->getId());

  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(DataPath({Constants::k_EdgeGeom}));
  CheckGeom1D(edgeGeom, vertexArray->getId(), edgeArray->getId());

  const auto* quadGeom = dataStructure.getDataAs<QuadGeom>(DataPath({Constants::k_QuadGeom}));
  CheckGeom2D(quadGeom, vertexArray->getId(), edgeArray->getId(), quadArray->getId());

  const auto* triGeom = dataStructure.getDataAs<TriangleGeom>(DataPath({Constants::k_TriangleGeom}));
  CheckGeom2D(triGeom, vertexArray->getId(), edgeArray->getId(), faceArray->getId());

  const auto* hexGeom = dataStructure.getDataAs<HexahedralGeom>(DataPath({Constants::k_HexGeom}));
  CheckGeom3D(hexGeom, vertexArray->getId(), edgeArray->getId(), hexaArray->getId(), polyArray->getId());

  const auto* tetraGeom = dataStructure.getDataAs<TetrahedralGeom>(DataPath({Constants::k_TetrahedralGeom}));
  CheckGeom3D(tetraGeom, vertexArray->getId(), edgeArray->getId(), tetraArray->getId(), polyArray->getId());

  // Image Geom
  DataPath imageGeomPath({Constants::k_ImageGeom});
  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  REQUIRE(imageGeom != nullptr);
  REQUIRE(imageGeom->getDimensions() == Constants::k_ImageShape);
  const auto* cellData = dataStructure.getDataAs<AttributeMatrix>(imageGeomPath.createChildPath(Constants::k_CellData));
  REQUIRE(cellData != nullptr);
  REQUIRE(imageGeom->getCellDataId() == cellData->getId());
  REQUIRE(cellData->getShape() == Constants::k_TupleShape);

  // RectGrid Geom
  DataPath rectGridGeomPath({Constants::k_RectGridGeom});
  const auto* rectGrid = dataStructure.getDataAs<RectGridGeom>(rectGridGeomPath);
  REQUIRE(rectGrid != nullptr);
  REQUIRE(rectGrid->getCellDataId() == cellData->getId());
  auto rectDims = rectGrid->getDimensions();
  REQUIRE(rectDims[0] == Constants::k_TupleShape[0]);
  REQUIRE(rectDims[1] == Constants::k_TupleShape[1]);
  REQUIRE(rectDims[2] == Constants::k_TupleShape[2]);
  DataPath xPath = rectGridGeomPath.createChildPath(Constants::k_XBounds);
  const auto* xBoundsArray = dataStructure.getDataAs<Float32Array>(xPath);
  REQUIRE(xBoundsArray != nullptr);
  DataPath yPath = rectGridGeomPath.createChildPath(Constants::k_YBounds);
  const auto* yBoundsArray = dataStructure.getDataAs<Float32Array>(yPath);
  REQUIRE(yBoundsArray != nullptr);
  DataPath zPath = rectGridGeomPath.createChildPath(Constants::k_ZBounds);
  const auto* zBoundsArray = dataStructure.getDataAs<Float32Array>(zPath);
  REQUIRE(zBoundsArray != nullptr);
  REQUIRE(rectGrid->getXBoundsId() == xBoundsArray->getId());
  REQUIRE(rectGrid->getYBoundsId() == yBoundsArray->getId());
  REQUIRE(rectGrid->getZBoundsId() == zBoundsArray->getId());
}

template <typename T>
void CreateScalarData(DataStructure& dataStructure, const std::string& name, DataObject::IdType parentId)
{
  auto* scalarData = ScalarData<T>::Create(dataStructure, name, static_cast<T>(Constants::k_ScalarValue), parentId);
  REQUIRE(scalarData != nullptr);
}

DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  auto group1 = DataGroup::Create(dataStructure, DataNames::k_Group1Name);
  auto group2 = DataGroup::Create(dataStructure, DataNames::k_Group2Name, group1->getId());
  auto group3 = DataGroup::Create(dataStructure, DataNames::k_Group3Name, group2->getId());

  ShapeType tupleShape = {10};
  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, DataNames::k_AttributeMatrixName, tupleShape, group1->getId());

  Result<> arrayCreationResults =
      ArrayCreationUtilities::CreateArray<int8>(dataStructure, tupleShape, std::vector<usize>{1}, DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name}),
                                                IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "1");

  // Create Arrays and DataGroup
  auto* dataGroup = DataGroup::Create(dataStructure, Constants::k_DataContainer);

  auto listStorePtr = std::make_shared<ListStore<int16>>(Constants::k_TupleShape);
  listStorePtr->setList(0, std::vector<int16>{1, 2});
  listStorePtr->setList(1, std::vector<int16>{1, 2});
  listStorePtr->setList(2, std::vector<int16>{1, 2});
  listStorePtr->setList(3, std::vector<int16>{1, 2});
  listStorePtr->setList(4, std::vector<int16>{1, 2});
  listStorePtr->setList(5, std::vector<int16>{1, 2});
  auto* neighborList = Int16NeighborList::Create(dataStructure, Constants::k_NeighborList, listStorePtr, dataGroup->getId());
  REQUIRE(neighborList != nullptr);

  auto vertices = std::make_shared<Float32DataStore>(Constants::k_TupleShape, ShapeType{3}, 0.0f);
  auto* vertexArray = Float32Array::Create(dataStructure, Constants::k_VertexList, vertices, dataGroup->getId());
  FillDataStore<float32>(*vertices.get());

  auto edges = std::make_shared<UInt64DataStore>(Constants::k_TupleShape, ShapeType{2}, 0);
  auto* edgesArray = IGeometry::SharedEdgeList::Create(dataStructure, Constants::k_Edges, edges, dataGroup->getId());
  FillDataStore<uint64>(*edges.get());

  auto triangles = std::make_shared<UInt64DataStore>(Constants::k_TupleShape, ShapeType{3}, 0);
  auto* trianglesArray = IGeometry::SharedTriList::Create(dataStructure, Constants::k_Faces, triangles, dataGroup->getId());
  FillDataStore<uint64>(*triangles.get());

  auto polyhedra = std::make_shared<UInt64DataStore>(Constants::k_TupleShape, ShapeType{4}, 0);
  auto* polyhedraArray = IGeometry::SharedFaceList::Create(dataStructure, Constants::k_Polyhedra, polyhedra, dataGroup->getId());
  FillDataStore<uint64>(*polyhedra.get());

  auto quadStore = std::make_shared<UInt64DataStore>(Constants::k_TupleShape, ShapeType{4}, 0);
  auto* quadArray = IGeometry::SharedFaceList::Create(dataStructure, Constants::k_QuadArray, quadStore, dataGroup->getId());
  FillDataStore<uint64>(*quadStore.get());

  auto hexStore = std::make_shared<UInt64DataStore>(Constants::k_TupleShape, ShapeType{8}, 0);
  auto* hexArray = IGeometry::SharedHexList::Create(dataStructure, Constants::k_HexaArray, hexStore, dataGroup->getId());
  FillDataStore<uint64>(*hexStore.get());

  auto tetraStore = std::make_shared<UInt64DataStore>(Constants::k_TupleShape, ShapeType{4}, 0);
  auto* tetraArray = IGeometry::SharedTetList::Create(dataStructure, Constants::k_TetraArray, tetraStore, dataGroup->getId());
  FillDataStore<uint64>(*tetraStore.get());

  StringArray::collection_type strings = {"1", "2", "3", "4", "5", "6"};
  auto* stringArray = StringArray::CreateWithValues(dataStructure, Constants::k_StringArray, Constants::k_TupleShape, strings, dataGroup->getId());
  REQUIRE(stringArray != nullptr);

  CreateScalarData<int8>(dataStructure, Constants::k_Int8, dataGroup->getId());
  CreateScalarData<int16>(dataStructure, Constants::k_Int16, dataGroup->getId());
  CreateScalarData<int32>(dataStructure, Constants::k_Int32, dataGroup->getId());
  CreateScalarData<int64>(dataStructure, Constants::k_Int64, dataGroup->getId());
  CreateScalarData<uint8>(dataStructure, Constants::k_UInt8, dataGroup->getId());
  CreateScalarData<uint16>(dataStructure, Constants::k_UInt16, dataGroup->getId());
  CreateScalarData<uint32>(dataStructure, Constants::k_UInt32, dataGroup->getId());
  CreateScalarData<uint64>(dataStructure, Constants::k_UInt64, dataGroup->getId());
  CreateScalarData<float32>(dataStructure, Constants::k_Float32, dataGroup->getId());
  CreateScalarData<float64>(dataStructure, Constants::k_Float64, dataGroup->getId());

  // Create Geometries and make sure special arrays are set.
  auto* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeom);
  REQUIRE(imageGeom != nullptr);
  auto* cellMatrix = AttributeMatrix::Create(dataStructure, Constants::k_CellData, Constants::k_TupleShape, imageGeom->getId());
  REQUIRE(cellMatrix != nullptr);
  imageGeom->setCellData(cellMatrix->getId());
  imageGeom->setDimensions(Constants::k_ImageShape);

  // RectGrid Data
  auto* rectGridGeom = RectGridGeom::Create(dataStructure, Constants::k_RectGridGeom);
  REQUIRE(rectGridGeom != nullptr);
  ShapeType xShape{Constants::k_TupleShape[0]};
  ShapeType componentBounds{1};
  auto xBounds = std::make_shared<Float32DataStore>(xShape, componentBounds, 0.0f);
  auto* xBoundsArray = Float32Array::Create(dataStructure, Constants::k_XBounds, xBounds, rectGridGeom->getId());
  FillDataStore<float32>(*xBounds.get());
  ShapeType yShape{Constants::k_TupleShape[1]};
  auto yBounds = std::make_shared<Float32DataStore>(yShape, componentBounds, 0.0f);
  auto* yBoundsArray = Float32Array::Create(dataStructure, Constants::k_YBounds, yBounds, rectGridGeom->getId());
  FillDataStore<float32>(*yBounds.get());
  ShapeType zShape{Constants::k_TupleShape[1]};
  auto zBounds = std::make_shared<Float32DataStore>(zShape, componentBounds, 0.0f);
  auto* zBoundsArray = Float32Array::Create(dataStructure, Constants::k_ZBounds, zBounds, rectGridGeom->getId());
  FillDataStore<float32>(*zBounds.get());

  rectGridGeom->setDimensions(Constants::k_TupleShape);
  rectGridGeom->setCellData(cellMatrix->getId());
  rectGridGeom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray);

  // 0D Geometry
  auto* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeom);
  REQUIRE(vertexGeom != nullptr);
  vertexGeom->setVertices(*vertexArray);

  // 1D Geometry
  auto* edgeGeom = EdgeGeom::Create(dataStructure, Constants::k_EdgeGeom);
  REQUIRE(edgeGeom != nullptr);
  edgeGeom->setVertices(*vertexArray);
  edgeGeom->setEdgeList(*edgesArray);

  // 2D Geometries
  auto* quadGeom = Create2DGeom<QuadGeom>(dataStructure, Constants::k_QuadGeom, *vertexArray, *edgesArray, *quadArray);
  REQUIRE(quadGeom != nullptr);
  auto* triangleGeom = Create2DGeom<TriangleGeom>(dataStructure, Constants::k_TriangleGeom, *vertexArray, *edgesArray, *trianglesArray);
  REQUIRE(triangleGeom != nullptr);

  // 3D Geometries
  auto* hexGeom = Create3DGeom<HexahedralGeom>(dataStructure, Constants::k_HexGeom, *vertexArray, *edgesArray, *hexArray, *polyhedraArray);
  REQUIRE(hexGeom != nullptr);
  auto* tetrahedralGeom = Create3DGeom<TetrahedralGeom>(dataStructure, Constants::k_TetrahedralGeom, *vertexArray, *edgesArray, *tetraArray, *polyhedraArray);
  REQUIRE(tetrahedralGeom != nullptr);

  return dataStructure;
}

Pipeline CreateExportPipeline()
{
  Pipeline pipeline("Export DREAM3D Pipeline 1");
  {
    Arguments args;
    args.insert("data_object_path", DataPath({DataNames::k_Group1Name}));
    pipeline.push_back(k_CreateDataGroupHandle, args);
  }
  {
    Arguments args;
    args.insert("set_tuple_dimensions", std::make_any<bool>(true));
    args.insert("numeric_type_index", std::make_any<NumericType>(NumericType::int8));
    args.insert("component_count", std::make_any<uint64>(3));

    args.insert("tuple_dimensions", DynamicTableInfo::TableDataType{{1.0}});
    args.insert("initialization_value_str", std::make_any<std::string>("7"));
    args.insert("output_array_path", DataPath({DataNames::k_ArrayName}));
    args.insert("data_format", std::string(""));
    pipeline.push_back(k_CreateDataArrayHandle, args);
  }
  {
    Arguments args;
    args.insert("export_file_path", GetExportDataPath());
    args.insert("write_xdmf_file", true);
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

Pipeline CreateImportPipeline()
{
  Pipeline pipeline("Import DREAM3D Pipeline");
  {
    Arguments args;
    auto filePath = GetExportDataPath();
    std::vector<DataPath> dataPaths = std::vector<DataPath>{DataPath({DataNames::k_Group1Name}), DataPath({DataNames::k_ArrayName})};
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::IncludeList, dataPaths);
    args.insert("import_data_object", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    args.insert("export_file_path", GetReExportDataPath());
    args.insert("write_xdmf_file", true);
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

void CreateMultiExportFiles()
{
  // Pipeline 1
  {
    Pipeline pipeline("Export Multi DREAM3D Pipeline 1");
    {
      Arguments args;
      args.insert("data_object_path", DataPath({DataNames::k_Group1Name}));
      pipeline.push_back(k_CreateDataGroupHandle, args);
    }
    {
      Arguments args;
      args.insert("export_file_path", GetMultiExportDataPath1());
      args.insert("write_xdmf_file", true);
      pipeline.push_back(k_ExportD3DHandle, args);
    }
    REQUIRE(pipeline.execute());
  }
  // Pipeline 2
  {
    Pipeline pipeline("Export Multi DREAM3D Pipeline 2");
    {
      Arguments args;
      args.insert("data_object_path", DataPath({DataNames::k_Group2Name}));
      pipeline.push_back(k_CreateDataGroupHandle, args);
    }
    {
      Arguments args;
      args.insert("export_file_path", GetMultiExportDataPath2());
      args.insert("write_xdmf_file", true);
      pipeline.push_back(k_ExportD3DHandle, args);
    }
    REQUIRE(pipeline.execute());
  }
}

Pipeline CreateMultiImportPipeline()
{
  Pipeline pipeline("Import DREAM3D Pipeline");
  {
    Arguments args;
    Dream3dImportParameter::ImportData importData(GetMultiExportDataPath1(), Dream3dImportParameter::PathImportPolicy::IncludeList, std::vector<DataPath>{DataPath({DataNames::k_Group1Name})});
    args.insert("import_data_object", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    Dream3dImportParameter::ImportData importData(GetMultiExportDataPath2(), Dream3dImportParameter::PathImportPolicy::IncludeList, std::vector<DataPath>{DataPath({DataNames::k_Group2Name})});
    args.insert("import_data_object", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    args.insert("export_file_path", GetReMultiExportDataPath());
    args.insert("write_xdmf_file", true);
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

DREAM3D::FileData CreateFileData()
{
  return {CreateExportPipeline(), CreateTestDataStructure()};
}

//------------------------------------------------------------------------------
// Helpers below build a small, valid instance of every Geometry type. Each accepts an
// optional parentId so the same geometry can be created at the top level or nested inside
// a DataGroup. They exercise the read/write code paths covered by issue #1642.

using MeshIndexType = IGeometry::MeshIndexType;

// Creates a 4-vertex coordinate array as a child of the geometry and wires it in.
Float32Array* CreateVertexList(DataStructure& dataStructure, INodeGeometry0D& geometry)
{
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "SharedVertexList", {4}, {3}, geometry.getId());
  for(usize i = 0; i < vertices->getSize(); i++)
  {
    (*vertices)[i] = static_cast<float32>(i);
  }
  geometry.setVertices(*vertices);
  AttributeMatrix* vertexMatrix = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{4}, geometry.getId());
  geometry.setVertexAttributeMatrix(*vertexMatrix);
  return vertices;
}

VertexGeom* CreateVertexGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* vertexGeom = VertexGeom::Create(dataStructure, geomName, parentId);
  CreateVertexList(dataStructure, *vertexGeom);
  return vertexGeom;
}

EdgeGeom* CreateEdgeGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* edgeGeom = EdgeGeom::Create(dataStructure, geomName, parentId);
  CreateVertexList(dataStructure, *edgeGeom);

  auto* edges = UnitTest::CreateTestDataArray<MeshIndexType>(dataStructure, "SharedEdgeList", {2}, {2}, edgeGeom->getId());
  (*edges)[0] = 0;
  (*edges)[1] = 1;
  (*edges)[2] = 2;
  (*edges)[3] = 3;
  edgeGeom->setEdgeList(*edges);
  AttributeMatrix* edgeMatrix = AttributeMatrix::Create(dataStructure, "Edge Data", ShapeType{2}, edgeGeom->getId());
  edgeGeom->setEdgeAttributeMatrix(*edgeMatrix);
  return edgeGeom;
}

TriangleGeom* CreateTriangleGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* triangleGeom = TriangleGeom::Create(dataStructure, geomName, parentId);
  CreateVertexList(dataStructure, *triangleGeom);

  auto* triangles = UnitTest::CreateTestDataArray<MeshIndexType>(dataStructure, "SharedTriList", {1}, {3}, triangleGeom->getId());
  (*triangles)[0] = 0;
  (*triangles)[1] = 1;
  (*triangles)[2] = 2;
  triangleGeom->setFaceList(*triangles);
  AttributeMatrix* faceMatrix = AttributeMatrix::Create(dataStructure, "Face Data", ShapeType{1}, triangleGeom->getId());
  triangleGeom->setFaceAttributeMatrix(*faceMatrix);
  return triangleGeom;
}

QuadGeom* CreateQuadGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* quadGeom = QuadGeom::Create(dataStructure, geomName, parentId);
  CreateVertexList(dataStructure, *quadGeom);

  auto* quads = UnitTest::CreateTestDataArray<MeshIndexType>(dataStructure, "SharedQuadList", {1}, {4}, quadGeom->getId());
  (*quads)[0] = 0;
  (*quads)[1] = 1;
  (*quads)[2] = 2;
  (*quads)[3] = 3;
  quadGeom->setFaceList(*quads);
  AttributeMatrix* faceMatrix = AttributeMatrix::Create(dataStructure, "Face Data", ShapeType{1}, quadGeom->getId());
  quadGeom->setFaceAttributeMatrix(*faceMatrix);
  return quadGeom;
}

TetrahedralGeom* CreateTetrahedralGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* tetGeom = TetrahedralGeom::Create(dataStructure, geomName, parentId);
  CreateVertexList(dataStructure, *tetGeom);

  auto* tets = UnitTest::CreateTestDataArray<MeshIndexType>(dataStructure, "SharedTetList", {1}, {4}, tetGeom->getId());
  (*tets)[0] = 0;
  (*tets)[1] = 1;
  (*tets)[2] = 2;
  (*tets)[3] = 3;
  tetGeom->setPolyhedraList(*tets);
  AttributeMatrix* cellMatrix = AttributeMatrix::Create(dataStructure, "Polyhedron Data", ShapeType{1}, tetGeom->getId());
  tetGeom->setPolyhedraAttributeMatrix(*cellMatrix);
  return tetGeom;
}

HexahedralGeom* CreateHexahedralGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* hexGeom = HexahedralGeom::Create(dataStructure, geomName, parentId);
  // A hexahedron references 8 vertices, so create 8 here instead of the default 4.
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "SharedVertexList", {8}, {3}, hexGeom->getId());
  for(usize i = 0; i < vertices->getSize(); i++)
  {
    (*vertices)[i] = static_cast<float32>(i);
  }
  hexGeom->setVertices(*vertices);
  AttributeMatrix* vertexMatrix = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{8}, hexGeom->getId());
  hexGeom->setVertexAttributeMatrix(*vertexMatrix);

  auto* hexes = UnitTest::CreateTestDataArray<MeshIndexType>(dataStructure, "SharedHexList", {1}, {8}, hexGeom->getId());
  for(usize i = 0; i < 8; i++)
  {
    (*hexes)[i] = i;
  }
  hexGeom->setPolyhedraList(*hexes);
  AttributeMatrix* cellMatrix = AttributeMatrix::Create(dataStructure, "Polyhedron Data", ShapeType{1}, hexGeom->getId());
  hexGeom->setPolyhedraAttributeMatrix(*cellMatrix);
  return hexGeom;
}

ImageGeom* CreateImageGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* imageGeom = ImageGeom::Create(dataStructure, geomName, parentId);
  imageGeom->setDimensions(SizeVec3{2, 2, 2});
  imageGeom->setOrigin(FloatVec3{0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing(FloatVec3{1.0F, 1.0F, 1.0F});
  AttributeMatrix* cellMatrix = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 2, 2}, imageGeom->getId());
  imageGeom->setCellData(*cellMatrix);
  return imageGeom;
}

RectGridGeom* CreateRectGridGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* rectGridGeom = RectGridGeom::Create(dataStructure, geomName, parentId);
  rectGridGeom->setDimensions(SizeVec3{2, 2, 2});

  auto* xBounds = UnitTest::CreateTestDataArray<float32>(dataStructure, "X Bounds", {3}, {1}, rectGridGeom->getId());
  auto* yBounds = UnitTest::CreateTestDataArray<float32>(dataStructure, "Y Bounds", {3}, {1}, rectGridGeom->getId());
  auto* zBounds = UnitTest::CreateTestDataArray<float32>(dataStructure, "Z Bounds", {3}, {1}, rectGridGeom->getId());
  for(usize i = 0; i < 3; i++)
  {
    (*xBounds)[i] = static_cast<float32>(i);
    (*yBounds)[i] = static_cast<float32>(i);
    (*zBounds)[i] = static_cast<float32>(i);
  }
  rectGridGeom->setBounds(xBounds, yBounds, zBounds);
  AttributeMatrix* cellMatrix = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 2, 2}, rectGridGeom->getId());
  rectGridGeom->setCellData(*cellMatrix);
  return rectGridGeom;
}

// A single named geometry-builder + a type-checked verifier used to drive the parameterized
// nested-geometry round-trip test below.
struct GeometryTestCase
{
  std::string typeName;
  std::function<void(DataStructure&, const std::string&, const std::optional<DataObject::IdType>&)> build;
  std::function<void(const DataStructure&, const DataPath&)> requireType;
};

template <typename GeomType>
GeometryTestCase MakeGeometryTestCase(std::string typeName, std::function<GeomType*(DataStructure&, const std::string&, const std::optional<DataObject::IdType>&)> builder)
{
  GeometryTestCase testCase;
  testCase.typeName = std::move(typeName);
  testCase.build = [builder](DataStructure& dataStructure, const std::string& name, const std::optional<DataObject::IdType>& parentId) { builder(dataStructure, name, parentId); };
  testCase.requireType = [](const DataStructure& dataStructure, const DataPath& path) { REQUIRE_NOTHROW(dataStructure.getDataRefAs<GeomType>(path)); };
  return testCase;
}

} // End Namespace

TEST_CASE("WriteDREAM3DFilter:Invalid Parameters", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;
  WriteDREAM3DFilter filter;

  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, std::make_any<FileSystemPathParameter::ValueType>(GetIODataPath()));
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, std::make_any<bool>(false));
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, std::make_any<bool>(false));
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, std::make_any<int32>(1));

  SECTION("Empty FilePath")
  {
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, std::make_any<FileSystemPathParameter::ValueType>(std::filesystem::path()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  SECTION("Bad Compression Level")
  {
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, std::make_any<bool>(true));
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, std::make_any<int32>(0));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
}

TEST_CASE("WriteDREAM3DFilter:Valid Parameters", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  {
    DataStructure dataStructure = CreateTestDataStructure();
    Arguments args;
    WriteDREAM3DFilter filter;

    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, std::make_any<FileSystemPathParameter::ValueType>(GetIODataPath()));
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, std::make_any<bool>(false));
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, std::make_any<bool>(false));
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, std::make_any<int32>(1));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }

  // Check that the output file exists
  REQUIRE(fs::exists(GetIODataPath()));

  // Check that the file can be read back in and that the imported DataStructure matches expected values.
  {
    auto fileReader = HDF5::FileIO::ReadFile(GetIODataPath());
    auto fileResult = DREAM3D::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(fileResult);

    auto [pipeline, dataStructureRead] = fileResult.value();

    CheckTestDataStructure(dataStructureRead);
  }
}

void CheckXdmfFile()
{
  auto filepath = GetXdmfPath();
  REQUIRE(fs::exists(filepath));
}

TEST_CASE("WriteDREAM3D:Pipeline / WriteXdmf combinations", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  bool writeXdmf = GENERATE(true, false);
  Pipeline exportPipeline = GENERATE(CreateExportPipeline(), Pipeline());

  auto writeResult = DREAM3D::WriteFile(GetIODataPath(), CreateTestDataStructure(), exportPipeline, writeXdmf);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  if(writeXdmf)
  {
    CheckXdmfFile();
  }
}

TEST_CASE("WriteDREAM3D:Invalid File", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  bool writeXdmf = GENERATE(true, false);
  Pipeline exportPipeline = GENERATE(CreateExportPipeline(), Pipeline());

  auto writeResult = DREAM3D::WriteFile(fs::path(), CreateTestDataStructure(), exportPipeline, writeXdmf);
  SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);

  if(writeXdmf)
  {
    CheckXdmfFile();
  }
}

TEST_CASE("DREAM3DFileTest:DREAM3D File IO Test", "[WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();

  std::lock_guard<std::mutex> lock(m_DataMutex);

  bool writeXdmf = GENERATE(true, false);
  // Write .dream3d file
  {
    auto writeResult = DREAM3D::WriteFile(GetIODataPath(), CreateTestDataStructure(), CreateExportPipeline(), writeXdmf);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

    if(writeXdmf)
    {
      CheckXdmfFile();
    }
  }

  // Read .dream3d file
  {
    auto fileReader = HDF5::FileIO::ReadFile(GetIODataPath());
    auto fileResult = DREAM3D::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(fileResult);

    auto [pipeline, dataStructure] = fileResult.value();

    CheckTestDataStructure(dataStructure);

    // Test reading the DataStructure
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name, DataNames::k_Group3Name})) != nullptr);
    auto attMatrix = dataStructure.getDataAs<AttributeMatrix>(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName}));
    REQUIRE(attMatrix != nullptr);
    REQUIRE(attMatrix->getShape() == ShapeType{10});
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name})) != nullptr);

    // Test reading the Pipeline
    REQUIRE(pipeline.size() == 3);
    REQUIRE(pipeline[0]->getName() == DataNames::k_CreateDataFilterName.str());
    REQUIRE(pipeline[2]->getName() == DataNames::k_ExportD3DFilterName.str());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("DREAM3DFileTest::StringArray", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  auto app = Application::GetOrCreateInstance();

  fs::path path = GetDataDir(*app) / "StringArray.dream3d";

  DataStructure exportDataStructure;

  DataPath stringArrayPath({"StringArray"});

  std::vector<std::string> values = {"foo", "bar", "baz"};

  REQUIRE(StringArray::CreateWithValues(exportDataStructure, stringArrayPath.getTargetName(), ShapeType{3}, values) != nullptr);

  WriteDREAM3DFilter writeDream3dFilter;
  Arguments writeArgs;
  writeArgs.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, path);
  writeArgs.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  Result<> writeResult = writeDream3dFilter.execute(exportDataStructure, writeArgs).result;
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  DataStructure importDataStructure;

  ReadDREAM3DFilter readDream3dFilter;
  Arguments readArgs;
  Dream3dImportParameter::ImportData importData(path);
  readArgs.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, importData);
  Result<> readResult = readDream3dFilter.execute(importDataStructure, readArgs).result;
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);

  const StringArray* stringArray = importDataStructure.getDataAs<StringArray>(stringArrayPath);
  REQUIRE(stringArray != nullptr);
  REQUIRE(std::equal(stringArray->begin(), stringArray->end(), values.begin(), values.end()));
}

TEST_CASE("DREAM3DFileTest:Import/Export DREAM3D Filter Test", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();

  std::lock_guard<std::mutex> lock(m_DataMutex);

  auto exportPipeline = CreateExportPipeline();
  REQUIRE(exportPipeline.execute());

  {
    auto importPipeline = CreateImportPipeline();
    REQUIRE(importPipeline.execute());
    auto importDataStructure = importPipeline[0]->getDataStructure();
    auto group1Obj = importDataStructure.getData(DataPath({DataNames::k_Group1Name}));
    auto size = importDataStructure.getSize();
    REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    auto* dataArray = importDataStructure.getDataAs<DataArray<int8>>(DataPath({DataNames::k_ArrayName}));
    REQUIRE(dataArray != nullptr);
    REQUIRE(dataArray->getIDataStoreAs<AbstractDataStore<int8>>() != nullptr);

    UnitTest::CheckArraysInheritTupleDims(importDataStructure);
  }
  {
    auto importPipeline = CreateImportPipeline();
    REQUIRE(importPipeline.preflight());
    auto importDataStructure = importPipeline[0]->getPreflightStructure();
    auto group1Obj = importDataStructure.getData(DataPath({DataNames::k_Group1Name}));
    auto size = importDataStructure.getSize();
    REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    auto* dataArray = importDataStructure.getDataAs<DataArray<int8>>(DataPath({DataNames::k_ArrayName}));
    REQUIRE(dataArray != nullptr);
    REQUIRE(dataArray->template getIDataStoreAs<EmptyDataStore<int8>>() != nullptr);

    UnitTest::CheckArraysInheritTupleDims(importDataStructure);
  }
}

TEST_CASE("DREAM3DFileTest: Preflight imports geometry connectivity as metadata-only stores", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();

  // geoms.dream3d ships one geometry of every type whose connectivity was formerly registered as
  // "required" preflight data: node geometries (vertex/edge/face/polyhedron lists) and a rectilinear
  // grid (X/Y/Z bounds). A metadata-only preflight must import all of these exactly like ordinary
  // attribute arrays: as empty stores that carry only shape and type, never eagerly bulk-read from disk.
  const fs::path inputFilePath = fs::path(unit_test::k_SimplnxTestDataSourceDir.view()) / "geoms.dream3d";

  // {connectivity array path, expected tuple count}
  const std::vector<std::pair<DataPath, usize>> connectivityPaths = {
      {DataPath({"EdgeGeometry", "Verts"}), 10},       {DataPath({"EdgeGeometry", "Edges"}), 5},       {DataPath({"TriangleGeometry", "Verts"}), 10}, {DataPath({"TriangleGeometry", "Triangles"}), 4},
      {DataPath({"QuadGeometry", "Quads"}), 2},        {DataPath({"TetrahedralGeometry", "Tets"}), 1}, {DataPath({"HexahedralGeometry", "Hexs"}), 3}, {DataPath({"RectGridGeometry", "XBounds"}), 10},
      {DataPath({"RectGridGeometry", "YBounds"}), 10}, {DataPath({"RectGridGeometry", "ZBounds"}), 10}};

  const auto isEmptyStore = [](const IDataStore* store) {
    return store != nullptr && (store->getStoreType() == IDataStore::StoreType::Empty || store->getStoreType() == IDataStore::StoreType::EmptyOutOfCore);
  };

  // Preflight (useEmptyDataStores == true): connectivity must remain an empty, metadata-only store.
  {
    auto readResult = DREAM3D::ImportDataStructureFromFile(inputFilePath, /*preflight=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    const DataStructure preflightStructure = std::move(readResult.value());

    for(const auto& [dataPath, numTuples] : connectivityPaths)
    {
      DYNAMIC_SECTION("Preflight: " << dataPath.toString())
      {
        const auto* dataArray = preflightStructure.getDataAs<IDataArray>(dataPath);
        REQUIRE(dataArray != nullptr);
        REQUIRE(isEmptyStore(dataArray->getIDataStore()));

        // Shape metadata is still available from the empty store, which is all preflight requires.
        REQUIRE(dataArray->getNumberOfTuples() == numTuples);
      }
    }
  }

  // Execute (useEmptyDataStores == false): connectivity is fully read from disk into a real store.
  {
    auto readResult = DREAM3D::ImportDataStructureFromFile(inputFilePath, /*preflight=*/false);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    const DataStructure executedStructure = std::move(readResult.value());

    for(const auto& [dataPath, numTuples] : connectivityPaths)
    {
      DYNAMIC_SECTION("Execute: " << dataPath.toString())
      {
        const auto* dataArray = executedStructure.getDataAs<IDataArray>(dataPath);
        REQUIRE(dataArray != nullptr);
        REQUIRE_FALSE(isEmptyStore(dataArray->getIDataStore()));
        REQUIRE(dataArray->getNumberOfTuples() == numTuples);
      }
    }
  }

  // Read directly through the DataStructureReader path overload with useEmptyDataStores == true. This
  // overload must honor the flag when it delegates to the FileIO overload; connectivity must come back
  // as an empty, metadata-only store rather than being fully read from disk.
  {
    auto readResult = HDF5::DataStructureReader::ReadFile(inputFilePath, /*useEmptyDataStores=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    const DataStructure preflightStructure = std::move(readResult.value());

    for(const auto& [dataPath, numTuples] : connectivityPaths)
    {
      DYNAMIC_SECTION("DataStructureReader::ReadFile: " << dataPath.toString())
      {
        const auto* dataArray = preflightStructure.getDataAs<IDataArray>(dataPath);
        REQUIRE(dataArray != nullptr);
        REQUIRE(isEmptyStore(dataArray->getIDataStore()));
      }
    }
  }
}

TEST_CASE("DREAM3DFileTest:Import/Export Multi-DREAM3D Filter Test", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();

  std::lock_guard<std::mutex> lock(m_DataMutex);

  CreateMultiExportFiles();

  auto importPipeline = CreateMultiImportPipeline();
  REQUIRE(importPipeline.execute());
  auto importDataStructure = importPipeline[1]->getDataStructure();
  auto size = importDataStructure.getSize();
  REQUIRE(size == 2);
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group2Name})) != nullptr);

  UnitTest::CheckArraysInheritTupleDims(importDataStructure);
}

TEST_CASE("DREAM3DFileTest: Existing Data Objects Test", "[ReadDREAM3DFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure ds;
  {
    CreateImageGeometryFilter filter;
    Arguments args;
    args.insert(CreateImageGeometryFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(DataPath({"New Geometry"})));
    args.insert(CreateImageGeometryFilter::k_CellDataName_Key, std::make_any<std::string>("Cell Data"));
    args.insert(CreateImageGeometryFilter::k_Dimensions_Key, std::make_any<std::vector<uint64_t>>(std::vector<uint64_t>{480, 640, 1}));
    args.insert(CreateImageGeometryFilter::k_Origin_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0, 0, 0}));
    args.insert(CreateImageGeometryFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0.5, 0.5, 0.12}));
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    CreateDataArrayFilter filter;
    Arguments args;
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"New Geometry", "Cell Data", "Array 1"})));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("0"));
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    CreateDataArrayFilter filter;
    Arguments args;
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"New Geometry", "Cell Data", "Array 2"})));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("0"));
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

    ReadDREAM3DFilter filter;
    Arguments args;
    Dream3dImportParameter::ImportData importData(fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir)));
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(ds, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("DREAM3DFileTest: Path Import Policy Tests", "[ReadDREAM3DFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");
  auto filePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure;
  ReadDREAM3DFilter filter;
  Arguments args;

  SECTION("All")
  {
    Dream3dImportParameter::ImportData importData(filePath);
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    std::vector<std::string> baseDCNames = {"DataContainer", "SmallerDataContainer"};
    std::vector<std::string> nlAndStringArrayDCs = {"MirroredXDataContainer",      "MirroredYDataContainer", "MirroredZDataContainer", "MirroredXInconsistentArrays", "MirroredYInconsistentArrays",
                                                    "MirroredZInconsistentArrays", "XInconsistentArrays",    "YInconsistentArrays",    "ZInconsistentArrays"};
    std::vector<std::string> fooArrayDCs = {"MirroredXInconsistentArrays", "MirroredYInconsistentArrays", "MirroredZInconsistentArrays",
                                            "XInconsistentArrays",         "YInconsistentArrays",         "ZInconsistentArrays"};
    std::vector<std::string> dcNames;
    dcNames.reserve(baseDCNames.size() + nlAndStringArrayDCs.size() + fooArrayDCs.size());
    dcNames.insert(dcNames.end(), baseDCNames.begin(), baseDCNames.end());
    dcNames.insert(dcNames.end(), nlAndStringArrayDCs.begin(), nlAndStringArrayDCs.end());
    dcNames.insert(dcNames.end(), fooArrayDCs.begin(), fooArrayDCs.end());

    for(const auto& dcName : dcNames)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Confidence Index"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "EulerAngles"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Fit"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Image Quality"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Phases"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "SEM Signal"})));
    }

    for(const auto& dcName : baseDCNames)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData", "CrystalStructures"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData", "LatticeConstants"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData", "MaterialName"})));
    }

    for(const auto& dcName : nlAndStringArrayDCs)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "NeighborList"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "StringArray"})));
    }

    for(const auto& dcName : fooArrayDCs)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Foo"})));
    }
  }
  SECTION("Include List - Leaf Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::IncludeList, {DataPath({"DataContainer", "CellData", "Confidence Index"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData", "Confidence Index"})));
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
  }
  SECTION("Include List - Parent Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::IncludeList, {DataPath({"DataContainer", "CellData"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData", "Confidence Index"})));
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
  }
  SECTION("Exclude List - Parent Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::ExcludeList, {DataPath({"DataContainer", "CellData"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  }

  SECTION("Exclude List - Leaf Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::ExcludeList, {DataPath({"DataContainer", "CellData", "Confidence Index"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellData", "Confidence Index"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData", "Fit"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::ReadDREAM3DFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadDREAM3DFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadDREAM3DFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadDREAM3DFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadDREAM3DFilter>::uuid);

      // Complex parameter type (DataContainerReaderFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

TEST_CASE("SimplnxCore::WriteDREAM3DFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteDREAM3DFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteDREAM3DFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteDREAM3DFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<WriteDREAM3DFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteDREAM3DFilter::k_ExportFilePath) == fs::path("/test/path/output.dream3d"));
      CHECK(args.value<bool>(WriteDREAM3DFilter::k_WriteXdmf) == true);
      CHECK(args.value<bool>(WriteDREAM3DFilter::k_UseCompression) == false);
    }
  }
}

TEST_CASE("DREAM3DFileTest: DataArray datasets are chunked+deflated when WriteOptions requests it", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();

  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dream3d_compressed.dream3d";
  fs::remove(outPath);

  DataStructure dataStructure;
  const DataPath arrayPath({"LargeArray"});
  constexpr usize k_Tuples = 500'000; // 2 MB, above the 16 KiB small-array bypass
  auto createRes = ArrayCreationUtilities::CreateArray<float32>(dataStructure, std::vector<usize>{k_Tuples}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute,
                                                                ArrayCreationUtilities::k_DefaultDataFormat, "0");
  SIMPLNX_RESULT_REQUIRE_VALID(createRes);
  {
    auto& arr = dataStructure.getDataRefAs<DataArray<float32>>(arrayPath);
    auto& store = arr.getDataStoreRef();
    for(usize i = 0; i < k_Tuples; ++i)
    {
      store[i] = static_cast<float32>(i);
    }
  }

  HDF5::DataStructureWriter::WriteOptions options;
  options.compressionLevel = 5;
  auto writeResult = DREAM3D::WriteFile(outPath, dataStructure, Pipeline{}, false, options);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  const std::string hdfPath = std::string("/") + nx::core::Constants::k_DataStructureTag + "/LargeArray";
  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, hdfPath);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(info->hasDeflate);
  REQUIRE(info->deflateLevel == 5);

  auto fileReader = HDF5::FileIO::ReadFile(outPath);
  REQUIRE(fileReader.isValid());
  auto fileResult = DREAM3D::ReadFile(fileReader);
  SIMPLNX_RESULT_REQUIRE_VALID(fileResult);
  auto [pipeline, importedDs] = std::move(fileResult.value());
  (void)pipeline;
  REQUIRE_NOTHROW(importedDs.getDataRefAs<DataArray<float32>>(arrayPath));
  const auto& imported = importedDs.getDataRefAs<DataArray<float32>>(arrayPath);
  const auto& original = dataStructure.getDataRefAs<DataArray<float32>>(arrayPath);
  REQUIRE(imported.getSize() == original.getSize());
  UnitTest::CompareDataArrays<float32>(original, imported);
  UnitTest::CheckArraysInheritTupleDims(importedDs);
}

TEST_CASE("WriteDREAM3DFilter: Compression_Off_IsContiguous", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_off.dream3d";
  fs::remove(outPath);

  DataStructure ds;
  const DataPath arrayPath({"A"});
  auto cr =
      ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{200'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "1.5");
  SIMPLNX_RESULT_REQUIRE_VALID(cr);

  WriteDREAM3DFilter filter;
  Arguments args;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(5));
  auto r = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(r);

  const std::string hdfPath = std::string("/") + nx::core::Constants::k_DataStructureTag + "/A";
  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, hdfPath);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Contiguous);
  REQUIRE(info->hasDeflate == false);
}

TEST_CASE("WriteDREAM3DFilter: Compression_On_IsChunkedAndDeflated", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_on.dream3d";
  fs::remove(outPath);

  DataStructure ds;
  const DataPath arrayPath({"A"});
  auto cr =
      ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{500'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "0");
  SIMPLNX_RESULT_REQUIRE_VALID(cr);
  {
    auto& arr = ds.getDataRefAs<DataArray<float32>>(arrayPath);
    auto& store = arr.getDataStoreRef();
    for(usize i = 0; i < 500'000; ++i)
    {
      store[i] = static_cast<float32>(i);
    }
  }

  WriteDREAM3DFilter filter;
  Arguments args;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(5));
  auto r = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(r);

  const std::string hdfPath = std::string("/") + nx::core::Constants::k_DataStructureTag + "/A";
  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, hdfPath);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(info->hasDeflate);
  REQUIRE(info->deflateLevel == 5);

  auto fr = nx::core::HDF5::FileIO::ReadFile(outPath);
  auto fileResult = nx::core::DREAM3D::ReadFile(fr);
  SIMPLNX_RESULT_REQUIRE_VALID(fileResult);
  auto [unusedPipeline, imported] = std::move(fileResult.value());
  (void)unusedPipeline;
  REQUIRE_NOTHROW(imported.getDataRefAs<DataArray<float32>>(arrayPath));
  const auto& importedArr = imported.getDataRefAs<DataArray<float32>>(arrayPath);
  const auto& originalArr = ds.getDataRefAs<DataArray<float32>>(arrayPath);
  UnitTest::CompareDataArrays<float32>(originalArr, importedArr);
  UnitTest::CheckArraysInheritTupleDims(imported);
}

TEST_CASE("WriteDREAM3DFilter: Compression_SmallArray_Bypasses", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_small_bypass.dream3d";
  fs::remove(outPath);

  DataStructure ds;
  auto crSmall = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{100}, std::vector<usize>{1}, DataPath({"Small"}), IDataAction::Mode::Execute,
                                                              ArrayCreationUtilities::k_DefaultDataFormat, "2");
  SIMPLNX_RESULT_REQUIRE_VALID(crSmall);
  auto crBig = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{500'000}, std::vector<usize>{1}, DataPath({"Big"}), IDataAction::Mode::Execute,
                                                            ArrayCreationUtilities::k_DefaultDataFormat, "3");
  SIMPLNX_RESULT_REQUIRE_VALID(crBig);

  WriteDREAM3DFilter filter;
  Arguments args;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(5));
  auto r = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(r);

  const std::string dsRoot = std::string("/") + nx::core::Constants::k_DataStructureTag;
  auto smallInfo = nx::core::UnitTest::ProbeHdf5Dataset(outPath, dsRoot + "/Small");
  auto bigInfo = nx::core::UnitTest::ProbeHdf5Dataset(outPath, dsRoot + "/Big");
  REQUIRE(smallInfo.has_value());
  REQUIRE(bigInfo.has_value());
  REQUIRE(smallInfo->layout == nx::core::UnitTest::DatasetLayout::Contiguous);
  REQUIRE(smallInfo->hasDeflate == false);
  REQUIRE(bigInfo->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(bigInfo->hasDeflate);
}

TEST_CASE("WriteDREAM3DFilter: Compression_LevelsRoundTrip", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();

  // Accumulate sizes across a single TEST_CASE run so the monotonicity check at the end actually fires.
  // (DYNAMIC_SECTION would restart the body per section, losing accumulated state.)
  std::vector<std::uintmax_t> sizesByLevel;
  const std::vector<int32> levels = {1, 5, 9};

  for(int32 level : levels)
  {
    const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("compression_rt_level_{}.dream3d", level);
    fs::remove(outPath);

    DataStructure ds;
    const DataPath arrayPath({"A"});
    auto cr =
        ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{1'000'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "0");
    SIMPLNX_RESULT_REQUIRE_VALID(cr);
    {
      auto& arr = ds.getDataRefAs<DataArray<float32>>(arrayPath);
      auto& store = arr.getDataStoreRef();
      for(usize i = 0; i < 1'000'000; ++i)
      {
        store[i] = static_cast<float32>(i % 1024);
      }
    }

    WriteDREAM3DFilter filter;
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, level);
    auto r = filter.execute(ds, args).result;
    SIMPLNX_RESULT_REQUIRE_VALID(r);

    auto fr = HDF5::FileIO::ReadFile(outPath);
    REQUIRE(fr.isValid());
    auto fileResult = DREAM3D::ReadFile(fr);
    SIMPLNX_RESULT_REQUIRE_VALID(fileResult);
    auto [unusedPipeline, imported] = std::move(fileResult.value());
    (void)unusedPipeline;
    REQUIRE_NOTHROW(imported.getDataRefAs<DataArray<float32>>(arrayPath));
    UnitTest::CompareDataArrays<float32>(ds.getDataRefAs<DataArray<float32>>(arrayPath), imported.getDataRefAs<DataArray<float32>>(arrayPath));

    // Push after round-trip validation — a corrupt level entry should not enter the monotonicity check.
    sizesByLevel.push_back(fs::file_size(outPath));
  }

  // Size non-increasing as level rises. Probabilistic in the general case but reliable for the i%1024 pattern
  // used here (small symbol alphabet, long runs → deflate dictionary is very effective).
  REQUIRE(sizesByLevel.size() == levels.size());
  REQUIRE(sizesByLevel[0] >= sizesByLevel[1]);
  REQUIRE(sizesByLevel[1] >= sizesByLevel[2]);
}

TEST_CASE("DREAM3DFileTest: Geometry Nested In DataGroup Round Trip", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  Application::GetOrCreateInstance();

  const std::vector<GeometryTestCase> testCases = {
      MakeGeometryTestCase<VertexGeom>("VertexGeom", CreateVertexGeometry),
      MakeGeometryTestCase<EdgeGeom>("EdgeGeom", CreateEdgeGeometry),
      MakeGeometryTestCase<TriangleGeom>("TriangleGeom", CreateTriangleGeometry),
      MakeGeometryTestCase<QuadGeom>("QuadGeom", CreateQuadGeometry),
      MakeGeometryTestCase<TetrahedralGeom>("TetrahedralGeom", CreateTetrahedralGeometry),
      MakeGeometryTestCase<HexahedralGeom>("HexahedralGeom", CreateHexahedralGeometry),
      MakeGeometryTestCase<ImageGeom>("ImageGeom", CreateImageGeometry),
      MakeGeometryTestCase<RectGridGeom>("RectGridGeom", CreateRectGridGeometry),
  };

  for(const auto& testCase : testCases)
  {
    DYNAMIC_SECTION(testCase.typeName)
    {
      const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("nested_geometry_{}.dream3d", testCase.typeName);
      fs::remove(outPath);

      // Build a DataStructure with the geometry at the top level (control) and an identical
      // geometry nested inside a DataGroup (the case that previously crashed).
      DataStructure dataStructure;
      const std::string topName = "Top";
      const std::string nestName = "Nested";
      testCase.build(dataStructure, topName, {});
      auto* group = DataGroup::Create(dataStructure, "Grp");
      testCase.build(dataStructure, nestName, group->getId());

      // Write the file
      {
        WriteDREAM3DFilter writeFilter;
        Arguments writeArgs;
        writeArgs.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
        writeArgs.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
        Result<> writeResult = writeFilter.execute(dataStructure, writeArgs).result;
        SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
      }

      // Read the file back. This must not crash and must succeed.
      DataStructure importDataStructure;
      {
        ReadDREAM3DFilter readFilter;
        Arguments readArgs;
        Dream3dImportParameter::ImportData importData(outPath);
        readArgs.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, importData);
        Result<> readResult = readFilter.execute(importDataStructure, readArgs).result;
        SIMPLNX_RESULT_REQUIRE_VALID(readResult);
      }

      // Both the nested geometry and the top-level control must survive the round trip with
      // the correct concrete type.
      const DataPath nestedPath({"Grp", nestName});
      testCase.requireType(importDataStructure, nestedPath);
      testCase.requireType(importDataStructure, DataPath({topName}));

      UnitTest::CheckArraysInheritTupleDims(importDataStructure);
    }
  }
}

TEST_CASE("WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_preflight.dream3d";
  DataStructure ds;

  WriteDREAM3DFilter filter;

  // use_compression=true + level below [1,9] -> preflight error
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(0));
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }

  // use_compression=true + level above [1,9] -> preflight error
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(10));
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }

  // use_compression=false -> level is ignored, even if out of range
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(0));
    REQUIRE(filter.preflight(ds, args).outputActions.valid());
  }
}

TEST_CASE("DREAM3DFileTest: PreflightCache avoids re-reading unchanged files", "[ReadDREAM3DFilter][PreflightCache]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Build and write a small file, then backdate its mtime past the cache's
  // trust window so hit/miss behavior is deterministic.
  DataStructure writtenData;
  auto* group = DataGroup::Create(writtenData, "CacheGroup");
  auto floatStore = std::make_shared<DataStore<float32>>(ShapeType{16}, ShapeType{1}, 2.5f);
  Float32Array::Create(writtenData, "CacheFloats", floatStore, group->getId());
  fs::path filePath = GetDataDir(*Application::Instance()) / "preflight_cache_integration.dream3d";
  fs::remove(filePath);
  REQUIRE(DREAM3D::WriteFile(filePath, writtenData, Pipeline{}, false).valid());
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));

  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  cache.resetStats();
  HDF5::FileIO::ResetReadOpenCount();

  // Pipeline with a single ReadDREAM3DFilter, exactly as the GUI/nxrunner run it.
  Pipeline pipeline;
  Arguments args;
  Dream3dImportParameter::ImportData importData(filePath);
  args.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, importData);
  auto filterNode = PipelineFilter::Create(k_ImportD3DHandle);
  filterNode->setArguments(args);
  pipeline.push_back(std::move(filterNode));

  // Preflight #1 populates the cache: exactly one miss (the filter's fetch reads
  // the file's metadata once). The import action's fetch on the same file is a
  // hit, so it performs no second metadata traversal.
  REQUIRE(pipeline.preflight());
  REQUIRE(cache.missCount() == 1);

  // Preflight #2 is the per-edit path. The cache serves the metadata, so there
  // is no new miss and no metadata re-traversal -- the property that keeps
  // re-preflight cheap on high-latency storage.
  HDF5::FileIO::ResetReadOpenCount();
  REQUIRE(pipeline.preflight());
  REQUIRE(cache.missCount() == 1);
  // A fully-cached preflight opens the file zero times: Dream3dImportParameter's
  // validate() only stats the path (no HDF5 open), and the cache serves the
  // filter's metadata fetch from memory instead of re-reading the file.
  REQUIRE(HDF5::FileIO::GetReadOpenCount() == 0);

  // Execute: bulk data must still be read correctly from disk.
  REQUIRE(pipeline.execute());
  const DataStructure executed = pipeline[0]->getDataStructure();
  const auto* readFloats = executed.getDataAs<Float32Array>(DataPath({"CacheGroup", "CacheFloats"}));
  REQUIRE(readFloats != nullptr);
  REQUIRE((*readFloats)[0] == 2.5f);

  // Rewriting the file must invalidate: preflight sees the new content.
  DataStructure newData;
  auto* group2 = DataGroup::Create(newData, "CacheGroup");
  auto intStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, 9);
  Int32Array::Create(newData, "CacheInts", intStore, group2->getId());
  fs::remove(filePath);
  REQUIRE(DREAM3D::WriteFile(filePath, newData, Pipeline{}, false).valid());
  fs::last_write_time(filePath, fs::last_write_time(filePath) - std::chrono::seconds(10));

  const uint64 missesBeforeRewritePreflight = cache.missCount();
  REQUIRE(pipeline.preflight());
  REQUIRE(cache.missCount() == missesBeforeRewritePreflight + 1);
}
