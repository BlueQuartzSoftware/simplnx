#pragma once

#include <vector>

#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include "ProgWorkshop/ProgWorkshop_export.hpp"

class PROGWORKSHOP_EXPORT ProgWorkshopPlugin : public complex::AbstractPlugin
{
public:
  ProgWorkshopPlugin();
  ~ProgWorkshopPlugin() override;

  ProgWorkshopPlugin(const ProgWorkshopPlugin&) = delete;
  ProgWorkshopPlugin(ProgWorkshopPlugin&&) = delete;

  ProgWorkshopPlugin& operator=(const ProgWorkshopPlugin&) = delete;
  ProgWorkshopPlugin& operator=(ProgWorkshopPlugin&&) = delete;

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
