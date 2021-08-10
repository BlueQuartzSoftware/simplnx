#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"

#include "test/testplugin_export.hpp"

class TESTPLUGIN_EXPORT TestPlugin : public complex::AbstractPlugin
{
public:
  static const IdType ID;

  TestPlugin();
  ~TestPlugin() override;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::IH5DataFactory*> getDataFactories() const override;
};

extern "C" {
TESTPLUGIN_EXPORT complex::AbstractPlugin* initPlugin();
}
