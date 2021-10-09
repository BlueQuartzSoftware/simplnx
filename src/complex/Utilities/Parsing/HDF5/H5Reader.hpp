#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"
#include "complex/complex_export.hpp"

#include <numeric>
#include <optional>
#include <string>

namespace complex
{
namespace H5
{
namespace Reader
{
namespace Generic
{
/**
 * @brief Returns the name of the HDF5 object at the specified group index.
 * @param id HDF5 group or file ID
 * @param idx HDF5 group index
 * @return std::string
 */
std::string COMPLEX_EXPORT getNameAtIdx(H5::IdType id, H5::SizeType idx);

/**
 * @brief Returns the name of the HDF5 object at the specified ID.
 * @param id HDF5 target ID
 * @return std::string
 */
std::string COMPLEX_EXPORT getName(H5::IdType id);

/**
 * @brief readRequiredAttributes Reads the required attributes from an H5 Data set
 * @param objType The type (subclass) of IDataArray that is stored in the H5 file
 * @param version The version of the DataArray class
 * @param tDims The Tuple Dimensions of the data array
 * @param cDims The Component Dimensions of the data array
 * @return H5::ErrorType
 */
H5::ErrorType COMPLEX_EXPORT readRequiredAttributes(H5::IdType gid, const std::string& name, std::string& objType, int32& version, std::vector<usize>& tDims, std::vector<usize>& cDims);

/**
 * @brief Reads a string attribute from an HDF object
 * @param locationID The Parent object that holds the object to which you want to read an attribute
 * @param objectName The name of the object to which the attribute is to be read
 * @param attributeName The name of the Attribute to read
 * @param data The memory to store the data
 * @return Standard HDF Error condition
 */
H5::ErrorType COMPLEX_EXPORT readStringAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, std::string& data);

/**
 * @brief Reads a scalar attribute value from a dataset
 * @param locationID
 * @param objectName The name of the dataset
 * @param attributeName The name of the Attribute
 * @param data The preallocated memory for the variable to be stored into
 * @return Standard H5 error condition
 */
template <typename T>
inline H5::ErrorType readScalarAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, T& data)
{
  H5O_info_t objectInfo;
  herr_t returnError = 0;
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  // std::cout << "Reading Scalar style Attribute at Path '" << objectName << "' with Key: '" << attributeName << "'" << std::endl;
  /* Get the type of object */
  herr_t error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }
  /* Open the object */
  hid_t objectID = Support::OpenId(locationID, objectName, objectInfo.type);
  if(objectID >= 0)
  {
    hid_t attributeID = H5Aopen_by_name(locationID, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeID >= 0)
    {
      error = H5Aread(attributeID, dataType, &data);
      if(error < 0)
      {
        std::cout << "Error Reading Attribute." << std::endl;
        returnError = error;
      }
      error = H5Aclose(attributeID);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    else
    {
      returnError = static_cast<herr_t>(attributeID);
    }
    error = Support::CloseId(objectID, objectInfo.type);
    if(error < 0)
    {
      std::cout << "Error Closing Object" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

/**
 * @brief Returns the information about an attribute.
 * You must close the typeID argument or resource leaks will occur. Use
 *  H5Tclose(typeID); after your call to this method if you do not need the id for
 *   anything.
 * @param locationID The parent location of the Dataset
 * @param objectName The name of the dataset
 * @param attributeName The name of the attribute
 * @param dims A std::vector that will hold the sizes of the dimensions
 * @param typeClass The H5 class type
 * @param typeSize THe H5 getSize of the data
 * @param typeID The Attribute ID - which needs to be closed after you are finished with the data
 * @return
 */
herr_t COMPLEX_EXPORT getAttributeInfo(hid_t locationID, const std::string& objectName, const std::string& attributeName, std::vector<hsize_t>& dims, H5T_class_t& typeClass, usize& typeSize,
                                       hid_t& typeID);

/**
 * @brief Reads an Attribute from an H5 Object.
 *
 * Use this method if you already know the datatype of the attribute. If you do
 * not know this already then use another form of this method.
 *
 * @param locationID The Parent object that holds the object to which you want to read an attribute
 * @param objectName The name of the object to which the attribute is to be read
 * @param attributeName The name of the Attribute to read
 * @param data The memory to store the data
 * @return Standard HDF Error condition
 */
template <typename T>
herr_t readVectorAttribute(hid_t locationID, const std::string& objectName, const std::string& attributeName, std::vector<T>& data)
{
  herr_t returnError = 0;
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return -1;
  }
  // std::cout << "   Reading Vector Attribute at Path '" << objectName << "' with Key: '" << attributeName << "'" << std::endl;
  /* Get the type of object */
  H5O_info_t objectInfo;
  herr_t error = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return error;
  }
  /* Open the object */
  hid_t objectID = Support::OpenId(locationID, objectName, objectInfo.type);
  if(objectID >= 0)
  {
    hid_t attributeID = H5Aopen_by_name(locationID, objectName.c_str(), attributeName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    if(attributeID >= 0)
    {
      // Need to allocate the array getSize
      H5T_class_t typeClass;
      usize typeSize = 0;
      std::vector<hsize_t> dims;
      hid_t typeID = 0;
      error = Generic::getAttributeInfo(locationID, objectName, attributeName, dims, typeClass, typeSize, typeID);
      hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());
      // std::cout << "    Vector Attribute has " << numElements << " elements." << std::endl;
      data.resize(numElements);
      error = H5Aread(attributeID, dataType, data.data());
      if(error < 0)
      {
        std::cout << "Error Reading Attribute." << error << std::endl;
        returnError = error;
      }
      error = H5Aclose(attributeID);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    else
    {
      returnError = static_cast<herr_t>(attributeID);
    }
    error = Support::CloseId(objectID, objectInfo.type);
    if(error < 0)
    {
      std::cout << "Error Closing Object" << std::endl;
      returnError = error;
    }
  }
  return returnError;
}

/**
 * @brief Reads a dataset of multiple strings into a std::vector<std::string>
 * @param locationID
 * @param datasetName
 * @param data
 * @return
 */
herr_t COMPLEX_EXPORT readVectorOfStringDataset(H5::IdType locationID, const std::string& datasetName, std::vector<std::string>& data);

/**
 * @brief Reads a string dataset into the supplied string. Any data currently in the 'data' variable
 * is cleared first before the new data is read into the string.
 * @param locationID The parent group that holds the data object to read
 * @param datasetName The name of the dataset.
 * @param data The std::string to hold the data
 * @return Standard HDF error condition
 */
herr_t COMPLEX_EXPORT readStringDataset(H5::IdType locationID, const std::string& datasetName, std::string& data);
} // namespace Generic

namespace DataStructure
{
/**
 * @brief Creates a DataStructure based on the specified legacy DREAM.3D filepath.
 * @param filepath
 * @return complex::DataStructure
 */
complex::DataStructure COMPLEX_EXPORT readLegacyFile(const std::string& filepath);

/**
 * @brief Creates a DataStructure based on the specified DREAM.3D filepath.
 * The appropriate reader will be used based on the DREAM.3D file version with readLegacyFile(const std::string&).
 * @param filepath
 * @return complex::DataStructure
 */
complex::DataStructure COMPLEX_EXPORT readFile(const std::string& filepath);

/**
 * @brief
 * @param ds
 * @param gid
 * @param name
 * @param metaDataOnly
 * @return complex::DataObject*
 */
// complex::DataObject* COMPLEX_EXPORT readDataArray(complex::DataStructure& ds, H5::IdType gid, const std::string& name, const std::optional<DataObject::IdType>& parent = {}, bool metaDataOnly =
// false);

/**
 * @brief
 * @param ds
 * @param gid
 * @param name
 * @param parent = {}
 * @return complex::DataObject*
 */
// complex::DataObject* COMPLEX_EXPORT readDataGroup(complex::DataStructure& ds, H5::IdType gid, const std::string& name, const std::optional<DataObject::IdType>& parent = {});

/**
 * @brief readNeighborListData
 * @param ds The DataStructure to insert the NeighborList into
 * @param gid The H5 Group to read the data array from
 * @param name The name of the data set
 * @param metaDataOnly Read just the meta data about the DataArray or actually read all the data
 * @return
 */
// complex::DataObject* COMPLEX_EXPORT readNeighborListData(complex::DataStructure& ds, H5::IdType gid, const std::string& name, bool metaDataOnly = false);

} // namespace DataStructure
} // namespace Reader
} // namespace H5
} // namespace complex
