#include "Dream3dIO.hpp"

#include "nlohmann/json.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_DataStructureGroupTag = "DataStructure";
constexpr StringLiteral k_LegacyDataStructureGroupTag = "DataContainers";
constexpr StringLiteral k_FileVersionTag = "FileVersion";
constexpr StringLiteral k_PipelineJsonTag = "Pipeline";
constexpr StringLiteral k_PipelineNameTag = "Current Pipeline";
constexpr StringLiteral k_PipelineVersionTag = "Pipeline Version";
constexpr StringLiteral k_CurrentFileVersion = "8.0";

constexpr int32_t k_CurrentPipelineVersion = 3;

namespace Legacy
{
constexpr StringLiteral DCATag = "DataContainers";
constexpr StringLiteral GeometryTag = "_SIMPL_GEOMETRY";
constexpr StringLiteral GeometryNameTag = "GeometryName";
constexpr StringLiteral PipelineName = "Pipeline";
constexpr StringLiteral CompDims = "ComponentDimensions";
constexpr StringLiteral TupleDims = "TupleDimensions";

constexpr StringLiteral FileVersion = "7.0";

constexpr StringLiteral VertexListName = "SharedVertexList";
constexpr StringLiteral EdgeListName = "SharedEdgeList";
constexpr StringLiteral TriListName = "SharedTriList";
constexpr StringLiteral QuadListName = "SharedQuadList";
constexpr StringLiteral TetraListName = "SharedTetList";
constexpr StringLiteral HexListName = "SharedHexList";
constexpr StringLiteral VerticesName = "Verts";
constexpr StringLiteral XBoundsName = "xBounds";
constexpr StringLiteral YBoundsName = "yBounds";
constexpr StringLiteral ZBoundsName = "zBounds";

namespace Type
{
constexpr StringLiteral ImageGeom = "ImageGeometry";
constexpr StringLiteral EdgeGeom = "EdgeGeometry";
constexpr StringLiteral HexGeom = "HexahedralGeometry";
constexpr StringLiteral QuadGeom = "QuadGeometry";
constexpr StringLiteral RectGridGeom = "RectGridGeometry";
constexpr StringLiteral TetrahedralGeom = "TetrahedralGeometry";
constexpr StringLiteral TriangleGeom = "TriangleGeometry";
constexpr StringLiteral VertexGeom = "VertexGeometry";
} // namespace Type
} // namespace Legacy
} // namespace

complex::DREAM3D::FileVersionType complex::DREAM3D::GetFileVersion(const H5::FileReader& fileReader)
{
  auto fileVersionAttribute = fileReader.getAttribute(k_FileVersionTag);
  if(!fileVersionAttribute.isValid())
  {
    return "";
  }
  return fileVersionAttribute.readAsString();
}

complex::DREAM3D::PipelineVersionType complex::DREAM3D::GetPipelineVersion(const H5::FileReader& fileReader)
{
  auto pipelineGroup = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineVersionAttribute = pipelineGroup.getAttribute(k_PipelineVersionTag);
  if(!pipelineVersionAttribute.isValid())
  {
    return 0;
  }
  return pipelineVersionAttribute.readAsValue<PipelineVersionType>();
}

Result<DataStructure> ImportDataStructureV8(const H5::FileReader& fileReader, bool preflight)
{
  H5::ErrorType errorCode = 0;
  H5::DataStructureReader dataStructureReader;
  auto dataStructure = dataStructureReader.readH5Group(fileReader, errorCode, preflight);
  if(errorCode < 0)
  {
    return MakeErrorResult<DataStructure>(errorCode, fmt::format("Failed to import DataStructure"));
  }
  return {std::move(dataStructure)};
}

// Begin legacy DCA importing

/**
 * @brief
 * @tparam T
 * @param ds
 * @param name
 * @param parentId
 * @param daId
 * @param tDims
 * @param cDims
 */
