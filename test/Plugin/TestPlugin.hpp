#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

#include "test/testplugin_export.hpp"

class TESTPLUGIN_EXPORT TestPlugin : public complex::AbstractPlugin
{
public:
  TestPlugin();
  ~TestPlugin() override;

  TestPlugin(const TestPlugin&) = delete;
  TestPlugin(TestPlugin&&) = delete;

  TestPlugin& operator=(const TestPlugin&) = delete;
  TestPlugin& operator=(TestPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::IH5DataFactory*> getDataFactories() const override;
};
