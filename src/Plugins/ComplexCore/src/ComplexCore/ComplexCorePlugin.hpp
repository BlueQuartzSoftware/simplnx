#pragma once

#include <vector>

#include "ComplexCoreLegacyUUIDMapping.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "ComplexCore/ComplexCore_export.hpp"

class COMPLEXCORE_EXPORT ComplexCorePlugin : public complex::AbstractPlugin
{
public:
  ComplexCorePlugin();
  ~ComplexCorePlugin() override;

  ComplexCorePlugin(const ComplexCorePlugin&) = delete;
  ComplexCorePlugin(ComplexCorePlugin&&) = delete;

  ComplexCorePlugin& operator=(const ComplexCorePlugin&) = delete;
  ComplexCorePlugin& operator=(ComplexCorePlugin&&) = delete;

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
    return complex::k_SIMPL_to_ComplexCore;
  }

  /**
   * @brief Returns a map of UUIDs as strings, where Complex UUIDs are keys to
   * their SIMPL counterpart(s)
   * @return std::map<std::string, std::string>
   */
  std::map<std::string, std::string> getComplexToSimplMap() const override
  {
    return complex::k_ComplexCore_to_SIMPL;
  }
};
