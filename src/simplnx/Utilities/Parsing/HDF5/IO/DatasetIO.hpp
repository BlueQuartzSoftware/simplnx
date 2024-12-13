#pragma once

#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

// #include "highfive/H5Attribute.hpp"
// #include "highfive/H5DataSet.hpp"
// #include "highfive/H5DataType.hpp"
// #include "highfive/H5File.hpp"
// #include "highfive/H5Group.hpp"

#include <H5Apublic.h>
#include <H5Dpublic.h>
#include <H5Ppublic.h>

#include "fmt/format.h"
#include <nonstd/span.hpp>

#include <string>
#include <vector>

#define H5_BOOL_TYPE uint8

namespace nx::core::HDF5
{
class GroupIO;
struct SIMPLNX_EXPORT ChunkedDataInfo
{
  hid_t dataType = 0;
  hid_t datasetId = 0;
  hid_t dataspaceId = 0;
  hid_t transferProp = 0;
  hid_t chunkProp = 0;
};

class SIMPLNX_EXPORT DatasetIO : public ObjectIO
{
public:
  friend class GroupIO;

  /**
   * @brief Constructs an invalid DatasetIO.
   */
  DatasetIO();

  DatasetIO(hid_t parentId, const std::string& dataName);

  DatasetIO(const DatasetIO& other) = delete;

  DatasetIO(DatasetIO&& other) noexcept;

  DatasetIO& operator=(const DatasetIO& rhs) = delete;
  DatasetIO& operator=(DatasetIO&& rhs) noexcept;

  /**
   * @brief Releases the HDF5 dataset.
   */
  ~DatasetIO() noexcept override;

  /**
   * @brief Attempts to determine the HDF5 data type for the dataset.
   * Returns an invalid Result if the process fails.
   * @return Result<HDF5::Type>
   */
  Result<nx::core::DataType> getDataType() const;

  hid_t getTypeId() const;

  hid_t getClassType() const;

  size_t getTypeSize() const;

  /**
   * @brief Returns the number of elements in the attribute.
   * @return size_t
   */
  size_t getNumElements() const;

  /**
   * @brief Returns the number of elements in the attribute.
   * @return size_t
   */
  size_t getNumChunkElements() const;

  /**
   * @brief Returns a string value for the dataset.
   * Returns an empty string if no dataset exists or the dataset is not a
   * string.
   * @return std::string
   */
  std::string readAsString() const;

  /**
   * @brief Returns a vector of string values for the dataset.
   * Returns an empty string if no dataset exists or the dataset is not a
   * string.
   * @return std::vector<std::string>
   */
  std::vector<std::string> readAsVectorOfStrings() const;

  /**
   * @brief Returns a vector of values for the attribute.
   * Returns an empty vector if no attribute exists or the attribute is not of
   * the specified type.
   * @tparam T
   * @return std::vector<T>
   */
  template <typename T>
  std::vector<T> readAsVector() const;

  /**
   * @brief Reads the dataset into the given span. Requires the span to be the
   * correct size. Returns false if unable to read.
   * @tparam T
   * @param data
   */
  template <class T>
  nx::core::Result<> readIntoSpan(nonstd::span<T>& data) const;

  /**
   * @brief Reads the dataset into the given span. Requires the span to be the
   * correct size. Returns false if unable to read.
   * Contains optional start and end positions for the existing HDF5 dataset.
   * @tparam T
   * @param data
   */
  template <class T>
  nx::core::Result<> readIntoSpan(nonstd::span<T>& data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const;

#if 0
  /**
   * @brief Reads a chunk of the dataset into the given span. Requires the span to be the
   * correct size. Returns false if unable to read.
   * @tparam T
   * @param data
   */
  template <class T>
  nx::core::Result<> readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const usize> offset, nonstd::span<const usize> chunkDims) const;
#endif

  /**
   * @brief Returns the current chunk dimensions as a vector.
   *
   * Returns an empty vector if no chunkShape could be found.
   * @return std::vector<usize>
   */
  std::vector<usize> getChunkDimensions() const;

