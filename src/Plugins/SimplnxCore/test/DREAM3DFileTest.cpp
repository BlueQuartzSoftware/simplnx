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
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
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
#include <fstream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @namespace Constants
 * @brief Provides temporary DREAM3D file names for this test translation unit.
 */
namespace Constants
{
const fs::path k_DataDir = "test/data";
const fs::path k_Dream3dFilename = "newFile.dream3d";
const fs::path k_ExportFilename1 = "export.dream3d";
const fs::path k_ExportFilename2 = "export2.dream3d";
const fs::path k_MultiExportFilename1 = "multi_export1.dream3d";
const fs::path k_MultiExportFilename2 = "multi_export2.dream3d";
const fs::path k_MultiExportFilename3 = "multi_export3.dream3d";
// Each TEST_CASE runs as a separate ctest process. Giving every file-writing case its own
// output filename keeps them independent, so one case can never observe or clobber a file
// another one wrote, and each can assert on its own output in isolation.
const fs::path k_ValidParamsFilename = "write_dream3d_valid_params.dream3d";
const fs::path k_PipelineComboFilename = "write_dream3d_pipeline_combos.dream3d";
const fs::path k_FileDataFilename = "write_dream3d_file_data.dream3d";
const fs::path k_UnwritableTypeFilename = "write_dream3d_unwritable_type.dream3d";

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
constexpr StringLiteral k_VertexData = "Vertex Data";
constexpr StringLiteral k_VertexValues = "Vertex Values";
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

/**
 * @var m_DataMutex
 * @brief Serializes tests that use the same temporary DREAM3D file paths.
 */
std::mutex m_DataMutex;

/**
 * @namespace DataNames
 * @brief Provides DataStructure and pipeline names for DREAM3D I/O tests.
 */
namespace DataNames
{
constexpr StringLiteral k_Group1Name = "Top-Level";
constexpr StringLiteral k_Group2Name = "Second-Level";
constexpr StringLiteral k_Group3Name = "Third-Level";
constexpr StringLiteral k_AttributeMatrixName = "AttributeMatrix";
constexpr StringLiteral k_ArrayName = "Test-Array";
constexpr StringLiteral k_Array2Name = "Test-Array2";
constexpr StringLiteral k_GridMontageName = "GridMontage";

constexpr StringLiteral k_CreateDataFilterName = "Create Data Group";
constexpr StringLiteral k_ExportD3DFilterName = "Write DREAM3D-NX File";
} // namespace DataNames

const FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_CreateDataArrayHandle(Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExportD3DHandle(Uuid::FromString("b3a95784-2ced-41ec-8d3d-0242ac130003").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ImportD3DHandle(Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());

/**
 * @brief Returns the binary test-output directory.
 * @param app Application that establishes the test runtime.
 * @return Binary test-output directory.
 */
fs::path GetDataDir(const Application& app)
{
  return fs::path(unit_test::k_BinaryTestOutputDir.view());
}

fs::path GetTestFilePath(const fs::path& filename)
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / filename;
}

fs::path GetIODataPath()
{
  return GetTestFilePath(Constants::k_Dream3dFilename);
}

fs::path GetValidParamsFilePath()
{
  return GetTestFilePath(Constants::k_ValidParamsFilename);
}

fs::path GetPipelineComboFilePath()
{
  return GetTestFilePath(Constants::k_PipelineComboFilename);
}

fs::path GetFileDataFilePath()
{
  return GetTestFilePath(Constants::k_FileDataFilename);
}

fs::path GetUnwritableTypeFilePath()
{
  return GetTestFilePath(Constants::k_UnwritableTypeFilename);
}

fs::path GetXdmfPath(const fs::path& dream3dPath)
{
  fs::path filePath = dream3dPath;
  filePath.replace_extension(".xdmf");
  return filePath;
}

/**
 * @brief Returns the first single-export file path.
 * @return Path in the binary test-output directory.
 * @throws std::runtime_error If no Application instance exists.
 */
fs::path GetExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename1;
}

/**
 * @brief Returns the re-exported single-file path.
 * @return Path in the binary test-output directory.
 * @throws std::runtime_error If no Application instance exists.
 */
fs::path GetReExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename2;
}

/**
 * @brief Returns the first multi-import source file path.
 * @return Path in the binary test-output directory.
 * @throws std::runtime_error If no Application instance exists.
 */
