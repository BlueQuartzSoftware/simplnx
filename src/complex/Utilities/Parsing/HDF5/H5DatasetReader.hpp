#pragma once

#include <string>
#include <vector>

#include <nonstd/span.hpp>

#include "complex/Common/Result.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectReader.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT DatasetReader : public ObjectReader
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
  DatasetReader(H5::IdType parentId, const std::string& dataName);

  /**
   * @brief Releases the HDF5 dataset.
   */
  virtual ~DatasetReader();

  /**
   * @brief Returns the dataset's ID. Returns 0 if the object is invalid.
   * @return H5::IdType
   */
  H5::IdType getId() const override;

  /**
   * @brief Returns the dataspace's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  H5::IdType getDataspaceId() const;

  /**
   * @brief Returns an enum representation of the attribute's type.
   * @return H5::Type
   */
  H5::Type getType() const;

  /**
   * @brief Returns a complex::DataType enum representation of the attribute's type.
   * @return H5::Type
   */
  Result<DataType> getDataType() const;

  /**
   * @brief Returns an HDF5 type ID for the target data type. Returns 0 if the
   * dataset is invalid.
   * @return H5::IdType
   */
  H5::IdType getTypeId() const;

  /**
   * @brief Returns the size of the datatype. Returns 0 if the dataset is invalid.
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
   * @brief Reads the dataset into the given span. Requires the span to be the correct size.
   * Returns false if unable to read.
   * @tparam T
   * @param data
   */
  template <class T>
  bool readIntoSpan(nonstd::span<T> data) const;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;

private:
  H5::IdType m_DatasetId = 0;
};
} // namespace H5
} // namespace complex
