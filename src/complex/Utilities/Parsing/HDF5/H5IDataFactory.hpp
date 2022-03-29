#pragma once

#include <optional>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"

namespace complex
{
namespace H5
{
class DataStructureReader;
class DatasetReader;
class GroupReader;
class ObjectReader;

/**
 * @class H5::IDataFactory
 * @brief
 */
class COMPLEX_EXPORT IDataFactory
{
public:
  virtual ~IDataFactory();

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @param preflight = false
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                    const std::optional<complex::DataObject::IdType>& parentId = {}, bool preflight = false) = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param datasetReader Wrapper around the HDF5 dataset.
   * @param parentId = {}
   * @param preflight = false
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                      const std::optional<complex::DataObject::IdType>& parentId = {}, bool preflight = false) = 0;

  // Copy and move constuctors / operators deleted
  IDataFactory(const IDataFactory& other) = delete;
  IDataFactory(IDataFactory&& other) = delete;
  IDataFactory& operator=(const IDataFactory& rhs) = delete;
  IDataFactory& operator=(IDataFactory&& rhs) = delete;

protected:
  /**
   * @brief Default constructor
   */
  IDataFactory();

  /**
   * @brief Reads and returns the target DataObject ID from a given
   * H5::ObjectReader.
   * @param dataReader
   * @return DataObject::IdType
   */
  static DataObject::IdType ReadObjectId(const H5::ObjectReader& dataReader);
};
} // namespace H5
} // namespace complex
