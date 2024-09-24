#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/h5.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include "highfive/H5Attribute.hpp"
#include "highfive/H5DataSet.hpp"
#include "highfive/H5DataType.hpp"
#include "highfive/H5File.hpp"
#include "highfive/H5Group.hpp"

#include <nonstd/span.hpp>

#include <string>
#include <vector>

#define H5_BOOL_TYPE uint8

namespace nx::core::HDF5
{
class GroupIO;

class SIMPLNX_EXPORT DatasetIO : public ObjectIO
{
public:
  friend class GroupIO;

  using DimsType = std::vector<usize>;

  /**
   * @brief Constructs an invalid DatasetIO.
   */
  DatasetIO();

  DatasetIO(GroupIO& parentGroup, const std::string& dataName);

  DatasetIO(const DatasetIO& other) = delete;

  DatasetIO(DatasetIO&& other) noexcept;

  DatasetIO& operator=(const DatasetIO& rhs) = delete;
  DatasetIO& operator=(DatasetIO&& rhs) noexcept;

  /**
   * @brief Releases the HDF5 dataset.
   */
  ~DatasetIO() noexcept override;

  /**
   * @brief Returns the IO Classes HighFive ObjectType
   * @return HighFive::ObjectType
   */
  HighFive::ObjectType getObjectType() const override;

  /**
   * @brief Returns an enum representation of the attribute's type.
   * @return HighFive::DataType
   */
  HighFive::DataType getType() const;

  /**
   * @brief Attempts to determine the HDF5 data type for the dataset.
   * Returns an invalid Result if the process fails.
   * @return Result<HDF5::Type>
   */
  Result<nx::core::HDF5::Type> getDataType() const;

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
  Result<> readIntoSpan(nonstd::span<T>& data) const;

  /**
   * @brief Reads the dataset into the given span. Requires the span to be the
   * correct size. Returns false if unable to read.
   * Contains optional start and end positions for the existing HDF5 dataset.
   * @tparam T
   * @param data
   */
  template <class T>
  Result<> readIntoSpan(nonstd::span<T>& data, const std::optional<std::vector<usize>>& start, const std::optional<std::vector<usize>>& count) const;

  /**
   * @brief Reads a chunk of the dataset into the given span. Requires the span to be the
   * correct size. Returns false if unable to read.
   * @tparam T
   * @param data
   */
  template <class T>
  Result<> readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const usize> offset, nonstd::span<const usize> chunkDims) const;

  /**
   * @brief Returns the current chunk dimensions as a vector.
   *
   * Returns an empty vector if no chunks could be found.
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
   * @return Result<>
   */
  Result<> writeString(const std::string& text);

  /**
   * @brief Writes a vector of strings to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @param text
   * @return Result<>
   */
  Result<> writeVectorOfStrings(const std::vector<std::string>& text);

  /**
   * @brief Writes a span of values to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @tparam T
   * @param dims
   * @param values
   * @return Result<>
   */
  template <typename T>
  Result<> writeSpan(const DimsType& dims, nonstd::span<const T> values);

  /**
   * @brief Writes a span of values to the dataset. Returns the HDF5 error,
   * should one occur.
   *
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @tparam T
   * @param dims
   * @param values
   * @return Result<>
   */
  template <typename T>
  Result<> writeChunk(const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkDims, nonstd::span<const usize> offset);

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
  void createAttribute(const std::string& attributeName, const T& value)
  {
    auto dataset = openH5Dataset();
    if(dataset.hasAttribute(attributeName))
    {
      dataset.deleteAttribute(attributeName);
    }
    dataset.createAttribute(attributeName, value);
  }

