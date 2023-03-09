#pragma once

#include "complex/DataStructure/IO/Generic/IDataIOManager.hpp"

namespace complex::HDF5
{
/**
 * @brief The HDF5::DataIOManager class stores and returns the corresponding IO class for HDF5.
 */
class COMPLEX_EXPORT DataIOManager : public IDataIOManager
{
public:
  /**
   * @brief Constructs a DattaIOManager and adds core IO classes.
   */
  DataIOManager();
  virtual ~DataIOManager() noexcept;

  /**
   * @brief Returns the format name for the IDataIOManager as a string.
   * @return std::string
   */
  std::string formatName() const override;

private:
  /**
   * @brief Adds all core complex DataObject IO classes.
   */
  void addCoreFactories();

  factory_collection m_FactoryCollection;
};
} // namespace complex::HDF5
