#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

class IMAGEPROCESSING_EXPORT ImageProcessingPlugin : public nx::core::AbstractPlugin
{
public:
  ImageProcessingPlugin();
  ~ImageProcessingPlugin() override;

  ImageProcessingPlugin(const ImageProcessingPlugin&) = delete;
  ImageProcessingPlugin(ImageProcessingPlugin&&) = delete;

  ImageProcessingPlugin& operator=(const ImageProcessingPlugin&) = delete;
  ImageProcessingPlugin& operator=(ImageProcessingPlugin&&) = delete;

  SIMPLMapType getSimplToSimplnxMap() const override;
  FilterReplacementMapType getFilterReplacementMap() const override;
};
