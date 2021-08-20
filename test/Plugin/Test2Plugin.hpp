#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

#include "test/test2plugin_export.hpp"

class TEST2PLUGIN_EXPORT Test2Plugin : public complex::AbstractPlugin
{
public:
  Test2Plugin();
  ~Test2Plugin() override;

  Test2Plugin(const Test2Plugin&) = delete;
  Test2Plugin(Test2Plugin&&) = delete;

  Test2Plugin& operator=(const Test2Plugin&) = delete;
  Test2Plugin& operator=(Test2Plugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::IH5DataFactory*> getDataFactories() const override;
};
