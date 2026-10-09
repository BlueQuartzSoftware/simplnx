#pragma once

#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/**
 * @namespace watershed_preflight_test
 * @brief Contains metadata-only fixtures for watershed preflight checks.
 */
namespace watershed_preflight_test
{
/**
 * @class ScopedMissingTemporaryProvider
 * @brief Replaces optional managers with metadata-only managers until scope exit.
 *
 * The fixture keeps each map key and the built-in in-memory manager unchanged. It allocates all replacements before the first swap.
 * @note The collection must outlive this guard. No other caller can access or modify the collection while this guard exists.
 */
class ScopedMissingTemporaryProvider
{
public:
  /**
   * @brief Removes temporary-record capability without creating or deleting registry entries.
   * @param collection Registry to isolate for one serial preflight check.
   * @throws std::bad_alloc If replacement allocation fails before any registry slot changes.
   */
  explicit ScopedMissingTemporaryProvider(nx::core::DataIOCollection& collection)
  {
    m_Replacements.reserve(static_cast<std::size_t>(std::distance(collection.begin(), collection.end())));
    for(auto& [name, manager] : collection)
    {
      if(name != nx::core::Preferences::k_InMemoryFormat)
      {
        m_Replacements.push_back({&manager, std::make_shared<MetadataOnlyManager>(name)});
      }
    }

    // All remaining operations are no-throw swaps, so construction cannot leave a partially changed registry.
    for(auto& replacement : m_Replacements)
    {
      replacement.slot->swap(replacement.manager);
    }
  }

  /**
   * @brief Restores the original manager objects without allocation or changes to map keys.
   */
  ~ScopedMissingTemporaryProvider() noexcept
  {
    for(auto& replacement : m_Replacements)
    {
      replacement.slot->swap(replacement.manager);
    }
  }

  ScopedMissingTemporaryProvider(const ScopedMissingTemporaryProvider&) = delete;
  ScopedMissingTemporaryProvider(ScopedMissingTemporaryProvider&&) = delete;
  ScopedMissingTemporaryProvider& operator=(const ScopedMissingTemporaryProvider&) = delete;
  ScopedMissingTemporaryProvider& operator=(ScopedMissingTemporaryProvider&&) = delete;

private:
  /**
   * @class MetadataOnlyManager
   * @brief Registers a numeric format that rejects all value-store creation.
   */
  class MetadataOnlyManager : public nx::core::IDataIOManager
  {
  public:
    /**
     * @brief Makes the existing format available for metadata validation only.
     * @param format Existing registry key to preserve.
     */
    explicit MetadataOnlyManager(std::string format)
    : m_Format(std::move(format))
    {
      addDataStoreCreationFnc(m_Format,
                              [](nx::core::DataType, const nx::core::ShapeType&, const nx::core::ShapeType&, const std::optional<nx::core::ShapeType>&, nx::core::DataStoreInitializationMode)
                                  -> std::unique_ptr<nx::core::IDataStore> { throw std::runtime_error("Watershed preflight must not create a value store from its metadata-only format"); });
    }

    std::string formatName() const override
    {
      return m_Format;
    }

  private:
    std::string m_Format;
  };

  struct Replacement
  {
    std::shared_ptr<nx::core::IDataIOManager>* slot = nullptr;
    std::shared_ptr<nx::core::IDataIOManager> manager;
  };

  std::vector<Replacement> m_Replacements;
};
} // namespace watershed_preflight_test
