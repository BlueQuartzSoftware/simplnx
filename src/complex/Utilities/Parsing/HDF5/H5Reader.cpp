#include "H5Reader.hpp"

#include <memory>
#include <numeric>

#include <H5Apublic.h>
#include <hdf5.h>

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IDataStore.hpp"

using namespace complex;

namespace H5Reader::Constants
{
inline const std::string FileVersionTag = "FileVersion";
}

namespace Legacy
{
inline const std::string DCATag = "DataContainers";
inline const std::string GeometryTag = "_SIMPL_GEOMETRY";
inline const std::string GeometryNameTag = "GeometryName";
inline const std::string PipelineName = "Pipeline";
inline const std::string CompDims = "ComponentDimensions";
inline const std::string TupleDims = "TupleDimensions";

constexpr float64 Version = 7.0;

namespace Type
{
inline const std::string ImageGeom = "ImageGeometry";
inline const std::string EdgeGeom = "EdgeGeometry";
inline const std::string HexGeom = "HexahedralGeometry";
inline const std::string QuadGeom = "QuadGeometry";
inline const std::string RectGridGeom = "RectGridGeometry";
inline const std::string TetrahedralGeom = "TetrahedralGeometry";
inline const std::string TriangleGeom = "TriangleGeometry";
inline const std::string VertexGeom = "VertexGeometry";
} // namespace Type
} // namespace Legacy

/**
 * @brief Reads data from an H5 file into the provided IDataStore<T> using IDataStore<T>::iterators.
 * @tparam T Datatype to read from H5
 * @param locationID H5 location ID
 * @param datasetName Name of the dataset to open in H5
 * @param dataStore IDataStore<T> pointer to iterate over when assigning values from H5
 * @return H5::ErrorType
 */
template <typename T>
complex::H5::ErrorType readIteratorDataset(hid_t locationID, const std::string& datasetName, complex::IDataStore<T>* dataStore)
{
  auto size = dataStore->getSize();
  if(size < 0)
  {
    std::cout << "There are less than 0 items in the data store. This is NOT allowed." << std::endl;
    return -3;
  }

  T* buffer = new T[size];

  herr_t returnError = 0;
  hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was not supported." << std::endl;
    return -10;
  }
  if(locationID < 0)
  {
    std::cout << "locationID was Negative: This is not allowed." << std::endl;
    return -2;
  }
  hid_t datasetID = H5Dopen(locationID, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << " Error opening Dataset: " << datasetID << std::endl;
    return -1;
  }
  if(datasetID >= 0)
  {
    herr_t error = H5Dread(datasetID, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
    if(error < 0)
    {
      std::cout << "Error Reading Data." << std::endl;
      returnError = error;
    }
    error = H5Dclose(datasetID);
    if(error < 0)
    {
      std::cout << "Error Closing Dataset id" << std::endl;
      returnError = error;
    }
  }

  // Apply values to dataStore
  size_t index = 0;
  for(auto value : *dataStore)
  {
    value = buffer[index++];
  }
  delete[] buffer;

  return returnError;
}

/**
 * @brief
 * @tparam T
 * @param locId
 * @param tupleCount
 * @param compCount
 * @return DataStore<T>*
 */
template <typename T>
complex::DataStore<T>* readH5Dataset(hid_t locId, const std::string& datasetName, size_t tupleCount, size_t compCount)
{
  auto dataStore = new DataStore<T>(tupleCount, compCount);

  herr_t err = readIteratorDataset<T>(locId, datasetName, dataStore);
  if(err < 0)
  {
    std::cout << "readH5Data read error: " << __FILE__ << "(" << __LINE__ << ")";
    return nullptr;
  }
  return dataStore;
}