template <typename T>
IDataArray* createLegacyDataArray(DataStructure& ds, DataObject::IdType parentId, const H5::DatasetReader& dataArrayReader, const std::vector<usize>& tDims, const std::vector<usize>& cDims,
                                  bool preflight = false)
{
  using DataArrayType = DataArray<T>;
  using EmptyDataStoreType = EmptyDataStore<T>;
  using DataStoreType = DataStore<T>;

  const std::string daName = dataArrayReader.getName();

  if(preflight)
  {
    return DataArrayType::template CreateWithStore<EmptyDataStoreType>(ds, daName, tDims, cDims, parentId);
  }
  auto dataStore = std::make_unique<DataStore<T>>(tDims, cDims, static_cast<T>(0));

  if(!dataArrayReader.readIntoSpan(dataStore->createSpan()))
  {
    throw std::runtime_error(fmt::format("Error reading HDF5 Data set {}", complex::H5::Support::GetObjectPath(dataArrayReader.getId())));
  }
  // Insert the DataArray into the DataStructure
  return DataArray<T>::Create(ds, daName, std::move(dataStore), parentId);
}

/**
 * @brief
 * @param daId
 * @param tDims
 * @param cDims
 */
void readLegacyDataArrayDims(const H5::DatasetReader& dataArrayReader, std::vector<usize>& tDims, std::vector<usize>& cDims)
{
  {
    auto compAttrib = dataArrayReader.getAttribute(Legacy::CompDims);
    if(!compAttrib.isValid())
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }

    cDims = compAttrib.readAsVector<usize>();
    std::reverse(cDims.begin(), cDims.end());
  }

  {
    auto tupleAttrib = dataArrayReader.getAttribute(Legacy::TupleDims);
    if(!tupleAttrib.isValid())
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }

    tDims = tupleAttrib.readAsVector<usize>();
    std::reverse(tDims.begin(), tDims.end());
  }
}

void readLegacyStringArray(DataStructure& ds, const H5::DatasetReader& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  const std::string daName = dataArrayReader.getName();

  if(preflight)
  {
    StringArray::Create(ds, daName, parentId);
  }
  else
  {
    const std::vector<std::string> strings = dataArrayReader.readAsVectorOfStrings();
    StringArray::CreateWithValues(ds, daName, strings, parentId);
  }
}

IDataArray* readLegacyDataArray(DataStructure& ds, const H5::DatasetReader& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  auto size = H5Dget_storage_size(dataArrayReader.getId());
  auto typeId = dataArrayReader.getTypeId();

  std::vector<usize> tDims;
  std::vector<usize> cDims;
  readLegacyDataArrayDims(dataArrayReader, tDims, cDims);

  IDataArray* dataArray = nullptr;

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    dataArray = createLegacyDataArray<float32>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    dataArray = createLegacyDataArray<float64>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    dataArray = createLegacyDataArray<int8>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    dataArray = createLegacyDataArray<int16>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    dataArray = createLegacyDataArray<int32>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    dataArray = createLegacyDataArray<int64>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    const auto typeAttrib = dataArrayReader.getAttribute(complex::Constants::k_ObjectTypeTag);
    if(typeAttrib.isValid() && typeAttrib.readAsString() == "DataArray<bool>")
    {
      dataArray = createLegacyDataArray<bool>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
    }
    else
    {
      dataArray = createLegacyDataArray<uint8>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
    }
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    dataArray = createLegacyDataArray<uint16>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    dataArray = createLegacyDataArray<uint32>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    dataArray = createLegacyDataArray<uint64>(ds, parentId, dataArrayReader, tDims, cDims, preflight);
  }

  H5Tclose(typeId);

  return dataArray;
}

