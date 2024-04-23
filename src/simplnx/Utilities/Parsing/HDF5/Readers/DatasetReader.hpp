#pragma once

#include "simplnx/Utilities/Parsing/HDF5/Readers/ObjectReader.hpp"

#include "simplnx/Common/Result.hpp"

#include <string>
#include <vector>

#include <nonstd/span.hpp>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT DatasetReader : public ObjectReader
{
public:
  /**
   * @brief Constructs an invalid DatasetReader.
   */
  DatasetReader();

  /**
   * @brief Constructs a DatasetReader wrapping a target HDF5 dataset
   * belonging to the specified parent with the target name.
   * @param parentId
   * @param dataName
   */
  DatasetReader(IdType parentId, const std::string& dataName);

  DatasetReader(const DatasetReader& other) = delete;
  DatasetReader(DatasetReader&& other) noexcept = default;

  DatasetReader& operator=(const DatasetReader& other) = delete;
  DatasetReader& operator=(DatasetReader&& other) noexcept = default;

  /**
   * @brief Releases the HDF5 dataset.
   */
  ~DatasetReader() noexcept override;

  /**
   * @brief Returns the dataspace's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  IdType getDataspaceId() const;

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
  bool readIntoSpan(nonstd::span<T> data) const;

  /**
   * @brief Returns a vector of the sizes of the dimensions for the dataset
   * Returns empty vector if unable to read.
   */
  std::vector<hsize_t> getDimensions() const;

  std::string getFilterName() const;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;
};
extern template bool DatasetReader::readIntoSpan<bool>(nonstd::span<bool>) const;
extern template bool DatasetReader::readIntoSpan<int8_t>(nonstd::span<int8_t>) const;
extern template bool DatasetReader::readIntoSpan<int16_t>(nonstd::span<int16_t>) const;
extern template bool DatasetReader::readIntoSpan<int32_t>(nonstd::span<int32_t>) const;
extern template bool DatasetReader::readIntoSpan<int64_t>(nonstd::span<int64_t>) const;
extern template bool DatasetReader::readIntoSpan<uint8_t>(nonstd::span<uint8_t>) const;
extern template bool DatasetReader::readIntoSpan<uint16_t>(nonstd::span<uint16_t>) const;
extern template bool DatasetReader::readIntoSpan<uint32_t>(nonstd::span<uint32_t>) const;
extern template bool DatasetReader::readIntoSpan<uint64_t>(nonstd::span<uint64_t>) const;
extern template bool DatasetReader::readIntoSpan<float>(nonstd::span<float>) const;
extern template bool DatasetReader::readIntoSpan<double>(nonstd::span<double>) const;

extern template std::vector<bool> DatasetReader::readAsVector<bool>() const;
extern template std::vector<int8_t> DatasetReader::readAsVector<int8_t>() const;
extern template std::vector<int16_t> DatasetReader::readAsVector<int16_t>() const;
extern template std::vector<int32_t> DatasetReader::readAsVector<int32_t>() const;
extern template std::vector<int64_t> DatasetReader::readAsVector<int64_t>() const;
extern template std::vector<uint8_t> DatasetReader::readAsVector<uint8_t>() const;
extern template std::vector<uint16_t> DatasetReader::readAsVector<uint16_t>() const;
extern template std::vector<uint32_t> DatasetReader::readAsVector<uint32_t>() const;
extern template std::vector<uint64_t> DatasetReader::readAsVector<uint64_t>() const;
extern template std::vector<float> DatasetReader::readAsVector<float>() const;
extern template std::vector<double> DatasetReader::readAsVector<double>() const;
} // namespace nx::core::HDF5