herr_t complex::H5::Reader::Generic::readVectorOfStringDataset(H5::IdType locationID, const std::string& datasetName, std::vector<std::string>& data)
{
  herr_t error = 0;
  herr_t returnError = 0;

  hid_t datasetID = H5Dopen(locationID, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << "H5Lite.cpp::readVectorOfStringDataset(" << __LINE__ << ") Error opening Dataset at locationID (" << locationID << ") with object name (" << datasetName << ")" << std::endl;
    return -1;
  }
  /*
   * Get the datatype.
   */
  hid_t typeID = H5Dget_type(datasetID);
  if(typeID >= 0)
  {
    hsize_t dims[1] = {0};
    /*
     * Get dataspace and allocate memory for read buffer.
     */
    hid_t dataspaceID = H5Dget_space(datasetID);
    int nDims = H5Sget_simple_extent_dims(dataspaceID, dims, nullptr);
    if(nDims != 1)
    {
      CloseH5S(dataspaceID, error, returnError);
      CloseH5T(typeID, error, returnError);
      CloseH5D(datasetID, error, returnError, datasetName);
      std::cout << "H5Lite.cpp::readVectorOfStringDataset(" << __LINE__ << ") Number of dims should be 1 but it was " << nDims << ". Returning early. Is your data file correct?" << std::endl;
      return -2;
    }

    std::vector<char*> rData(dims[0], nullptr);

    /*
     * Create the memory datatype.
     */
    hid_t memtype = H5Tcopy(H5T_C_S1);
    herr_t status = H5Tset_size(memtype, H5T_VARIABLE);

    H5T_cset_t characterSet = H5Tget_cset(typeID);
    status = H5Tset_cset(memtype, characterSet);

    /*
     * Read the data.
     */
    status = H5Dread(datasetID, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rData.data());
    if(status < 0)
    {
      status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
      CloseH5S(dataspaceID, error, returnError);
      CloseH5T(typeID, error, returnError);
      CloseH5T(memtype, error, returnError);
      CloseH5D(datasetID, error, returnError, datasetName);
      std::cout << "H5Lite.cpp::readVectorOfStringDataset(" << __LINE__ << ") Error reading Dataset at locationID (" << locationID << ") with object name (" << datasetName << ")" << std::endl;
      return -3;
    }
    /*
     * copy the data into the vector of strings
     */
    data.resize(dims[0]);
    for(size_t i = 0; i < dims[0]; i++)
    {
      data[i] = std::string(rData[i]);
    }
    /*
     * Close and release resources.  Note that H5Dvlen_reclaim works
     * for variable-length strings as well as variable-length arrays.
     * Also note that we must still free the array of pointers stored
     * in rData, as H5Tvlen_reclaim only frees the data these point to.
     */
    status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
    CloseH5S(dataspaceID, error, returnError);
    CloseH5T(typeID, error, returnError);
    CloseH5T(memtype, error, returnError);
  }

  CloseH5D(datasetID, error, returnError, datasetName);

  return returnError;
}

