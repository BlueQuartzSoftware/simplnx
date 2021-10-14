#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

class ITKIMAGEPROCESSING_EXPORT ITKImageProcessingPlugin : public complex::AbstractPlugin
{
public:
  ITKImageProcessingPlugin();
  ~ITKImageProcessingPlugin() override;

  ITKImageProcessingPlugin(const ITKImageProcessingPlugin&) = delete;
  ITKImageProcessingPlugin(ITKImageProcessingPlugin&&) = delete;

  ITKImageProcessingPlugin& operator=(const ITKImageProcessingPlugin&) = delete;
  ITKImageProcessingPlugin& operator=(ITKImageProcessingPlugin&&) = delete;

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
