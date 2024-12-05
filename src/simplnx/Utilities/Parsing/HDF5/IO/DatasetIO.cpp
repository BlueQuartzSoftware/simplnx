#include "DatasetIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include "fmt/format.h"

#include "H5Dpublic.h"
#include "H5Spublic.h"
#include "H5Tpublic.h"

#include <array>
#include <iostream>
#include <numeric>
#include <span>
#include <vector>

using namespace nx::core;

namespace
{
std::string GetNameFromFilterType(H5Z_filter_t id)
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
} // namespace

namespace nx::core::HDF5
{
constexpr int64 k_DimensionMismatchError = -5138;

DatasetIO::DatasetIO() = default;

DatasetIO::DatasetIO(hid_t parentId, const std::string& dataName)
: ObjectIO(parentId, dataName)
{
#if 0
  if(!tryOpeningDataset(datasetName, dataType))
  {
    tryCreatingDataset(datasetName, dataType);
  }
#endif
}

DatasetIO::DatasetIO(DatasetIO&& other) noexcept
: ObjectIO(std::move(other))
{
}

DatasetIO::~DatasetIO() noexcept
{
  close();
}

void DatasetIO::close()
{
  if(isOpen())
  {
    H5Dclose(getId());
    setId(0);
  }
}

hid_t DatasetIO::open() const
{
  if(isOpen())
  {
    return getId();
  }
  hid_t id = H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT);
  setId(id);
  return id;
}

hid_t DatasetIO::createOrOpenDataset(IdType typeId, IdType dataspaceId, IdType propertiesId) const
{
  if(isOpen())
  {
    return getId();
  }

  HDF_ERROR_HANDLER_OFF
  setId(H5Dopen(getParentId(), getName().c_str(), H5P_DEFAULT));
  HDF_ERROR_HANDLER_ON
  if(!isOpen()) // dataset does not exist so create it
  {
    setId(H5Dcreate(getParentId(), getName().c_str(), typeId, dataspaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT));
  }

  return getId();
}

// template <typename T>
// HighFive::DataSet createOrOpenDataset(DatasetIO& datasetIO, const HighFive::DataSpace& dims, HighFive::DataType& dataType)
//{
//   std::string name = datasetIO.getName();
//   std::string datapath = datasetIO.getObjectPath();
//   try
//   {
//     auto& parentGroup = datasetIO.parentGroup()->groupRef();
//     if(parentGroup.exist(name))
//     {
//       return parentGroup.getDataSet(name);
//     }
//     else
//     {
//       return parentGroup.createDataSet(name, dims, dataType);
//     }
//   } catch(const std::exception& e)
//   {
//     throw e;
//   }
// }
//
// template <>
// HighFive::DataSet createOrOpenDataset<bool>(DatasetIO& datasetIO, const HighFive::DataSpace& dims)
//{
//   std::string name = datasetIO.getName();
//   std::string datapath = datasetIO.getObjectPath();
//   try
//   {
//     auto& parentGroup = datasetIO.parentGroup()->groupRef();
//     if(parentGroup.exist(name))
//     {
//       return std::move(parentGroup.getDataSet(name));
//     }
//     else
//     {
//       return std::move(parentGroup.createDataSet<H5_BOOL_TYPE>(name, dims));
//     }
//   } catch(const std::exception& e)
//   {
//     throw e;
//   }
// }

DatasetIO& DatasetIO::operator=(DatasetIO&& rhs) noexcept
{
  moveObj(std::move(rhs));
  return *this;
}

#if 0
Result<> DatasetIO::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  //int32_t hasAttribute = H5Aiterate(getParentId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute())
  {
    herr_t error = H5Adelete(getId(), getName().c_str());
    if(error < 0)
    {
      std::string ss = fmt::format("Error Deleting Attribute '{}' from Object '{}'", getName(), getParentName());
      return MakeErrorResult(error, ss);
    }
  }
  return {};
}
#endif

// DataType DatasetIO::getType() const
//{
//   auto dataset = open();
//   return dataset.getDataType();
// }

hid_t DatasetIO::getTypeId() const
{
  auto identifier = getId();
  return H5Dget_type(identifier);
}

