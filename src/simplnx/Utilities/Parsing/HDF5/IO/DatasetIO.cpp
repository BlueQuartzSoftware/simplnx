#include "DatasetIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "fmt/format.h"

#include <H5Apublic.h>

#include <iostream>
#include <numeric>

namespace nx::core::HDF5
{
DatasetIO::DatasetIO()
{
}

DatasetIO::DatasetIO(IdType parentId, const std::string& datasetName)
: ObjectIO(parentId)
, m_DatasetName(datasetName)
{
  // open();
}

DatasetIO::DatasetIO(DatasetIO&& other) noexcept
: ObjectIO(other)
, m_DatasetName(std::move(other.m_DatasetName))
{
}

DatasetIO::~DatasetIO()
{
  closeHdf5();
}

void DatasetIO::closeHdf5()
{
  if(isValid())
  {
    H5Dclose(getId());
    setId(0);
  }
}

DatasetIO& DatasetIO::operator=(DatasetIO&& rhs) noexcept
{
  setParentId(rhs.getParentId());
  setId(rhs.getId());
  m_DatasetName = std::move(rhs.m_DatasetName);

  rhs.clear();

  return *this;
}

bool DatasetIO::isValid() const
{
  return (getParentId() > 0) && (m_DatasetName.empty() == false);
}

std::string DatasetIO::getName() const
{
  if(!isValid())
  {
    return "";
  }

  return m_DatasetName;
}

bool DatasetIO::open()
{
  if(!isValid())
  {
    return false;
  }

#if 0
  H5O_info_t objectInfo{};
  auto error = H5Oget_info_by_name(getParentId(), getName().c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return false;
  }
#endif

  setId(H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT));
  return getId() > 0;
}

Result<> DatasetIO::findAndDeleteAttribute()
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

void DatasetIO::createOrOpenDataset(IdType typeId, IdType dataspaceId, IdType propertiesId)
{
  if(getId() > 0)
  {
    return;
  }

  HDF_ERROR_HANDLER_OFF
  setId(H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT));
  HDF_ERROR_HANDLER_ON
  if(getId() < 0) // dataset does not exist so create it
  {
    setId(H5Dcreate(getParentId(), getName().c_str(), typeId, dataspaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT));
  }
}

void DatasetIO::createOrOpenDatasetChunk(IdType typeId, IdType dataspaceId, const DimsType& chunkDims)
{
  auto propertiesId = CreateDatasetChunkProperties(chunkDims);
  createOrOpenDataset(typeId, dataspaceId, propertiesId);
}

IdType DatasetIO::getDataspaceId() const
{
  return H5Dget_space(getId());
}

IdType DatasetIO::getPListId() const
{
  return H5Dget_create_plist(getId());
}

Type DatasetIO::getType() const
{
  auto typeId = getTypeId();
  return getTypeFromId(typeId);
}

IdType DatasetIO::getClassType() const
{
  auto typeId = getTypeId();
  return H5Tget_class(typeId);
}

Result<Type> DatasetIO::getDataType() const
{
  switch(getType())
  {
  case Type::float32:
    return {Type::float32};
  case Type::float64:
    return {Type::float64};
  case Type::int8:
    return {Type::int8};
  case Type::int16:
    return {Type::int16};
  case Type::int32:
    return {Type::int32};
  case Type::int64:
    return {Type::int64};
  case Type::uint8:
    return {Type::uint8};
  case Type::uint16:
    return {Type::uint16};
  case Type::uint32:
    return {Type::uint32};
  case Type::uint64:
    return {Type::uint64};
  default:
    break;
  }
  return {nonstd::make_unexpected(std::vector<Error>{Error{-20012, "The selected datatset is not a supported type for "
                                                                   "importing. Please select a different data set"}})};
}

IdType DatasetIO::getTypeId() const
{
  auto identifier = getId();
  return H5Dget_type(identifier);
}

size_t DatasetIO::getTypeSize() const
{
  return H5Tget_size(getTypeId());
}

size_t DatasetIO::getNumElements() const
{
  std::vector<hsize_t> dims = getDimensions();
  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
  return numElements;
}

size_t DatasetIO::getNumChunkElements() const
{
  std::vector<hsize_t> dims = getChunkDimensions();
  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
  return numElements;
}

