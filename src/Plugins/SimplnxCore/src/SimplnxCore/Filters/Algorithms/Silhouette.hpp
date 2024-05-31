#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT SilhouetteInputValues
{
  ClusterUtilities::DistanceMetric DistanceMetric;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath SilhouetteArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT Silhouette
{
public:
  Silhouette(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SilhouetteInputValues* inputValues);
  ~Silhouette() noexcept;

  Silhouette(const Silhouette&) = delete;
  Silhouette(Silhouette&&) noexcept = delete;
  Silhouette& operator=(const Silhouette&) = delete;
  Silhouette& operator=(Silhouette&&) noexcept = delete;

  Result<> operator()();
  void updateProgress(const std::string& message);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SilhouetteInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
