#pragma once

#include <vector>

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
};
