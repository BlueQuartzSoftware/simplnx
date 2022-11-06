#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeatureCentroidsInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);

  return FindFeatureCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT FindFeatureCentroidsInputValues
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

class CORE_EXPORT FindFeatureCentroids
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

} // namespace complex
