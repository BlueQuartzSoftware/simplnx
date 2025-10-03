#pragma once

#include <vector>

#include "simplnx/Plugin/AbstractPlugin.hpp"

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

class ITKIMAGEPROCESSING_EXPORT ITKImageProcessingPlugin : public nx::core::AbstractPlugin
{
public:
  ITKImageProcessingPlugin();
  ~ITKImageProcessingPlugin() noexcept override;

  ITKImageProcessingPlugin(const ITKImageProcessingPlugin&) = delete;
  ITKImageProcessingPlugin(ITKImageProcessingPlugin&&) = delete;

  ITKImageProcessingPlugin& operator=(const ITKImageProcessingPlugin&) = delete;
  ITKImageProcessingPlugin& operator=(ITKImageProcessingPlugin&&) = delete;

  static void RegisterITKImageIO();

  /**
   * @brief Returns a map of UUIDs as strings, where SIMPL UUIDs are keys to
   * their simplnx counterpart
   * @return std::map<nx::core::Uuid, nx::core::Uuid>
   */
  SIMPLMapType getSimplToSimplnxMap() const override;

  static std::vector<std::string> GetList2DSupportedFileExtensions();
};