herr_t complex::H5::Reader::Generic::readStringDataset(H5::IdType locationID, const std::string& datasetName, std::string& data)
{
  herr_t error = 0;
  herr_t returnError = 0;
  data.clear();
  hid_t datasetID = H5Dopen(locationID, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << "H5Reader.hpp::readStringDataset(" << __LINE__ << ") Error opening Dataset at locationID (" << locationID << ") with object name (" << datasetName << ")" << std::endl;
    return -1;
  }
  /*
   * Get the datatype.
   */
  hid_t typeID = H5Dget_type(datasetID);
  if(typeID >= 0)
  {
    htri_t isVariableString = H5Tis_variable_str(typeID); // Test if the string is variable length

    if(isVariableString == 1)
    {
      std::vector<std::string> strings;
      error = readVectorOfStringDataset(locationID, datasetName, strings); // Read the string
      if(error < 0 || (strings.size() > 1 && !strings.empty()))
      {
        std::cout << "Error Reading string dataset. There were multiple Strings and the program asked for a single string." << std::endl;
        returnError = error;
      }
      else
      {
        data.assign(strings[0]);
      }
    }
    else
    {
      hsize_t size = H5Dget_storage_size(datasetID);
      std::vector<char> buffer(static_cast<usize>(size + 1), 0x00); // Allocate and Zero and array
      error = H5Dread(datasetID, typeID, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
      if(error < 0)
      {
        std::cout << "Error Reading string dataset." << std::endl;
        returnError = error;
      }
      else
      {
        data.append(buffer.data()); // Append the string to the given string
      }
    }
  }
  CloseH5D(datasetID, error, returnError, datasetName);
  CloseH5T(typeID, error, returnError);
  return returnError;
}

complex::H5::ErrorType complex::H5::Reader::Generic::readRequiredAttributes(H5::IdType gid, const std::string& name, std::string& objType, int32& version, std::vector<usize>& tDims,
                                                                            std::vector<usize>& cDims)
{
  H5::ErrorType err = 0;
  H5::ErrorType retErr = 0;
  err = readStringAttribute(gid, name, "ObjectType", objType);
  if(err < 0)
  {
    retErr = err;
  }

  err = readScalarAttribute(gid, name, "DataArrayVersion", version);
  if(err < 0)
  {
    retErr = err;
    version = 1;
  }

  // Read the tuple dimensions as an attribute
  err = readVectorAttribute(gid, name, "TupleDimensions", tDims);
  if(err < 0)
  {
    retErr = err;
    std::cout << "Missing TupleDimensions for Array with Name: " << name;
  }

  // Read the component dimensions as  an attribute
  err = readVectorAttribute(gid, name, "ComponentDimensions", cDims);
  if(err < 0)
  {
    retErr = err;
    std::cout << "Missing ComponentDimensions for Array with Name: " << name;
  }

  return retErr;
}

complex::H5::ErrorType complex::H5::Reader::Generic::readStringAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, std::string& data)
{
  H5O_info_t objectInfo{};
  std::vector<char> attributeOutput;
  herr_t returnError = 0;
  data.clear();
  HDF_ERROR_HANDLER_OFF;

  /* Get the type of object */
  herr_t error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }

  /* Open the object */
  hid_t objectID = Support::openId(locationID, objectName, objectInfo.type);
  if(objectID >= 0)
  {
    hid_t attributeID = H5Aopen_by_name(locationID, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    hid_t attrTypeId = H5Aget_type(attributeID);
    htri_t isVariableString = H5Tis_variable_str(attrTypeId); // Test if the string is variable length
    H5Tclose(attrTypeId);
    if(isVariableString == 1)
    {
      data.clear();
      returnError = -1;
      CloseH5A(attributeID, error, returnError);
      return returnError;
    }
    if(attributeID >= 0)
    {
      hsize_t size = H5Aget_storage_size(attributeID);
      attributeOutput.resize(static_cast<usize>(size)); // Resize the vector to the proper length
      hid_t attributeType = H5Aget_type(attributeID);
      if(attributeType >= 0)
      {
        error = H5Aread(attributeID, attributeType, attributeOutput.data());
        if(error < 0)
        {
          std::cout << "Error Reading Attribute." << std::endl;
          returnError = error;
        }
        else
        {
          if(attributeOutput[size - 1] == 0) // null Terminated string
          {
            size -= 1;
          }
          data.append(attributeOutput.data(), size); // Append the data to the passed in string
        }
        CloseH5T(attributeType, error, returnError);
      }
      CloseH5A(attributeID, error, returnError);
    }
    else
    {
      // returnError = attributeID;
    }
    error = Support::closeId(objectID, objectInfo.type);
    if(error < 0)
    {
      std::cout << "Error Closing Object ID" << std::endl;
      returnError = error;
    }
  }
  HDF_ERROR_HANDLER_ON;
  return returnError;
}

