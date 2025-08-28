#include "DatasetIO.hpp"

#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>

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
  HDF_ERROR_HANDLER_OFF /* Does not matter what the 'id' is, we are accepting that value. */
      hid_t id = H5Dopen(getParentId(), getNamePath().c_str(), H5P_DEFAULT);
  HDF_ERROR_HANDLER_ON
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
  setId(H5Dopen(getParentId(), getNamePath().c_str(), H5P_DEFAULT));
  HDF_ERROR_HANDLER_ON
  if(!isOpen()) // dataset does not exist so create it
  {
    setId(H5Dcreate(getParentId(), getNamePath().c_str(), typeId, dataspaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT));
  }

  return getId();
}

// template <typename T>
// HighFive::DataSet createOrOpenDataset(DatasetIO& datasetIO, const HighFive::DataSpace& dims, HighFive::DataType& dataType)
//{
//   std::string name = datasetIO.getNamePath();
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
//   std::string name = datasetIO.getNamePath();
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

Result<nx::core::DataType> DatasetIO::getDataType() const
{
  auto datasetId = open();
  if(datasetId < 0)
  {
    return MakeErrorResult<nx::core::DataType>(-20013, fmt::format("The selected data set '{}' could not be opened.", getNamePath()));
  }
  H5DatatypeCloser typeId(H5Dget_type(datasetId));
  auto type = getTypeFromId(typeId.id);
  if(type == Type::unknown)
  {
    return MakeErrorResult<nx::core::DataType>(-20014, fmt::format("The selected data set '{}' typeid is unknown.", getNamePath()));
  }

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
    result = {nonstd::make_unexpected(std::vector<Error>{Error{-20012, "The selected datat set is not a supported type for "
                                                                       "importing. Please select a different data set"}})};
    break;
  }

  return result;
}

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
  auto datasetId = open();
  if(!isValid())
  {
    return "";
  }

  std::string data;

  // Test if the string is variable length
  const H5DatatypeCloser typeID(H5Dget_type(datasetId));
  const htri_t isVariableString = H5Tis_variable_str(typeID.id);

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
    auto error = H5Dread(datasetId, typeID.id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
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

  std::vector<std::string> strings;
  auto datasetId = open();

  H5DatatypeCloser dataTypeId(H5Dget_type(datasetId));

  if(dataTypeId.id)
  {
    hsize_t dims[1] = {0};
    /*
     * Get dataspace and allocate memory for read buffer.
     */
    H5DataspaceCloser dataspaceID(H5Dget_space(datasetId));
    int nDims = H5Sget_simple_extent_dims(dataspaceID.id, dims, nullptr);
    if(nDims != 1)
    {

      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Number of dims should be 1 but it was " << nDims << ". Returning early. Is your data file correct?" << std::endl;
      return {};
    }

    std::vector<char*> rData(dims[0], nullptr);

    /*
     * Create the memory datatype.
     */
    H5DatatypeCloser memtype(H5Tcopy(H5T_C_S1));
    herr_t status = H5Tset_size(memtype.id, H5T_VARIABLE);

    H5T_cset_t characterSet = H5Tget_cset(dataTypeId.id);
    status = H5Tset_cset(memtype.id, characterSet);

    /*
     * Read the data.
     */
    status = H5Dread(datasetId, memtype.id, H5S_ALL, H5S_ALL, H5P_DEFAULT, rData.data());
    if(status < 0)
    {
      status = H5Dvlen_reclaim(memtype.id, dataspaceID.id, H5P_DEFAULT, rData.data());
      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Error reading Dataset at locationID (" << getParentId() << ") with object name (" << getNamePath() << ")" << std::endl;
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
    status = H5Dvlen_reclaim(memtype.id, dataspaceID.id, H5P_DEFAULT, rData.data());
  }

  return strings;
}

template <typename T>
std::shared_ptr<AbstractDataStore<T>> DatasetIO::readAsDataStore() const
{
  using ShapeType = typename IDataStore::ShapeType;

  auto dataset = open();
  size_t numElements = getNumElements();

  ShapeType tupleShape{numElements};
  ShapeType componentShape{1};

  std::shared_ptr<AbstractDataStore<T>> dataStorePtr = DataStoreUtilities::CreateDataStore<T>(tupleShape, componentShape, IDataAction::Mode::Execute);
  dataStorePtr->readHdf5(*this);
  return dataStorePtr;
}

template <typename T>
std::shared_ptr<AbstractDataStore<T>> DatasetIO::readAsDataStore(const IDataStore::ShapeType& tupleShape, const IDataStore::ShapeType& componentShape) const
{
  using ShapeType = typename IDataStore::ShapeType;

  auto dataset = open();
  size_t numElements = getNumElements();

  size_t numTuples = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<size_t>(1), std::multiplies<>());
  size_t numComponents = std::accumulate(componentShape.begin(), componentShape.end(), static_cast<size_t>(1), std::multiplies<>());
  if(numTuples * numComponents != numElements)
  {
    return nullptr;
  }

  std::shared_ptr<AbstractDataStore<T>> dataStorePtr = DataStoreUtilities::CreateDataStore<T>(tupleShape, componentShape, IDataAction::Mode::Execute);
  dataStorePtr->readHdf5(*this);
  return dataStorePtr;
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

  T* data = new T[numElements];
  nonstd::span<T> span(data, numElements);

  Result<> result = readIntoSpan<T>(span);
  if(result.invalid())
  {
    delete[] data;
    return {};
  }

  std::vector<T> output(numElements);
  for(usize i = 0; i < numElements; i++)
  {
    output[i] = data[i];
  }
  delete[] data;
  return output;
}

