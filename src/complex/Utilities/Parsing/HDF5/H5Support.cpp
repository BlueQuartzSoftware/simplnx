#include "H5Support.hpp"

#include <cstring>
#include <iostream>

hid_t complex::H5::Support::OpenId(hid_t locationId, const std::string& objectName, H5O_type_t objectType)
{
hid_t H5::Support::openId(hid_t locationID, const std::string& objectName, H5O_type_t objectType)
{
  hid_t objectID = -1;

  switch(objectType)
  {
  case H5O_TYPE_DATASET:
    /* Open the dataset. */
    if((objectID = H5Dopen(locationId, objectName.c_str(), H5P_DEFAULT)) < 0)
    {
      return -1;
    }
    break;
  case H5O_TYPE_GROUP:
    /* Open the group. */
    if((objectID = H5Gopen(locationId, objectName.c_str(), H5P_DEFAULT)) < 0)
    {
      return -1;
    }
    break;
  default:
    return -1;
  }

  return objectID;
}

herr_t complex::H5::Support::CloseId(hid_t objectId, int32_t objectType)
{
  switch(objectType)
  {
  case H5O_TYPE_DATASET:
    if(H5Dclose(objectId) < 0)
    {
      return -1;
    }
    break;
  case H5O_TYPE_GROUP:
    if(H5Gclose(objectId) < 0)
    {
      return -1;
    }
    break;
  default:
    return -1;
  }

  return 1;
}

bool complex::H5::Support::DatasetExists(hid_t locationId, const std::string& datasetName)
{
  H5O_info_t objectInfo{};
  HDF_ERROR_HANDLER_OFF
  herr_t error = H5Oget_info_by_name(locationId, datasetName.c_str(), &objectInfo, H5P_DEFAULT);
  HDF_ERROR_HANDLER_ON
  return error >= 0;
}

herr_t complex::H5::Support::GetDatasetInfo(hid_t locationId, const std::string& datasetName, std::vector<hsize_t>& dims, H5T_class_t& classType, usize& sizeType)
{
  hid_t datasetID;
  herr_t error = 0;
  herr_t returnError = 0;

  /* Open the dataset. */
  if((datasetID = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT)) < 0)
  {
    return -1;
  }

  /* Get an identifier for the datatype. */
  hid_t typeID = H5Dget_type(datasetID);
  if(typeID >= 0)
  {
    /* Get the class. */
    classType = H5Tget_class(typeID);
    /* Get the getSize. */
    sizeType = H5Tget_size(typeID);
    /* Release the datatype. */
    error = H5Tclose(typeID);
    if(error < 0)
    {
      std::cout << "Error Closing H5Type" << std::endl;
      returnError = error;
    }
  }
  /* Get the dataspace handle */
  hid_t dataspaceID = H5Dget_space(datasetID);
  if(dataspaceID >= 0)
  {
    /* Get the Number of Dimensions */
    hid_t rank = H5Sget_simple_extent_ndims(dataspaceID);
    if(rank > 0)
    {
      std::vector<hsize_t> _dims(rank, 0);
      /* Get dimensions */
      error = H5Sget_simple_extent_dims(dataspaceID, _dims.data(), nullptr);
      if(error < 0)
      {
        std::cout << "Error Getting Simple Extents for dataset" << std::endl;
        returnError = error;
      }
      // Copy the dimensions into the dims vector
      dims.clear(); // Erase everything in the Vector
      std::copy(_dims.cbegin(), _dims.cend(), std::back_inserter(dims));
    }
    else if(classType == H5T_STRING)
    {
      dims.clear(); // Erase everything in the Vector
      dims.push_back(sizeType);
    }
    /* Terminate access to the dataspace */
    H5S_CLOSE_H5_DATASPACE(dataspaceID, error, returnError);
  }

  /* End access to the dataset */
  H5_CLOSE_H5_DATASET(datasetID, error, returnError, datasetName);
  return returnError;
}