hid_t DatasetIO::getClassType() const
{
  auto typeId = getTypeId();
  return H5Tget_class(typeId);
}

size_t DatasetIO::getTypeSize() const
{
  return H5Tget_size(getTypeId());
}

#if 0
Result<nx::core::HDF5::Type> DatasetIO::getDataType() const
{
  auto datasetId = open();
  hid_t typeId = H5Dget_type(datasetId);
  auto type = getTypeFromId(typeId);
  Result<Type> result;
  switch(type)
  {
  case Type::float32:
    result = {Type::float32};
    break;
  case Type::float64:
    result = {Type::float64};
    break;
  case Type::int8:
    result = {Type::int8};
    break;
  case Type::int16:
    result = {Type::int16};
    break;
  case Type::int32:
    result = {Type::int32};
    break;
  case Type::int64:
    result = {Type::int64};
    break;
  case Type::uint8:
    result = {Type::uint8};
    break;
  case Type::uint16:
    result = {Type::uint16};
    break;
  case Type::uint32:
    result = {Type::uint32};
    break;
  case Type::uint64:
    result = {Type::uint64};
    break;
  default:
    result = {nonstd::make_unexpected(std::vector<Error>{Error{-20012, "The selected datatset is not a supported type for "
                                                                       "importing. Please select a different data set"}})};
    break;
  }
  H5Tclose(typeId);
  return result;
}
#else
Result<nx::core::DataType> DatasetIO::getDataType() const
{
  auto datasetId = open();
  hid_t typeId = H5Dget_type(datasetId);
  auto type = getTypeFromId(typeId);
  Result<DataType> result;
  switch(type)
  {
  case Type::float32:
    result = {DataType::float32};
    break;
  case Type::float64:
    result = {DataType::float64};
    break;
  case Type::int8:
    result = {DataType::int8};
    break;
  case Type::int16:
    result = {DataType::int16};
    break;
  case Type::int32:
    result = {DataType::int32};
    break;
  case Type::int64:
    result = {DataType::int64};
    break;
  case Type::uint8:
    result = {DataType::uint8};
    break;
  case Type::uint16:
    result = {DataType::uint16};
    break;
  case Type::uint32:
    result = {DataType::uint32};
    break;
  case Type::uint64:
    result = {DataType::uint64};
    break;
  default:
    result = {nonstd::make_unexpected(std::vector<Error>{Error{-20012, "The selected datatset is not a supported type for "
                                                                       "importing. Please select a different data set"}})};
    break;
  }
  H5Tclose(typeId);
  return result;
}
#endif

size_t DatasetIO::getNumElements() const
{
  if(!exists())
  {
    return 0;
  }
  std::vector<usize> dims = getDimensions();
  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
  return numElements;
}

size_t DatasetIO::getNumChunkElements() const
{
  std::vector<usize> dims = getChunkDimensions();
  usize numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>());
  return numElements;
}

