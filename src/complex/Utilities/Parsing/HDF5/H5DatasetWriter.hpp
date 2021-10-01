#pragma once

#include <vector>

#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT DatasetWriter : public ObjectWriter
{
public:
  using DimsType = std::vector<H5::SizeType>;

  /**
   * @brief Constructs an invalid DatasetWriter.
   */
  DatasetWriter();

  /**
   * @brief Constructs a DatasetWriter nested under the given parent with the
   * specified name. Constructs an invalid DatasetWriter if the parentId is 0
   * or the datasetName is empty.
   * @param parentId
   * @param datasetName
   */
  DatasetWriter(H5::IdType parentId, const std::string& datasetName);

  /**
   * @brief Default destructor
   */
  virtual ~DatasetWriter();

  /**
   * @brief Returns true if the DatasetWriter has a valid target. Otherwise,
   * this method returns false.
   * @return bool
   */
  bool isValid() const override;

  /**
   * @brief Returns the dataset's HDF5 ID. Until one of the write* methods are
   * called, this method returns 0.
   * @return H5::IdType
   */
  H5::IdType getId() const override;

  /**
   * @brief Returns the target dataset name. Returns an empty string if the
   * DatasetWriter is invalid.
   * @return std::string
   */
  std::string getName() const override;

  /**
   * @brief Writes a given string to the dataset. Returns the HDF5 error,
   * should one occur.
   * 
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @param text
   * @return H5::ErrorType
   */
  H5::ErrorType writeString(const std::string& text);

  /**
   * @brief Writes a vector of strings to the dataset. Returns the HDF5 error,
   * should one occur.
   * 
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @param text
   * @return H5::ErrorType
   */
  H5::ErrorType writeVectorOfStrings(std::vector<std::string>& text);

  /**
   * @brief Writes a vector of values to the dataset. Returns the HDF5 error,
   * should one occur.
   * 
   * Any one of the write* methods must be called before adding attributes to
   * the HDF5 dataset.
   * @tparam T
   * @param dims
   * @param values
   * @return H5::ErrorType
   */
  template <typename T>
  H5::ErrorType writeVector(const DimsType& dims, const std::vector<T>& values);

protected:
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return H5::ErrorType
   */
  H5::ErrorType findAndDeleteAttribute();

private:
#if 0
  bool tryOpeningDataset(const std::string& datasetName, H5::Type dataType);
  bool tryCreatingDataset(const std::string& datasetName, H5::Type dataType);
#endif

  /**
   * @brief Closes the HDF5 dataset and resets the ID to 0.
   */
  void closeHdf5();

  const std::string m_DatasetName;
  hid_t m_DatasetId = 0;
};
} // namespace H5
} // namespace complex