herr_t complex::H5::Support::FindAttr(hid_t /*locationID*/, const char* name, const H5A_info_t* /*info*/, void* opData)
{
  /* Define a default zero value for return. This will cause the iterator to continue if
   * the palette attribute is not found yet.
   */
  int32 returnError = 0;

  char* attributeName = reinterpret_cast<char*>(opData);

  /* Define a positive value for return value if the attribute was found. This will
   * cause the iterator to immediately return that positive value,
   * indicating short-circuit success
   */
  if(std::strcmp(name, attributeName) == 0)
  {
    returnError = 1;
  }

  return returnError;
}

herr_t complex::H5::Support::FindAttribute(hid_t locationId, const std::string& attributeName)
{
  hsize_t attributeNum;
  herr_t returnError = 0;

  attributeNum = 0;
  returnError = H5Aiterate(locationId, H5_INDEX_NAME, H5_ITER_INC, &attributeNum, FindAttr, const_cast<char*>(attributeName.c_str()));

  return returnError;
}

std::string complex::H5::Support::HdfClassTypeAsStr(hid_t classType)
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
hid_t H5::Support::getDatasetType(hid_t locationID, const std::string& datasetName)
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

herr_t complex::H5::Support::ReadVectorOfStringDataset(hid_t locationId, const std::string& datasetName, std::vector<std::string>& data)
{
  herr_t error = 0;
  herr_t returnError = 0;

  hid_t datasetID = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << "H5Lite.cpp::ReadVectorOfStringDataset(" << __LINE__ << ") Error opening Dataset at locationID (" << locationId << ") with object name (" << datasetName << ")" << std::endl;
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
      H5S_CLOSE_H5_DATASPACE(dataspaceID, error, returnError);
      H5S_CLOSE_H5_TYPE(typeID, error, returnError);
      H5_CLOSE_H5_DATASET(datasetID, error, returnError, datasetName);
      std::cout << "H5Lite.cpp::ReadVectorOfStringDataset(" << __LINE__ << ") Number of dims should be 1 but it was " << nDims << ". Returning early. Is your data file correct?" << std::endl;
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
      H5S_CLOSE_H5_DATASPACE(dataspaceID, error, returnError);
      H5S_CLOSE_H5_TYPE(typeID, error, returnError);
      H5S_CLOSE_H5_TYPE(memtype, error, returnError);
      H5_CLOSE_H5_DATASET(datasetID, error, returnError, datasetName);
      std::cout << "H5Lite.cpp::ReadVectorOfStringDataset(" << __LINE__ << ") Error reading Dataset at locationID (" << locationId << ") with object name (" << datasetName << ")" << std::endl;
      return -3;
    }
    /*
     * copy the data into the vector of strings
     */
    data.resize(dims[0]);
    for(usize i = 0; i < dims[0]; i++)
    {
      // printf("%s[%d]: %s\n", "VlenStrings", i, rData[i].p);
      data[i] = std::string(rData[i]);
    }
    /*
     * Close and release resources.  Note that H5Dvlen_reclaim works
     * for variable-length strings as well as variable-length arrays.
     * Also note that we must still free the array of pointers stored
     * in rData, as H5Tvlen_reclaim only frees the data these point to.
     */
    status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
    H5S_CLOSE_H5_DATASPACE(dataspaceID, error, returnError);
    H5S_CLOSE_H5_TYPE(typeID, error, returnError);
    H5S_CLOSE_H5_TYPE(memtype, error, returnError);
  }

  H5_CLOSE_H5_DATASET(datasetID, error, returnError, datasetName);

  return returnError;
}

herr_t complex::H5::Support::ReadStringDataset(hid_t locationId, const std::string& datasetName, std::string& data)
{
  herr_t error = 0;
  herr_t returnError = 0;
  data.clear();
  hid_t datasetID = H5Dopen(locationId, datasetName.c_str(), H5P_DEFAULT);
  if(datasetID < 0)
  {
    std::cout << "H5Lite.cpp::ReadStringDataset(" << __LINE__ << ") Error opening Dataset at locationID (" << locationId << ") with object name (" << datasetName << ")" << std::endl;
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
      error = ReadVectorOfStringDataset(locationId, datasetName, strings); // Read the string
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
  H5_CLOSE_H5_DATASET(datasetID, error, returnError, datasetName);
  H5S_CLOSE_H5_TYPE(typeID, error, returnError);
  return returnError;
}
