#pragma once

#include <optional>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

namespace complex
{
namespace H5
{
class DatasetReader;
class GroupReader;
} // namespace H5

class COMPLEX_EXPORT IH5DataFactory
{
public:
  virtual ~IH5DataFactory();

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID
   * @param ds DataStructure to add the created DataObject to.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId = {}) = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID.
   * @param ds The target DataStructure to read from.
   * @param datasetReader Wrapper around the HDF5 dataset.
   * @param parentId = {}
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId = {}) = 0;

  // Copy and move constuctors / operators deleted
  IH5DataFactory(const IH5DataFactory& other) = delete;
  IH5DataFactory(IH5DataFactory&& other) = delete;
  IH5DataFactory& operator=(const IH5DataFactory& rhs) = delete;
  IH5DataFactory& operator=(IH5DataFactory&& rhs) = delete;

protected:
  /**
   * @brief Default constructor
   */
  IH5DataFactory();
};
} // namespace complex
