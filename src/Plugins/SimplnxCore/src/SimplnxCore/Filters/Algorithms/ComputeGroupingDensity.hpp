#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeGroupingDensityInputValues
{
  DataPath volumes_path;
  DataPath contiguous_nl_path;
  bool use_non_contiguous_neighbors{};
  DataPath non_contiguous_nl_path;
  DataPath parent_ids_path;
  DataPath parent_volumes_path;
  bool find_checked_features{};
  DataPath checked_features_path;
  DataPath grouping_densities_path;
};

/**
 * @class ComputeGroupingDensity
 * @brief Computes grouping densities for parent features in hierarchical reconstructions.
 */

class SIMPLNXCORE_EXPORT ComputeGroupingDensity
{
public:
  ComputeGroupingDensity(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGroupingDensityInputValues* inputValues);
  ~ComputeGroupingDensity() noexcept = default;

  ComputeGroupingDensity(const ComputeGroupingDensity&) = delete;
  ComputeGroupingDensity(ComputeGroupingDensity&&) noexcept = delete;
  ComputeGroupingDensity& operator=(const ComputeGroupingDensity&) = delete;
  ComputeGroupingDensity& operator=(ComputeGroupingDensity&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGroupingDensityInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
