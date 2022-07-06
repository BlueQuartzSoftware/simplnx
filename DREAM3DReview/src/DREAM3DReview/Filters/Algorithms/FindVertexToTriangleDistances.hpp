#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindVertexToTriangleDistancesInputValues inputValues;

  inputValues.VertexDataContainer = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  inputValues.TriangleDataContainer = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  inputValues.TriangleNormalsArrayPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  inputValues.DistancesArrayPath = filterArgs.value<DataPath>(k_DistancesArrayPath_Key);
  inputValues.ClosestTriangleIdArrayPath = filterArgs.value<DataPath>(k_ClosestTriangleIdArrayPath_Key);

  return FindVertexToTriangleDistances(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindVertexToTriangleDistancesInputValues
{
  DataPath VertexDataContainer;
  DataPath TriangleDataContainer;
  DataPath TriangleNormalsArrayPath;
  DataPath DistancesArrayPath;
  DataPath ClosestTriangleIdArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindVertexToTriangleDistances
{
public:
  FindVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindVertexToTriangleDistancesInputValues* inputValues);
  ~FindVertexToTriangleDistances() noexcept;

  FindVertexToTriangleDistances(const FindVertexToTriangleDistances&) = delete;
  FindVertexToTriangleDistances(FindVertexToTriangleDistances&&) noexcept = delete;
  FindVertexToTriangleDistances& operator=(const FindVertexToTriangleDistances&) = delete;
  FindVertexToTriangleDistances& operator=(FindVertexToTriangleDistances&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindVertexToTriangleDistancesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