template <class T>
nx::core::Result<> DatasetIO::readIntoSpan(nonstd::span<T> data) const
{
  hid_t datasetId = open();
  if(datasetId <= 0)
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  H5DataspaceCloser fileSpaceId(H5Dget_space(datasetId));
  if(fileSpaceId.invalid())
  {
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  }

  std::vector<hsize_t> memDims;
  int rank = H5Sget_simple_extent_ndims(fileSpaceId.id);
  std::vector<hsize_t> dims(rank), maxDims(rank);
  H5Sget_simple_extent_dims(fileSpaceId.id, dims.data(), maxDims.data());

  memDims = dims;

  hsize_t totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  H5DataspaceCloser memSpaceId(H5Screate_simple(memDims.size(), memDims.data(), NULL));
  if(memSpaceId.invalid())
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }

  if(H5Dread(datasetId, dataType, memSpaceId.id, fileSpaceId.id, H5P_DEFAULT, data.data()) < 0)
  {
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getNamePath()));
  }
  return {};
}

template <>
nx::core::Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool> data) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
  }

  std::vector<H5_BOOL_TYPE> data2(data.begin(), data.end());
  nonstd::span<H5_BOOL_TYPE> span2(data2.data(), data2.size());
  auto result = readIntoSpan(span2);
  std::copy(span2.begin(), span2.end(), data.begin());
  return result;
}

template <class T>
Result<> DatasetIO::readIntoSpan(nonstd::span<T> data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} / {}", getFilePath().string(), getNamePath()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  hid_t datasetId = open();
  H5DataspaceCloser fileSpaceId(H5Dget_space(datasetId));
  if(fileSpaceId.invalid())
  {
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  }

  std::vector<hsize_t> memDims;
  int rank = H5Sget_simple_extent_ndims(fileSpaceId.id);
  std::vector<hsize_t> dims(rank), maxDims(rank);
  H5Sget_simple_extent_dims(fileSpaceId.id, dims.data(), maxDims.data());
  if(start.has_value() && count.has_value())
  {
    // Both start and count are provided
#if defined(__APPLE__)
    std::vector<unsigned long long> startData(start->begin(), start->end());
    std::vector<unsigned long long> countVec(count->begin(), count->end());
    if(H5Sselect_hyperslab(fileSpaceId.id, H5S_SELECT_SET, startData.data(), NULL, countVec.data(), NULL) < 0)
    {
      return MakeErrorResult(-1003, "DatasetReader error: Unable to select hyperslab.");
    }
#else
    if(H5Sselect_hyperslab(fileSpaceId.id, H5S_SELECT_SET, start->data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1003, "DatasetReader error: Unable to select hyper slab.");
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
    if(H5Sselect_hyperslab(fileSpaceId.id, H5S_SELECT_SET, startData.data(), NULL, countRemaining.data(), NULL) < 0)
    {
      return MakeErrorResult(-1004, "DatasetReader error: Unable to select hyperslab.");
    }
#else
    if(H5Sselect_hyperslab(fileSpaceId.id, H5S_SELECT_SET, start->data(), NULL, countRemaining.data(), NULL) < 0)
    {
      return MakeErrorResult(-1004, "DatasetReader error: Unable to select hyper slab.");
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
    if(H5Sselect_hyperslab(fileSpaceId.id, H5S_SELECT_SET, startZeros.data(), NULL, countVec.data(), NULL) < 0)
    {
      return MakeErrorResult(-1005, "DatasetReader error: Unable to select hyperslab.");
    }
#else
    if(H5Sselect_hyperslab(fileSpaceId.id, H5S_SELECT_SET, startZeros.data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1005, "DatasetReader error: Unable to select hyper slab.");
    }
#endif
    memDims = std::vector<hsize_t>(count->begin(), count->end());
  }
  else
  {
    // Neither start nor count is provided
    memDims = dims;
  }

  hsize_t totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  H5DataspaceCloser memSpaceId(H5Screate_simple(memDims.size(), memDims.data(), NULL));
  if(memSpaceId.invalid())
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }

  if(H5Dread(datasetId, dataType, memSpaceId.id, fileSpaceId.id, H5P_DEFAULT, data.data()) < 0)
  {
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getNamePath()));
  }
  return {};
}

