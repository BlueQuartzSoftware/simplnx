#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

class ITKIMAGEPROCESSING_EXPORT ITKImageProcessingPlugin : public complex::AbstractPlugin
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
   * their complex counterpart
   * @return std::map<complex::Uuid, complex::Uuid>
   */
  SIMPLMapType getSimplToComplexMap() const override;

  static std::vector<std::string> GetList2DSupportedFileExtensions();
};