std::string DatasetIO::readAsString() const
{
  if(!isValid())
  {
    return "";
  }
  auto datasetId = open();
  std::string data;

  // Test if the string is variable length
  const hid_t typeID = H5Dget_type(datasetId);
  const htri_t isVariableString = H5Tis_variable_str(typeID);

  if(isVariableString == 1)
  {
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
      data.assign(stringVec[0]);
    }
  }
  else
  {
    hsize_t size = H5Dget_storage_size(datasetId);
    std::vector<char> buffer(static_cast<size_t>(size + 1),
                             0x00); // Allocate and Zero and array
    auto error = H5Dread(datasetId, typeID, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
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

  return std::move(data);
}

std::vector<std::string> DatasetIO::readAsVectorOfStrings() const
{
  if(!isValid())
  {
    return {};
  }
  // auto dataset = openH5Dataset();

  std::vector<std::string> strings;
  auto datasetId = open();

  hid_t typeID = H5Dget_type(datasetId);

  if(typeID >= 0)
  {
    hsize_t dims[1] = {0};
    /*
     * Get dataspace and allocate memory for read buffer.
     */
    hid_t dataspaceID = H5Dget_space(datasetId);
    int nDims = H5Sget_simple_extent_dims(dataspaceID, dims, nullptr);
    if(nDims != 1)
    {
      // H5Sclose(dataspaceID);
      // H5Tclose(typeID);
      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Number of dims should be 1 but it was " << nDims << ". Returning early. Is your data file correct?" << std::endl;
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
    status = H5Dread(datasetId, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rData.data());
    if(status < 0)
    {
      status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
      H5Sclose(dataspaceID);
      H5Tclose(typeID);
      H5Tclose(memtype);
      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Error reading Dataset at locationID (" << getParentId() << ") with object name (" << getName() << ")" << std::endl;
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
  // if(!isValid())
  //{
  //   return {};
  // }

  auto dataset = open();
  size_t numElements = getNumElements();

  std::vector<T> data(numElements);
  nonstd::span<T> span(data.data(), data.size());

  Result<> result = readIntoSpan<T>(span);
  if(result.invalid())
  {
    return {};
  }

  return data;
}

template <class T>
nx::core::Result<> DatasetIO::readIntoSpan(nonstd::span<T>& data) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  hid_t datasetId = open();
  hid_t fileSpaceId = H5Dget_space(datasetId);
  if(fileSpaceId < 0)
  {
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  }

  hsize_t totalElements;
  std::vector<hsize_t> memDims;
  int rank = H5Sget_simple_extent_ndims(fileSpaceId);
  std::vector<hsize_t> dims(rank), maxDims(rank);
  H5Sget_simple_extent_dims(fileSpaceId, dims.data(), maxDims.data());

  memDims = dims;

  totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  hid_t memSpaceId = H5Screate_simple(memDims.size(), memDims.data(), NULL);
  if(memSpaceId < 0)
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }

  if(H5Dread(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, data.data()) < 0)
  {
    H5Sclose(memSpaceId);
    H5Sclose(fileSpaceId);
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getName()));
  }

  H5Sclose(memSpaceId);
  H5Sclose(fileSpaceId);

  return {};
}

template <>
nx::core::Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>& data) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
  }

  std::vector<H5_BOOL_TYPE> data2(data.begin(), data.end());
  nonstd::span<H5_BOOL_TYPE> span2(data2.data(), data2.size());
  auto result = readIntoSpan(span2);
  std::copy(span2.begin(), span2.end(), data.begin());
  return result;
}

template <class T>
Result<> DatasetIO::readIntoSpan(nonstd::span<T>& data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} / {}", getFilePath().string(), getName()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  hid_t datasetId = open();
  hid_t fileSpaceId = H5Dget_space(datasetId);
  if(fileSpaceId < 0)
  {
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  }

  hsize_t totalElements;
  std::vector<hsize_t> memDims;
  int rank = H5Sget_simple_extent_ndims(fileSpaceId);
  std::vector<hsize_t> dims(rank), maxDims(rank);
  H5Sget_simple_extent_dims(fileSpaceId, dims.data(), maxDims.data());
  if(start.has_value() && count.has_value())
  {
    // Both start and count are provided
#if defined(__APPLE__)
    std::vector<unsigned long long> startData(start->begin(), start->end());
    std::vector<unsigned long long> countVec(count->begin(), count->end());
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startData.data(), NULL, countVec.data(), NULL) < 0)
    {
      return MakeErrorResult(-1003, "DatasetReader error: Unable to select hyperslab.");
    }
#else
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1003, "DatasetReader error: Unable to select hyperslab.");
    }
#endif
    memDims = std::vector<hsize_t>(count->begin(), count->end());
  }
  else if(start.has_value())
  {
    // Only start is provided
    std::vector<hsize_t> countRemaining(rank);
    for(int i = 0; i < rank; ++i)
    {
      countRemaining[i] = dims[i] - start->at(i);
    }
#if defined(__APPLE__)
    std::vector<unsigned long long> startData(start->begin(), start->end());
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startData.data(), NULL, countRemaining.data(), NULL) < 0)
    {
      return MakeErrorResult(-1004, "DatasetReader error: Unable to select hyperslab.");
    }
#else
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, countRemaining.data(), NULL) < 0)
    {
      return MakeErrorResult(-1004, "DatasetReader error: Unable to select hyperslab.");
    }
