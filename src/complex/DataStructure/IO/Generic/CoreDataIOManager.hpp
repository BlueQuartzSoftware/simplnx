#pragma once

#include "complex/DataStructure/IO/Generic/IDataIOManager.hpp"

namespace complex
{
namespace Generic
{
/**
 * @brief The HDF5::DataIOManager class stores and returns the corresponding IO class for HDF5.
 */
class COMPLEX_EXPORT CoreDataIOManager : public IDataIOManager
{
public:
  /**
   * @brief Constructs a DattaIOManager and adds core IO classes.
   */
  CoreDataIOManager();
  virtual ~CoreDataIOManager() noexcept;

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

  void addDataStoreFnc();

  factory_collection m_FactoryCollection;
};
} // namespace Generic
} // namespace complex
