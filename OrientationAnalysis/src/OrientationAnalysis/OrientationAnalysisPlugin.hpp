#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

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

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return std::map<complex::Uuid, complex::Uuid>
   */
  std::map<complex::Uuid, complex::Uuid> getSimplToComplexMap() const override;
};
