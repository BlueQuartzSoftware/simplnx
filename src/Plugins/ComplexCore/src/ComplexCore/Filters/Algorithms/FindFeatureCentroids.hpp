#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindFeatureCentroidsInputValues
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

class COMPLEXCORE_EXPORT FindFeatureCentroids
{
public:
  FindFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeatureCentroidsInputValues* inputValues);
  ~FindFeatureCentroids() noexcept;

  FindFeatureCentroids(const FindFeatureCentroids&) = delete;
  FindFeatureCentroids(FindFeatureCentroids&&) noexcept = delete;
  FindFeatureCentroids& operator=(const FindFeatureCentroids&) = delete;
  FindFeatureCentroids& operator=(FindFeatureCentroids&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Allows thread safe progress updates
   * @param counter
   */
  void sendThreadSafeProgressMessage(size_t counter);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
};

} // namespace complex