herr_t complex::H5::Reader::Generic::getAttributeInfo(hid_t locationID, const std::string& objectName, const std::string& attributeName, std::vector<hsize_t>& dims, H5T_class_t& typeClass,
                                                      size_t& typeSize, hid_t& typeID)
{
  H5O_info_t objectInfo{};
  herr_t returnError = 0;

  herr_t error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }

  /* Open the object */
  hid_t objectID = Support::openId(locationID, objectName, objectInfo.type);
  if(objectID >= 0)
  {
    hid_t attributeID = H5Aopen_by_name(locationID, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeID >= 0)
    {
      /* Get an identifier for the datatype. */
      typeID = H5Aget_type(attributeID);
      if(typeID > 0)
      {
        /* Get the class. */
        typeClass = H5Tget_class(typeID);
        /* Get the getSize. */
        typeSize = H5Tget_size(typeID);
        hid_t dataspaceID = H5Aget_space(attributeID);
        if(dataspaceID >= 0)
        {
          if(typeClass == H5T_STRING)
          {
            hid_t rank = 1;
            dims.resize(rank);
            dims[0] = typeSize;
          }
          else
          {
            hid_t rank = H5Sget_simple_extent_ndims(dataspaceID);
            std::vector<hsize_t> _dims(rank, 0);
            /* Get dimensions */
            error = H5Sget_simple_extent_dims(dataspaceID, _dims.data(), nullptr);
            if(error < 0)
            {
              std::cout << "Error Getting Attribute dims" << std::endl;
              returnError = error;
            }
            // Copy the dimensions into the dims vector
            dims.clear(); // Erase everything in the Vector
            dims.resize(rank);
            std::copy(_dims.cbegin(), _dims.cend(), dims.begin());
          }
          CloseH5S(dataspaceID, error, returnError);
          dataspaceID = 0;
        }
      }
      CloseH5A(attributeID, error, returnError);
      attributeID = 0;
    }
    else
    {
      returnError = -1;
    }
    error = Support::closeId(objectID, objectInfo.type);
    if(error < 0)
    {
      std::cout << "Error Closing Object ID" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

//////////////////////////
// Begin Legacy Support //
//////////////////////////
std::string H5::Reader::Generic::getNameAtIdx(H5::IdType id, H5::SizeType idx)
{
  constexpr size_t size = 120;
  char name[size];
  auto length = H5Gget_objname_by_idx(id, idx, name, size);
  std::string nameStr(name);
  return nameStr;
}

std::string H5::Reader::Generic::getName(H5::IdType id)
{
  constexpr size_t size = 120;
  char name[size];
  auto length = H5Iget_name(id, name, size);
  std::string nameStr(name);
  return nameStr;
}

template <typename T>
void createLegacyDataArray(complex::DataStructure& ds, const std::string& name, complex::DataObject::IdType parentId, hid_t daId, size_t tDims, size_t cDims)
{
  auto buffer = std::make_unique<T[]>(tDims * cDims);
  hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
  auto err = H5Dread(daId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.get());
  if(err < 0)
  {
    throw std::runtime_error("");
  }

  auto dataStore = new complex::DataStore<T>(cDims, tDims, std::move(buffer));
  auto dataArray = DataArray<T>::Create(ds, name, dataStore, parentId);
}

void readLegacyDataArrayDims(hid_t daId, size_t& tDims, size_t& cDims)
{
  hid_t compType = H5::Support::HDFTypeForPrimitive<int64>();

  {
    hid_t compId = H5Aopen(daId, Legacy::CompDims.c_str(), H5P_DEFAULT);
    if(compId < 0)
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }

    size_t compSize = H5Aget_storage_size(compId) / 8;
    auto buffer = new int64[compSize];
    H5Aread(compId, compType, buffer);
    H5Aclose(compId);

    cDims = std::accumulate(buffer, buffer + compSize, static_cast<usize>(0));
    delete[] buffer;
  }

  {
    hid_t tupleId = H5Aopen(daId, Legacy::TupleDims.c_str(), H5P_DEFAULT);
    if(tupleId < 0)
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }

    size_t tupleSize = H5Aget_storage_size(tupleId) / 8;
    auto buffer = new int64[tupleSize];
    H5Aread(tupleId, compType, buffer);
    H5Aclose(tupleId);

    tDims = std::accumulate(buffer, buffer + tupleSize, static_cast<usize>(0));
    delete[] buffer;
  }
}

void readLegacyDataArray(complex::DataStructure& ds, hid_t daId, const std::string& name, complex::DataObject::IdType parentId)
{
  auto size = H5Dget_storage_size(daId);
  auto typeId = H5Dget_type(daId);

  size_t tDims;
  size_t cDims;
  readLegacyDataArrayDims(daId, tDims, cDims);

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    createLegacyDataArray<float32>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    createLegacyDataArray<float64>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    createLegacyDataArray<int8>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    createLegacyDataArray<int16>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    createLegacyDataArray<int32>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    createLegacyDataArray<int64>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    createLegacyDataArray<uint8>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    createLegacyDataArray<uint16>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    createLegacyDataArray<uint32>(ds, name, parentId, daId, tDims, cDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    createLegacyDataArray<uint64>(ds, name, parentId, daId, tDims, cDims);
  }

  H5Tclose(typeId);
}

