#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
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
  /**
   * @brief Identifies how a data store manages its backing storage.
   *
   * Algorithms and I/O routines use this enum to determine whether data is
   * immediately accessible in RAM or must be fetched from disk, and to
   * distinguish real stores from preflight-only placeholders.
   *
   * - **InMemory** -- The store's data lives in a heap-allocated array that is
   *   always resident in RAM (DataStore<T>). Element access via getValue/setValue
   *   and the bulk copyIntoBuffer/copyFromBuffer API are both cheap memory
   *   copies.
   *
   * - **OutOfCore** -- The store's data lives on disk in a chunked HDF5 dataset.
   *   Element access goes through chunk caching; the bulk copyIntoBuffer/
   *   copyFromBuffer API translates flat ranges into efficient multi-chunk I/O.
   *   An earlier "EmptyOutOfCore" value was removed because the Empty type
   *   already covers placeholder semantics regardless of the eventual storage
   *   strategy.
   *
   * - **Empty** -- A metadata-only placeholder used during preflight
   *   (EmptyDataStore<T>). Records tuple/component shape but holds no data.
   *   All data access methods throw. After preflight the Empty store is
   *   replaced with an InMemory or OutOfCore store before execution begins.
   */
  enum class StoreType : int32
  {
    InMemory = 0, ///< Data is fully resident in a heap-allocated array (DataStore<T>)
    OutOfCore,    ///< Data lives on disk in a chunked HDF5 dataset
    Empty         ///< Metadata-only placeholder used during preflight (EmptyDataStore<T>)
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
    return getSize();
  }

  /**
   * @brief Returns if there are any elements in the array object
   * @return bool, true if the DataStore has a size() == 0
   */
  bool empty() const
  {
    return getNumberOfTuples() == 0;
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
   * @brief Returns store-specific metadata needed for crash recovery.
   *
   * When the pipeline runner writes a recovery (.dream3d) file at
   * the end of pipeline execution, it calls this method on every
   * data store to capture whatever information is needed to reconnect
   * the store to its data after a crash or unexpected termination.
   *
   * **In-memory stores (DataStore)** return an empty map because their
   * data is written directly into the recovery file's HDF5 datasets;
   * no extra metadata is required.
   *
   * **Out-of-core stores** return key-value pairs describing their
   * backing file path, HDF5 dataset path, chunk shape, and any other
   * parameters needed to reconstruct the OOC store from the file on
   * disk.
   *
   * Each key-value pair is written as an HDF5 string attribute on the
   * array's dataset inside the recovery file. The recovery loader
   * reads these attributes to reconstruct the appropriate store
   * subclass without loading the data into RAM.
   *
   * @return std::map<std::string, std::string> Key-value pairs of
   *         recovery metadata. Empty for in-memory stores.
   */
  virtual std::map<std::string, std::string> getRecoveryMetadata() const = 0;

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
