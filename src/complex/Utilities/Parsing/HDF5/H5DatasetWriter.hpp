#pragma once

#include <vector>

#include "complex/DataStructure/AbstractDataStore.hpp"
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

  static constexpr int64 OOC_CHUNK_SIZE = 1024;

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
        createOrOpenDataset(dataType, dataspaceId);
        if(getId() >= 0)
        {
          /* Write the attribute data. */
          const void* data = static_cast<const void*>(values.data());
          error = H5Dwrite(getId(), dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
          if(error < 0)
          {
            std::cout << "Error Writing Attribute" << std::endl;
            returnError = error;
          }
        }
        else
        {
          std::cout << "Error Creating Dataset" << std::endl;
          returnError = static_cast<herr_t>(getId());
        }
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

  template <typename T>
  H5::ErrorType writeOOCData(const DimsType& dims, const AbstractDataStore<T>& values)
  {
    herr_t returnError = 0;
    int32_t rank = static_cast<int32_t>(dims.size());
    hid_t dataType = H5::Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      std::cout << "dataType was unknown" << std::endl;
      return -1;
    }

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
        createOrOpenDataset(dataType, dataspaceId);
        if(m_DatasetId >= 0)
        {
          /* Write the attribute data. */
          /*
           * Set mem_space_id and iterate over const-sized chunks.
           * Update the start index on each iteration
           */
          const uint64 count = values.size();
          for(uint64 i = 0; i < count; i += OOC_CHUNK_SIZE)
          {
            std::runtime_error("Writing Out-of-Core data to HDF5 not implemented");
#if 0
            const void* data = static_cast<const void*>(values.data());
            error = H5Dwrite(m_DatasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
            if(error < 0)
            {
              std::cout << "Error Writing Attribute" << std::endl;
              returnError = error;
            }
#endif
          }
        }
        else
        {
          std::cout << "Error Creating Dataset" << std::endl;
          returnError = static_cast<herr_t>(m_DatasetId);
        }
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

  /**
   * @brief Closes the HDF5 dataset and resets the ID to 0.
   */
  void closeHdf5() override;

private:
#if 0
  bool tryOpeningDataset(const std::string& datasetName, H5::Type dataType);
  bool tryCreatingDataset(const std::string& datasetName, H5::Type dataType);
#endif

  const std::string m_DatasetName;
};
} // namespace H5
} // namespace complex
