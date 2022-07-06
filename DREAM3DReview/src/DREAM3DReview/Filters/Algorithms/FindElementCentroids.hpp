#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindElementCentroidsInputValues inputValues;

  inputValues.CreateVertexDataContainer = filterArgs.value<bool>(k_CreateVertexDataContainer_Key);
  inputValues.NewDataContainerName = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  inputValues.CellCentroidsArrayPath = filterArgs.value<DataPath>(k_CellCentroidsArrayPath_Key);

  return FindElementCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindElementCentroidsInputValues
{
  bool CreateVertexDataContainer;
  DataPath NewDataContainerName;
  DataPath VertexAttributeMatrixName;
  DataPath CellCentroidsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindElementCentroids
{
public:
  FindElementCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindElementCentroidsInputValues* inputValues);
  ~FindElementCentroids() noexcept;

  FindElementCentroids(const FindElementCentroids&) = delete;
  FindElementCentroids(FindElementCentroids&&) noexcept = delete;
  FindElementCentroids& operator=(const FindElementCentroids&) = delete;
  FindElementCentroids& operator=(FindElementCentroids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindElementCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