  /**
   * @brief Returns a vector of the sizes of the dimensions for the dataset
   * Returns empty vector if unable to read.
   */
  std::vector<usize> getDimensions() const;

  /**
   * @brief Writes a given string to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @param text
   * @return nx::core::Result<>
   */
  nx::core::Result<> writeString(const std::string& text);

  /**
   * @brief Writes a vector of strings to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @param text
   * @return nx::core::Result<>
   */
  nx::core::Result<> writeVectorOfStrings(const std::vector<std::string>& text);

  /**
   * @brief Writes a span of values to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @tparam T
   * @param dims
   * @param values
   * @return nx::core::Result<>
   */
  template <typename T>
  nx::core::Result<> writeSpan(const DimsType& dims, nonstd::span<const T> values);

  template <typename T>
  Result<ChunkedDataInfo> initChunkedDataset(const DimsType& dims, const DimsType& chunkDims) const;
  nx::core::Result<> closeChunkedDataset(const ChunkedDataInfo& datasetInfo) const;

  /**
   * @brief Writes a span of values to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @tparam T
   * @param dims
   * @param values
   * @return nx::core::Result<>
   */
  template <typename T>
  nx::core::Result<> readChunk(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<T> values, const DimsType& chunkDims, nonstd::span<const usize> offset) const;

  /**
   * @brief Writes a span of values to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @tparam T
   * @param dims
   * @param values
   * @return nx::core::Result<>
   */
  template <typename T>
  nx::core::Result<> writeChunk(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkDims, nonstd::span<const usize> offset);

#if 0
  /**
   * @brief Returns the number of attributes in the object. Returns 0 if the
   * object is not valid.
   * @return usize
   */
  usize getNumAttributes() const override;

  /**
   * @brief Returns a vector with each attribute name.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getAttributeNames() const override;

  /**
   * @brief Deletes the attribute with the specified name.
   * @param name
   */
  void deleteAttribute(const std::string& name) override;

  /**
   * @brief Creates an attribute with the specified name and value.
   * @param name
   * @param value
   */
  template <typename T>
  nx::core::Result<> createAttribute(const std::string& attributeName, const T& value)
  {
    try
    {
      auto dataset = openH5Dataset();
      if(dataset.hasAttribute(attributeName))
      {
        dataset.deleteAttribute(attributeName);
      }
      dataset.createAttribute(attributeName, value);
      return {};
    } catch(const std::exception& e)
    {
      try
      {
        return createScalarAttribute(attributeName, value);
      }
      catch (const std::exception& e)
      {
        std::string msg = fmt::format("Error writing Dataset Attribute '{}': {}", attributeName, e.what());
        return MakeErrorResult(-6452, msg);
      }
    }
  }

  /**
   * @brief Creates a string attribute with the specified name and value.
   * @param name
   * @param value
   */
  template <>
  nx::core::Result<> createAttribute<std::string>(const std::string& attributeName, const std::string& value)
  {
    auto dataset = openH5Dataset();
    if(dataset.hasAttribute(attributeName))
    {
      dataset.deleteAttribute(attributeName);
    }
    writeStringAttribute(dataset.getId(), attributeName, value);
    return {};
  }

  template <typename T>
  nx::core::Result<> createScalarAttribute(const std::string& attributeName, const T& value)
  {
    herr_t error = 0;
    nx::core::Result<> returnError = {};

    hid_t dataType = HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-101, "Cannot write specified data type");
    }

    int32_t rank = 1;
    hsize_t dims = 1;
    hid_t dataspaceId = H5Screate_simple(rank, &dims, nullptr);
    if(dataspaceId >= 0)
    {
      // Delete existing attribute
      auto result = deleteH5Attribute(attributeName);
      if(result.valid())
      {
        /* Create the attribute. */
        hid_t attributeId = H5Acreate(getId(), attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, &value);
          if(error < 0)
          {
            returnError = MakeErrorResult(error, "Error Writing Attribute");
          }
        }
        /* Close the attribute. */
        error = H5Aclose(attributeId);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Closing Attribute");
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
      returnError = MakeErrorResult(dataspaceId, "Invalid Dataspace ID");
    }

