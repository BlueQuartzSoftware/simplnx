#pragma once

#include <optional>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

namespace complex
{
namespace H5
{
class DataStructureReader;
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
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readDataStructureGroup(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId = {}) = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param datasetReader Wrapper around the HDF5 dataset.
   * @param parentId = {}
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readDataStructureDataset(H5::DataStructureReader& dataStructureReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId = {}) = 0;

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