  /**
   * @brief Creates a string attribute with the specified name and value.
   * @param name
   * @param value
   */
  template <>
  void createAttribute<std::string>(const std::string& attributeName, const std::string& value)
  {
    auto dataset = openH5Dataset();
    if(dataset.hasAttribute(attributeName))
    {
      dataset.deleteAttribute(attributeName);
    }
    writeStringAttribute(dataset.getId(), attributeName, value);
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

  /**
   * @brief Checks if the dataset already exists in HDF5 file.
   * @return Returns true if it exists, otherwise this method returns false.
   */
  bool exists() const;

  /**
   * @param Returns the HDF5 object id.
   * Should only be called by HDF5 IO wrapper classes.
   * @return hid_t
   */
  hid_t getH5Id() const override;

protected:
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return Result<>
   */
  Result<> findAndDeleteAttribute();

  /**
   * @brief Returns a reference to the parent HighFive::Group.
   * @return HighFive::Group&
   */
  HighFive::Group& parentGroupRef() const;

  /**
   * @brief Opens and returns the target HighFive DataSet.
   * @return HighFive::DataSet
   */
  HighFive::DataSet openH5Dataset() const;
};

extern template Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&) const;
extern template Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&) const;
extern template Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&) const;
extern template Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&) const;

extern template Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;
extern template Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>&, const std::optional<std::vector<usize>>&, const std::optional<std::vector<usize>>&) const;

extern template Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<char>(nonstd::span<char>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<int8_t>(nonstd::span<int8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<int16_t>(nonstd::span<int16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<int32_t>(nonstd::span<int32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<int64_t>(nonstd::span<int64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<uint8_t>(nonstd::span<uint8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<uint16_t>(nonstd::span<uint16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<uint32_t>(nonstd::span<uint32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<uint64_t>(nonstd::span<uint64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<float>(nonstd::span<float>, nonstd::span<const usize>, nonstd::span<const usize>) const;
extern template Result<> DatasetIO::readChunkIntoSpan<double>(nonstd::span<double>, nonstd::span<const usize>, nonstd::span<const usize>) const;

extern template Result<> DatasetIO::writeSpan<int8_t>(const DimsType&, nonstd::span<const int8_t>);
extern template Result<> DatasetIO::writeSpan<int16_t>(const DimsType&, nonstd::span<const int16_t>);
extern template Result<> DatasetIO::writeSpan<int32_t>(const DimsType&, nonstd::span<const int32_t>);
extern template Result<> DatasetIO::writeSpan<int64_t>(const DimsType&, nonstd::span<const int64_t>);
extern template Result<> DatasetIO::writeSpan<uint8_t>(const DimsType&, nonstd::span<const uint8_t>);
extern template Result<> DatasetIO::writeSpan<uint16_t>(const DimsType&, nonstd::span<const uint16_t>);
extern template Result<> DatasetIO::writeSpan<uint32_t>(const DimsType&, nonstd::span<const uint32_t>);
extern template Result<> DatasetIO::writeSpan<uint64_t>(const DimsType&, nonstd::span<const uint64_t>);
extern template Result<> DatasetIO::writeSpan<float>(const DimsType&, nonstd::span<const float>);
extern template Result<> DatasetIO::writeSpan<double>(const DimsType&, nonstd::span<const double>);
extern template Result<> DatasetIO::writeSpan<bool>(const DimsType&, nonstd::span<const bool>);

extern template Result<> DatasetIO::writeChunk<int8_t>(const DimsType&, nonstd::span<const int8_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<int16_t>(const DimsType&, nonstd::span<const int16_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<int32_t>(const DimsType&, nonstd::span<const int32_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<int64_t>(const DimsType&, nonstd::span<const int64_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<uint8_t>(const DimsType&, nonstd::span<const uint8_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<uint16_t>(const DimsType&, nonstd::span<const uint16_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<uint32_t>(const DimsType&, nonstd::span<const uint32_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<uint64_t>(const DimsType&, nonstd::span<const uint64_t>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<float>(const DimsType&, nonstd::span<const float>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<double>(const DimsType&, nonstd::span<const double>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<bool>(const DimsType&, nonstd::span<const bool>, const DimsType&, nonstd::span<const usize>);
extern template Result<> DatasetIO::writeChunk<char>(const DimsType&, nonstd::span<const char>, const DimsType&, nonstd::span<const usize>);
} // namespace nx::core::HDF5
