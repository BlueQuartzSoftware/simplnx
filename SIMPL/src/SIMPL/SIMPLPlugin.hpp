#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "SIMPL/SIMPL_export.hpp"

class SIMPL_EXPORT SIMPLPlugin : public complex::AbstractPlugin
{
public:
  SIMPLPlugin();
  ~SIMPLPlugin() override;

  SIMPLPlugin(const SIMPLPlugin&) = delete;
  SIMPLPlugin(SIMPLPlugin&&) = delete;

  SIMPLPlugin& operator=(const SIMPLPlugin&) = delete;
  SIMPLPlugin& operator=(SIMPLPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;

private:
  /**
   * @brief This will register all the filters that are contained in this plugin that are to be exposed to external programs
   */
  void registerPublicFilters();

  /**
   * @brief This will register all the filters that are contained in this plugin that are NOT to be exposed to external programs.
   * This will allow the filters to be compiled and tested but not exposed in the user interface
   */
  void registerPrivateFilters();
};
