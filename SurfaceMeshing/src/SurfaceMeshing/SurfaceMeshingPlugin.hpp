#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

class SURFACEMESHING_EXPORT SurfaceMeshingPlugin : public complex::AbstractPlugin
{
public:
  SurfaceMeshingPlugin();
  ~SurfaceMeshingPlugin() override;

  SurfaceMeshingPlugin(const SurfaceMeshingPlugin&) = delete;
  SurfaceMeshingPlugin(SurfaceMeshingPlugin&&) = delete;

  SurfaceMeshingPlugin& operator=(const SurfaceMeshingPlugin&) = delete;
  SurfaceMeshingPlugin& operator=(SurfaceMeshingPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
