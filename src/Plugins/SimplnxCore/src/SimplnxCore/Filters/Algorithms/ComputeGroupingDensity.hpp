#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeGroupingDensityInputValues
{
  DataPath VolumesPath;
  DataPath ContiguousNLPath;
  bool UseNonContiguousNeighbors;
  DataPath NonContiguousNLPath;
  DataPath ParentIdsPath;
  DataPath ParentVolumesPath;
  bool FindCheckedFeatures;
  DataPath CheckedFeaturesPath;
  DataPath GroupingDensitiesPath;
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
