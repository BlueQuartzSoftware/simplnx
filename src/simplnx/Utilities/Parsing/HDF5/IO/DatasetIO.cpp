#include "DatasetIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "fmt/format.h"

#include "highfive/H5DataSet.hpp"
#include "highfive/H5DataSpace.hpp"
#include "highfive/H5DataType.hpp"
#include "highfive/H5File.hpp"

#include "H5Dpublic.h"
#include "H5Spublic.h"
#include "H5Tpublic.h"

#include <array>
#include <iostream>
#include <numeric>
#include <span>

namespace nx::core::HDF5
{
DatasetIO::DatasetIO() = default;

DatasetIO::DatasetIO(GroupIO& parentGroup, const std::string& dataName)
: ObjectIO(parentGroup, dataName)
{
}

DatasetIO::DatasetIO(DatasetIO&& other) noexcept
: ObjectIO(std::move(other))
{
}

DatasetIO::~DatasetIO() noexcept
{
}

HighFive::Group& DatasetIO::parentGroupRef() const
{
  return parentGroup()->groupRef();
}

HighFive::DataSet DatasetIO::openH5Dataset() const
{
  std::string datapath = getObjectPath();
  return parentGroup()->openH5Dataset(getName());
}

//template <typename T>
//HighFive::DataSet createOrOpenDataset(DatasetIO& datasetIO, const HighFive::DataSpace& dims, HighFive::DataType& dataType)
//{
//  std::string name = datasetIO.getName();
//  std::string datapath = datasetIO.getObjectPath();
//  try
//  {
//    auto& parentGroup = datasetIO.parentGroup()->groupRef();
//    if(parentGroup.exist(name))
//    {
//      return parentGroup.getDataSet(name);
//    }
//    else
//    {
//      return parentGroup.createDataSet(name, dims, dataType);
//    }
//  } catch(const std::exception& e)
//  {
//    throw e;
//  }
//}
//
//template <>
//HighFive::DataSet createOrOpenDataset<bool>(DatasetIO& datasetIO, const HighFive::DataSpace& dims)
//{
//  std::string name = datasetIO.getName();
//  std::string datapath = datasetIO.getObjectPath();
//  try
//  {
//    auto& parentGroup = datasetIO.parentGroup()->groupRef();
//    if(parentGroup.exist(name))
//    {
//      return std::move(parentGroup.getDataSet(name));
//    }
//    else
//    {
//      return std::move(parentGroup.createDataSet<H5_BOOL_TYPE>(name, dims));
//    }
//  } catch(const std::exception& e)
//  {
//    throw e;
//  }
//}

DatasetIO& DatasetIO::operator=(DatasetIO&& rhs) noexcept
{
  moveObj(std::move(rhs));
  return *this;
}

HighFive::ObjectType DatasetIO::getObjectType() const
{
  return HighFive::ObjectType::Dataset;
}

Result<> DatasetIO::findAndDeleteAttribute()
{
  if(!parentGroupRef().exist(getName()))
  {
    return {};
  }
  auto dataset = openH5Dataset();
  auto attributeNames = dataset.listAttributeNames();
  for(const auto& attributeName : attributeNames)
  {
    dataset.deleteAttribute(attributeName);
  }
  return {};
}

HighFive::DataType DatasetIO::getType() const
{
  auto dataset = openH5Dataset();
  return dataset.getDataType();
}

Result<Type> DatasetIO::getDataType() const
{
  auto dataset = openH5Dataset();
  auto datasetId = dataset.getId();
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

size_t DatasetIO::getNumElements() const
{
  if(!exists())
  {
    return 0;
  }
  auto dataset = openH5Dataset();
  return dataset.getElementCount();
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
  auto dataset = openH5Dataset();
  std::string data;

  // Test if the string is variable length
  const hid_t typeID = H5Dget_type(dataset.getId());
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
    hsize_t size = H5Dget_storage_size(dataset.getId());
    std::vector<char> buffer(static_cast<size_t>(size + 1),
                             0x00); // Allocate and Zero and array
    auto error = H5Dread(dataset.getId(), typeID, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
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

  //return std::move(data);
}

std::vector<std::string> DatasetIO::readAsVectorOfStrings() const
{
  if(!isValid())
  {
    return {};
  }
  auto dataset = openH5Dataset();

  std::vector<std::string> strings;
  auto datasetId = dataset.getId();

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
      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Error reading Dataset at locationID (" << parentGroup()->getName() << ") with object name (" << getName() << ")" << std::endl;
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

  auto dataset = openH5Dataset();
  size_t numElements = dataset.getElementCount();

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
Result<> DatasetIO::readIntoSpan(nonstd::span<T>& data) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
  }

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  auto dataset = openH5Dataset();

  hid_t datasetId = dataset.getId();
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
Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>& data) const
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
Result<> DatasetIO::readIntoSpan(nonstd::span<T>& data, const std::optional<std::vector<usize>>& start, const std::optional<std::vector<usize>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} / {}", getFilePath().string(), getName()));
  }

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  auto dataset = openH5Dataset();
  hid_t datasetId = dataset.getId();
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

template <>
Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>& data, const std::optional<std::vector<usize>>& start, const std::optional<std::vector<usize>>& count) const
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

