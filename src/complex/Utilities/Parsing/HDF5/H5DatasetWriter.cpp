#include "H5DatasetWriter.hpp"

#include <iostream>

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::DatasetWriter::DatasetWriter()
: ObjectWriter()
{
}

H5::DatasetWriter::DatasetWriter(H5::IdType parentId, const std::string& datasetName)
: ObjectWriter(parentId)
, m_DatasetName(datasetName)
{
#if 0
  if(!tryOpeningDataset(datasetName, dataType))
  {
    tryCreatingDataset(datasetName, dataType);
  }
#endif
}

H5::DatasetWriter::~DatasetWriter()
{
  closeHdf5();
}

#if 0
bool H5::DatasetWriter::tryOpeningDataset(const std::string& datasetName, H5::Type dataType)
{
  m_DatasetId = H5Dopen(getParentId(), datasetName.c_str(), H5P_DEFAULT);
  if(m_DatasetId <= 0)
  {
    return false;
  }

  // Check type
  if(getDataObjectType() != dataType)
  {
    closeHdf5();
    return false;
  }

  // Check dimensions
  {
    hid_t dataspaceId = H5Dget_space(m_DatasetId);
    int32_t rank = H5Sget_simple_extent_ndims(dataspaceId);
    if(rank != getRank())
    {
      closeHdf5();
      return false;
    }
    auto dimensions = getDims();
    hsize_t* dims;
    hsize_t* maxDims;
    bool dimsMatch = true;
    H5Sget_simple_extent_dims(dataspaceId, dims, maxDims);
    for(size_t i = 0; i < rank; i++)
    {
      if(dimensions[i] != dims[i])
      {
        dimsMatch = false;
      }
    }
    delete[] dims;
    delete[] maxDims;
    if(!dimsMatch)
    {
      return false;
    }
  } // end dimension check

  return true;
}

