#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT KMedoidsInputValues
{
  uint64 InitClusters;
  ClusterUtilities::DistanceMetric DistanceMetric;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MedoidsArrayPath;
  uint64 Seed;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeKMedoids
{
public:
  ComputeKMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues);
  ~ComputeKMedoids() noexcept;

  ComputeKMedoids(const ComputeKMedoids&) = delete;
  ComputeKMedoids(ComputeKMedoids&&) noexcept = delete;
  ComputeKMedoids& operator=(const ComputeKMedoids&) = delete;
  ComputeKMedoids& operator=(ComputeKMedoids&&) noexcept = delete;

  Result<> operator()();
  void updateProgress(const std::string& message);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KMedoidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