std::string DatasetIO::readAsString() const
{
  if(!isValid())
  {
    return "";
  }

  std::string data;

  // Test if the string is variable length
  const hid_t typeID = H5Dget_type(getId());
  const htri_t isVariableString = H5Tis_variable_str(typeID);

  if(isVariableString == 1)
  {
    std::vector<std::string> strings;
    auto stringVec = readAsVectorOfStrings();
    if(stringVec.size() > 1 && !stringVec.empty())
    {
      std::cout << "Error Reading string dataset. There were multiple strings "
                   "and the program asked for a single string."
                << std::endl;
      return "";
    }
    else
    {
      data.assign(strings[0]);
    }
  }
  else
  {
    hsize_t size = H5Dget_storage_size(getId());
    std::vector<char> buffer(static_cast<size_t>(size + 1),
                             0x00); // Allocate and Zero and array
    auto error = H5Dread(getId(), typeID, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
    if(error < 0)
    {
      std::cout << "Error Reading string dataset." << std::endl;
      return "";
    }
    else
    {
      data.append(buffer.data()); // Append the string to the given string
    }
  }

  return data;
}

std::vector<std::string> DatasetIO::readAsVectorOfStrings() const
{
  if(!isValid())
  {
    return {};
  }

  std::vector<std::string> strings;

  hid_t typeID = getTypeId();
  if(typeID >= 0)
  {
    hsize_t dims[1] = {0};
    /*
     * Get dataspace and allocate memory for read buffer.
     */
    hid_t dataspaceID = getDataspaceId();
    int nDims = H5Sget_simple_extent_dims(dataspaceID, dims, nullptr);
    if(nDims != 1)
    {
      // H5Sclose(dataspaceID);
      // H5Tclose(typeID);
      std::cout << "H5DatasetIO.cpp::readVectorOfStrings(" << __LINE__ << ") Number of dims should be 1 but it was " << nDims << ". Returning early. Is your data file correct?" << std::endl;
      return {};
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
      // H5Sclose(dataspaceID);
      // H5Tclose(typeID);
      H5Tclose(memtype);
      std::cout << "H5DatasetIO.cpp::readVectorOfStrings(" << __LINE__ << ") Error reading Dataset at locationID (" << getParentId() << ") with object name (" << getName() << ")" << std::endl;
      return {};
    }
    /*
     * copy the data into the vector of strings
     */
    strings.resize(dims[0]);
    for(size_t i = 0; i < dims[0]; i++)
    {
      // printf("%s[%d]: %s\n", "VlenStrings", i, rData[i].p);
      strings[i] = std::string(rData[i]);
    }
    /*
     * Close and release resources.  Note that H5Dvlen_reclaim works
     * for variable-length strings as well as variable-length arrays.
     * Also note that we must still free the array of pointers stored
     * in rData, as H5Tvlen_reclaim only frees the data these point to.
     */
    status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
  }

  return strings;
}

template <typename T>
std::vector<T> DatasetIO::readAsVector() const
{
  if(!isValid())
  {
    return {};
  }

  size_t numElements = getNumElements();

  std::vector<T> data(numElements);
  nonstd::span<T> span(data.data(), data.size());
  if(!readIntoSpan<T>(span))
  {
    return {};
  }
  return data;
}

template <class T>
bool DatasetIO::readIntoSpan(nonstd::span<T>& data) const
{
  if(!isValid())
  {
    return false;
  }

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return false;
  }

  auto spaceId = getDataspaceId();
  if(spaceId > 0)
  {
    int32_t rank = H5Sget_simple_extent_ndims(spaceId);
    if(rank > 0)
    {
      hsize_t numElements = getNumElements();
      if(numElements != data.size())
      {
        return false;
      }
      herr_t error = H5Dread(getId(), dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
      if(error < 0)
      {
        std::cout << "Error Reading Data.'" << getName() << "'" << std::endl;
        return false;
      }
    }
  }
  else
  {
    std::cout << "Error Opening SpaceID" << std::endl;
    return false;
  }

  return true;
}

template <class T>
bool DatasetIO::readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const hsize_t> chunkOffset) const
{
  if(!isValid())
  {
    return false;
  }

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return false;
  }

  auto spaceId = getDataspaceId();
  if(spaceId > 0)
  {
    int32_t rank = H5Sget_simple_extent_ndims(spaceId);
    if(rank > 0)
    {
      hsize_t numElements = getNumChunkElements();
      if(numElements != data.size())
      {
        return false;
      }
      void* buffer = reinterpret_cast<void*>(data.data());
      const hsize_t* offset = chunkOffset.data();
      uint32_t filterMask;
      herr_t error = H5Dread_chunk(getId(), H5P_DEFAULT, offset, &filterMask, buffer);
      if(error < 0)
      {
        std::cout << "Error Reading Data.'" << getName() << "'" << std::endl;
        return false;
      }
    }
  }
  else
  {
    std::cout << "Error Opening SpaceID" << std::endl;
    return false;
  }

  return true;
}

