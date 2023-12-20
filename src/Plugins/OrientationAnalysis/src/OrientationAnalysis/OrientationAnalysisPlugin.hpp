#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

class ORIENTATIONANALYSIS_EXPORT OrientationAnalysisPlugin : public nx::core::AbstractPlugin
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
   * their simplnx counterpart
   * @return std::map<nx::core::Uuid, nx::core::Uuid>
   */
  SIMPLMapType getSimplToSimplnxMap() const override;
};