#endif
    memDims = countRemaining;
  }
  else if(count.has_value())
  {
    // Only count is provided
    std::vector<hsize_t> startZeros(rank, 0);
#if defined(__APPLE__)
    std::vector<unsigned long long> countVec(count->begin(), count->end());
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startZeros.data(), NULL, countVec.data(), NULL) < 0)
    {
      return MakeErrorResult(-1005, "DatasetReader error: Unable to select hyperslab.");
    }
#else
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startZeros.data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1005, "DatasetReader error: Unable to select hyperslab.");
    }
#endif
    memDims = std::vector<hsize_t>(count->begin(), count->end());
  }
  else
  {
    // Neither start nor count is provided
    memDims = dims;
  }

  totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  hid_t memSpaceId = H5Screate_simple(memDims.size(), memDims.data(), NULL);
  if(memSpaceId < 0)
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }

  if(H5Dread(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, data.data()) < 0)
  {
    H5Sclose(memSpaceId);
    H5Sclose(fileSpaceId);
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getName()));
  }

  H5Sclose(memSpaceId);
  H5Sclose(fileSpaceId);

  return {};
}

template <>
Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>& data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
  }
  if(start->size() != count->size())
  {
    return MakeErrorResult(-506, fmt::format("Cannot read HDF5 data at {} called {}. Requested dimensions do not match: '{}', '{}'", getFilePath().string(), getName(), start->size(), count->size()));
  }

  std::vector<H5_BOOL_TYPE> data2(data.begin(), data.end());
  nonstd::span<H5_BOOL_TYPE> span2(data2.data(), data2.size());
  auto result = readIntoSpan(span2, start, count);
  std::copy(span2.begin(), span2.end(), data.begin());
  return result;
}

#if 0
template <class T>
Result<> DatasetIO::readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const usize> chunkOffset, nonstd::span<const usize> chunkDims) const
{
  if(!isValid())
  {
    return MakeErrorResult(-1000, "DatasetReader error: DatasetReader object is not valid.");
  }

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  hid_t datasetId = getId();
  hid_t fileSpaceId = H5Dget_space(datasetId);
  if(fileSpaceId < 0)
  {
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  }

  hsize_t totalElements;
  std::vector<hsize_t> memDims;
  int rank = H5Sget_simple_extent_ndims(fileSpaceId);
  std::vector<hsize_t> dims(rank), maxDims(rank);
  H5Sget_simple_extent_dims(fileSpaceId, dims.data(), maxDims.data());
  if(start.has_value() && count.has_value())
  {
    // Both start and count are provided
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1003, "DatasetReader error: Unable to select hyperslab.");
    }
    memDims = count.value();
  }
  else if(start.has_value())
  {
    // Only start is provided
    std::vector<hsize_t> countRemaining(rank);
    for(int i = 0; i < rank; ++i)
    {
      countRemaining[i] = dims[i] - start->at(i);
    }
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, countRemaining.data(), NULL) < 0)
    {
      return MakeErrorResult(-1004, "DatasetReader error: Unable to select hyperslab.");
    }
    memDims = countRemaining;
  }
  else if(count.has_value())
  {
    // Only count is provided
    std::vector<hsize_t> startZeros(rank, 0);
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startZeros.data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1005, "DatasetReader error: Unable to select hyperslab.");
    }
    memDims = count.value();
  }
  else
  {
    // Neither start nor count is provided
    memDims = dims;
  }

  totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  hid_t memSpaceId = H5Screate_simple(memDims.size(), memDims.data(), NULL);
  if(memSpaceId < 0)
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }

  if(H5Dread(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, data.data()) < 0)
  {
    H5Sclose(memSpaceId);
    H5Sclose(fileSpaceId);
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getName()));
  }

  H5Sclose(memSpaceId);
  H5Sclose(fileSpaceId);

  return {};
}
#endif