fs::path GetMultiExportDataPath1()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename1;
}

/**
 * @brief Returns the second multi-import source file path.
 * @return Path in the binary test-output directory.
 * @throws std::runtime_error If no Application instance exists.
 */
fs::path GetMultiExportDataPath2()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename2;
}

/**
 * @brief Returns the combined multi-import re-export path.
 * @return Path in the binary test-output directory.
 * @throws std::runtime_error If no Application instance exists.
 */
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

  DataPath vertexDataPath = DataPath({Constants::k_TriangleGeom}).createChildPath(Constants::k_VertexData);
  const auto* triVertexMatrix = dataStructure.getDataAs<AttributeMatrix>(vertexDataPath);
  REQUIRE(triVertexMatrix != nullptr);
  REQUIRE(triGeom->getVertexAttributeMatrixId() == triVertexMatrix->getId());
  const auto* vertexValuesArray = dataStructure.getDataAs<Float32Array>(vertexDataPath.createChildPath(Constants::k_VertexValues));
  REQUIRE(vertexValuesArray != nullptr);
  CheckDataStore<float32>(vertexValuesArray->getDataStoreRef(), 1);

  const auto* hexGeom = dataStructure.getDataAs<HexahedralGeom>(DataPath({Constants::k_HexGeom}));
  CheckGeom3D(hexGeom, vertexArray->getId(), edgeArray->getId(), quadArray->getId(), hexaArray->getId());

  const auto* tetraGeom = dataStructure.getDataAs<TetrahedralGeom>(DataPath({Constants::k_TetrahedralGeom}));
  CheckGeom3D(tetraGeom, vertexArray->getId(), edgeArray->getId(), faceArray->getId(), tetraArray->getId());

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
  REQUIRE(xBoundsArray->getNumberOfTuples() == Constants::k_TupleShape[0] + 1);
  DataPath yPath = rectGridGeomPath.createChildPath(Constants::k_YBounds);
  const auto* yBoundsArray = dataStructure.getDataAs<Float32Array>(yPath);
  REQUIRE(yBoundsArray != nullptr);
  REQUIRE(yBoundsArray->getNumberOfTuples() == Constants::k_TupleShape[1] + 1);
  DataPath zPath = rectGridGeomPath.createChildPath(Constants::k_ZBounds);
  const auto* zBoundsArray = dataStructure.getDataAs<Float32Array>(zPath);
  REQUIRE(zBoundsArray != nullptr);
  REQUIRE(zBoundsArray->getNumberOfTuples() == Constants::k_TupleShape[2] + 1);
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

  Result<> arrayCreationResults = ArrayCreationUtilities::CreateArray<int8>(
      dataStructure, tupleShape, std::vector<usize>{1}, DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name}), IDataAction::Mode::Execute, "1");

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
  REQUIRE(polyhedraArray != nullptr);
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
  // N cells along a dimension require N+1 bounds values
  ShapeType xShape{Constants::k_TupleShape[0] + 1};
  ShapeType componentBounds{1};
  auto xBounds = std::make_shared<Float32DataStore>(xShape, componentBounds, 0.0f);
  auto* xBoundsArray = Float32Array::Create(dataStructure, Constants::k_XBounds, xBounds, rectGridGeom->getId());
  FillDataStore<float32>(*xBounds.get());
  ShapeType yShape{Constants::k_TupleShape[1] + 1};
  auto yBounds = std::make_shared<Float32DataStore>(yShape, componentBounds, 0.0f);
  auto* yBoundsArray = Float32Array::Create(dataStructure, Constants::k_YBounds, yBounds, rectGridGeom->getId());
  FillDataStore<float32>(*yBounds.get());
  ShapeType zShape{Constants::k_TupleShape[2] + 1};
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

  // Vertex data on a node geometry so the xdmf writer emits node-centered attribute references
  auto* vertexMatrix = AttributeMatrix::Create(dataStructure, Constants::k_VertexData, Constants::k_TupleShape, triangleGeom->getId());
  REQUIRE(vertexMatrix != nullptr);
  triangleGeom->setVertexAttributeMatrix(*vertexMatrix);
  auto vertexValues = std::make_shared<Float32DataStore>(Constants::k_TupleShape, ShapeType{1}, 0.0f);
  auto* vertexValuesArray = Float32Array::Create(dataStructure, Constants::k_VertexValues, vertexValues, vertexMatrix->getId());
  REQUIRE(vertexValuesArray != nullptr);
  FillDataStore<float32>(*vertexValues.get());

  // 3D Geometries. Hexahedra have quad faces (4 vertices) and 8-vertex cells; tetrahedra have
  // triangle faces (3 vertices) and 4-vertex cells.
  auto* hexGeom = Create3DGeom<HexahedralGeom>(dataStructure, Constants::k_HexGeom, *vertexArray, *edgesArray, *quadArray, *hexArray);
  REQUIRE(hexGeom != nullptr);
  auto* tetrahedralGeom = Create3DGeom<TetrahedralGeom>(dataStructure, Constants::k_TetrahedralGeom, *vertexArray, *edgesArray, *trianglesArray, *tetraArray);
  REQUIRE(tetrahedralGeom != nullptr);

  return dataStructure;
}

