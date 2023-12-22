#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/KUtilities.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT KMedoidsInputValues
{
  uint64 InitClusters;
  KUtilities::DistanceMetric DistanceMetric;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MedoidsArrayPath;
  uint64 Seed;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT KMedoids
{
public:
  KMedoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMedoidsInputValues* inputValues);
  ~KMedoids() noexcept;

  KMedoids(const KMedoids&) = delete;
  KMedoids(KMedoids&&) noexcept = delete;
  KMedoids& operator=(const KMedoids&) = delete;
  KMedoids& operator=(KMedoids&&) noexcept = delete;

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