    return returnError;
  }

  template <typename T>
  nx::core::Result<> createVectorAttribute(const std::string& attributeName, const std::vector<T>& vector)
  {
    try
    {
      auto dataset = openH5Dataset();
      if(dataset.hasAttribute(attributeName))
      {
        dataset.deleteAttribute(attributeName);
      }
      dataset.createAttribute(attributeName, vector);
      return {};
    } catch(const std::exception& e)
    {
      DimsType dims{vector.size()};
      return createVectorAttribute(attributeName, dims, vector);
    }
  }

  template <typename T>
  nx::core::Result<> createVectorAttribute(const std::string& attributeName, const DimsType& dims, const std::vector<T>& vector)
  {
    nx::core::Result<> returnError = {};
    ErrorType error = 0;
    int32_t rank = static_cast<int32_t>(dims.size());

    hid_t dataType = HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-101, "Unknown data type");
    }
    std::vector<hsize_t> hDims(dims.size());
    std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
    hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
    if(dataspaceId >= 0)
    {
      // Delete any existing attribute
      auto result = findAndDeleteAttribute();
      if(result.valid())
      {
        /* Create the attribute. */
        hid_t h5dId = getRawH5Id();
        hid_t attributeId = H5Acreate(h5dId, attributeName.c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        if(attributeId >= 0)
        {
          /* Write the attribute data. */
          error = H5Awrite(attributeId, dataType, static_cast<const void*>(vector.data()));
          if(error < 0)
          {
            returnError = MakeErrorResult(error, "Error Writing Attribute");
          }
        }
        /* Close the attribute. */
        error = H5Aclose(attributeId);
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Closing Attribute");
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
      returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace ID");
    }

    return returnError;
  }

  /**
   * @brief Reads an attribute with the specified name.
   * @param name
   * @param value
   */
  template <typename T>
  void readAttribute(const std::string& attributeName, T& value) const
  {
    auto dataset = openH5Dataset();
    if(dataset.hasAttribute(attributeName))
    {
      dataset.getAttribute(attributeName).read(value);
    }
  }

  /**
   * @brief Reads a string attribute with the specified name.
   * @param name
   * @param value
   */
  template <>
  void readAttribute<std::string>(const std::string& attributeName, std::string& value) const
  {
    auto dataset = openH5Dataset();
    if(dataset.hasAttribute(attributeName))
    {
      auto attrib = dataset.getAttribute(attributeName);
      value = readStringAttribute(attrib.getId());
    }
  }
#endif

  /**
   * @brief Checks if the dataset already exists in HDF5 file.
   * @return Returns true if it exists, otherwise this method returns false.
   */
  bool exists() const;

  std::string getFilterName() const;

protected:
  hid_t createOrOpenDataset(hid_t typeId, hid_t dataspaceId, hid_t propertiesId = H5P_DEFAULT) const;

  template <typename T>
  hid_t createOrOpenDataset(hid_t dataspaceId, hid_t propertiesId = H5P_DEFAULT) const
  {
    return createOrOpenDataset(HdfTypeForPrimitive<T>(), dataspaceId, propertiesId);
  }

  nx::core::Result<> deleteH5Attribute(const std::string& name);

#if 0
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return nx::core::Result<>
   */
  nx::core::Result<> findAndDeleteAttribute();
#endif

  static hid_t CreateH5DatasetChunkProperties(const DimsType& chunkDims);

  /**
   * @brief Opens and returns the target HDF5 DataSet.
   * @return hid_t
   */
  hid_t open() const override;

  void close() override;

private:
};

extern template std::vector<int8_t> DatasetIO::readAsVector() const;
extern template std::vector<int16_t> DatasetIO::readAsVector() const;
extern template std::vector<int32_t> DatasetIO::readAsVector() const;
extern template std::vector<int64_t> DatasetIO::readAsVector() const;
extern template std::vector<uint8_t> DatasetIO::readAsVector() const;
extern template std::vector<uint16_t> DatasetIO::readAsVector() const;
extern template std::vector<uint32_t> DatasetIO::readAsVector() const;
extern template std::vector<uint64_t> DatasetIO::readAsVector() const;
#ifdef __APPLE__
extern template std::vector<usize> DatasetIO::readAsVector() const;
#endif
extern template std::vector<float> DatasetIO::readAsVector() const;
extern template std::vector<double> DatasetIO::readAsVector() const;
#ifdef _WIN32
extern template std::vector<bool> DatasetIO::readAsVector() const;
#endif

