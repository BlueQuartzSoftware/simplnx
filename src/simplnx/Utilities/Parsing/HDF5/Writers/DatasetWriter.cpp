#include "DatasetWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "fmt/format.h"

#include <array>
#include <iostream>

#include <H5Apublic.h>

namespace nx::core::HDF5
{
DatasetWriter::DatasetWriter() = default;

DatasetWriter::DatasetWriter(IdType parentId, const std::string& datasetName)
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

DatasetWriter::DatasetWriter(DatasetWriter&& other) noexcept
: ObjectWriter(std::move(other))
, m_DatasetName(std::move(other.m_DatasetName))
{
}

DatasetWriter& DatasetWriter::operator=(DatasetWriter&& other) noexcept
{
  setParentId(other.getParentId());
  setId(other.getId());
  m_DatasetName = std::move(other.m_DatasetName);

  other.setId(0);
  other.setParentId(0);

  return *this;
}

DatasetWriter::~DatasetWriter() noexcept
{
  closeHdf5();
}

#if 0
bool DatasetWriter::tryOpeningDataset(const std::string& datasetName, Type dataType)
{
  setId(H5Dopen(getParentId(), datasetName.c_str(), H5P_DEFAULT));
  if(getId() <= 0)
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

bool DatasetWriter::tryCreatingDataset(const std::string& datasetName, Type dataType)
{
  hid_t h5DataType = getIdForType(dataType);
  hid_t dataspaceId = H5Screate_simple(getRank(), getDims().data(), nullptr);
  setId(H5Dcreate(getParentId(), datasetName.c_str(), h5DataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
  return getId() > 0;
}
#endif

void DatasetWriter::closeHdf5()
{
  if(getId() > 0)
  {
    H5Dclose(getId());
    setId(0);
  }
}

Result<> DatasetWriter::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(getParentId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(getParentId(), getName().c_str());
    if(error < 0)
    {
      std::string ss = fmt::format("Error Deleting Attribute '{}' from Object '{}'", getName(), getParentName());
      return MakeErrorResult(error, ss);
    }
  }
  return {};
}

void DatasetWriter::createOrOpenDataset(IdType typeId, IdType dataspaceId, IdType propertiesId)
{
  HDF_ERROR_HANDLER_OFF
  setId(H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT));
  HDF_ERROR_HANDLER_ON
  if(getId() < 0) // dataset does not exist so create it
  {
    setId(H5Dcreate(getParentId(), getName().c_str(), typeId, dataspaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT));
  }
}

IdType DatasetWriter::CreateDatasetChunkProperties(const DimsType& chunkDims)
{
  std::vector<hsize_t> hDims(chunkDims.size());
  std::transform(chunkDims.begin(), chunkDims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  auto cparms = H5Pcreate(H5P_DATASET_CREATE);
  auto status = H5Pset_chunk(cparms, hDims.size(), hDims.data());
  if(status < 0)
  {
    return H5P_DEFAULT;
  }
  return cparms;
}

IdType DatasetWriter::CreateTransferChunkProperties(const DimsType& chunkDims)
{
  auto cparms = H5Pcreate(H5P_DATASET_XFER);
  return cparms;
}

void DatasetWriter::createOrOpenDatasetChunk(IdType typeId, IdType dataspaceId, const DimsType& chunkDims)
{
  auto propertiesId = CreateDatasetChunkProperties(chunkDims);
  createOrOpenDataset(typeId, dataspaceId, propertiesId);
}

IdType DatasetWriter::getPListId() const
{
  return H5Dget_create_plist(getId());
}

bool DatasetWriter::isValid() const
{
  return (getParentId() > 0) && (m_DatasetName.empty() == false);
}

std::string DatasetWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  return m_DatasetName;
}

Result<> DatasetWriter::writeString(const std::string& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid DatasetWriter");
  }

  closeHdf5();

  herr_t error = 0;
  Result<> returnError = {};

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
          if(getId() >= 0)
          {
            if(!text.empty())
            {
              error = H5Dwrite(getId(), typeId, H5S_ALL, H5S_ALL, H5P_DEFAULT, text.c_str());
              if(error < 0)
              {
                returnError = MakeErrorResult(error, "Error Writing String Data");
              }
            }
          }
          else
          {
            returnError = {};
          }
          // H5_CLOSE_H5_DATASET(getId(), error, returnError, getName())
        }
        H5S_CLOSE_H5_DATASPACE(dataspaceId, error, returnError)
      }
    }
    H5S_CLOSE_H5_TYPE(typeId, error, returnError)
  }
  return returnError;
}

Result<> DatasetWriter::writeVectorOfStrings(std::vector<std::string>& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid DatasetWriter");
  }

  hid_t dataspaceID = -1;
  hid_t memSpace = -1;
  hid_t datatype = -1;
  herr_t error = -1;
  Result<> returnError = {};

  std::array<hsize_t, 1> dims = {text.size()};
  if((dataspaceID = H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr)) >= 0)
  {
    dims[0] = 1;

    if((memSpace = H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr)) >= 0)
    {
      datatype = H5Tcopy(H5T_C_S1);
      H5Tset_size(datatype, H5T_VARIABLE);

      setId(H5Dcreate(getParentId(), getName().c_str(), datatype, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
      if(getId() >= 0)
      {
        // Select the "memory" to be written out - just 1 record.
        hsize_t dataset_offset[] = {0};
        hsize_t dataset_count[] = {1};
        H5Sselect_hyperslab(memSpace, H5S_SELECT_SET, dataset_offset, nullptr, dataset_count, nullptr);
        hsize_t pos = 0;
        for(const auto& element : text)
        {
          // Select the file position, 1 record at position 'pos'
          hsize_t element_count[] = {1};
          hsize_t element_offset[] = {pos};
          pos++;
          H5Sselect_hyperslab(dataspaceID, H5S_SELECT_SET, element_offset, nullptr, element_count, nullptr);
          const char* strPtr = element.c_str();
          error = H5Dwrite(getId(), datatype, memSpace, dataspaceID, H5P_DEFAULT, &strPtr);
          if(error < 0)
          {
            std::cout << "Error Writing String Data: " __FILE__ << "(" << __LINE__ << ")" << std::endl;
            returnError = MakeErrorResult(error, "Error Writing String Data");
          }
        }
      }
      H5Tclose(datatype);
    }
  }

  return returnError;
}

} // namespace nx::core::HDF5
