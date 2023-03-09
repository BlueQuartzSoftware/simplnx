#include "H5Support.hpp"

#include <cstring>
#include <iostream>

herr_t complex::HDF5::Support::FindAttr(hid_t /*locationID*/, const char* name, const H5A_info_t* /*info*/, void* opData)
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

herr_t complex::HDF5::Support::FindAttribute(hid_t locationId, const std::string& attributeName)
{
  hsize_t attributeNum;
  herr_t returnError = 0;

  attributeNum = 0;
  returnError = H5Aiterate(locationId, H5_INDEX_NAME, H5_ITER_INC, &attributeNum, FindAttr, const_cast<char*>(attributeName.c_str()));

  return returnError;
}

std::string complex::HDF5::Support::HdfClassTypeAsStr(hid_t classType)
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
