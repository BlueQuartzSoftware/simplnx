#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nx::core
{
/**
 * @class IDataStore
 * @brief The IDataStore class serves as an interface class for the
 * various types of data stores used in DataArrays. The basic API and iterators
 * are defined but the specifics relating to how data is stored are implemented
 * in subclasses.
 */
class SIMPLNX_EXPORT IDataStore
{
public:
  using ShapeType = typename std::vector<usize>;

  enum class StoreType : int32
  {
    InMemory = 0,
    OutOfCore,
    Empty,
    EmptyOutOfCore
  };

  virtual ~IDataStore() = default;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return usize
   */
  virtual usize getNumberOfTuples() const = 0;
  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  virtual const ShapeType& getTupleShape() const = 0;

  /**
   * @brief Returns the number of components.
   * @return usize
   */
  virtual usize getNumberOfComponents() const = 0;

  /**
   * @brief Returns the dimensions of the Components
   * @return
   */
  virtual const ShapeType& getComponentShape() const = 0;

  /**
   * @brief Returns the chunk shape if the DataStore is separated into chunks.
   * If the DataStore does not have chunks, this method returns a null optional.
   * @return optional Shapetype
   */
  virtual std::optional<ShapeType> getChunkShape() const = 0;

  /**
   * @brief Returns the number of values stored within the DataStore.
   * @return usize
   */
  usize getSize() const
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Returns the number of values stored within the DataStore.
   * @return usize
   */
  usize size() const
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Resizes the DataStore to handle the specified number of tuples.
   * @param numTuples
   */
  virtual void resizeTuples(const ShapeType& tupleShape) = 0;

  /**
   * @brief Returns the DataStore's DataType as an enum
   * @return DataType
   */
  virtual DataType getDataType() const = 0;

  /**
   * @brief Returns the store type e.g. in memory, out of core, etc.
   * @return StoreType
   */
  virtual StoreType getStoreType() const = 0;

  /**
   * @brief Returns the data format used for storing the array data.
   * @return data format as string
   */
  virtual std::string getDataFormat() const
  {
    return "";
  }

  /**
   * @brief Returns the size of the stored type of the data store.
   * @return usize
   */
  virtual usize getTypeSize() const = 0;

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  virtual std::unique_ptr<IDataStore> deepCopy() const = 0;

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  virtual std::unique_ptr<IDataStore> createNewInstance() const = 0;

  /**
   * @brief Writes a binary file to the specified file path.
   * @param absoluteFilePath
   * @return std::pair<int32, std::string>
   */
  virtual std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const = 0;

  /**
   * @brief Writes a binary file using the specified output stream.
   * @param outputStream
   * @return std::pair<int32, std::string>
   */
  virtual std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const = 0;

protected:
  /**
   * @brief Default constructor
   */
  IDataStore() = default;
};
} // namespace nx::core
