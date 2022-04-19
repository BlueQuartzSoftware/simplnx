#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "EMMPM/EMMPM_export.hpp"

class EMMPM_EXPORT EMMPMPlugin : public complex::AbstractPlugin
{
public:
  EMMPMPlugin();
  ~EMMPMPlugin() override;

  EMMPMPlugin(const EMMPMPlugin&) = delete;
  EMMPMPlugin(EMMPMPlugin&&) = delete;

  EMMPMPlugin& operator=(const EMMPMPlugin&) = delete;
  EMMPMPlugin& operator=(EMMPMPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