/**
 * @brief Creates a pipeline that builds test data and exports one DREAM3D file.
 * @return The configured export pipeline.
 */
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

/**
 * @brief Creates a pipeline that imports selected paths and re-exports them.
 * @return The configured import and re-export pipeline.
 */
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

/**
 * @brief Creates two DREAM3D files whose top-level groups have different names.
 */
void CreateMultiExportFiles()
{
  // The first file contains Top-Level.
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
  // The second file contains Second-Level.
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

/**
 * @brief Creates a pipeline that combines selected groups from two DREAM3D files.
 * @return The configured multi-import and re-export pipeline.
 */
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

/**
 * @brief Creates matching pipeline and DataStructure content for direct writer tests.
 * @return File data with the standard export pipeline and test DataStructure.
 */
DREAM3D::FileData CreateFileData()
{
  return {CreateExportPipeline(), CreateTestDataStructure()};
}

// Each geometry builder accepts an optional parent identifier.
// This contract lets round-trip tests compare top-level and nested ownership paths.

using MeshIndexType = IGeometry::MeshIndexType;

/**
 * @brief Creates a four-vertex array and attaches it to a node geometry.
 * @param dataStructure Receives the array and AttributeMatrix.
 * @param geometry Geometry that owns the new objects.
 * @return The created vertex array.
 */
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

/**
 * @brief Creates a valid VertexGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
VertexGeom* CreateVertexGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* vertexGeom = VertexGeom::Create(dataStructure, geomName, parentId);
  CreateVertexList(dataStructure, *vertexGeom);
  return vertexGeom;
}

/**
 * @brief Creates a valid EdgeGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
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

/**
 * @brief Creates a valid TriangleGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
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

/**
 * @brief Creates a valid QuadGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
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

/**
 * @brief Creates a valid TetrahedralGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
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

/**
 * @brief Creates a valid HexahedralGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
HexahedralGeom* CreateHexahedralGeometry(DataStructure& dataStructure, const std::string& geomName, const std::optional<DataObject::IdType>& parentId)
{
  auto* hexGeom = HexahedralGeom::Create(dataStructure, geomName, parentId);
  // One hexahedron requires eight vertices instead of the four-vertex helper fixture.
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

/**
 * @brief Creates a valid ImageGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
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

/**
 * @brief Creates a valid RectGridGeom for round-trip tests.
 * @param dataStructure Receives the geometry.
 * @param geomName Geometry name.
 * @param parentId Optional parent DataObject identifier.
 * @return The created geometry.
 */
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

/**
 * @struct GeometryTestCase
 * @brief Couples one named geometry builder with its concrete-type verifier.
 */
struct GeometryTestCase
{
  std::string typeName;
  std::function<void(DataStructure&, const std::string&, const std::optional<DataObject::IdType>&)> build;
  std::function<void(const DataStructure&, const DataPath&)> requireType;
};

/**
 * @brief Creates a type-erased geometry round-trip test case.
 * @tparam GeomType Specifies the expected concrete geometry type.
 * @param typeName Geometry type label and generated object name.
 * @param builder Creates the geometry at a selected parent.
 * @return A test case with build and concrete-type verification callables.
 */
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

  // Both sections only exercise preflight parameter guards, so no data is required.
  DataStructure dataStructure;
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("WriteDREAM3DFilter:Valid Parameters", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  const fs::path exportFilePath = GetValidParamsFilePath();

  {
    DataStructure dataStructure = CreateTestDataStructure();
    Arguments args;
    WriteDREAM3DFilter filter;

    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, std::make_any<FileSystemPathParameter::ValueType>(exportFilePath));
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
  REQUIRE(fs::exists(exportFilePath));

  // Check that the file can be read back in and that the imported DataStructure matches expected values.
  {
    auto fileReader = HDF5::FileIO::ReadFile(exportFilePath);
    auto fileResult = DREAM3D::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(fileResult);

    auto [pipeline, dataStructureRead] = fileResult.value();

    CheckTestDataStructure(dataStructureRead);
    UnitTest::CheckArraysInheritTupleDims(dataStructureRead);
  }
}

void CheckXdmfFile(const fs::path& dream3dPath)
{
  auto filepath = GetXdmfPath(dream3dPath);
  REQUIRE(fs::exists(filepath));

  // Every heavy-data reference in the sidecar must point into the .dream3d file that was
  // written; a reference built from any other token (e.g. a geometry name) cannot be
  // resolved by ParaView/VisIt.
  const std::string expectedPrefix = dream3dPath.filename().string() + ":/DataStructure/";
  std::ifstream xdmfFile(filepath);
  REQUIRE(xdmfFile.good());
  usize referenceCount = 0;
  std::string line;
  while(std::getline(xdmfFile, line))
  {
    const usize refPos = line.find(":/DataStructure/");
    if(refPos == std::string::npos)
    {
      continue;
    }
    referenceCount++;
    const usize start = line.find_first_not_of(" \t");
    REQUIRE(line.compare(start, expectedPrefix.size(), expectedPrefix) == 0);
  }
  REQUIRE(referenceCount > 0);
}

TEST_CASE("WriteDREAM3D:Pipeline / WriteXdmf combinations", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  bool writeXdmf = GENERATE(true, false);
  Pipeline exportPipeline = GENERATE(CreateExportPipeline(), Pipeline());

  const fs::path exportFilePath = GetPipelineComboFilePath();
  DataStructure dataStructure = CreateTestDataStructure();
  auto writeResult = DREAM3D::WriteFile(exportFilePath, dataStructure, exportPipeline, writeXdmf);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  if(writeXdmf)
  {
    CheckXdmfFile(exportFilePath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("WriteDREAM3D:FileData Overload", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  const fs::path exportFilePath = GetFileDataFilePath();

  // Write through the exported FileData overload so its pipeline/DataStructure ordering stays covered.
  {
    auto fileWriter = HDF5::FileIO::WriteFile(exportFilePath);
    REQUIRE(fileWriter.isValid());
    auto writeResult = DREAM3D::WriteFile(fileWriter, CreateFileData());
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  auto fileReader = HDF5::FileIO::ReadFile(exportFilePath);
  auto fileResult = DREAM3D::ReadFile(fileReader);
  SIMPLNX_RESULT_REQUIRE_VALID(fileResult);

  auto [pipeline, dataStructure] = fileResult.value();
  REQUIRE(pipeline.size() == CreateExportPipeline().size());
  CheckTestDataStructure(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("WriteDREAM3D:Invalid File", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  bool writeXdmf = GENERATE(true, false);
  Pipeline exportPipeline = GENERATE(CreateExportPipeline(), Pipeline());

  // The write fails before any file (or .xdmf sidecar) can be produced; there is no
  // target path whose sidecar could be checked.
  DataStructure dataStructure = CreateTestDataStructure();
  auto writeResult = DREAM3D::WriteFile(fs::path(), dataStructure, exportPipeline, writeXdmf);
  SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("WriteDREAM3DFilter:Unwritable DataObject Type", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  UnitTest::LoadPlugins();
  std::lock_guard<std::mutex> lock(m_DataMutex);

  // The subject of this test is the write-failure contract, not GridMontage: when
  // DREAM3D::WriteFile returns invalid, the AtomicFile must never be committed, so the write
  // fails *and* leaves no file behind at the destination path.
  //
  // GridMontage is merely a convenient way to reach that path. Its GridMontageIO class is never
  // registered by HDF5::DataIOManager::addCoreFactories(), so DataStructureWriter cannot resolve
  // a factory for it and fails with error -5. That is current behavior only: montage support in
  // SIMPLNX is an open design question, and this test deliberately makes no claim about how
  // montages ought to behave. If montages later become writable, re-point this test at another
  // unwritable type rather than deleting it -- the write-failure contract still needs coverage.
  const fs::path exportFilePath = GetUnwritableTypeFilePath();
  std::error_code removeError;
  fs::remove(exportFilePath, removeError);
  REQUIRE_FALSE(fs::exists(exportFilePath));

  DataStructure dataStructure;
  REQUIRE(GridMontage::Create(dataStructure, DataNames::k_GridMontageName) != nullptr);

  Arguments args;
  WriteDREAM3DFilter filter;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, std::make_any<FileSystemPathParameter::ValueType>(exportFilePath));
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, std::make_any<bool>(false));
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, std::make_any<bool>(false));
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, std::make_any<int32>(1));

  // Preflight does not inspect DataObject types, so it must still succeed.
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -5);

  // The AtomicFile must not have been committed.
  REQUIRE_FALSE(fs::exists(exportFilePath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
      CheckXdmfFile(GetIODataPath());
    }
  }

  // Read the same file and validate both stored sections.
  {
    auto fileReader = HDF5::FileIO::ReadFile(GetIODataPath());
    auto pipelineResult = DREAM3D::ImportPipelineFromFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(pipelineResult);
    auto pipeline = std::move(pipelineResult.value());

    auto dsResult = DREAM3D::LoadDataStructure(GetIODataPath());
    SIMPLNX_RESULT_REQUIRE_VALID(dsResult);
    DataStructure dataStructure = std::move(dsResult.value());

    CheckTestDataStructure(dataStructure);

    // Test reading the DataStructure
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name, DataNames::k_Group3Name})) != nullptr);
    auto attMatrix = dataStructure.getDataAs<AttributeMatrix>(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName}));
    REQUIRE(attMatrix != nullptr);
    REQUIRE(attMatrix->getShape() == ShapeType{10});
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name})) != nullptr);

    // The pipeline must retain its filters and arguments.
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

  // geoms.dream3d contains each node-connectivity type and RectGrid bounds.
  // Metadata-only preflight must import these arrays without reading their values.
  // Each empty store must still provide its array shape and value type.
  const fs::path inputFilePath = fs::path(unit_test::k_SimplnxTestDataSourceDir.view()) / "geoms.dream3d";

  // Each case contains a connectivity-array path and its expected tuple count.
  const std::vector<std::pair<DataPath, usize>> connectivityPaths = {
      {DataPath({"EdgeGeometry", "Verts"}), 10},       {DataPath({"EdgeGeometry", "Edges"}), 5},       {DataPath({"TriangleGeometry", "Verts"}), 10}, {DataPath({"TriangleGeometry", "Triangles"}), 4},
      {DataPath({"QuadGeometry", "Quads"}), 2},        {DataPath({"TetrahedralGeometry", "Tets"}), 1}, {DataPath({"HexahedralGeometry", "Hexs"}), 3}, {DataPath({"RectGridGeometry", "XBounds"}), 10},
      {DataPath({"RectGridGeometry", "YBounds"}), 10}, {DataPath({"RectGridGeometry", "ZBounds"}), 10}};

  const auto isEmptyStore = [](const IDataStore* store) { return store != nullptr && store->getStoreType() == IDataStore::StoreType::Empty; };

  // With useEmptyDataStores enabled, connectivity must remain metadata-only.
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

        // Preflight requires shape metadata but does not require stored values.
        REQUIRE(dataArray->getNumberOfTuples() == numTuples);
      }
    }
  }

  // With useEmptyDataStores disabled, each connectivity array must load its values.
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

  // The path overload must preserve useEmptyDataStores when it delegates to FileIO.
  // Thus, connectivity from this overload must also remain metadata-only.
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

      // Successful pipeline loading verifies the DataContainerReaderFilterParameterConverter value.
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
  auto createRes = ArrayCreationUtilities::CreateArray<float32>(dataStructure, std::vector<usize>{k_Tuples}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, "", "0");
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

  DataStructure importedDs = UnitTest::LoadDataStructure(outPath);
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
  auto cr = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{200'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, "", "1.5");
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
  auto cr = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{500'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, "", "0");
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

  DataStructure imported = UnitTest::LoadDataStructure(outPath);
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
  auto crSmall = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{100}, std::vector<usize>{1}, DataPath({"Small"}), IDataAction::Mode::Execute, "", "2");
  SIMPLNX_RESULT_REQUIRE_VALID(crSmall);
  auto crBig = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{500'000}, std::vector<usize>{1}, DataPath({"Big"}), IDataAction::Mode::Execute, "", "3");
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

  // One TEST_CASE invocation retains all file sizes for the final monotonicity check.
  // DYNAMIC_SECTION would restart the body and discard that accumulated state.
  std::vector<std::uintmax_t> sizesByLevel;
  const std::vector<int32> levels = {1, 5, 9};

  for(int32 level : levels)
  {
    const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("compression_rt_level_{}.dream3d", level);
    fs::remove(outPath);

    DataStructure ds;
    const DataPath arrayPath({"A"});
    auto cr = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{1'000'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, "", "0");
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

    DataStructure imported = UnitTest::LoadDataStructure(outPath);
    REQUIRE_NOTHROW(imported.getDataRefAs<DataArray<float32>>(arrayPath));
    UnitTest::CompareDataArrays<float32>(ds.getDataRefAs<DataArray<float32>>(arrayPath), imported.getDataRefAs<DataArray<float32>>(arrayPath));

    // Add a size only after its file passes round-trip validation.
    sizesByLevel.push_back(fs::file_size(outPath));
  }

  // Higher compression levels must not increase size for this repeating 1,024-value pattern.
  // Its small symbol set and long runs make the deflate result deterministic for this check.
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

      // Build one top-level geometry as a control and one equal geometry in a DataGroup.
      // The nested object verifies that ownership depth does not change round-trip behavior.
      DataStructure dataStructure;
      const std::string topName = "Top";
      const std::string nestName = "Nested";
      testCase.build(dataStructure, topName, {});
      auto* group = DataGroup::Create(dataStructure, "Grp");
      testCase.build(dataStructure, nestName, group->getId());

      // Write both ownership layouts to one file.
      {
        WriteDREAM3DFilter writeFilter;
        Arguments writeArgs;
        writeArgs.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
        writeArgs.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
        Result<> writeResult = writeFilter.execute(dataStructure, writeArgs).result;
        SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
      }

      // Read both layouts through the normal DREAM3D reader.
      DataStructure importDataStructure;
      {
        ReadDREAM3DFilter readFilter;
        Arguments readArgs;
        Dream3dImportParameter::ImportData importData(outPath);
        readArgs.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, importData);
        Result<> readResult = readFilter.execute(importDataStructure, readArgs).result;
        SIMPLNX_RESULT_REQUIRE_VALID(readResult);
      }

      // Both layouts must retain the selected concrete geometry type.
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

  // Enabled compression rejects a level below the valid range [1, 9].
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(0));
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }

  // Enabled compression rejects a level above the valid range [1, 9].
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(10));
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }

  // Disabled compression ignores the level value, including an out-of-range value.
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

  // Backdate the small input past the cache trust window for deterministic hit and miss counts.
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

  // A one-filter pipeline matches the GUI and nxrunner preflight path.
  Pipeline pipeline;
  Arguments args;
  Dream3dImportParameter::ImportData importData(filePath);
  args.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, importData);
  auto filterNode = PipelineFilter::Create(k_ImportD3DHandle);
  filterNode->setArguments(args);
  pipeline.push_back(std::move(filterNode));

  // The first preflight has one miss when the filter reads file metadata.
  // The import action then hits the same cache entry and avoids a second traversal.
  REQUIRE(pipeline.preflight());
  REQUIRE(cache.missCount() == 1);

  // The second preflight represents an edit-triggered repeat.
  // The cache serves its metadata without another miss or traversal.
  HDF5::FileIO::ResetReadOpenCount();
  REQUIRE(pipeline.preflight());
  REQUIRE(cache.missCount() == 1);
  // A fully cached preflight does not open the HDF5 file.
  // Parameter validation only reads file status, and the filter gets metadata from memory.
  REQUIRE(HDF5::FileIO::GetReadOpenCount() == 0);

  // Execution must still read the complete array values from disk.
  REQUIRE(pipeline.execute());
  const DataStructure executed = pipeline[0]->getDataStructure();
  const auto* readFloats = executed.getDataAs<Float32Array>(DataPath({"CacheGroup", "CacheFloats"}));
  REQUIRE(readFloats != nullptr);
  REQUIRE((*readFloats)[0] == 2.5f);

  // Rewriting the file must invalidate the cache so preflight sees its new content.
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
