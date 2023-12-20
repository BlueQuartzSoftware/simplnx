#include "H5Support.hpp"

#include <cstring>
#include <iostream>

bool nx::core::HDF5::Support::IsGroup(hid_t nodeId, const std::string& objectName)
{
  H5SUPPORT_MUTEX_LOCK()

  bool isGroup = true;
  herr_t error = -1;
  H5O_info_t objectInfo{};
  error = H5Oget_info_by_name(nodeId, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    std::cout << "Error in method H5Oget_info_by_name()" << std::endl;
    return false;
  }
  switch(objectInfo.type)
  {
  case H5O_TYPE_GROUP:
    isGroup = true;
    break;
  case H5O_TYPE_DATASET:
    isGroup = false;
    break;
  case H5O_TYPE_NAMED_DATATYPE:
    isGroup = false;
    break;
  default:
    isGroup = false;
  }
  return isGroup;
}

std::string nx::core::HDF5::Support::GetObjectPath(hid_t locationId)
{
  H5SUPPORT_MUTEX_LOCK()

  size_t nameSize = 1 + H5Iget_name(locationId, nullptr, 0);
  std::vector<char> objectName(nameSize, 0);
  H5Iget_name(locationId, objectName.data(), nameSize);
  std::string objectPath(objectName.data());

  if((objectPath != "/") && (objectPath.at(0) == '/'))
  {
    objectPath.erase(0, 1);
  }

  return objectPath;
}

hid_t nx::core::HDF5::Support::GetDatasetType(hid_t locationId, const std::string& datasetName)
{
  H5SUPPORT_MUTEX_LOCK()

  herr_t error = 0;
  herr_t returnError = 0;
  hid_t datasetId = -1;
  /* Open the dataset. */
  if((datasetId = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT)) < 0)
  {
    return -1;
  }
  /* Get an identifier for the datatype. */
  hid_t typeId = H5Dget_type(datasetId);
  H5_CLOSE_H5_DATASET(datasetId, error, returnError, datasetName);
  if(returnError < 0)
  {
    return static_cast<hid_t>(returnError);
  }
  return typeId;
}

herr_t nx::core::HDF5::Support::FindAttr(hid_t /*locationID*/, const char* name, const H5A_info_t* /*info*/, void* opData)
{
  /* Define a default zero value for return. This will cause the iterator to
   * continue if the palette attribute is not found yet.
   */
  int32 returnError = 0;

  char* attributeName = reinterpret_cast<char*>(opData);

  /* Define a positive value for return value if the attribute was found. This
   * will cause the iterator to immediately return that positive value,
   * indicating short-circuit success
   */
  if(std::strcmp(name, attributeName) == 0)
  {
    returnError = 1;
  }

  return returnError;
}

herr_t nx::core::HDF5::Support::FindAttribute(hid_t locationId, const std::string& attributeName)
{
  hsize_t attributeNum;
  herr_t returnError = 0;

  attributeNum = 0;
  returnError = H5Aiterate(locationId, H5_INDEX_NAME, H5_ITER_INC, &attributeNum, FindAttr, const_cast<char*>(attributeName.c_str()));

  return returnError;
}

std::string nx::core::HDF5::Support::HdfClassTypeAsStr(hid_t classType)
{
  switch(classType)
  {
  case H5T_INTEGER:
    return "H5T_INTEGER";
    break;
  case H5T_FLOAT:
    return "H5T_FLOAT";
    break;
  case H5T_STRING:
    return "H5T_STRING";
    break;
  case H5T_TIME:
    return "H5T_TIME";
    break;
  case H5T_BITFIELD:
    return "H5T_BITFIELD";
    break;
  case H5T_OPAQUE:
    return "H5T_OPAQUE";
    break;
  case H5T_COMPOUND:
    return "H5T_COMPOUND";
    break;
  case H5T_REFERENCE:
    return "H5T_REFERENCE";
    break;
  case H5T_ENUM:
    return "H5T_ENUM";
    break;
  case H5T_VLEN:
    return "H5T_VLEN";
    break;
  case H5T_ARRAY:
    return "H5T_ARRAY";
    break;
  default:
    return "OTHER";
  }
}

std::string nx::core::HDF5::Support::StringForHDFType(hid_t dataTypeIdentifier)
{
  H5SUPPORT_MUTEX_LOCK()

  if(dataTypeIdentifier == H5T_STRING)
  {
    return "H5T_STRING";
  }

  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT8) > 0)
  {
    return "H5T_NATIVE_INT8";
  }
  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT8) > 0)
  {
    return "H5T_NATIVE_UINT8";
  }

  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT16) > 0)
  {
    return "H5T_NATIVE_INT16";
  }
  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT16) > 0)
  {
    return "H5T_NATIVE_UINT16";
  }

  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT32) > 0)
  {
    return "H5T_NATIVE_INT32";
  }
  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT32) > 0)
  {
    return "H5T_NATIVE_UINT32";
  }

  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT64) > 0)
  {
    return "H5T_NATIVE_INT64";
  }
  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT64) > 0)
  {
    return "H5T_NATIVE_UINT64";
  }

  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_FLOAT) > 0)
  {
    return "H5T_NATIVE_FLOAT";
  }
  if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_DOUBLE) > 0)
  {
    return "H5T_NATIVE_DOUBLE";
  }

  std::cout << "Error: StringForHDFType - Match for numeric type not found: " << dataTypeIdentifier << std::endl;
  return "Unknown";
}

std::string nx::core::HDF5::Support::GetNameFromFilterType(H5Z_filter_t id)
{
  switch(id)
  {
  case H5Z_FILTER_DEFLATE:
    return "GZIP";
  case H5Z_FILTER_SHUFFLE:
    return "SHUFFLE";
  case H5Z_FILTER_FLETCHER32:
    return "FLETCHER32";
  case H5Z_FILTER_SZIP:
    return "SZIP";
  case H5Z_FILTER_NBIT:
    return "N-BIT";
  case H5Z_FILTER_SCALEOFFSET:
    return "SCALE-OFFSET";
  case H5Z_FILTER_ERROR:
  case H5Z_FILTER_NONE:
    return "NONE";
  default:
    return "UNKNOWN";
  }
}

#if 0
hid_t Support::getDatasetType(hid_t locationID, const std::string& datasetName)
{
  herr_t error = 0;
  herr_t returnError = 0;
  hid_t datasetID = -1;
  /* Open the dataset. */
  if((datasetID = H5Dopen(locationID, datasetName.c_str(), H5P_DEFAULT)) < 0)
  {
    return -1;
  }
  /* Get an identifier for the datatype. */
  hid_t typeID = H5Dget_type(datasetID);
  H5_CLOSE_H5_DATASET(datasetID, error, returnError, datasetName);
  if(returnError < 0)
  {
    return static_cast<hid_t>(returnError);
  }
  return typeID;
}
#endif
