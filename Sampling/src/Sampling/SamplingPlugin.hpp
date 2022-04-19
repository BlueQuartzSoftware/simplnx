#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "Sampling/Sampling_export.hpp"

class SAMPLING_EXPORT SamplingPlugin : public complex::AbstractPlugin
{
public:
  SamplingPlugin();
  ~SamplingPlugin() override;

  SamplingPlugin(const SamplingPlugin&) = delete;
  SamplingPlugin(SamplingPlugin&&) = delete;

  SamplingPlugin& operator=(const SamplingPlugin&) = delete;
  SamplingPlugin& operator=(SamplingPlugin&&) = delete;

  /**
   * @brief Returns a collection of HDF5 DataStructure factories available
   * through the plugin.
   * @return std::vector<complex::IH5DataFactory*>
   */
  std::vector<complex::H5::IDataFactory*> getDataFactories() const override;
};
