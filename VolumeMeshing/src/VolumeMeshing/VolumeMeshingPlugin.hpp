#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "VolumeMeshing/VolumeMeshing_export.hpp"

class VOLUMEMESHING_EXPORT VolumeMeshingPlugin : public complex::AbstractPlugin
{
public:
  VolumeMeshingPlugin();
  ~VolumeMeshingPlugin() override;

  VolumeMeshingPlugin(const VolumeMeshingPlugin&) = delete;
  VolumeMeshingPlugin(VolumeMeshingPlugin&&) = delete;

  VolumeMeshingPlugin& operator=(const VolumeMeshingPlugin&) = delete;
  VolumeMeshingPlugin& operator=(VolumeMeshingPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