extern template nx::core::Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&) const;
#ifdef _WIN32
extern template nx::core::Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&) const;
#endif

extern template nx::core::Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
extern template nx::core::Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef _WIN32
extern template nx::core::Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif

#if 0
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<char>(nonstd::span<char>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<int8_t>(nonstd::span<int8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<int16_t>(nonstd::span<int16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<int32_t>(nonstd::span<int32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<int64_t>(nonstd::span<int64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<uint8_t>(nonstd::span<uint8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<uint16_t>(nonstd::span<uint16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<uint32_t>(nonstd::span<uint32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<uint64_t>(nonstd::span<uint64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<float>(nonstd::span<float>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunkIntoSpan<double>(nonstd::span<double>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#endif

extern template nx::core::Result<> DatasetIO::writeSpan<int8_t>(const DimsType&, nonstd::span<const int8_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<int16_t>(const DimsType&, nonstd::span<const int16_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<int32_t>(const DimsType&, nonstd::span<const int32_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<int64_t>(const DimsType&, nonstd::span<const int64_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<uint8_t>(const DimsType&, nonstd::span<const uint8_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<uint16_t>(const DimsType&, nonstd::span<const uint16_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<uint32_t>(const DimsType&, nonstd::span<const uint32_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<uint64_t>(const DimsType&, nonstd::span<const uint64_t>);
extern template nx::core::Result<> DatasetIO::writeSpan<float>(const DimsType&, nonstd::span<const float>);
extern template nx::core::Result<> DatasetIO::writeSpan<double>(const DimsType&, nonstd::span<const double>);
#ifdef _WIN32
extern template nx::core::Result<> DatasetIO::writeSpan<bool>(const DimsType&, nonstd::span<const bool>);
#endif

extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int8_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int16_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int32_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int64_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint8_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint16_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint32_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint64_t>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<float>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<double>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<bool>(const DimsType&, const DimsType&) const;
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<char>(const DimsType&, const DimsType&) const;
#ifdef _WIN32
extern template Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<bool>(const DimsType&, const DimsType&) const;
#endif

extern template nx::core::Result<> DatasetIO::readChunk<int8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int8_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<int16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int16_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<int32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int32_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<int64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int64_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<uint8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint8_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<uint16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint16_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<uint32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint32_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<uint64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint64_t>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<float>(const ChunkedDataInfo&, const DimsType&, nonstd::span<float>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<double>(const ChunkedDataInfo&, const DimsType&, nonstd::span<double>, const DimsType&, nonstd::span<const usize>) const;
extern template nx::core::Result<> DatasetIO::readChunk<char>(const ChunkedDataInfo&, const DimsType&, nonstd::span<char>, const DimsType&, nonstd::span<const usize>) const;
#ifdef _WIN32
extern template nx::core::Result<> DatasetIO::readChunk<bool>(const ChunkedDataInfo&, const DimsType&, nonstd::span<bool>, const DimsType&, nonstd::span<const usize>) const;
#endif

extern template nx::core::Result<> DatasetIO::writeChunk<int8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int8_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<int16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int16_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<int32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int32_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<int64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int64_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<uint8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint8_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<uint16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint16_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<uint32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint32_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<uint64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint64_t>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<float>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const float>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<double>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const double>, const DimsType&, nonstd::span<const usize>);
extern template nx::core::Result<> DatasetIO::writeChunk<char>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const char>, const DimsType&, nonstd::span<const usize>);
#ifdef _WIN32
extern template nx::core::Result<> DatasetIO::writeChunk<bool>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const bool>, const DimsType&, nonstd::span<const usize>);
#endif
} // namespace nx::core::HDF5
