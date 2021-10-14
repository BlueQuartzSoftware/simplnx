#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "ImageProcessing/ImageProcessing_export.hpp"

class IMAGEPROCESSING_EXPORT ImageProcessingPlugin : public complex::AbstractPlugin
{
public:
  ImageProcessingPlugin();
  ~ImageProcessingPlugin() override;

  ImageProcessingPlugin(const ImageProcessingPlugin&) = delete;
  ImageProcessingPlugin(ImageProcessingPlugin&&) = delete;

  ImageProcessingPlugin& operator=(const ImageProcessingPlugin&) = delete;
  ImageProcessingPlugin& operator=(ImageProcessingPlugin&&) = delete;

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
