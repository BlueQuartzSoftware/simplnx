#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  LaplacianSmoothPointCloudInputValues inputValues;

  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.NumIterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.Lambda = filterArgs.value<float32>(k_Lambda_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return LaplacianSmoothPointCloud(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT LaplacianSmoothPointCloudInputValues
{
  bool UseMask;
  int32 NumIterations;
  float32 Lambda;
  DataPath DataContainerName;
  DataPath MaskArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT LaplacianSmoothPointCloud
{
public:
  LaplacianSmoothPointCloud(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LaplacianSmoothPointCloudInputValues* inputValues);
  ~LaplacianSmoothPointCloud() noexcept;

  LaplacianSmoothPointCloud(const LaplacianSmoothPointCloud&) = delete;
  LaplacianSmoothPointCloud(LaplacianSmoothPointCloud&&) noexcept = delete;
  LaplacianSmoothPointCloud& operator=(const LaplacianSmoothPointCloud&) = delete;
  LaplacianSmoothPointCloud& operator=(LaplacianSmoothPointCloud&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LaplacianSmoothPointCloudInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
