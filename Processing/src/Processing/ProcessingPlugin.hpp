#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "Processing/Processing_export.hpp"

class PROCESSING_EXPORT ProcessingPlugin : public complex::AbstractPlugin
{
public:
  ProcessingPlugin();
  ~ProcessingPlugin() override;

  ProcessingPlugin(const ProcessingPlugin&) = delete;
  ProcessingPlugin(ProcessingPlugin&&) = delete;

  ProcessingPlugin& operator=(const ProcessingPlugin&) = delete;
  ProcessingPlugin& operator=(ProcessingPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;

private:
  /**
   * @brief This will register all the filters that are contained in this plugin
   */
  void registerFilters();
};
