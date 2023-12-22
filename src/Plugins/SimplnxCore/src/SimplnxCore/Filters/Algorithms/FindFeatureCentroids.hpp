#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindFeatureCentroidsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath CentroidsArrayPath;
  DataPath ImageGeometryPath;
  DataPath FeatureAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT FindFeatureCentroids
{
public:
  FindFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeatureCentroidsInputValues* inputValues);
  ~FindFeatureCentroids() noexcept;

  FindFeatureCentroids(const FindFeatureCentroids&) = delete;
  FindFeatureCentroids(FindFeatureCentroids&&) noexcept = delete;
  FindFeatureCentroids& operator=(const FindFeatureCentroids&) = delete;
  FindFeatureCentroids& operator=(FindFeatureCentroids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