std::vector<hsize_t> DatasetIO::getDimensions() const
{
  std::vector<hsize_t> dims;
  auto dataspaceId = getDataspaceId();
  if(dataspaceId >= 0)
  {
    if(getClassType() == H5T_STRING)
    {
      auto typeId = getTypeId();
      size_t typeSize = H5Tget_size(typeId);
      dims = {typeSize};
    }
    else
    {
      size_t rank = H5Sget_simple_extent_ndims(dataspaceId);
      std::vector<hsize_t> hdims(rank, 0);
      /* Get dimensions */
      auto error = H5Sget_simple_extent_dims(dataspaceId, hdims.data(), nullptr);
      if(error < 0)
      {
        std::cout << "Error Getting Attribute dims" << std::endl;
        return dims;
      }
      // Copy the dimensions into the dims vector
      dims.clear(); // Erase everything in the Vector
      dims.resize(rank);
      std::copy(hdims.cbegin(), hdims.cend(), dims.begin());
    }
  }
  return dims;
}

IdType DatasetIO::CreateDatasetChunkProperties(const DimsType& dims)
{
  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  auto cparms = H5Pcreate(H5P_DATASET_CREATE);
  auto status = H5Pset_chunk(cparms, hDims.size(), hDims.data());
  if(status < 0)
  {
    return H5P_DEFAULT;
  }
  return cparms;
}

IdType DatasetIO::CreateTransferChunkProperties(const DimsType& chunkDims)
{
  auto cparms = H5Pcreate(H5P_DATASET_XFER);
  return cparms;
}

std::vector<hsize_t> DatasetIO::getChunkDimensions() const
{
  auto plist = getPListId();
  int32_t ndims = 10;
  std::vector<hsize_t> dims(ndims);
  int32_t newSize = H5Pget_chunk(plist, ndims, dims.data());

  dims.resize(newSize);
  return dims;
}

template <typename T>
Result<> DatasetIO::writeSpan(const DimsType& dims, nonstd::span<const T> values)
{
  Result<> returnError = {};
  herr_t error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-105, "DataType was unknown");
  }

  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
  if(dataspaceId >= 0)
  {
    Result<> result = findAndDeleteAttribute();
    if(result.invalid())
    {
      returnError = MakeErrorResult(result.errors()[0].code, "Error Removing Existing Attribute");
    }
    else
    {
      /* Create the attribute. */
      createOrOpenDataset(dataType, dataspaceId);
      if(getId() >= 0)
      {
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());
        error = H5Dwrite(getId(), dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Data");
        }
      }
      else
      {
        returnError = MakeErrorResult(getId(), "Error Creating Dataset");
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceId);
    if(error < 0)
    {
      returnError = MakeErrorResult(error, "Error Closing Dataspace");
    }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

template <typename T>
Result<> DatasetIO::writeChunk(const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkShape, nonstd::span<const hsize_t> offset)
{
  Result<> returnError = {};
  herr_t error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-100, "DataType was unknown");
  }

  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
  if(dataspaceId >= 0)
  {
    Result<> result = findAndDeleteAttribute();
    if(result.invalid())
    {
      std::cout << "Error Removing Existing Attribute" << std::endl;
      returnError = MakeErrorResult(result.errors()[0].code, "Error Removing Existing Attribute");
    }
    else
    {
      /* Create the attribute. */
      createOrOpenDatasetChunk(dataType, dataspaceId, chunkShape);
      if(getId() >= 0)
      {
        auto plistId = getPListId();
        if(plistId <= 0)
        {
          std::cout << "Error Writing Chunk: No PList ID found" << std::endl;
        }
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());
        size_t size = values.size() * sizeof(T);
        // auto properties = CreateTransferChunkProperties(chunkShape);
        error = H5Dwrite_chunk(getId(), H5P_DEFAULT, H5P_DEFAULT, offset.data(), size, data);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Attribute");
        }
      }
      else
      {
        returnError = MakeErrorResult(getId(), "Error Creating Dataset Chunk");
      }
    }
    /* Close the dataspace. */
    error = H5Sclose(dataspaceId);
    if(error < 0)
    {
      returnError = MakeErrorResult(error, "Error Closing Dataspace");
    }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

