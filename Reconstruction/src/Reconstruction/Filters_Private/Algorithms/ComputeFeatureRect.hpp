#pragma once

#include "Reconstruction/Reconstruction_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ComputeFeatureRectInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureRectArrayPath = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);

  return ComputeFeatureRect(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct RECONSTRUCTION_EXPORT ComputeFeatureRectInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureRectArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class RECONSTRUCTION_EXPORT ComputeFeatureRect
{
public:
  ComputeFeatureRect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureRectInputValues* inputValues);
  ~ComputeFeatureRect() noexcept;

  ComputeFeatureRect(const ComputeFeatureRect&) = delete;
  ComputeFeatureRect(ComputeFeatureRect&&) noexcept = delete;
  ComputeFeatureRect& operator=(const ComputeFeatureRect&) = delete;
  ComputeFeatureRect& operator=(ComputeFeatureRect&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureRectInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