template <typename T>
void createLegacyNeighborList(DataStructure& ds, DataObject ::IdType parentId, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader, const std::vector<usize>& tupleDims)
{
  auto numTuples = std::accumulate(tupleDims.cbegin(), tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());
  auto data = NeighborList<T>::ReadHdf5Data(parentReader, datasetReader);
  auto* neighborList = NeighborList<T>::Create(ds, datasetReader.getName(), numTuples, parentId);
  for(usize i = 0; i < data.size(); ++i)
  {
    neighborList->setList(i, data[i]);
  }
}

void readLegacyNeighborList(DataStructure& ds, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader, DataObject::IdType parentId)
{
  auto typeId = datasetReader.getTypeId();

  auto tupleAttrib = datasetReader.getAttribute(Legacy::TupleDims);
  if(!tupleAttrib.isValid())
  {
    throw std::runtime_error("Error reading legacy DataArray dimensions");
  }

  auto tDims = tupleAttrib.readAsVector<usize>();

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    createLegacyNeighborList<float32>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    createLegacyNeighborList<float64>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    createLegacyNeighborList<int8>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    createLegacyNeighborList<int16>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    createLegacyNeighborList<int32>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    createLegacyNeighborList<int64>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    createLegacyNeighborList<uint8>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    createLegacyNeighborList<uint16>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    createLegacyNeighborList<uint32>(ds, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    createLegacyNeighborList<uint64>(ds, parentId, parentReader, datasetReader, tDims);
  }

  H5Tclose(typeId);
}

bool isLegacyNeighborList(const H5::DatasetReader& arrayReader)
{
  const auto objectTypeAttribute = arrayReader.getAttribute("ObjectType");
  const std::string objectType = objectTypeAttribute.readAsString();
  return objectType == "NeighborList<T>";
}

bool isLegacyStringArray(const H5::DatasetReader& arrayReader)
{
  const auto objectTypeAttribute = arrayReader.getAttribute("ObjectType");
  const std::string objectType = objectTypeAttribute.readAsString();
  return objectType == "StringDataArray";
}

void readLegacyAttributeMatrix(DataStructure& ds, const H5::GroupReader& amGroupReader, DataObject::IdType parentId, bool preflight = false)
{
  const std::string amName = amGroupReader.getName();
  auto attributeMatrix = DataGroup::Create(ds, amName, parentId);

  auto dataArrayNames = amGroupReader.getChildNames();
  for(const auto& daName : dataArrayNames)
  {
    auto dataArraySet = amGroupReader.openDataset(daName);

    if(isLegacyNeighborList(dataArraySet))
    {
      readLegacyNeighborList(ds, amGroupReader, dataArraySet, attributeMatrix->getId());
    }
    else if(isLegacyStringArray(dataArraySet))
    {
      readLegacyStringArray(ds, dataArraySet, attributeMatrix->getId(), preflight);
    }
    else
    {
      readLegacyDataArray(ds, dataArraySet, attributeMatrix->getId(), preflight);
    }
  }
}

// Begin legacy geometry import methods
void readGenericGeomDims(AbstractGeometry* geom, const H5::GroupReader& geomGroup)
{
  auto sDimAttribute = geomGroup.getAttribute("SpatialDimensionality");
  auto sDims = sDimAttribute.readAsValue<int32>();

  auto uDimAttribute = geomGroup.getAttribute("UnitDimensionality");
  auto uDims = uDimAttribute.readAsValue<int32>();

  geom->setSpatialDimensionality(sDims);
  geom->setUnitDimensionality(uDims);
}

IDataArray* readLegacyGeomArray(DataStructure& ds, AbstractGeometry* geometry, const H5::GroupReader& geomGroup, const std::string& arrayName)
{
  auto dataArraySet = geomGroup.openDataset(arrayName);
  return readLegacyDataArray(ds, dataArraySet, geometry->getId());
}

template <typename T>
T* readLegacyGeomArrayAs(DataStructure& ds, AbstractGeometry* geometry, const H5::GroupReader& geomGroup, const std::string& arrayName)
{
  return dynamic_cast<T*>(readLegacyGeomArray(ds, geometry, geomGroup, arrayName));
}