template <class T>
Result<> DatasetIO::readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const usize> chunkOffset, nonstd::span<const usize> chunkDims) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
  }

  std::vector<T> data2(data.size());
  auto dataset = openH5Dataset();
  dataset.select({chunkOffset.data(), chunkDims.data()}).read(data2);
  std::copy(data2.begin(), data2.end(), data.begin());
  return {};
}

template <>
Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool> data, nonstd::span<const usize> chunkOffset, nonstd::span<const usize> chunkDims) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getName()));
  }

  // DataSet does not support bool data.
  std::vector<uint8> data2(data.size());
  auto dataset = openH5Dataset();
  dataset.select({chunkOffset.data(), chunkDims.data()}).read(data2);
  // Copy data back into span<bool>
  std::copy(data2.begin(), data2.end(), data.begin());
  return {};
}

std::vector<usize> DatasetIO::getChunkDimensions() const
{
  auto dataset = openH5Dataset();
  auto id = dataset.getId();
  auto propertyListId = dataset.getCreatePropertyList().getId();
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

std::vector<usize> DatasetIO::getDimensions() const
{
  auto dataset = openH5Dataset();
  auto dims = dataset.getDimensions();
  return std::vector<usize>(dims.begin(), dims.end());
}

template <typename T>
Result<> DatasetIO::writeSpan(const DimsType& dims, nonstd::span<const T> values)
{
  Result<> returnError = {};
  ErrorType error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1, "DataType was unknown");
  }

  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);

  if(dataspaceId >= 0)
  {
    auto result = findAndDeleteAttribute();
    if(result.invalid())
    {
      returnError = MakeErrorResult(result.errors()[0].code, "Error Removing existing Attribute");
    }
    else
    {
      /* Create the attribute. */
      auto dataset = parentGroup()->createOrOpenH5Dataset<T>(getName(), HighFive::DataSpace(hDims));
      if(dataset.getId() >= 0)
      {
        /* Write the attribute data. */
        const void* data = static_cast<const void*>(values.data());
        error = H5Dwrite(dataset.getId(), dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Attribute");
        }
      }
      else
      {
        returnError = MakeErrorResult(dataset.getId(), "Error Creating Dataset");
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
Result<> DatasetIO::writeChunk(const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkShape, nonstd::span<const usize> offset)
{
  if(parentGroupRef().exist(getName()) == false)
  {
    auto dataSpace = HighFive::DataSpace(dims, {HighFive::DataSpace::UNLIMITED, HighFive::DataSpace::UNLIMITED});
    HighFive::DataSetCreateProps props;
    props.add(HighFive::Chunking(chunkShape));

    try
    {
      parentGroupRef().createDataSet(getName(), dataSpace, HighFive::create_datatype<T>(), props);
    } catch(const std::exception& e)
    {
      return MakeErrorResult(-853535, fmt::format("Failed to create chunked Dataset with error: '{}'", e.what()));
    }
  }

  try
  {
    std::vector<T> data2(values.begin(), values.end());
    auto dataset = openH5Dataset();
    dataset.select({offset.data(), chunkShape.data()}).write(data2);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-853535, fmt::format("Failed to write to Dataset with error: '{}'", e.what()));
  }

  return {};
}

template <>
Result<> DatasetIO::writeChunk<bool>(const DimsType& dims, nonstd::span<const bool> values, const DimsType& chunkShape, nonstd::span<const usize> offset)
{
  if(parentGroupRef().exist(getName()) == false)
  {
    auto dataSpace = HighFive::DataSpace(dims, {HighFive::DataSpace::UNLIMITED, HighFive::DataSpace::UNLIMITED});
    HighFive::DataSetCreateProps props;
    props.add(HighFive::Chunking(chunkShape));

    try
    {
      parentGroupRef().createDataSet(getName(), dataSpace, HighFive::create_datatype<H5_BOOL_TYPE>(), props);
    } catch(const std::exception& e)
    {
      return MakeErrorResult(-853535, fmt::format("Failed to create chunked Dataset with error: '{}'", e.what()));
    }
  }

  try
  {
    // DataSet does not support bool data.
    std::vector<H5_BOOL_TYPE> data2(values.begin(), values.end());
    auto dataset = openH5Dataset();
    dataset.select({offset.data(), chunkShape.data()}).write(data2);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-853535, fmt::format("Failed to write to Dataset with error: '{}'", e.what()));
  }

  return {};
}

Result<> DatasetIO::writeString(const std::string& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
  }

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
          hid_t id = parentGroup()->createOrOpenHDF5Dataset(getName(), typeId, dataspaceId);
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
        H5S_CLOSE_H5_DATASPACE(dataspaceId, error, returnError)
      }
    }
    H5S_CLOSE_H5_TYPE(typeId, error, returnError)
  }
  return returnError;
}

