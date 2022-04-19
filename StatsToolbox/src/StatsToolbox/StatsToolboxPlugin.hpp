#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "StatsToolbox/StatsToolbox_export.hpp"

class STATSTOOLBOX_EXPORT StatsToolboxPlugin : public complex::AbstractPlugin
{
public:
  StatsToolboxPlugin();
  ~StatsToolboxPlugin() override;

  StatsToolboxPlugin(const StatsToolboxPlugin&) = delete;
  StatsToolboxPlugin(StatsToolboxPlugin&&) = delete;

  StatsToolboxPlugin& operator=(const StatsToolboxPlugin&) = delete;
  StatsToolboxPlugin& operator=(StatsToolboxPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
