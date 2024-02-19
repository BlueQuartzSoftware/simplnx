#pragma once

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp"

#include "simplnx/Common/Result.hpp"

#include <nonstd/span.hpp>

#include <string>
#include <vector>

namespace nx::core::HDF5
{
class GroupIO;

class SIMPLNX_EXPORT DatasetIO : public ObjectIO
{
public:
  friend class GroupIO;

  using DimsType = std::vector<SizeType>;

  /**
   * @brief Constructs an invalid DatasetIO.
   */
  DatasetIO();

  /**
   * @brief Constructs a DatasetIO wrapping a target HDF5 dataset
   * belonging to the specified parent with the target name.
   * @param parentId
   * @param dataName
   */
  DatasetIO(IdType parentId, const std::string& dataName);

  DatasetIO(const DatasetIO& other) = delete;

  DatasetIO(DatasetIO&& other) noexcept;

  /**
   * @brief Releases the HDF5 dataset.
   */
  virtual ~DatasetIO();

  /**
   * @brief Returns true if the DatasetIO has a valid target. Otherwise,
   * this method returns false.
   * @return bool
   */
  bool isValid() const override;

  /**
   * @brief Returns the target dataset name. Returns an empty string if the
   * DatasetIO is invalid.
   * @return std::string
   */
  std::string getName() const override;

  bool open();

  /**
   * @brief Returns the dataspace's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  IdType getDataspaceId() const;

  /**
   * @brief Returns the property's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  IdType getPListId() const;

  /**
   * @brief Returns an enum representation of the attribute's type.
   * @return Type
   */
  Type getType() const;

  /**
   * @brief Returns an H5T_class_t enum representation of the attribute's class
   * type.
   * @return Type
   */
  IdType getClassType() const;

  /**
   * @brief Returns a DataType enum representation of the attribute's
   * type or an error if there is no conversion.
   * @return DataType
   */
  Result<Type> getDataType() const;

  /**
   * @brief Returns an HDF5 type ID for the target data type. Returns 0 if the
   * dataset is invalid.
   * @return IdType
   */
  IdType getTypeId() const;

  /**
   * @brief Returns the size of the datatype. Returns 0 if the dataset is
   * invalid.
   * @return size_t
   */
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
  bool readIntoSpan(nonstd::span<T>& data) const;

  /**
   * @brief Reads a chunk of the dataset into the given span. Requires the span to be the
   * correct size. Returns false if unable to read.
   * @tparam T
   * @param data
   */
  template <class T>
  bool readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const hsize_t> offset) const;

  /**
   * @brief Returns the current chunk dimensions as a vector.
   *
   * Returns an empty vector if no chunks could be found.
   * @return std::vector<hsize_t>
   */
  std::vector<hsize_t> getChunkDimensions() const;

  /**
   * @brief Returns a vector of the sizes of the dimensions for the dataset
   * Returns empty vector if unable to read.
   */
  std::vector<hsize_t> getDimensions() const;

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
  Result<> writeVectorOfStrings(std::vector<std::string>& text);

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
  Result<> writeChunk(const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkDims, nonstd::span<const hsize_t> offset);

  template <typename T>
  void createOrOpenDataset(const DimsType& dimensions, IdType propertiesId = 0)
  {
    hid_t dataType = Support::HdfTypeForPrimitive<T>();
    hid_t dataspaceId = H5Screate_simple(dimensions.size(), dimensions.data(), nullptr);
    if(dataspaceId >= 0)
    {
      auto result = findAndDeleteAttribute();
      if(result.invalid())
      {
        std::cout << "Error Removing Existing Attribute" << std::endl;
        return;
      }
      else
      {
        /* Create the attribute. */
        createOrOpenDataset(dataType, dataspaceId, propertiesId);
        if(getId() < 0)
        {
          std::cout << "Error Creating or Opening Dataset" << std::endl;
          return;
        }
      }
    }
  }

  template <typename T>
  void createOrOpenChunkedDataset(const DimsType& dimensions, const DimsType& chunkDimensions)
  {
    auto properties = CreateDatasetChunkProperties(chunkDimensions);
    createOrOpenDataset<T>(dimensions, properties);
  }

  DatasetIO& operator=(const DatasetIO& rhs) = delete;
  DatasetIO& operator=(DatasetIO&& rhs) noexcept;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;

  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return Result<>
   */
  Result<> findAndDeleteAttribute();