Result<> DatasetIO::writeVectorOfStrings(const std::vector<std::string>& text)
{
  if(!isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
  }

  hid_t parentId = parentGroupRef().getId();
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
        H5Dclose(datasetId);
      }
      H5Tclose(datatype);
      H5Sclose(memSpace);
    }

    H5Sclose(dataspaceID);
  }

  return returnError;
}

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
  if(!exists())
  {
    return;
  }
  openH5Dataset().deleteAttribute(name);
}

bool DatasetIO::exists() const
{
  return parentGroup()->groupRef().exist(getName());
}

hid_t DatasetIO::getH5Id() const
{
  return openH5Dataset().getId();
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
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>&) const;
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
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;

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
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<bool>(const DimsType&, nonstd::span<const bool>);

template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int8_t>(const DimsType&, nonstd::span<const int8_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int16_t>(const DimsType&, nonstd::span<const int16_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int32_t>(const DimsType&, nonstd::span<const int32_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int64_t>(const DimsType&, nonstd::span<const int64_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint8_t>(const DimsType&, nonstd::span<const uint8_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint16_t>(const DimsType&, nonstd::span<const uint16_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint32_t>(const DimsType&, nonstd::span<const uint32_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint64_t>(const DimsType&, nonstd::span<const uint64_t>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<float>(const DimsType&, nonstd::span<const float>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<double>(const DimsType&, nonstd::span<const double>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<bool>(const DimsType&, nonstd::span<const bool>, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<char>(const DimsType&, nonstd::span<const char>, const DimsType&, nonstd::span<const usize>);
} // namespace nx::core::HDF5
