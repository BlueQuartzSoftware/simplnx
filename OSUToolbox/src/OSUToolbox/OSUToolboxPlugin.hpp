#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "OSUToolbox/OSUToolbox_export.hpp"

class OSUTOOLBOX_EXPORT OSUToolboxPlugin : public complex::AbstractPlugin
{
public:
  OSUToolboxPlugin();
  ~OSUToolboxPlugin() override;

  OSUToolboxPlugin(const OSUToolboxPlugin&) = delete;
  OSUToolboxPlugin(OSUToolboxPlugin&&) = delete;

  OSUToolboxPlugin& operator=(const OSUToolboxPlugin&) = delete;
  OSUToolboxPlugin& operator=(OSUToolboxPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;

private:
  /**
   * @brief This will register all the filters that are contained in this plugin
   */
  void registerFilters();
};
