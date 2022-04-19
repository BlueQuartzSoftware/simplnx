#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "Generic/Generic_export.hpp"

class GENERIC_EXPORT GenericPlugin : public complex::AbstractPlugin
{
public:
  GenericPlugin();
  ~GenericPlugin() override;

  GenericPlugin(const GenericPlugin&) = delete;
  GenericPlugin(GenericPlugin&&) = delete;

  GenericPlugin& operator=(const GenericPlugin&) = delete;
  GenericPlugin& operator=(GenericPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
