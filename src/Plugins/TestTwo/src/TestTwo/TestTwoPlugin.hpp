#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "TestTwo/TestTwo_export.hpp"

class TESTTWO_EXPORT TestTwoPlugin : public complex::AbstractPlugin
{
public:
  TestTwoPlugin();
  ~TestTwoPlugin() override;

  TestTwoPlugin(const TestTwoPlugin&) = delete;
  TestTwoPlugin(TestTwoPlugin&&) = delete;

  TestTwoPlugin& operator=(const TestTwoPlugin&) = delete;
  TestTwoPlugin& operator=(TestTwoPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::H5::IDataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return std::map<std::string, std::string>
   */
  std::map<std::string, std::string> getSimplToComplexMap() const override
  {
    return {};
  }

  /**
   * @brief Returns a map of UUIDs as strings, where Complex UUIDs are keys to
   * their SIMPL counterpart(s)
   * @return std::map<std::string, std::string>
   */
  std::map<std::string, std::string> getComplexToSimplMap() const override
  {
    return {};
  }
};
