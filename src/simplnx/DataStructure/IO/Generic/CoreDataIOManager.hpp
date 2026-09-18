#pragma once

#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @namespace nx::core::Generic
 * @brief Contains core generic data I/O implementations.
 */
namespace Generic
{

/**
 * @class CoreDataIOManager
 * @brief Creates resident DataStore and ListStore implementations.
 *
 * The reserved in-memory format name distinguishes an explicit resident request
 * from an empty automatic-format request.
 */
class SIMPLNX_EXPORT CoreDataIOManager : public IDataIOManager
{
public:
  /**
   * @brief Registers resident store factory functions.
   */
  CoreDataIOManager();

  /**
   * @brief Destroys the core data I/O manager.
   */
  ~CoreDataIOManager() noexcept override;

  /**
   * @brief Returns the reserved explicit in-memory format name.
   * @return Preferences::k_InMemoryFormat.
   */
  std::string formatName() const override;

private:
  /**
   * @brief Runs the reserved core DataObject-factory hook.
   *
   * The current manager registers only store factory functions.
   */
  void addCoreFactories();

  /**
   * @brief Registers resident numeric and Boolean DataStore factories.
   */
  void addDataStoreFnc();

  /**
   * @brief Registers supported resident numeric ListStore factories.
   */
  void addListStoreFnc();

  factory_collection m_FactoryCollection;
};
} // namespace Generic
} // namespace nx::core
