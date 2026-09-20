#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
class DataPath;
class DataStructure;

/**
 * @class IDataStoreFormatResolver
 * @brief Selects a storage format for array and NeighborList creation.
 *
 * Creation and import consult the resolver only when no explicit format is set.
 * The returned name must identify a registered DataIOCollection format. An empty
 * name selects the default in-memory format.
 *
 * One shared resolver can serve several DataStructures concurrently. Implementations
 * must make const calls thread-safe and must not depend on mutable request state.
 */
class SIMPLNX_EXPORT IDataStoreFormatResolver
{
public:
  /**
   * @brief Destroys the storage-format resolver.
   */
  virtual ~IDataStoreFormatResolver() noexcept;

  /**
   * @brief Selects the storage format for an array or NeighborList.
   * @param dataStructure Contains or will contain the object.
   * @param arrayPath Identifies the object location.
   * @param numericType Specifies the element data type.
   * @param dataSizeBytes Total array bytes, or zero when size is unknown.
   * @return Registered format name, or an empty name for the in-memory default.
   */
  virtual std::string resolveFormat(const DataStructure& dataStructure, const DataPath& arrayPath, DataType numericType, uint64 dataSizeBytes) const = 0;

protected:
  /**
   * @brief Constructs a storage-format resolver.
   */
  IDataStoreFormatResolver() = default;

  /**
   * @brief Copies a storage-format resolver.
   * @param other Resolver to copy.
   */
  IDataStoreFormatResolver(const IDataStoreFormatResolver& other) = default;

  /**
   * @brief Moves a storage-format resolver.
   * @param other Resolver to move.
   */
  IDataStoreFormatResolver(IDataStoreFormatResolver&& other) noexcept = default;

  /**
   * @brief Copies resolver state.
   * @param other Resolver to copy.
   * @return This resolver.
   */
  IDataStoreFormatResolver& operator=(const IDataStoreFormatResolver& other) = default;

  /**
   * @brief Moves resolver state.
   * @param other Resolver to move.
   * @return This resolver.
   */
  IDataStoreFormatResolver& operator=(IDataStoreFormatResolver&& other) noexcept = default;
};
} // namespace nx::core
