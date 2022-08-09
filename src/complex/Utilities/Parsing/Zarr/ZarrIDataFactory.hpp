#pragma once

#include <optional>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"

namespace FileVec
{
class BaseCollection;
class Group;
} // namespace FileVec

namespace complex
{
namespace Zarr
{
class DataStructureReader;

class COMPLEX_EXPORT IDataFactory
{
public:
  using error_type = int64_t;

  virtual ~IDataFactory();

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const = 0;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target Zarr object.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param parentReader Wrapper around the parent Zarr group.
   * @param baseReader Wrapper around an Zarr object reader.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @param preflight = false
   * @return error_type
   */
  virtual error_type read(Zarr::DataStructureReader& dataStructureReader, const FileVec::Group& parentReader, const FileVec::BaseCollection& baseReader,
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
   * Zarr::ObjectReader.
   * @param dataReader
   * @return DataObject::IdType
   */
  static DataObject::IdType ReadObjectId(const FileVec::BaseCollection& dataReader);
};
} // namespace Zarr
} // namespace complex