// template <>
// Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool> data, nonstd::span<const usize> chunkOffset, nonstd::span<const usize> chunkDims) const
//{
//   if(!isValid())
//   {
//     return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
//   }
//
//   // DataSet does not support bool data.
//   std::vector<uint8> data2(data.size());
//   auto dataset = openH5Dataset();
//   dataset.select({chunkOffset.data(), chunkDims.data()}).read(data2);
//   // Copy data back into span<bool>
//   std::copy(data2.begin(), data2.end(), data.begin());
//   return {};
// }

std::vector<nx::core::usize> DatasetIO::getChunkDimensions() const
{
  auto id = open();
  auto propertyListId = H5Dget_create_plist(getId());
  H5D_layout_t layout = H5Pget_layout(propertyListId);
  if(layout == H5D_CHUNKED)
  {
    usize numDims = getDimensions().size();
    std::vector<hsize_t> chunk_dims_out(numDims);
    H5Pget_chunk(propertyListId, numDims, chunk_dims_out.data());
    return std::vector<usize>(chunk_dims_out.begin(), chunk_dims_out.end());
  }
  else
  {
    return {};
  }
}

std::vector<nx::core::usize> DatasetIO::getDimensions() const
{
  std::vector<hsize_t> dims;
  auto dataspaceId = H5Dget_space(getId());

  if(dataspaceId >= 0)
  {
    if(getClassType() == H5T_STRING)
    {
      auto typeId = H5Dget_type(getId());
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
        return std::vector<nx::core::usize>(dims.begin(), dims.end());
      }
      // Copy the dimensions into the dims vector
      dims.clear(); // Erase everything in the Vector
      dims.resize(rank);
      std::copy(hdims.cbegin(), hdims.cend(), dims.begin());
    }
  }
  return std::vector<nx::core::usize>(dims.begin(), dims.end());
}

template <typename T>
Result<> DatasetIO::writeSpan(const DimsType& dims, nonstd::span<const T> values)
{
  Result<> returnError = {};
  ErrorType error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1, "DataType was unknown");
  }

  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);

  if(dataspaceId >= 0)
  {
    // auto result = findAndDeleteAttribute();
    // if(result.invalid())
    //{
    //   returnError = MakeErrorResult(result.errors()[0].code, "Error Removing existing Attribute");
    // }
    // else
    {
      /* Create the attribute. */
      auto datasetId = createOrOpenDataset<T>(dataspaceId);
      if(datasetId >= 0)
      {
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());
        error = H5Dwrite(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Attribute");
        }
      }
      else
      {
        returnError = MakeErrorResult(datasetId, "Error Creating Dataset");
      }
    }
    /* Close the dataspace. */
    // error = H5Sclose(dataspaceId);
    // if(error < 0)
    //{
    //   returnError = MakeErrorResult(error, "Error Closing Dataspace");
    // }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

template <>
Result<> DatasetIO::writeSpan<bool>(const DimsType& dims, nonstd::span<const bool> values)
{
  try
  {
    // DataSet does not support bool data.
    const std::vector<H5_BOOL_TYPE> data2(values.begin(), values.end());
    nonstd::span<const H5_BOOL_TYPE> span2(data2.begin(), data2.size());
    return writeSpan(dims, span2);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-853535, fmt::format("Failed to write to Dataset with error: '{}'", e.what()));
  }
  return {};
}

template <typename T>
nx::core::Result<ChunkedDataInfo> DatasetIO::initChunkedDataset(const DimsType& h5Dims, const DimsType& chunkDims) const
{
  ChunkedDataInfo dataInfo;
  std::vector<unsigned long long> h5DimsVec(h5Dims.begin(), h5Dims.end());
  dataInfo.dataspaceId = H5Screate_simple(h5Dims.size(), h5DimsVec.data(), nullptr);
  if(dataInfo.dataspaceId < 0)
  {
    return MakeErrorResult<ChunkedDataInfo>(-120, "Failed to open HDF5 Dataspace");
  }

  dataInfo.dataType = HdfTypeForPrimitive<T>();
  if(dataInfo.dataType == -1)
  {
    return MakeErrorResult<ChunkedDataInfo>(-100, "DataType was unkown");
  }

  dataInfo.chunkProp = CreateH5DatasetChunkProperties(chunkDims);
  dataInfo.datasetId = createOrOpenDataset(dataInfo.dataType, dataInfo.dataspaceId, dataInfo.chunkProp);
  if(dataInfo.datasetId < 0)
  {
    return MakeErrorResult<ChunkedDataInfo>(-110, "Failed to open HDF5 Dataset");
  }

  dataInfo.transferProp = H5Pcreate(H5P_DATASET_XFER);
  if(dataInfo.transferProp < 0)
  {
    return MakeErrorResult<ChunkedDataInfo>(-130, "Failed to create HDF5 transfer properties");
  }

  setId(dataInfo.datasetId);
  return {dataInfo};
}