void readLegacyAttributeMatrix(complex::DataStructure& ds, hid_t amId, const std::string& name, complex::DataObject::IdType parentId)
{
  auto attributeMatrix = DataGroup::Create(ds, name, parentId);

  hsize_t count;
  H5Gget_num_objs(amId, &count);
  for(hsize_t i = 0; i < count; i++)
  {
    const std::string name = complex::H5::Reader::Generic::getNameAtIdx(amId, i);
    if(name != "NeighborList")
    {
      hid_t daId = H5Dopen(amId, name.c_str(), H5P_DEFAULT);
      readLegacyDataArray(ds, daId, name, attributeMatrix->getId());
      H5Dclose(daId);
    }
  }
}

void readGenericGeomDims(complex::AbstractGeometry* geom, hid_t geomId)
{
  int32 sDims;
  int32 uDims;

  hid_t dataType = H5::Support::HDFTypeForPrimitive<int32>();
  hid_t sDimId = H5Aopen(geomId, "SpatialDimensionality", H5P_DEFAULT);
  H5Aread(sDimId, dataType, &sDims);
  H5Aclose(sDimId);

  hid_t uDimId = H5Aopen(geomId, "UnitDimensionality", H5P_DEFAULT);
  H5Aread(uDimId, dataType, &uDims);
  H5Aclose(uDimId);

  geom->setSpatialDimensionality(sDims);
  geom->setUnitDimensionality(uDims);
}

DataObject* readLegacyVertexGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = VertexGeom::Create(ds, name);
  readGenericGeomDims(geom, geomId);
  return geom;
}

DataObject* readLegacyTriangleGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = TriangleGeom::Create(ds, name);
  readGenericGeomDims(geom, geomId);
  return geom;
}

DataObject* readLegacyTetrahedralGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = TetrahedralGeom::Create(ds, name);
  readGenericGeomDims(geom, geomId);
  return geom;
}

DataObject* readLegacyRectGridGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = RectGridGeom::Create(ds, name);
  readGenericGeomDims(geom, geomId);
  return geom;
}

DataObject* readLegacyQuadGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = QuadGeom::Create(ds, name);
  readGenericGeomDims(geom, geomId);
  return geom;
}

DataObject* readLegacyHexGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = HexahedralGeom::Create(ds, name);
  readGenericGeomDims(geom, geomId);
  return geom;
}

DataObject* readLegacyEdgeGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = EdgeGeom::Create(ds, name);
  auto edge = dynamic_cast<EdgeGeom*>(geom);

  readGenericGeomDims(geom, geomId);

  return geom;
}

DataObject* readLegacyImageGeom(complex::DataStructure& ds, hid_t geomId, const std::string& name)
{
  auto geom = ImageGeom::Create(ds, name);
  auto image = dynamic_cast<ImageGeom*>(geom);

  readGenericGeomDims(geom, geomId);

  // DIMENSIONS array
  {
    hid_t dimsType = H5::Support::HDFTypeForPrimitive<int64>();
    int64 dims[3];
    hid_t dimsId = H5Dopen(geomId, "DIMENSIONS", H5P_DEFAULT);
    herr_t error = H5Dread(dimsId, dimsType, H5S_ALL, H5S_ALL, H5P_DEFAULT, dims);
    image->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
    H5Dclose(dimsId);
  }

  // ORIGIN array
  {
    hid_t originType = H5::Support::HDFTypeForPrimitive<float32>();
    float32 origin[3];
    hid_t originId = H5Dopen(geomId, "ORIGIN", H5P_DEFAULT);
    herr_t error = H5Dread(originId, originType, H5S_ALL, H5S_ALL, H5P_DEFAULT, origin);
    image->setOrigin(FloatVec3(origin[0], origin[1], origin[2]));
    H5Dclose(originId);
  }

  // SPACING array
  {
    hid_t sType = H5::Support::HDFTypeForPrimitive<float32>();
    float32 spacing[3];
    hid_t spacingId = H5Dopen(geomId, "SPACING", H5P_DEFAULT);
    herr_t error = H5Dread(spacingId, sType, H5S_ALL, H5S_ALL, H5P_DEFAULT, spacing);
    image->setSpacing(FloatVec3(spacing[0], spacing[1], spacing[2]));
    H5Dclose(spacingId);
  }

  return image;
}