DataObject* readLegacyVertexGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto* geom = VertexGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::VertexListName);
  auto* verts = readLegacyGeomArray(ds, geom, geomGroup, Legacy::VerticesName);

  geom->setVertices(sharedVertexList);
  return geom;
}

DataObject* readLegacyTriangleGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = TriangleGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::VertexListName);
  auto* sharedTriList = readLegacyGeomArrayAs<UInt64Array>(ds, geom, geomGroup, Legacy::TriListName);

  geom->setVertices(sharedVertexList);
  geom->setFaces(sharedTriList);

  return geom;
}

DataObject* readLegacyTetrahedralGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = TetrahedralGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::VertexListName);
  auto* sharedTetList = readLegacyGeomArrayAs<UInt64Array>(ds, geom, geomGroup, Legacy::TetraListName);

  geom->setVertices(sharedVertexList);
  geom->setTetrahedra(sharedTetList);

  return geom;
}

DataObject* readLegacyRectGridGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = RectGridGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);

  // DIMENSIONS array
  {
    auto dimsDataset = geomGroup.openDataset("DIMENSIONS");
    auto dims = dimsDataset.readAsVector<int64>();
    geom->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
  }

  auto* xBoundsArray = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::XBoundsName);
  auto* yBoundsArray = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::YBoundsName);
  auto* zBoundsArray = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::ZBoundsName);

  geom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray);

  return geom;
}

DataObject* readLegacyQuadGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = QuadGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::VertexListName);
  auto* sharedQuadList = readLegacyGeomArrayAs<UInt64Array>(ds, geom, geomGroup, Legacy::QuadListName);

  geom->setVertices(sharedVertexList);
  geom->setFaces(sharedQuadList);

  return geom;
}

DataObject* readLegacyHexGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = HexahedralGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::VertexListName);
  auto* sharedHexList = readLegacyGeomArrayAs<UInt64Array>(ds, geom, geomGroup, Legacy::HexListName);

  geom->setVertices(sharedVertexList);
  geom->setHexahedra(sharedHexList);

  return geom;
}

DataObject* readLegacyEdgeGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = EdgeGeom::Create(ds, name);
  auto edge = dynamic_cast<EdgeGeom*>(geom);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(ds, geom, geomGroup, Legacy::VertexListName);
  auto* sharedEdgeList = readLegacyGeomArrayAs<UInt64Array>(ds, geom, geomGroup, Legacy::EdgeListName);

  geom->setVertices(sharedVertexList);
  geom->setEdges(sharedEdgeList);

  return geom;
}

DataObject* readLegacyImageGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = ImageGeom::Create(ds, name);
  auto image = dynamic_cast<ImageGeom*>(geom);

  readGenericGeomDims(geom, geomGroup);

  // DIMENSIONS array
  {
    auto dimsDataset = geomGroup.openDataset("DIMENSIONS");
    auto dims = dimsDataset.readAsVector<int64>();
    image->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
  }

  // ORIGIN array
  {
    auto originDataset = geomGroup.openDataset("ORIGIN");
    auto origin = originDataset.readAsVector<float32>();
    image->setOrigin(FloatVec3(origin[0], origin[1], origin[2]));
  }

  // SPACING array
  {
    auto spacingDataset = geomGroup.openDataset("SPACING");
    auto spacing = spacingDataset.readAsVector<float32>();
    image->setSpacing(FloatVec3(spacing[0], spacing[1], spacing[2]));
  }

  return image;
}
// End legacy Geometry importing

