#pragma once

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp"

#include <nonstd/span.hpp>

#include <vector>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT DatasetWriter : public ObjectWriter
{
public:
  using DimsType = std::vector<SizeType>;

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
  DatasetWriter(IdType parentId, const std::string& datasetName);

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
  Result<> writeSpan(const DimsType& dims, nonstd::span<const T> values)
  {
    Result<> returnError = {};
    ErrorType error = 0;
    int32_t rank = static_cast<int32_t>(dims.size());
    hid_t dataType = Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-1, "DataType was unknown");
    }

    hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
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
        createOrOpenDataset(dataType, dataspaceId);
        if(getId() >= 0)
        {
          /* Write the attribute data. */
          const void* data = static_cast<const void*>(values.data());
          error = H5Dwrite(getId(), dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
          if(error < 0)
          {
            returnError = MakeErrorResult(error, "Error Writing Attribute");
          }
        }
        else
        {
          returnError = MakeErrorResult(getId(), "Error Creating Dataset");
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

  template <typename T>
  Result<> writeChunk(const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkShape, nonstd::span<const hsize_t> offset)
  {
    Result<> returnError = {};
    herr_t error = 0;
    int32_t rank = static_cast<int32_t>(dims.size());
    hid_t dataType = Support::HdfTypeForPrimitive<T>();
    if(dataType == -1)
    {
      return MakeErrorResult(-100, "DataType was unkown");
    }

    hid_t dataspaceId = H5Screate_simple(rank, dims.data(), nullptr);
    if(dataspaceId >= 0)
    {
      auto result = findAndDeleteAttribute();
      if(result.invalid())
      {
        returnError = MakeErrorResult(result.errors()[0].code, "Error Removing Existing Attribute");
      }
      else
      {
        /* Create the attribute. */
        createOrOpenDatasetChunk(dataType, dataspaceId, chunkShape);
        if(getId() >= 0)
        {
          auto plistId = getPListId();
          if(plistId <= 0)
          {
            std::cout << "Error Writing Chunk: No PList ID found" << std::endl;
          }
          /* Write the attribute data. */
          const void* data = static_cast<const void*>(values.data());
          auto properties = CreateTransferChunkProperties(chunkShape);
          // error = H5Dwrite_chunk(getId(), properties, H5P_DEFAULT, offset.data(), values.size(), data);
          error = H5Dwrite_chunk(getId(), H5P_DEFAULT, H5P_DEFAULT, offset.data(), values.size() * sizeof(T), data);
          if(error < 0)
          {
            returnError = MakeErrorResult(error, "Error Writing Dataset Chunk");
          }
        }
        else
        {
          returnError = MakeErrorResult(getId(), "Error Creating Dataset Chunk");
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

  /**
   * @brief Returns the property's HDF5 ID. Returns 0 if the attribute is
   * invalid.
   * @return IdType
   */
  IdType getPListId() const;

protected:
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
   * @param properties = 0
   */
  void createOrOpenDataset(IdType typeId, IdType dataspaceId, IdType properties = 0);

  void createOrOpenDatasetChunk(IdType typeId, IdType dataspaceId, const DimsType& chunkDims);

  /**
   * @brief Applies chunking to the dataset and sets the chunk dimensions.
   * @param dims
   * @return Returns the property ID if successful. Returns H5P_DEFAULT otherwise.
   */
  static IdType CreateDatasetChunkProperties(nonstd::span<const hsize_t> dims);

  static IdType CreateTransferChunkProperties(const DimsType& chunkDims);

  /**
   * @brief Closes the HDF5 dataset and resets the ID to 0.
   */
  void closeHdf5() override;

private:
  const std::string m_DatasetName;
};
} // namespace nx::core::HDF5