hid_t DatasetIO::CreateH5DatasetChunkProperties(const DimsType& chunkDims)
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

nx::core::Result<> DatasetIO::closeChunkedDataset(const ChunkedDataInfo& datasetInfo) const
{
  herr_t error = H5Pclose(datasetInfo.transferProp);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing Transfer Property");
  }
  /*error = H5Dclose(datasetInfo.datasetId);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing DataSet");
  }*/
  // else
  //{
  //   setId(-1);
  // }

  error = H5Pclose(datasetInfo.chunkProp);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing Chunk Property");
  }
  error = H5Sclose(datasetInfo.dataspaceId);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing Dataspace");
  }

  return {};
}

template <typename T>
nx::core::Result<> DatasetIO::readChunk(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<T> values, const DimsType& chunkShape, nonstd::span<const usize> offset) const
{
  if(chunkShape.size() != dims.size())
  {
    std::string ss = fmt::format("Dimension mismatch when writing DataStore chunk. Num Shape Dimensions: {} Num Chunk Dimensions: {}", dims.size(), chunkShape.size());
    return MakeErrorResult(k_DimensionMismatchError, ss);
  }

  Result<> returnError = {};
  herr_t error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = chunkInfo.dataType;
  if(dataType == -1)
  {
    return MakeErrorResult(-100, "DataType was unkown");
  }
  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = chunkInfo.dataspaceId;
  if(dataspaceId >= 0)
  {
    // auto result = findAndDeleteAttribute();
    // if(result.invalid())
    //{
    //   returnError = MakeErrorResult(result.errors()[0].code, "Error Removing Existing Attribute");
    // }
    // else
    {
      /* Create the attribute. */
      auto h5Id = chunkInfo.datasetId;
      if(h5Id >= 0)
      {
        auto plistId = H5Dget_create_plist(h5Id);
        if(plistId <= 0)
        {
          std::cout << "Error Writing Chunk: No PList ID found" << std::endl;
        }
        /* Write the attribute data. */
        void* data = static_cast<void*>(values.data());
        auto properties = CreateH5DatasetChunkProperties(chunkShape);
        // error = H5Dwrite_chunk(h5Id, properties, offset.data(), H5P_DEFAULT, data);
        std::vector<unsigned long long> offsetVec(offset.begin(), offset.end());
        error = H5Dread_chunk(h5Id, H5P_DEFAULT, offsetVec.data(), H5P_DEFAULT, data);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Dataset Chunk");
        }
      }
      else
      {
        returnError = MakeErrorResult(h5Id, "Error Creating Dataset Chunk");
      }
    }
    /* Close the dataspace after reading all required chunks. */
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

template <>
Result<> DatasetIO::readChunk<bool>(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<bool> values, const DimsType& chunkShape, nonstd::span<const usize> offset) const
{
  std::vector<H5_BOOL_TYPE> h5ValuesVec(values.begin(), values.end());
  nonstd::span<H5_BOOL_TYPE> h5Values(h5ValuesVec.data(), h5ValuesVec.size());

  auto result = readChunk(chunkInfo, dims, h5Values, chunkShape, offset);
  if(result.invalid())
  {
    return result;
  }
  std::copy(h5Values.begin(), h5Values.end(), values.begin());
  return {};
}

template <typename T>
Result<> DatasetIO::writeChunk(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkShape, nonstd::span<const usize> offset)
{
  if(chunkShape.size() != dims.size())
  {
    std::string ss = fmt::format("Dimension mismatch when writing DataStore chunk. Num Shape Dimensions: {} Num Chunk Dimensions: {}", dims.size(), chunkShape.size());
    return MakeErrorResult(k_DimensionMismatchError, ss);
  }

  Result<> returnError = {};
  herr_t error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = chunkInfo.dataType;
  if(dataType == -1)
  {
    return MakeErrorResult(-100, "DataType was unkown");
  }
  // std::vector<hsize_t> hDims(chunkShape.size());
  // std::transform(chunkShape.begin(), chunkShape.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  // hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
  hid_t dataspaceId = chunkInfo.dataspaceId;
  if(dataspaceId >= 0)
  {
    /*auto result = findAndDeleteAttribute();
    if(result.invalid())
    {
      returnError = MakeErrorResult(result.errors()[0].code, "Error Removing Existing Attribute");
    }
    else*/
    {
      /* Create the attribute. */
      auto h5Id = chunkInfo.datasetId;
      if(h5Id >= 0)
      {
        // auto plistId = H5Dget_create_plist(h5Id);
        // if(plistId <= 0)
        //{
        //   std::cout << "Error Writing Chunk: No PList ID found" << std::endl;
        // }
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());

// debug
#if 0
        hid_t dspace = H5Dget_space(h5Id);
        const int ndimsDebug = H5Sget_simple_extent_ndims(dspace);
        std::vector<hsize_t> dimsDebug(ndimsDebug);
        H5Sget_simple_extent_dims(dspace, dimsDebug.data(), NULL);
#endif
        // end debug

        std::vector<unsigned long long> offsetVec(offset.begin(), offset.end());
        error = H5Dwrite_chunk(h5Id, chunkInfo.transferProp, H5P_DEFAULT, offsetVec.data(), values.size() * sizeof(T), data);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Dataset Chunk");
        }
      }
      else
      {
        returnError = MakeErrorResult(h5Id, "Error Creating Dataset Chunk");
      }
    }
    // error = H5Sclose(dataspaceId);
    // if(error < 0)
    //{
    //   return MakeErrorResult(error, "Error Closing Dataspace");
    // }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

template <>
nx::core::Result<> DatasetIO::writeChunk<bool>(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<const bool> values, const DimsType& chunkShape, nonstd::span<const usize> offset)
{
  std::vector<H5_BOOL_TYPE> h5ValuesVec(values.begin(), values.end());
  nonstd::span<const H5_BOOL_TYPE> h5Values(h5ValuesVec.data(), h5ValuesVec.size());

  return writeChunk(chunkInfo, dims, h5Values, chunkShape, offset);
}

nx::core::Result<> DatasetIO::writeString(const std::string& text)
{
  // if(!isValid())
  //{
  //   return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
  // }

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
          hid_t id = H5Dcreate(getParentId(), getName().c_str(), typeId, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
          if(id >= 0)
          {
            if(!text.empty())
            {
              error = H5Dwrite(id, typeId, H5S_ALL, H5S_ALL, H5P_DEFAULT, text.c_str());
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
          H5Dclose(id);
        }
        if(H5Sclose(dataspaceId) < 0)
        {
          returnError = MakeErrorResult(error, "Error closing Dataspace");
        }
      }
    }
    // if(H5Sclose(typeId) < 0)
    //{
    //   returnError = MakeErrorResult(error, "Error closing DataType");
    // }
  }
  return returnError;
}