bool H5::DatasetWriter::tryCreatingDataset(const std::string& datasetName, H5::Type dataType)
{
  hid_t h5DataType = H5::getIdForType(dataType);
  hid_t dataspaceId = H5Screate_simple(getRank(), getDims().data(), nullptr);
  m_DatasetId = H5Dcreate(getParentId(), datasetName.c_str(), h5DataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  return m_DatasetId > 0;
}
#endif

void H5::DatasetWriter::closeHdf5()
{
  if(m_DatasetId > 0)
  {
    H5Dclose(m_DatasetId);
    m_DatasetId = 0;
  }
}

H5::ErrorType H5::DatasetWriter::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(getParentId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, H5::Support::FindAttr, const_cast<char*>(getName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(getParentId(), getName().c_str());
    if(error < 0)
    {
      std::cout << "Error Deleting Attribute '" << getName() << "' from Object '" << getParentName() << "'" << std::endl;
      return error;
    }
  }
  return 0;
}

void H5::DatasetWriter::createOrOpenDataset(H5::IdType typeId, H5::IdType dataspaceId)
{
  HDF_ERROR_HANDLER_OFF
  m_DatasetId = H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT);
  HDF_ERROR_HANDLER_ON
  if(m_DatasetId < 0) // dataset does not exist so create it
  {
    m_DatasetId = H5Dcreate(getParentId(), getName().c_str(), typeId, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
}

bool H5::DatasetWriter::isValid() const
{
  return (getParentId() > 0) && (m_DatasetName.empty() == false);
}

std::string H5::DatasetWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  return m_DatasetName;
}

H5::IdType H5::DatasetWriter::getId() const
{
  return m_DatasetId;
}

H5::ErrorType H5::DatasetWriter::writeString(const std::string& text)
{
  if(!isValid())
  {
    return -1;
  }

  closeHdf5();

  herr_t error = 0;
  herr_t returnError = 0;

  /* create a string data type */
  hid_t typeId;
  if((typeId = H5Tcopy(H5T_C_S1)) >= 0)
  {
    size_t size = text.size() + 1;
    if(H5Tset_size(typeId, size) >= 0)
    {
      if(H5Tset_strpad(typeId, H5T_STR_NULLTERM) >= 0)
      {
        /* Create the data space for the dataset. */
        hid_t dataspaceId;
        if((dataspaceId = H5Screate(H5S_SCALAR)) >= 0)
        {
          /* Create or open the dataset. */
          createOrOpenDataset(typeId, dataspaceId);
          if(m_DatasetId >= 0)
          {
            if(!text.empty())
            {
              error = H5Dwrite(m_DatasetId, typeId, H5S_ALL, H5S_ALL, H5P_DEFAULT, text.c_str());
              if(error < 0)
              {
                std::cout << "Error Writing String Data" << std::endl;
                returnError = error;
              }
            }
          }
          else
          {
            returnError = 0;
          }
          // H5_CLOSE_H5_DATASET(m_DatasetId, error, returnError, getName())
        }
        H5S_CLOSE_H5_DATASPACE(dataspaceId, error, returnError)
      }
    }
    H5S_CLOSE_H5_TYPE(typeId, error, returnError)
  }
  return returnError;
}

H5::ErrorType H5::DatasetWriter::writeVectorOfStrings(std::vector<std::string>& text)
{
  if(!isValid())
  {
    return -1;
  }

  closeHdf5();

  herr_t error = 0;
  herr_t returnError = 0;

  m_DatasetId = H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT);
  if(m_DatasetId < 0)
  {
    std::cout << "H5Lite.cpp::readVectorOfStringDataset(" << __LINE__ << ") Error opening Dataset at locationID (" << getParentId() << ") with object name (" << getParentName() << ")" << std::endl;
    return -1;
  }
  /*
   * Get the datatype.
   */
  hid_t typeID = H5Dget_type(getId());
  if(typeID >= 0)
  {
    hsize_t dims[1] = {0};
    /*
     * Get dataspace and allocate memory for read buffer.
     */
    hid_t dataspaceID = H5Dget_space(getId());
    int nDims = H5Sget_simple_extent_dims(dataspaceID, dims, nullptr);
    if(nDims != 1)
    {
      H5S_CLOSE_H5_DATASPACE(dataspaceID, error, returnError);
      H5S_CLOSE_H5_TYPE(typeID, error, returnError);
      closeHdf5();
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
    status = H5Dread(getId(), memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rData.data());
    if(status < 0)
    {
      status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
      H5S_CLOSE_H5_DATASPACE(dataspaceID, error, returnError);
      H5S_CLOSE_H5_TYPE(typeID, error, returnError);
      H5S_CLOSE_H5_TYPE(memtype, error, returnError);
      closeHdf5();
      std::cout << "H5Lite.cpp::readVectorOfStringDataset(" << __LINE__ << ") Error reading Dataset at locationID (" << getParentId() << ") with object name (" << getName() << ")" << std::endl;
      return -3;
    }
    /*
     * copy the data into the vector of strings
     */
    text.resize(dims[0]);
    for(size_t i = 0; i < dims[0]; i++)
    {
      // printf("%s[%d]: %s\n", "VlenStrings", i, rData[i].p);
      text[i] = std::string(rData[i]);
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

  return returnError;
}

#if 0
template <typename T>
H5::ErrorType H5::DatasetWriter::writeVector(const DimsType& dims, const std::vector<T>& values)
{
  herr_t returnError = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    std::cout << "dataType was unknown" << std::endl;
    return -1;
  }

  /* Get the type of object */
  H5O_info_t objectInfo;
  if(H5Oget_info_by_name(getParentId(), getName().c_str(), &objectInfo, H5P_DEFAULT) < 0)
  {
    std::cout << "Error getting object info at locationId (" << getParentId() << ") with object name (" << getName() << ")" << std::endl;
    return -1;
  }
  /* Open the object */
  m_DatasetId = H5::Support::OpenId(getParentId(), getName(), objectInfo.type);
  if(m_DatasetId < 0)
  {
    std::cout << "Error opening Object for Attribute operations." << std::endl;
    return -1;
  }

  hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
  if(dataspaceId >= 0)
  {
    herr_t error = findAndDeleteAttribute();

    if(error >= 0)
    {
      /* Create the attribute. */
      hid_t attributeId = H5Acreate(getId(), getName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
      if(attributeId >= 0)
      {
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());
        error = H5Awrite(attributeId, dataType, data);
        if(error < 0)
        {
          std::cout << "Error Writing Attribute" << std::endl;
          returnError = error;
        }
      }
      /* Close the attribute. */
      error = H5Aclose(attributeId);
      if(error < 0)
      {
        std::cout << "Error Closing Attribute" << std::endl;
        returnError = error;
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceId);
    if(error < 0)
    {
      std::cout << "Error Closing Dataspace" << std::endl;
      returnError = error;
    }
  }
  else
  {
    returnError = static_cast<herr_t>(dataspaceId);
  }
  return returnError;
}

// Declare writeVector
template H5::ErrorType H5::DatasetWriter::writeVector<int8_t>(const DimsType& dims, const std::vector<int8_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<int16_t>(const DimsType& dims, const std::vector<int16_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<int32_t>(const DimsType& dims, const std::vector<int32_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<int64_t>(const DimsType& dims, const std::vector<int64_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<uint8_t>(const DimsType& dims, const std::vector<uint8_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<uint16_t>(const DimsType& dims, const std::vector<uint16_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<uint32_t>(const DimsType& dims, const std::vector<uint32_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<uint64_t>(const DimsType& dims, const std::vector<uint64_t>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<float>(const DimsType& dims, const std::vector<float>& values);
template H5::ErrorType H5::DatasetWriter::writeVector<double>(const DimsType& dims, const std::vector<double>& values);
#endif
