#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  IdentifyDislocationSegmentsInputValues inputValues;

  inputValues.BurgersVectorsArrayPath = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  inputValues.SlipPlaneNormalsArrayPath = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  inputValues.DislocationIdsArrayName = filterArgs.value<DataPath>(k_DislocationIdsArrayName_Key);
  inputValues.EdgeFeatureAttributeMatrixName = filterArgs.value<DataPath>(k_EdgeFeatureAttributeMatrixName_Key);
  inputValues.ActiveArrayName = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  return IdentifyDislocationSegments(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT IdentifyDislocationSegmentsInputValues
{
  DataPath BurgersVectorsArrayPath;
  DataPath SlipPlaneNormalsArrayPath;
  DataPath DislocationIdsArrayName;
  DataPath EdgeFeatureAttributeMatrixName;
  DataPath ActiveArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT IdentifyDislocationSegments
{
public:
  IdentifyDislocationSegments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifyDislocationSegmentsInputValues* inputValues);
  ~IdentifyDislocationSegments() noexcept;

  IdentifyDislocationSegments(const IdentifyDislocationSegments&) = delete;
  IdentifyDislocationSegments(IdentifyDislocationSegments&&) noexcept = delete;
  IdentifyDislocationSegments& operator=(const IdentifyDislocationSegments&) = delete;
  IdentifyDislocationSegments& operator=(IdentifyDislocationSegments&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IdentifyDislocationSegmentsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