nx::core::Result<> DatasetIO::writeVectorOfStrings(const std::vector<std::string>& text)
{
  // if(!isValid())
  //{
  //   return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
  // }

  hid_t parentId = getParentId();
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

      auto datasetId = H5Dcreate(parentId, getName().c_str(), datatype, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      setId(datasetId);
      if(datasetId >= 0)
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
          error = H5Dwrite(datasetId, datatype, memSpace, dataspaceID, H5P_DEFAULT, &strPtr);
          if(error < 0)
          {
            std::cout << "Error Writing String Data: " __FILE__ << "(" << __LINE__ << ")" << std::endl;
            returnError = MakeErrorResult(error, "Error Writing String Data");
          }
        }
        // H5Dclose(datasetId);
      }
      H5Tclose(datatype);
      H5Sclose(memSpace);
    }

    H5Sclose(dataspaceID);
  }

  return returnError;
}

#if 0
usize DatasetIO::getNumAttributes() const
{
  if(!exists())
  {
    return 0;
  }
  return openH5Dataset().getNumberAttributes();
}

std::vector<std::string> DatasetIO::getAttributeNames() const
{
  if(!exists())
  {
    return {};
  }
  return openH5Dataset().listAttributeNames();
}

void DatasetIO::deleteAttribute(const std::string& name)
{
  try
  {
    if(!exists())
    {
      return;
    }
    openH5Dataset().deleteAttribute(name);
  }
  catch (const std::exception& e)
  {
    deleteH5Attribute(name);
  }
}