template <>
Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool> data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
  }
  if(start->size() != count->size())
  {
    return MakeErrorResult(-506,
                           fmt::format("Cannot read HDF5 data at {} called {}. Requested dimensions do not match: '{}', '{}'", getFilePath().string(), getNamePath(), start->size(), count->size()));
  }

  std::vector<H5_BOOL_TYPE> data2(data.begin(), data.end());
  nonstd::span<H5_BOOL_TYPE> span2(data2.data(), data2.size());
  auto result = readIntoSpan(span2, start, count);
  std::copy(span2.begin(), span2.end(), data.begin());
  return result;
}

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
  H5DataspaceCloser dataspaceId(H5Dget_space(getId()));

  if(dataspaceId.valid())
  {
    if(getClassType() == H5T_STRING)
    {
      H5DatatypeCloser typeId(H5Dget_type(getId()));
      size_t typeSize = H5Tget_size(typeId.id);
      dims = {typeSize};
    }
    else
    {
      size_t rank = H5Sget_simple_extent_ndims(dataspaceId.id);
      std::vector<hsize_t> hdims(rank, 0);
      /* Get dimensions */
      auto error = H5Sget_simple_extent_dims(dataspaceId.id, hdims.data(), nullptr);
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
  H5DataspaceCloser dataspaceId(H5Screate_simple(rank, hDims.data(), nullptr));

  if(dataspaceId.valid())
  {
    /* Create the attribute. */
    H5DatasetCloser datasetId(createOrOpenDataset<T>(dataspaceId.id));
    if(datasetId.valid())
    {
      /* Write the attribute data. */
      const void* data = static_cast<const void*>(values.data());
      error = H5Dwrite(datasetId.id, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
      if(error < 0)
      {
        returnError = MakeErrorResult(error, "Error Writing Attribute");
      }
    }
    else
    {
      returnError = MakeErrorResult(datasetId.id, "Error Creating Dataset");
    }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId.id, "Error Opening Dataspace");
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
    nonstd::span<const H5_BOOL_TYPE> span2(data2.data(), data2.size());
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
  std::vector<hsize_t> h5DimsVec(h5Dims.begin(), h5Dims.end());
  dataInfo.dataspaceId = H5Screate_simple(h5Dims.size(), h5DimsVec.data(), nullptr);
  if(dataInfo.dataspaceId < 0)
  {
    return MakeErrorResult<ChunkedDataInfo>(-120, "Failed to open HDF5 Dataspace");
  }

  dataInfo.dataType = HdfTypeForPrimitive<T>();
  if(dataInfo.dataType == -1)
  {
    H5Sclose(dataInfo.dataspaceId);
    return MakeErrorResult<ChunkedDataInfo>(-100, "DataType was unknown");
  }

  dataInfo.datasetId = createOrOpenDataset(dataInfo.dataType, dataInfo.dataspaceId);
  if(dataInfo.datasetId < 0)
  {
    H5Sclose(dataInfo.dataspaceId);
    return MakeErrorResult<ChunkedDataInfo>(-110, "Failed to open HDF5 Dataset");
  }
  setId(dataInfo.datasetId);
  return {dataInfo};
}

nx::core::Result<> DatasetIO::closeChunkedDataset(const ChunkedDataInfo& datasetInfo) const
{
  // herr_t error = H5Pclose(datasetInfo.transferProp);
  // if(error < 0)
  //{
  //   return MakeErrorResult(error, "Error Closing Transfer Property");
  // }
  /*herr_t error = H5Dclose(datasetInfo.datasetId);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing DataSet");
  }*/
  // else
  //{
  //   setId(-1);
  // }

  /*error = H5Pclose(datasetInfo.chunkProp);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing Chunk Property");
  }*/
  herr_t error = H5Sclose(datasetInfo.dataspaceId);
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
    return MakeErrorResult(-100, "DataType was unknown");
  }
  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = chunkInfo.dataspaceId;
  if(dataspaceId >= 0)
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

      // Select hyperslab
      std::vector<hsize_t> offsetVec(offset.begin(), offset.end());
      std::vector<hsize_t> chunkShapeVec(chunkShape.begin(), chunkShape.end());
      error = H5Sselect_hyperslab(dataspaceId, H5S_SELECT_SET, offsetVec.data(), NULL, chunkShapeVec.data(), NULL);

      // Create memory dataspace for the hyperslab
      H5DataspaceCloser memspaceId(H5Screate_simple(rank, chunkShapeVec.data(), nullptr));

      // Read hyper slab from the dataset
      error = H5Dread(h5Id, HdfTypeForPrimitive<T>(), memspaceId.id, dataspaceId, H5P_DEFAULT, data);
      if(error < 0)
      {
        returnError = MakeErrorResult(error, "Error Writing Dataset Chunk");
      }
    }
    else
    {
      returnError = MakeErrorResult(h5Id, "Error Creating Dataset Chunk");
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
    return MakeErrorResult(-100, "DataType was unknown");
  }
  hid_t dataspaceId = chunkInfo.dataspaceId;
  if(dataspaceId >= 0)
  {
    {
      /* Create the attribute. */
      auto h5Id = chunkInfo.datasetId;
      if(h5Id >= 0)
      {
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());

        // Select hyperslab
        std::vector<hsize_t> offsetVec(offset.begin(), offset.end());
        std::vector<hsize_t> chunkShapeVec(chunkShape.begin(), chunkShape.end());
        error = H5Sselect_hyperslab(dataspaceId, H5S_SELECT_SET, offsetVec.data(), NULL, chunkShapeVec.data(), NULL);

        // Create memory dataspace for the hyperslab
        H5DataspaceCloser memspaceId(H5Screate_simple(rank, chunkShapeVec.data(), nullptr));

        // Read hyper slab from the dataset
        error = H5Dwrite(h5Id, HdfTypeForPrimitive<T>(), memspaceId.id, dataspaceId, H5P_DEFAULT, data);
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
  Result<> returnError = {};

  /* create a string data type */
  H5DatatypeCloser typeId(H5Tcopy(H5T_C_S1));
  if(typeId.valid())
  {
    size_t size = text.size() + 1;
    if(H5Tset_size(typeId.id, size) >= 0)
    {
      if(H5Tset_strpad(typeId.id, H5T_STR_NULLTERM) >= 0)
      {
        /* Create the data space for the dataset. */
        H5DataspaceCloser dataspaceId(H5Screate(H5S_SCALAR));
        if(dataspaceId.valid())
        {
          /* Create or open the dataset. */
          H5DatasetCloser datasetId(H5Dcreate(getParentId(), getNamePath().c_str(), typeId.id, dataspaceId.id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
          if(datasetId.valid())
          {
            if(!text.empty())
            {
              herr_t error = 0;
              error = H5Dwrite(datasetId.id, typeId.id, H5S_ALL, H5S_ALL, H5P_DEFAULT, text.c_str());
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
      }
    }
  }
  return returnError;
}

nx::core::Result<> DatasetIO::writeVectorOfStrings(const std::vector<std::string>& text)
{
  hid_t parentId = getParentId();
  herr_t error = -1;
  Result<> returnError = {};

  std::array<hsize_t, 1> dims = {text.size()};
  H5DataspaceCloser dataspaceID(H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr));
  if(dataspaceID.valid())
  {
    dims[0] = 1;
    H5DataspaceCloser memSpace(H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr));
    if(memSpace.valid())
    {
      H5DatatypeCloser datatype(H5Tcopy(H5T_C_S1));
      H5Tset_size(datatype.id, H5T_VARIABLE);
      H5DatasetCloser datasetId(H5Dcreate(parentId, getNamePath().c_str(), datatype.id, dataspaceID.id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
      setId(datasetId.id);
      if(datasetId.valid())
      {
        // Select the "memory" to be written out - just 1 record.
        hsize_t dataset_offset[] = {0};
        hsize_t dataset_count[] = {1};
        H5Sselect_hyperslab(memSpace.id, H5S_SELECT_SET, dataset_offset, nullptr, dataset_count, nullptr);
        hsize_t pos = 0;
        for(const auto& element : text)
        {
          // Select the file position, 1 record at position 'pos'
          hsize_t element_count[] = {1};
          hsize_t element_offset[] = {pos};
          pos++;
          H5Sselect_hyperslab(dataspaceID.id, H5S_SELECT_SET, element_offset, nullptr, element_count, nullptr);
          const char* strPtr = element.c_str();
          error = H5Dwrite(datasetId.id, datatype.id, memSpace.id, dataspaceID.id, H5P_DEFAULT, &strPtr);
          if(error < 0)
          {
            std::cout << "Error Writing String Data: " __FILE__ << "(" << __LINE__ << ")" << std::endl;
            returnError = MakeErrorResult(error, "Error Writing String Data");
          }
        }
      }
      datasetId.reset(); // Release ownership of the hid_t
    }
  }

  return returnError;
}

bool DatasetIO::exists() const
{
  if(getId() > 0)
  {
    return true;
  }
  return false;
}

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
#ifdef _WIN32
template SIMPLNX_EXPORT std::vector<bool> DatasetIO::readAsVector<bool>() const;
#endif

template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int8_t>> DatasetIO::readAsDataStore<int8_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int16_t>> DatasetIO::readAsDataStore<int16_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int32_t>> DatasetIO::readAsDataStore<int32_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int64_t>> DatasetIO::readAsDataStore<int64_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint8_t>> DatasetIO::readAsDataStore<uint8_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint16_t>> DatasetIO::readAsDataStore<uint16_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint32_t>> DatasetIO::readAsDataStore<uint32_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint64_t>> DatasetIO::readAsDataStore<uint64_t>() const;
#ifdef __APPLE__
// template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<size_t>> DatasetIO::readAsDataStore<size_t>() const;
#endif
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<bool>> DatasetIO::readAsDataStore<bool>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<float>> DatasetIO::readAsDataStore<float>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<double>> DatasetIO::readAsDataStore<double>() const;

template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int8_t>> DatasetIO::readAsDataStore<int8_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int16_t>> DatasetIO::readAsDataStore<int16_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int32_t>> DatasetIO::readAsDataStore<int32_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int64_t>> DatasetIO::readAsDataStore<int64_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint8_t>> DatasetIO::readAsDataStore<uint8_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint16_t>> DatasetIO::readAsDataStore<uint16_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint32_t>> DatasetIO::readAsDataStore<uint32_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint64_t>> DatasetIO::readAsDataStore<uint64_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
#ifdef __APPLE__
// template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<size_t>> DatasetIO::readAsDataStore<size_t>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
#endif
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<bool>> DatasetIO::readAsDataStore<bool>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<float>> DatasetIO::readAsDataStore<float>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<double>> DatasetIO::readAsDataStore<double>(const IDataStore::ShapeType&, const IDataStore::ShapeType&) const;

template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>) const;
#else
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>) const;

template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif

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
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<bool>(const DimsType&, nonstd::span<const bool>);
#endif

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
#ifdef _WIN32
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<bool>(const DimsType&, const DimsType&) const;
#endif

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
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<bool>(const ChunkedDataInfo&, const DimsType&, nonstd::span<bool>, const DimsType&, nonstd::span<const usize>) const;
#endif

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
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<bool>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const bool>, const DimsType&, nonstd::span<const usize>);
#endif
} // namespace nx::core::HDF5