void readLegacyDataContainer(complex::DataStructure& ds, hid_t dcId, const std::string& name)
{
  DataObject* container = nullptr;

  // Check for geometry
  hid_t geomId = H5Gopen(dcId, Legacy::GeometryTag.c_str(), H5P_DEFAULT);
  if(geomId >= 0)
  {
    std::string data;
    H5::Reader::Generic::readStringAttribute(geomId, ".", Legacy::GeometryNameTag.c_str(), data);
    if(data == Legacy::Type::ImageGeom)
    {
      container = readLegacyImageGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::EdgeGeom)
    {
      container = readLegacyEdgeGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::HexGeom)
    {
      container = readLegacyHexGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::QuadGeom)
    {
      container = readLegacyQuadGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::RectGridGeom)
    {
      container = readLegacyRectGridGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::TetrahedralGeom)
    {
      container = readLegacyTetrahedralGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::TriangleGeom)
    {
      container = readLegacyTriangleGeom(ds, geomId, name);
    }
    else if(data == Legacy::Type::VertexGeom)
    {
      container = readLegacyVertexGeom(ds, geomId, name);
    }
    H5Gclose(geomId);
  }

  // No geometry found. Create a DataGroup instead
  if(!container)
  {
    container = DataGroup::Create(ds, name);
  }

  hsize_t count;
  H5Gget_num_objs(dcId, &count);
  for(hsize_t i = 0; i < count; i++)
  {
    const std::string name = complex::H5::Reader::Generic::getNameAtIdx(dcId, i);
    if(name == Legacy::GeometryTag)
    {
      continue;
    }
    hid_t amId = H5Gopen(dcId, name.c_str(), H5P_DEFAULT);
    readLegacyAttributeMatrix(ds, amId, name, container->getId());
    H5Gclose(amId);
  }
}

complex::DataStructure readLegacyId(H5::IdType fileId)
{
  complex::DataStructure ds;

  hid_t dcaId = H5Gopen(fileId, Legacy::DCATag.c_str(), H5P_DEFAULT);

  // Iterate over DataContainers
  hsize_t count;
  H5Gget_num_objs(dcaId, &count);
  for(hsize_t i = 0; i < count; i++)
  {
    const std::string name = complex::H5::Reader::Generic::getNameAtIdx(dcaId, i);
    hid_t dcId = H5Gopen(dcaId, name.c_str(), H5P_DEFAULT);
    readLegacyDataContainer(ds, dcId, name);
    H5Gclose(dcId);
  }

  H5Gclose(dcaId);

  return ds;
}

complex::DataStructure complex::H5::Reader::DataStructure::readLegacyFile(const std::string& filepath)
{
  hid_t fileId = H5Fopen(filepath.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
  if(fileId < 0)
  {
    throw std::runtime_error("Error opening legacy DREAM.3D HDF5 file: " + filepath);
  }

  complex::DataStructure ds = readLegacyId(fileId);
  H5Fclose(fileId);
  return ds;
}
////////////////////////
// End Legacy Support //
////////////////////////

complex::DataStructure H5::Reader::DataStructure::readFile(const std::string& filepath)
{
  hid_t fileId = H5Fopen(filepath.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);

  float64 fileVersion;
  herr_t err = Generic::readScalarAttribute<float64>(fileId, ".", H5Reader::Constants::FileVersionTag.c_str(), fileVersion);

  complex::DataStructure ds;
  if(err >= 0)
  {
    if(std::abs(fileVersion - Legacy::Version) <= 0.001f)
    {
      ds = std::move(readLegacyId(fileId));
    }
    else
    {
      ds = std::move(complex::DataStructure::ReadFromHdf5(fileId, err));
    }
  }

  H5Fclose(fileId);
  return ds;
}

/**
 * @brief
 * @tparam T
 * @param ds
 * @param gid
 * @param tupleCount
 * @param compCount
 * @param name
 * @param parent
 * @param metaDataOnly
 * @return DataObject*
 */
template <typename T>
complex::DataObject* createDataArray(complex::DataStructure& ds, hid_t gid, size_t tupleCount, size_t compCount, const std::string& name, const std::optional<DataObject::IdType>& parent,
                                     bool metaDataOnly)
{
  IDataStore<T>* dataStore;
  if(!metaDataOnly)
  {
    dataStore = readH5Dataset<T>(gid, name, tupleCount, compCount);
  }
  else
  {
    dataStore = new EmptyDataStore<T>(tupleCount, compCount);
  }
  return DataArray<T>::Create(ds, name, dataStore, parent);
}