  /**
   * @brief Opens the target HDF5 dataset or creates a new one using the given
   * datatype and dataspace IDs.
   * @param typeId
   * @param dataspaceId
   */
  void createOrOpenDataset(IdType typeId, IdType dataspaceId, IdType properties = 0);

  void createOrOpenDatasetChunk(IdType typeId, IdType dataspaceId, const DimsType& chunkDims);

  /**
   * @brief Applies chunking to the dataset and sets the chunk dimensions.
   * @param dims
   * @return Returns the property ID if successful. Returns H5P_DEFAULT otherwise.
   */
  static IdType CreateDatasetChunkProperties(const DimsType& dims);

  static IdType CreateTransferChunkProperties(const DimsType& chunkDims);

private:
  std::string m_DatasetName;
};
extern template bool DatasetIO::readIntoSpan<bool>(nonstd::span<bool>&) const;
extern template bool DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>&) const;
extern template bool DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>&) const;
extern template bool DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>&) const;
extern template bool DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>&) const;
extern template bool DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>&) const;
extern template bool DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>&) const;
extern template bool DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>&) const;
extern template bool DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>&) const;
extern template bool DatasetIO::readIntoSpan<float>(nonstd::span<float>&) const;
extern template bool DatasetIO::readIntoSpan<double>(nonstd::span<double>&) const;

extern template bool DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<char>(nonstd::span<char>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<int8_t>(nonstd::span<int8_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<int16_t>(nonstd::span<int16_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<int32_t>(nonstd::span<int32_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<int64_t>(nonstd::span<int64_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<uint8_t>(nonstd::span<uint8_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<uint16_t>(nonstd::span<uint16_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<uint32_t>(nonstd::span<uint32_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<uint64_t>(nonstd::span<uint64_t>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<float>(nonstd::span<float>, nonstd::span<const hsize_t>) const;
extern template bool DatasetIO::readChunkIntoSpan<double>(nonstd::span<double>, nonstd::span<const hsize_t>) const;

extern template Result<> DatasetIO::writeSpan<int8_t>(const DimsType& dims, nonstd::span<const int8_t>);
extern template Result<> DatasetIO::writeSpan<int16_t>(const DimsType& dims, nonstd::span<const int16_t>);
extern template Result<> DatasetIO::writeSpan<int32_t>(const DimsType& dims, nonstd::span<const int32_t>);
extern template Result<> DatasetIO::writeSpan<int64_t>(const DimsType& dims, nonstd::span<const int64_t>);
extern template Result<> DatasetIO::writeSpan<uint8_t>(const DimsType& dims, nonstd::span<const uint8_t>);
extern template Result<> DatasetIO::writeSpan<uint16_t>(const DimsType& dims, nonstd::span<const uint16_t>);
extern template Result<> DatasetIO::writeSpan<uint32_t>(const DimsType& dims, nonstd::span<const uint32_t>);
extern template Result<> DatasetIO::writeSpan<uint64_t>(const DimsType& dims, nonstd::span<const uint64_t>);
extern template Result<> DatasetIO::writeSpan<float>(const DimsType& dims, nonstd::span<const float>);
extern template Result<> DatasetIO::writeSpan<double>(const DimsType& dims, nonstd::span<const double>);

extern template Result<> DatasetIO::writeChunk<int8_t>(const DimsType& dims, nonstd::span<const int8_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<int16_t>(const DimsType& dims, nonstd::span<const int16_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<int32_t>(const DimsType& dims, nonstd::span<const int32_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<int64_t>(const DimsType& dims, nonstd::span<const int64_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<uint8_t>(const DimsType& dims, nonstd::span<const uint8_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<uint16_t>(const DimsType& dims, nonstd::span<const uint16_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<uint32_t>(const DimsType& dims, nonstd::span<const uint32_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<uint64_t>(const DimsType& dims, nonstd::span<const uint64_t> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<float>(const DimsType& dims, nonstd::span<const float> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<double>(const DimsType& dims, nonstd::span<const double> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<bool>(const DimsType& dims, nonstd::span<const bool> values, const DimsType&, nonstd::span<const hsize_t> offset);
extern template Result<> DatasetIO::writeChunk<char>(const DimsType& dims, nonstd::span<const char> values, const DimsType&, nonstd::span<const hsize_t> offset);
} // namespace nx::core::HDF5
