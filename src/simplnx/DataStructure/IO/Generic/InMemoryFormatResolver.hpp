#pragma once

#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @class InMemoryFormatResolver
 * @brief Selects in-memory storage for every object.
 *
 * libsimplnx installs this process-wide policy when an application does not
 * supply another resolver. The resolver ignores array size and location.
 */
class InMemoryFormatResolver : public IDataStoreFormatResolver
{
public:
  /**
   * @brief Selects the default in-memory store for every array.
   * @param dataStructure Unused containing DataStructure.
   * @param arrayPath Unused object path.
   * @param numericType Unused element data type.
   * @param dataSizeBytes Unused object size in bytes.
   * @return Empty format name, which selects the default in-memory store.
   *
   * Array-creation utilities interpret the empty name as the default in-memory format.
   */
  std::string resolveFormat(const DataStructure& /*dataStructure*/, const DataPath& /*arrayPath*/, DataType /*numericType*/, uint64 /*dataSizeBytes*/) const override
  {
    return {};
  }
};
} // namespace nx::core