void readLegacyDataContainer(DataStructure& ds, const H5::GroupReader& dcGroup, bool preflight = false)
{
  DataObject* container = nullptr;
  const std::string dcName = dcGroup.getName();

  // Check for geometry
  auto geomGroup = dcGroup.openGroup(Legacy::GeometryTag.c_str());
  if(geomGroup.isValid())
  {
    auto geomNameAttribute = geomGroup.getAttribute(Legacy::GeometryNameTag);
    const std::string geomName = geomNameAttribute.readAsString();
    if(geomName == Legacy::Type::ImageGeom)
    {
      container = readLegacyImageGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::EdgeGeom)
    {
      container = readLegacyEdgeGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::HexGeom)
    {
      container = readLegacyHexGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::QuadGeom)
    {
      container = readLegacyQuadGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::RectGridGeom)
    {
      container = readLegacyRectGridGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::TetrahedralGeom)
    {
      container = readLegacyTetrahedralGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::TriangleGeom)
    {
      container = readLegacyTriangleGeom(ds, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::VertexGeom)
    {
      container = readLegacyVertexGeom(ds, geomGroup, dcName);
    }
  }

  // No geometry found. Create a DataGroup instead
  if(!container)
  {
    container = DataGroup::Create(ds, dcName);
  }

  auto attribMatrixNames = dcGroup.getChildNames();
  for(const auto& amName : attribMatrixNames)
  {
    if(amName == Legacy::GeometryTag)
    {
      continue;
    }

    auto attributeMatrixGroup = dcGroup.openGroup(amName);
    readLegacyAttributeMatrix(ds, attributeMatrixGroup, container->getId(), preflight);
  }
}

Result<DataStructure> ImportLegacyDataStructure(const H5::FileReader& fileReader, bool preflight)
{
  DataStructure ds;

  auto dcaGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);

  // Iterate over DataContainers
  const auto dcNames = dcaGroup.getChildNames();
  for(const auto& dcName : dcNames)
  {
    auto dcGroup = dcaGroup.openGroup(dcName);
    readLegacyDataContainer(ds, dcGroup, preflight);
  }

  return {std::move(ds)};
}

Result<complex::DataStructure> complex::DREAM3D::ImportDataStructureFromFile(const H5::FileReader& fileReader, bool preflight)
{
  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return ImportDataStructureV8(fileReader, preflight);
  }
  else if(fileVersion == Legacy::FileVersion)
  {
    return ImportLegacyDataStructure(fileReader, preflight);
  }
  // Unsupported file version
  return MakeErrorResult<DataStructure>(k_InvalidDataStructureVersion,
                                        fmt::format("Could not parse DataStructure version {}. Expected versions: {} or {}", fileVersion, k_CurrentFileVersion, Legacy::FileVersion));
}

