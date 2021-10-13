#pragma once

#include <vector>

#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <nonstd/span.hpp>

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
   * @brief Writes a span of values to the dataset. Returns the HDF5 error,
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
  H5::ErrorType writeSpan(const DimsType& dims, nonstd::span<const T> values)
  {
    herr_t returnError = 0;
    int32_t rank = static_cast<int32_t>(dims.size());
    hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      std::cout << "dataType was unknown" << std::endl;
      return -1;
    }

    /* Get the type of object */
    // H5O_info_t objectInfo;
    // if(H5Oget_info_by_name(getParentId(), getName().c_str(), &objectInfo, H5P_DEFAULT) < 0)
    //{
    //  std::cout << "Error getting object info at locationId (" << getParentId() << ") with object name (" << getName() << ")" << std::endl;
    //  return -1;
    //}
    /* Open the object */
    // m_DatasetId = H5Dcreate(getParentId(), getName(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // if(m_DatasetId < 0)
    //{
    //  std::cout << "Error opening Object for Attribute operations." << std::endl;
    //  return -1;
    //}

    hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
    if(dataspaceId >= 0)
    {
      herr_t error = findAndDeleteAttribute();
      if(error < 0)
      {
        std::cout << "Error Removing Existing Attribute" << std::endl;
        returnError = error;
      }
      else
      {
        /* Create the attribute. */
        // hid_t attributeId = H5Acreate(getId(), getName().c_str(), dataType, dataspaceId, H5P_DEFAULT, H5P_DEFAULT);
        createOrOpenDataset(dataType, dataspaceId);
        if(m_DatasetId >= 0)
        {
          /* Write the attribute data. */
          const void* data = static_cast<const void*>(values.data());
          error = H5Dwrite(m_DatasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
          if(error < 0)
          {
            std::cout << "Error Writing Attribute" << std::endl;
            returnError = error;
          }
        }
        else
        {
          std::cout << "Error Creating Dataset" << std::endl;
          returnError = static_cast<herr_t>(m_DatasetId);
        }
        /* Close the attribute. */
        // error = H5Aclose(attributeId);
        // if(error < 0)
        //{
        //  std::cout << "Error Closing Attribute" << std::endl;
        //  returnError = error;
        //}
      }
      /* Close the dataspace. */
      error = H5Sclose(dataspaceId);
      if(error < 0)
      {
        std::cout << "Error Closing Dataspace" << std::endl;
        returnError = error;
      }
    }
    else
    {
      returnError = static_cast<herr_t>(dataspaceId);
    }
    return returnError;
  }

protected:
  /**
   * @brief Finds and deletes any existing attribute with the current name.
   * Returns any error that might occur when deleting the attribute.
   * @return H5::ErrorType
   */
  H5::ErrorType findAndDeleteAttribute();

  /**
   * @brief Opens the target HDF5 dataset or creates a new one using the given
   * datatype and dataspace IDs.
   * @param typeId
   * @param dataspaceId
   */
  void createOrOpenDataset(H5::IdType typeId, H5::IdType dataspaceId);

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
