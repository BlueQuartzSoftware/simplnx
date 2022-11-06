#pragma once

#include "Generic/Generic_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindSurfaceFeaturesInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.SurfaceFeaturesArrayPath = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);

  return FindSurfaceFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct GENERIC_EXPORT FindSurfaceFeaturesInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath SurfaceFeaturesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class GENERIC_EXPORT FindSurfaceFeatures
{
public:
  FindSurfaceFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSurfaceFeaturesInputValues* inputValues);
  ~FindSurfaceFeatures() noexcept;

  FindSurfaceFeatures(const FindSurfaceFeatures&) = delete;
  FindSurfaceFeatures(FindSurfaceFeatures&&) noexcept = delete;
  FindSurfaceFeatures& operator=(const FindSurfaceFeatures&) = delete;
  FindSurfaceFeatures& operator=(FindSurfaceFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