Result<> DatasetIO::writeString(const std::string& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
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
        }
        H5S_CLOSE_H5_DATASPACE(dataspaceId, error, returnError)
      }
    }
    H5S_CLOSE_H5_TYPE(typeId, error, returnError)
  }
  return returnError;
}

Result<> DatasetIO::writeVectorOfStrings(std::vector<std::string>& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
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

// declare readAsVector
template SIMPLNX_EXPORT std::vector<int8_t> DatasetIO::readAsVector<int8_t>() const;
template SIMPLNX_EXPORT std::vector<int16_t> DatasetIO::readAsVector<int16_t>() const;
template SIMPLNX_EXPORT std::vector<int32_t> DatasetIO::readAsVector<int32_t>() const;
template SIMPLNX_EXPORT std::vector<int64_t> DatasetIO::readAsVector<int64_t>() const;
template SIMPLNX_EXPORT std::vector<uint8_t> DatasetIO::readAsVector<uint8_t>() const;
template SIMPLNX_EXPORT std::vector<uint16_t> DatasetIO::readAsVector<uint16_t>() const;
template SIMPLNX_EXPORT std::vector<uint32_t> DatasetIO::readAsVector<uint32_t>() const;
template SIMPLNX_EXPORT std::vector<uint64_t> DatasetIO::readAsVector<uint64_t>() const;
#ifdef __APPLE__
template SIMPLNX_EXPORT std::vector<size_t> DatasetIO::readAsVector<size_t>() const;
#endif
template SIMPLNX_EXPORT std::vector<float> DatasetIO::readAsVector<float>() const;
template SIMPLNX_EXPORT std::vector<double> DatasetIO::readAsVector<double>() const;

template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>&) const;
#endif
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<float>(nonstd::span<float>&) const;
template SIMPLNX_EXPORT bool DatasetIO::readIntoSpan<double>(nonstd::span<double>&) const;

template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<int8_t>(nonstd::span<int8_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<int16_t>(nonstd::span<int16_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<int32_t>(nonstd::span<int32_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<int64_t>(nonstd::span<int64_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<uint8_t>(nonstd::span<uint8_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<uint16_t>(nonstd::span<uint16_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<uint32_t>(nonstd::span<uint32_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<uint64_t>(nonstd::span<uint64_t>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<char>(nonstd::span<char>, nonstd::span<const hsize_t>) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<size_t>(nonstd::span<size_t>, nonstd::span<const hsize_t>) const;
#endif
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<float>(nonstd::span<float>, nonstd::span<const hsize_t>) const;
template SIMPLNX_EXPORT bool DatasetIO::readChunkIntoSpan<double>(nonstd::span<double>, nonstd::span<const hsize_t>) const;

template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int8_t>(const DimsType&, nonstd::span<const int8_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int16_t>(const DimsType&, nonstd::span<const int16_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int32_t>(const DimsType&, nonstd::span<const int32_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int64_t>(const DimsType&, nonstd::span<const int64_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint8_t>(const DimsType&, nonstd::span<const uint8_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint16_t>(const DimsType&, nonstd::span<const uint16_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint32_t>(const DimsType&, nonstd::span<const uint32_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint64_t>(const DimsType&, nonstd::span<const uint64_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<float>(const DimsType&, nonstd::span<const float>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<double>(const DimsType&, nonstd::span<const double>);

template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int8_t>(const DimsType&, nonstd::span<const int8_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int16_t>(const DimsType&, nonstd::span<const int16_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int32_t>(const DimsType&, nonstd::span<const int32_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int64_t>(const DimsType&, nonstd::span<const int64_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint8_t>(const DimsType&, nonstd::span<const uint8_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint16_t>(const DimsType&, nonstd::span<const uint16_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint32_t>(const DimsType&, nonstd::span<const uint32_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint64_t>(const DimsType&, nonstd::span<const uint64_t>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<float>(const DimsType&, nonstd::span<const float>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<double>(const DimsType&, nonstd::span<const double>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<bool>(const DimsType&, nonstd::span<const bool>, const DimsType&, nonstd::span<const hsize_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<char>(const DimsType&, nonstd::span<const char>, const DimsType&, nonstd::span<const hsize_t>);
} // namespace nx::core::HDF5
