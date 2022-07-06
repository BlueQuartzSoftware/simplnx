#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DecimatePointCloudInputValues inputValues;

  inputValues.DecimationFreq = filterArgs.value<float32>(k_DecimationFreq_Key);
  inputValues.VertexAttrMatPath = filterArgs.value<DataPath>(k_VertexAttrMatPath_Key);

  return DecimatePointCloud(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT DecimatePointCloudInputValues
{
  float32 DecimationFreq;
  DataPath VertexAttrMatPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT DecimatePointCloud
{
public:
  DecimatePointCloud(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DecimatePointCloudInputValues* inputValues);
  ~DecimatePointCloud() noexcept;

  DecimatePointCloud(const DecimatePointCloud&) = delete;
  DecimatePointCloud(DecimatePointCloud&&) noexcept = delete;
  DecimatePointCloud& operator=(const DecimatePointCloud&) = delete;
  DecimatePointCloud& operator=(DecimatePointCloud&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DecimatePointCloudInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
