#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @class IDataStore
 * @brief Defines shared metadata and lifecycle operations for data stores.
 */
class SIMPLNX_EXPORT IDataStore
{
public:
  /**
   * @enum StoreType
   * @brief Identifies the data-store residency state.
   *
   * Algorithms select storage-aware access paths from this state. Empty stores
   * preserve preflight metadata without values.
   * In-memory stores provide direct resident access.
   * Out-of-core stores use backing storage and benefit from bulk access instead of per-value I/O.
   * Execution replaces an empty store with its planned concrete storage before value access.
   */
  enum class StoreType : int32
  {
    InMemory = 0, ///< Selects resident data storage.
    OutOfCore,    ///< Selects nonresident backing storage.
    Empty         ///< Identifies a preflight metadata placeholder.
  };

  /**
   * @brief Destroys the data store.
   */
  virtual ~IDataStore() = default;

  virtual usize getNumberOfTuples() const = 0;
  /**
   * @brief Returns the tuple shape.
   * @return Reference that remains valid until the store changes shape or is destroyed.
   */
  virtual const ShapeType& getTupleShape() const = 0;

  virtual usize getNumberOfComponents() const = 0;

  /**
   * @brief Returns the component shape.
   * @return Reference that remains valid until the store is destroyed.
   */
  virtual const ShapeType& getComponentShape() const = 0;

  usize getSize() const
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  usize size() const
  {
    return getSize();
  }

  bool empty() const
  {
    return getNumberOfTuples() == 0;
  }

  /**
   * @brief Changes the tuple shape.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   * @return Valid on success. A resize failure returns error -6035.
   *
   * Callers must inspect the result because a failed resize can leave the prior shape and values unchanged.
   */
  [[nodiscard]] virtual Result<> resizeTuples(const ShapeType& tupleShape) = 0;

  virtual DataType getDataType() const = 0;

  virtual StoreType getStoreType() const = 0;

  /**
   * @brief Returns the store type that materializes after preflight.
   *
   * Real stores return their current type. EmptyDataStore exposes the planned
   * in-memory or out-of-core type without allocating data.
   * @return Current or planned store type.
   */
  virtual StoreType getPlannedStoreType() const
  {
    return getStoreType();
  }

  /**
   * @brief Returns the actual format or the recorded placeholder selection.
   * @return Empty string for memory, or the validated out-of-core format name.
   *
   * A placeholder returns its stored plan. The getter does not refresh resolver policy or preferences.
   */
  virtual std::string getDataFormat() const
  {
    return "";
  }

  /**
   * @brief Returns the physical chunk shape when the backing store exposes one.
   *
   * Algorithms may use this optional capability to align bounded bulk transfers without depending on a concrete
   * out-of-core implementation. Resident and unchunked stores return std::nullopt by default.
   */
  virtual std::optional<ShapeType> getChunkShape() const
  {
    return std::nullopt;
  }

  /**
   * @brief Returns store-specific metadata needed for crash recovery.
   *
   * In-memory stores return no metadata because recovery stores their values.
   * Out-of-core stores return the information needed to reopen backing data.
   * The recovery writer stores these pairs as HDF5 attributes.
   * The loader reconstructs the store without materializing its complete backing data.
   * @return Key-value recovery metadata.
   */
  virtual std::map<std::string, std::string> getRecoveryMetadata() const = 0;

  virtual usize getTypeSize() const = 0;

  /**
   * @brief Copies numeric storage into the required destination format.
   * @param destinationFormat Resolved destination format; empty and the canonical in-memory name select memory.
   * @return Independent values with the same shape, or an independent placeholder without values.
   * @throws std::runtime_error If the selected factory or copy fails, or an OOC build rejects an unavailable format.
   * @throws std::bad_alloc If an allocation fails.
   *
   * In-core builds use memory for unavailable formats. OOC builds reject unavailable formats. Factory failures never fall back.
   * The numeric transfer buffer uses at most 1 MiB. Backend buffers can hold a complete tuple.
   * @warning For materialized stores, explicit in-memory selection requires a complete resident destination and can exhaust available RAM.
   * This store obeys destinationFormat and does not resolve storage policy itself.
   * Resolve the destination policy before this call. Use DataArray::deepCopy for automatic policy selection.
   * @see DataArray::deepCopy
   */
  virtual std::unique_ptr<IDataStore> deepCopy(const std::string& destinationFormat) const = 0;

  /**
   * @brief Creates a store of the same concrete type.
   * @return Owning store with default values.
   */
  virtual std::unique_ptr<IDataStore> createNewInstance() const = 0;

  /**
   * @brief Writes store values to a binary file.
   * @param absoluteFilePath Destination file path.
   * @return Error code and message.
   */
  virtual std::pair<int32, std::string> writeBinaryFile(const std::string& absoluteFilePath) const = 0;

  /**
   * @brief Writes store values to a binary stream.
   * @param outputStream Destination stream.
   * @return Error code and message.
   */
  virtual std::pair<int32, std::string> writeBinaryFile(std::ostream& outputStream) const = 0;

protected:
  /**
   * @brief Creates a data store.
   */
  IDataStore() = default;
};
} // namespace nx::core
