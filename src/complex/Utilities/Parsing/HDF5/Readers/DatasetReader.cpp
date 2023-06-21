#include "DatasetReader.hpp"

#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Apublic.h>

#include <iostream>
#include <numeric>

namespace complex::HDF5
{
DatasetReader::DatasetReader()
{
}

DatasetReader::DatasetReader(IdType parentId, const std::string& dataName)
: ObjectReader(parentId, H5Dopen(parentId, dataName.c_str(), H5P_DEFAULT))
{
}

DatasetReader::~DatasetReader()
{
  closeHdf5();
}

void DatasetReader::closeHdf5()
{
  if(isValid())
  {
    H5Dclose(getId());
    setId(0);
  }
}

IdType DatasetReader::getDataspaceId() const
{
  return H5Dget_space(getId());
}

Type DatasetReader::getType() const
{
  auto typeId = getTypeId();
  return getTypeFromId(typeId);
}

IdType DatasetReader::getClassType() const
{
  auto typeId = getTypeId();
  return H5Tget_class(typeId);
}

Result<Type> DatasetReader::getDataType() const
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

IdType DatasetReader::getTypeId() const
{
  auto identifier = getId();
  return H5Dget_type(identifier);
}

size_t DatasetReader::getTypeSize() const
{
  return H5Tget_size(getTypeId());
}

size_t DatasetReader::getNumElements() const
{
  std::vector<hsize_t> dims = getDimensions();
  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
  return numElements;
}

std::string DatasetReader::readAsString() const
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

std::vector<std::string> DatasetReader::readAsVectorOfStrings() const
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
    status = H5Dread(getId(), memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rData.data());
    if(status < 0)
    {
      status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
      // H5Sclose(dataspaceID);
      // H5Tclose(typeID);
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
std::vector<T> DatasetReader::readAsVector() const
{
  if(!isValid())
  {
    return {};
  }

  size_t numElements = getNumElements();

  std::vector<T> data(numElements);

  if(!readIntoSpan<T>(data))
  {
    return {};
  }

  return data;
}

template <class T>
bool DatasetReader::readIntoSpan(nonstd::span<T> data) const
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
      // std::cout << "NumElements: " << numElements << std::endl;
      if(numElements != data.size())
      {
        return false;
      }
      herr_t error = H5Dread(getId(), dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
      if(error < 0)
      {
        std::cout << "Error Reading Data.'" << getName() << "'" << std::endl;
      }
    }
    // auto error = H5Sclose(spaceId);
    // if(error < 0)
    //{
    //  std::cout << "Error Closing Data Space" << std::endl;
    //}
  }
  else
  {
    std::cout << "Error Opening SpaceID" << std::endl;
  }

  return true;
}

std::vector<hsize_t> DatasetReader::getDimensions() const
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

std::string DatasetReader::getFilterName() const
{
  std::string filterNames;
  const hid_t cpListId = H5Dget_create_plist(getId());
  // const hid_t apListId = H5Dget_access_plist(getId());
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
    filterNames += HDF5::Support::GetNameFromFilterType(filter);
  }
  if(filterNames.empty())
  {
    filterNames = "NONE";
  }
  return filterNames;
  // return name;
}

// declare readAsVector
template COMPLEX_EXPORT std::vector<int8_t> DatasetReader::readAsVector<int8_t>() const;
template COMPLEX_EXPORT std::vector<int16_t> DatasetReader::readAsVector<int16_t>() const;
template COMPLEX_EXPORT std::vector<int32_t> DatasetReader::readAsVector<int32_t>() const;
template COMPLEX_EXPORT std::vector<int64_t> DatasetReader::readAsVector<int64_t>() const;
template COMPLEX_EXPORT std::vector<uint8_t> DatasetReader::readAsVector<uint8_t>() const;
template COMPLEX_EXPORT std::vector<uint16_t> DatasetReader::readAsVector<uint16_t>() const;
template COMPLEX_EXPORT std::vector<uint32_t> DatasetReader::readAsVector<uint32_t>() const;
template COMPLEX_EXPORT std::vector<uint64_t> DatasetReader::readAsVector<uint64_t>() const;
#ifdef __APPLE__
template COMPLEX_EXPORT std::vector<size_t> DatasetReader::readAsVector<size_t>() const;
#endif
template COMPLEX_EXPORT std::vector<float> DatasetReader::readAsVector<float>() const;
template COMPLEX_EXPORT std::vector<double> DatasetReader::readAsVector<double>() const;

template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<int8_t>(nonstd::span<int8_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<int16_t>(nonstd::span<int16_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<int32_t>(nonstd::span<int32_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<int64_t>(nonstd::span<int64_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<uint8_t>(nonstd::span<uint8_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<uint16_t>(nonstd::span<uint16_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<uint32_t>(nonstd::span<uint32_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<uint64_t>(nonstd::span<uint64_t>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<bool>(nonstd::span<bool>) const;
#ifdef __APPLE__
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<size_t>(nonstd::span<size_t>) const;
#endif
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<float>(nonstd::span<float>) const;
template COMPLEX_EXPORT bool DatasetReader::readIntoSpan<double>(nonstd::span<double>) const;
} // namespace complex::HDF5
