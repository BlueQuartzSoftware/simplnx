#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

class NXIMAGEPROCESSING_EXPORT NXImageProcessingPlugin : public nx::core::AbstractPlugin
{
public:
  NXImageProcessingPlugin();
  ~NXImageProcessingPlugin() override;

  NXImageProcessingPlugin(const NXImageProcessingPlugin&) = delete;
  NXImageProcessingPlugin(NXImageProcessingPlugin&&) = delete;

  NXImageProcessingPlugin& operator=(const NXImageProcessingPlugin&) = delete;
  NXImageProcessingPlugin& operator=(NXImageProcessingPlugin&&) = delete;

  SIMPLMapType getSimplToSimplnxMap() const override;
  FilterReplacementMapType getFilterReplacementMap() const override;
};
