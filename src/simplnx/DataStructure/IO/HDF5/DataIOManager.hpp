#pragma once

#include "simplnx/DataStructure/IO/Generic/AbstractDataIOManager.hpp"

namespace nx::core::HDF5
{
/**
 * @brief The HDF5::DataIOManager class stores and returns the corresponding IO class for HDF5.
 */
class SIMPLNX_EXPORT DataIOManager : public AbstractDataIOManager
{
public:
  /**
   * @brief Constructs a DattaIOManager and adds core IO classes.
   */
  DataIOManager();
  ~DataIOManager() noexcept override;

  /**
   * @brief Returns the format name for the AbstractDataIOManager as a string.
   * @return std::string
   */
  std::string formatName() const override;

private:
  /**
   * @brief Adds all core simplnx AbstractDataObject IO classes.
   */
  void addCoreFactories();

  factory_collection m_FactoryCollection;
};
} // namespace nx::core::HDF5
