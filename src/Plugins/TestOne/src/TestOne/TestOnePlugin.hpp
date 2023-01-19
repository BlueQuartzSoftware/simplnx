#pragma once

#include "TestOne/TestOne_export.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

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

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return std::map<complex::Uuid, complex::Uuid>
   */
  std::map<complex::Uuid, complex::Uuid> getSimplToComplexMap() const override;
};
