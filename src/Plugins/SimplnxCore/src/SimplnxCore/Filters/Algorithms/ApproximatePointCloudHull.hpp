#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ApproximatePointCloudHullInputValues
{
  VectorFloat32ParameterType::ValueType grid_resolution;
  GeometrySelectionParameter::ValueType input_vertex_geometry_path;
  UInt64ParameterType::ValueType min_empty_neighbors{};
  DataGroupCreationParameter::ValueType output_vertex_geometry_path;
};

/**
 * @class ApproximatePointCloudHull
 * @brief This algorithm implements support code for the ApproximatePointCloudHullFilter
 */

class SIMPLNXCORE_EXPORT ApproximatePointCloudHull
{
public:
  ApproximatePointCloudHull(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApproximatePointCloudHullInputValues* inputValues);
  ~ApproximatePointCloudHull() noexcept = default;

  ApproximatePointCloudHull(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull(ApproximatePointCloudHull&&) noexcept = delete;
  ApproximatePointCloudHull& operator=(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull& operator=(ApproximatePointCloudHull&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ApproximatePointCloudHullInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
