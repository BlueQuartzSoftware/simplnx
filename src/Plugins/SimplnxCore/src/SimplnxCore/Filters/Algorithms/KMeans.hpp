#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/KUtilities.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT KMeansInputValues
{
  uint64 InitClusters;
  KUtilities::DistanceMetric DistanceMetric;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath MeansArrayPath;
  uint64 Seed;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT KMeans
{
public:
  KMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMeansInputValues* inputValues);
  ~KMeans() noexcept;

  KMeans(const KMeans&) = delete;
  KMeans(KMeans&&) noexcept = delete;
  KMeans& operator=(const KMeans&) = delete;
  KMeans& operator=(KMeans&&) noexcept = delete;

  Result<> operator()();
  void updateProgress(const std::string& message);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
