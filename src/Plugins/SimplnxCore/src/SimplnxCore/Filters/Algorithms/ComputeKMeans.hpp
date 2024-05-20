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

struct SIMPLNXCORE_EXPORT ComputeKMeansInputValues
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
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeKMeans
{
public:
  ComputeKMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeKMeansInputValues* inputValues);
  ~ComputeKMeans() noexcept;

  ComputeKMeans(const ComputeKMeans&) = delete;
  ComputeKMeans(ComputeKMeans&&) noexcept = delete;
  ComputeKMeans& operator=(const ComputeKMeans&) = delete;
  ComputeKMeans& operator=(ComputeKMeans&&) noexcept = delete;

  Result<> operator()();
  void updateProgress(const std::string& message);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeKMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