Result<complex::DataStructure> complex::DREAM3D::ImportDataStructureFromFile(const std::filesystem::path& filePath)
{
  H5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<DataStructure>(-1, fmt::format("complex::DREAM3D::ImportDataStructureFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  H5::ErrorType error = 0;
  return ImportDataStructureFromFile(fileReader, error);
}

Result<complex::Pipeline> complex::DREAM3D::ImportPipelineFromFile(const H5::FileReader& fileReader)
{
  if(GetPipelineVersion(fileReader) != k_CurrentPipelineVersion)
  {
    return MakeErrorResult<Pipeline>(k_InvalidPipelineVersion, fmt::format("Could not parse Pipeline version '{}'. Expected version: '{}'", GetPipelineVersion(fileReader), k_CurrentFileVersion));
  }

  auto pipelineGroupReader = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineDatasetReader = pipelineGroupReader.openDataset(k_PipelineJsonTag);
  if(!pipelineDatasetReader.isValid())
  {
    return MakeErrorResult<Pipeline>(k_PipelineGroupUnavailable, "Could not open '/Pipeline' HDF5 Group.");
  }

  auto pipelineJsonString = pipelineDatasetReader.readAsString();
  auto pipelineJson = nlohmann::json::parse(pipelineJsonString);
  return Pipeline::FromJson(pipelineJson);
}

Result<complex::Pipeline> complex::DREAM3D::ImportPipelineFromFile(const std::filesystem::path& filePath)
{
  if(!std::filesystem::exists(filePath))
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("complex::DREAM3D::ImportPipelineFromFile: File does not exist. '{}'", filePath.string()));
  }
  H5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("complex::DREAM3D::ImportPipelineFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportPipelineFromFile(fileReader);
}

Result<complex::DREAM3D::FileData> complex::DREAM3D::ReadFile(const H5::FileReader& fileReader, bool preflight)
{
  // Pipeline pipeline;
  auto pipeline = ImportPipelineFromFile(fileReader);
  if(pipeline.invalid())
  {
    return {{nonstd::make_unexpected(std::move(pipeline.errors()))}, std::move(pipeline.warnings())};
  }

  auto dataStructure = ImportDataStructureFromFile(fileReader, preflight);
  if(pipeline.invalid())
  {
    return {{nonstd::make_unexpected(std::move(dataStructure.errors()))}, std::move(dataStructure.warnings())};
  }

  return {DREAM3D::FileData{std::move(pipeline.value()), std::move(dataStructure.value())}};
}

Result<complex::DREAM3D::FileData> complex::DREAM3D::ReadFile(const std::filesystem::path& path)
{
  H5::FileReader reader(path);
  H5::ErrorType error = 0;

  Result<FileData> fileData = ReadFile(reader, error);
  if(error < 0)
  {
    return MakeErrorResult<FileData>(-1, fmt::format("complex::DREAM3D::ReadFile: Unable to read '{}'", path.string()));
  }
  return {std::move(fileData)};
}

H5::ErrorType WritePipeline(H5::FileWriter& fileWriter, const Pipeline& pipeline)
{
  if(!fileWriter.isValid())
  {
    return -1;
  }

  auto pipelineGroupWriter = fileWriter.createGroupWriter(k_PipelineJsonTag);
  auto versionAttribute = pipelineGroupWriter.createAttribute(k_PipelineVersionTag);
  auto errorCode = versionAttribute.writeValue<DREAM3D::PipelineVersionType>(k_CurrentPipelineVersion);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto nameAttribute = pipelineGroupWriter.createAttribute(k_PipelineNameTag);
  errorCode = nameAttribute.writeString(pipeline.getName());
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto pipelineDatasetWriter = pipelineGroupWriter.createDatasetWriter(k_PipelineJsonTag);
  std::string pipelineString = pipeline.toJson().dump();
  return pipelineDatasetWriter.writeString(pipelineString);
}

H5::ErrorType WriteDataStructure(H5::FileWriter& fileWriter, const DataStructure& dataStructure)
{
  return dataStructure.writeHdf5(fileWriter);
}

H5::ErrorType WriteFileVersion(H5::FileWriter& fileWriter)
{
  auto fileVersionAttribute = fileWriter.createAttribute(k_FileVersionTag);
  return fileVersionAttribute.writeString(k_CurrentFileVersion);
}

H5::ErrorType complex::DREAM3D::WriteFile(H5::FileWriter& fileWriter, const FileData& fileData)
{
  return WriteFile(fileWriter, fileData.first, fileData.second);
}

H5::ErrorType complex::DREAM3D::WriteFile(H5::FileWriter& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure)
{
  auto errorCode = WriteFileVersion(fileWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WritePipeline(fileWriter, pipeline);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteDataStructure(fileWriter, dataStructure);
  return errorCode;
}

Result<> complex::DREAM3D::WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline)
{
  Result<H5::FileWriter> fileWriterResult = H5::FileWriter::CreateFile(path);
  if(fileWriterResult.invalid())
  {
    return ConvertResult(std::move(fileWriterResult));
  }

  H5::FileWriter fileWriter = std::move(fileWriterResult.value());

  H5::ErrorType error = WriteFile(fileWriter, Pipeline(), dataStructure);
  if(error < 0)
  {
    return MakeErrorResult(-2, fmt::format("complex::DREAM3D::WriteFile: Unable to write DREAM3D file with HDF5 error {}", error));
  }

  return {};
}
