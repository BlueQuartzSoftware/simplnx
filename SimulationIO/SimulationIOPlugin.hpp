#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "SimulationIO/SimulationIO_export.hpp"

class SIMULATIONIO_EXPORT SimulationIOPlugin : public complex::AbstractPlugin
{
public:
  SimulationIOPlugin();
  ~SimulationIOPlugin() override;

  SimulationIOPlugin(const SimulationIOPlugin&) = delete;
  SimulationIOPlugin(SimulationIOPlugin&&) = delete;

  SimulationIOPlugin& operator=(const SimulationIOPlugin&) = delete;
  SimulationIOPlugin& operator=(SimulationIOPlugin&&) = delete;

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