Result<> DatasetIO::deleteH5Attribute(const std::string& name)
{
  auto parentId = parentGroupRef().getId();
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(parentId, H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getName().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(parentId, getName().c_str());
    if(error < 0)
    {
      std::string ss = fmt::format("Error Deleting Attribute '{}' from Object '{}'", getName(), getParentName());
      return MakeErrorResult(error, ss);
    }
  }
  return {};
}
#endif

bool DatasetIO::exists() const
{
  if(getId() > 0)
  {
    return true;
  }
  return false;
  // return parentGroup()->isDataset(getName());
}

// hid_t DatasetIO::getH5Id() const
//{
//   if (m_Id < 0)
//   {
//     m_Id = openH5Dataset().getId();
//   }
//   return m_Id;
// }

// hid_t DatasetIO::getRawH5Id() const
//{
//   if (m_Id < 0)
//   {
//     auto parentId = parentGroup()->getH5Id();
//     m_Id = H5Dopen(parentId, getName().c_str(), H5P_DEFAULT);
//   }
//   return m_Id;
// }

std::string DatasetIO::getFilterName() const
{
  std::string filterNames;
  const hid_t cpListId = H5Dget_create_plist(getId());
  const int numFilters = H5Pget_nfilters(cpListId);
  for(int j = 0; j < numFilters; ++j)
  {
    unsigned int flags;
    unsigned int filterConfig;
    size_t cdNElements = 0;
    char name[1024];
    H5Z_filter_t filter = H5Pget_filter2(cpListId, j, &flags, &cdNElements, nullptr, std::size(name) / sizeof(*name), name, &filterConfig);
    std::vector<unsigned int> cdValues(cdNElements);
    filter = H5Pget_filter2(cpListId, j, &flags, &cdNElements, cdValues.data(), std::size(name) / sizeof(*name), name, &filterConfig);
    if(j != 0)
    {
      filterNames += ", ";
    }
    filterNames += GetNameFromFilterType(filter);
  }
  if(filterNames.empty())
  {
    filterNames = "NONE";
  }
  return filterNames;
  // return name;
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

template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>&) const;
#else
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&) const;

template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;

#if 0
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int8_t>(nonstd::span<int8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int16_t>(nonstd::span<int16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int32_t>(nonstd::span<int32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int64_t>(nonstd::span<int64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint8_t>(nonstd::span<uint8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint16_t>(nonstd::span<uint16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint32_t>(nonstd::span<uint32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint64_t>(nonstd::span<uint64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<char>(nonstd::span<char>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<size_t>(nonstd::span<size_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<float>(nonstd::span<float>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<double>(nonstd::span<double>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#endif

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

template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int8_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int16_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int32_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int64_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint8_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint16_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint32_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint64_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<float>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<double>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<bool>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<char>(const DimsType&, const DimsType&) const;

template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int8_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int16_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int32_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int64_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint8_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint16_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint32_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint64_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<float>(const ChunkedDataInfo&, const DimsType&, nonstd::span<float>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<double>(const ChunkedDataInfo&, const DimsType&, nonstd::span<double>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<char>(const ChunkedDataInfo&, const DimsType&, nonstd::span<char>, const DimsType&, nonstd::span<const usize>) const;

template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int8_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int16_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int32_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int64_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint8_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint16_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint32_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint64_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<float>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const float>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<double>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const double>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<char>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const char>, const DimsType&, nonstd::span<const usize>);
} // namespace nx::core::HDF5
