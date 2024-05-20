#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeatureCentroidsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath CentroidsArrayPath;
  DataPath ImageGeometryPath;
  DataPath FeatureAttributeMatrixPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeFeatureCentroids
{
public:
  ComputeFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureCentroidsInputValues* inputValues);
  ~ComputeFeatureCentroids() noexcept;

  ComputeFeatureCentroids(const ComputeFeatureCentroids&) = delete;
  ComputeFeatureCentroids(ComputeFeatureCentroids&&) noexcept = delete;
  ComputeFeatureCentroids& operator=(const ComputeFeatureCentroids&) = delete;
  ComputeFeatureCentroids& operator=(ComputeFeatureCentroids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
