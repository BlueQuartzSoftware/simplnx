#include "Dream3dIO.hpp"

#include "nlohmann/json.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
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

DataStructure ImportDataStructureV8(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  H5::DataStructureReader dataStructureReader;
  auto dataStructure = dataStructureReader.readH5Group(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {};
  }
  return dataStructure;
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
void createLegacyDataArray(DataStructure& ds, DataObject::IdType parentId, const H5::DatasetReader& dataArrayReader, usize tDims, usize cDims)
{
  const std::string daName = dataArrayReader.getName();

  auto dataStore = new complex::DataStore<T>({tDims}, {cDims});
  auto data = dataArrayReader.readAsVector<T>();
  if(data.size() != dataStore->getSize())
  {
    throw std::runtime_error(fmt::format("Error reading HDF5 Data set {}", complex::H5::Support::GetObjectPath(dataArrayReader.getId())));
  }
  // Copy values
  for(usize i = 0; i < data.size(); i++)
  {
    dataStore->setValue(i, data.at(i));
  }
  // Insert the DataArray into the DataStructure
  auto dataArray = DataArray<T>::Create(ds, daName, dataStore, parentId);
}

/**
 * @brief
 * @param daId
 * @param tDims
 * @param cDims
 */
void readLegacyDataArrayDims(const H5::DatasetReader& dataArrayReader, usize& tDims, usize& cDims)
{
  hid_t compType = H5::Support::HdfTypeForPrimitive<int64>();

  {
    auto compAttrib = dataArrayReader.getAttribute(Legacy::CompDims);
    if(!compAttrib.isValid())
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }

    auto buffer = compAttrib.readAsVector<int64>();
    cDims = std::accumulate(buffer.begin(), buffer.end(), static_cast<usize>(0));
  }

  {
    auto tupleAttrib = dataArrayReader.getAttribute(Legacy::TupleDims);
    if(!tupleAttrib.isValid())
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }

    auto buffer = tupleAttrib.readAsVector<int64>();
    tDims = std::accumulate(buffer.begin(), buffer.end(), static_cast<usize>(0));
  }
}

void readLegacyDataArray(DataStructure& ds, const H5::DatasetReader& dataArrayReader, DataObject::IdType parentId)
{
  auto size = H5Dget_storage_size(dataArrayReader.getId());
  auto typeId = dataArrayReader.getTypeId();

  usize tDims;
  usize cDims;
  readLegacyDataArrayDims(dataArrayReader, tDims, cDims);

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    createLegacyDataArray<float32>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    createLegacyDataArray<float64>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    createLegacyDataArray<int8>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    createLegacyDataArray<int16>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    createLegacyDataArray<int32>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    createLegacyDataArray<int64>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    createLegacyDataArray<uint8>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    createLegacyDataArray<uint16>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    createLegacyDataArray<uint32>(ds, parentId, dataArrayReader, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    createLegacyDataArray<uint64>(ds, parentId, dataArrayReader, tDims, cDims);
  }

  H5Tclose(typeId);
}

void readLegacyAttributeMatrix(DataStructure& ds, const H5::GroupReader& amGroupReader, DataObject::IdType parentId)
{
  const std::string amName = amGroupReader.getName();
  auto attributeMatrix = DataGroup::Create(ds, amName, parentId);

  auto dataArrayNames = amGroupReader.getChildNames();
  for(const auto& daName : dataArrayNames)
  {
    if(daName != "NeighborList")
    {
      auto dataArraySet = amGroupReader.openDataset(daName);
      readLegacyDataArray(ds, dataArraySet, attributeMatrix->getId());
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

DataObject* readLegacyVertexGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = VertexGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  return geom;
}

DataObject* readLegacyTriangleGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = TriangleGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  return geom;
}

DataObject* readLegacyTetrahedralGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = TetrahedralGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  return geom;
}

DataObject* readLegacyRectGridGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = RectGridGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  return geom;
}

DataObject* readLegacyQuadGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = QuadGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  return geom;
}

DataObject* readLegacyHexGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = HexahedralGeom::Create(ds, name);
  readGenericGeomDims(geom, geomGroup);
  return geom;
}

DataObject* readLegacyEdgeGeom(DataStructure& ds, const H5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = EdgeGeom::Create(ds, name);
  auto edge = dynamic_cast<EdgeGeom*>(geom);
  readGenericGeomDims(geom, geomGroup);
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

void readLegacyDataContainer(DataStructure& ds, const H5::GroupReader& dcGroup)
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
    readLegacyAttributeMatrix(ds, attributeMatrixGroup, container->getId());
  }
}

DataStructure ImportLegacyDataStructure(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  // auto dataStructureGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);
  DataStructure ds;

  auto dcaGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);

  // Iterate over DataContainers
  const auto dcNames = dcaGroup.getChildNames();
  for(const auto& dcName : dcNames)
  {
    auto dcGroup = dcaGroup.openGroup(dcName);
    readLegacyDataContainer(ds, dcGroup);
  }

  return ds;

  throw std::runtime_error("Not implemented: ImportLegacyDataStructure from dream3d file");
}

complex::DataStructure complex::DREAM3D::ImportDataStructureFromFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  errorCode = 0;

  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return ImportDataStructureV8(fileReader, errorCode);
  }
  else if(fileVersion == Legacy::FileVersion)
  {
    return ImportLegacyDataStructure(fileReader, errorCode);
  }
  // Unsupported file version
  return DataStructure();
}

complex::Pipeline complex::DREAM3D::ImportPipelineFromFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  errorCode = 0;

  if(GetPipelineVersion(fileReader) != k_CurrentPipelineVersion)
  {
    return {};
  }

  auto pipelineGroupReader = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineDatasetReader = pipelineGroupReader.openDataset(k_PipelineJsonTag);
  if(!pipelineDatasetReader.isValid())
  {
    return {};
  }

  auto pipelineJsonString = pipelineDatasetReader.readAsString();
  auto pipelineJson = nlohmann::json::parse(pipelineJsonString);
  return Pipeline::FromJson(pipelineJson);
}

complex::DREAM3D::FileData complex::DREAM3D::ReadFile(const H5::FileReader& fileReader, H5::ErrorType& errorCode)
{
  errorCode = 0;
  // Pipeline pipeline;
  auto pipeline = ImportPipelineFromFile(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {};
  }

  auto dataStructure = ImportDataStructureFromFile(fileReader, errorCode);
  if(errorCode < 0)
  {
    return {};
  }

  return {pipeline, dataStructure};
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
  auto errorCode = WriteFileVersion(fileWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WritePipeline(fileWriter, fileData.first);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteDataStructure(fileWriter, fileData.second);
  return errorCode;
}
