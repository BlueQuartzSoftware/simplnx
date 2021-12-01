#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "TestOne/TestOne_export.hpp"

class TESTONE_EXPORT TestOnePlugin : public complex::AbstractPlugin
{
public:
  TestOnePlugin();
  ~TestOnePlugin() override;

  TestOnePlugin(const TestOnePlugin&) = delete;
  TestOnePlugin(TestOnePlugin&&) = delete;

  TestOnePlugin& operator=(const TestOnePlugin&) = delete;
  TestOnePlugin& operator=(TestOnePlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::H5::IDataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
