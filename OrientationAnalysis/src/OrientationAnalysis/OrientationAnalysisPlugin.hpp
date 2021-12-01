#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

class ORIENTATIONANALYSIS_EXPORT OrientationAnalysisPlugin : public complex::AbstractPlugin
{
public:
  OrientationAnalysisPlugin();
  ~OrientationAnalysisPlugin() override;

  OrientationAnalysisPlugin(const OrientationAnalysisPlugin&) = delete;
  OrientationAnalysisPlugin(OrientationAnalysisPlugin&&) = delete;

  OrientationAnalysisPlugin& operator=(const OrientationAnalysisPlugin&) = delete;
  OrientationAnalysisPlugin& operator=(OrientationAnalysisPlugin&&) = delete;

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
