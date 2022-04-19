#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

class SYNTHETICBUILDING_EXPORT SyntheticBuildingPlugin : public complex::AbstractPlugin
{
public:
  SyntheticBuildingPlugin();
  ~SyntheticBuildingPlugin() override;

  SyntheticBuildingPlugin(const SyntheticBuildingPlugin&) = delete;
  SyntheticBuildingPlugin(SyntheticBuildingPlugin&&) = delete;

  SyntheticBuildingPlugin& operator=(const SyntheticBuildingPlugin&) = delete;
  SyntheticBuildingPlugin& operator=(SyntheticBuildingPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
