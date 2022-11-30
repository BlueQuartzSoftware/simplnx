#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "Core/Core_export.hpp"

class CORE_EXPORT CorePlugin : public complex::AbstractPlugin
{
public:
  CorePlugin();
  ~CorePlugin() override;

  CorePlugin(const CorePlugin&) = delete;
  CorePlugin(CorePlugin&&) = delete;

  CorePlugin& operator=(const CorePlugin&) = delete;
  CorePlugin& operator=(CorePlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return std::map<complex::Uuid, complex::Uuid>
   */
  std::map<complex::Uuid, complex::Uuid> getSimplToComplexMap() const override;
};
