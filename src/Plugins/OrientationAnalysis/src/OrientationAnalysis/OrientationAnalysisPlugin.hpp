#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"

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
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their complex counterpart
   * @return std::map<complex::Uuid, complex::Uuid>
   */
  SIMPLMapType getSimplToComplexMap() const override;
};
