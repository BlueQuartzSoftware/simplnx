#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT CreateAMScanPathsInputValues
{
  float32 stripe_width{};
  float32 hatch_spacing{};
  float32 slice_hatch_rotation_angle{};
  DataPath cad_slice_data_container_name;
  DataPath cad_slice_ids_array_path;
  DataPath cad_region_ids_array_path;
  DataPath hatch_data_container_name;
  DataObjectNameParameter::ValueType vertex_attribute_matrix_name;
  DataObjectNameParameter::ValueType hatch_attribute_matrix_name;
  DataObjectNameParameter::ValueType region_ids_array_name;
};

/**
 * @class CreateAMScanPaths
 * @brief This filter will generate additive manufacturing scan paths from an edge geometry
 */

class SIMPLNXCORE_EXPORT CreateAMScanPaths
{
public:
  CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues);
  ~CreateAMScanPaths() noexcept = default;

  CreateAMScanPaths(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths(CreateAMScanPaths&&) noexcept = delete;
  CreateAMScanPaths& operator=(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths& operator=(CreateAMScanPaths&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateAMScanPathsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
