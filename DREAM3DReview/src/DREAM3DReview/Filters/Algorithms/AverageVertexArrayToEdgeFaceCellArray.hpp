#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AverageVertexArrayToEdgeFaceCellArrayInputValues inputValues;

  inputValues.WeightedAverage = filterArgs.value<bool>(k_WeightedAverage_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.AverageCellArrayPath = filterArgs.value<DataPath>(k_AverageCellArrayPath_Key);

  return AverageVertexArrayToEdgeFaceCellArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT AverageVertexArrayToEdgeFaceCellArrayInputValues
{
  bool WeightedAverage;
  DataPath SelectedArrayPath;
  DataPath AverageCellArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT AverageVertexArrayToEdgeFaceCellArray
{
public:
  AverageVertexArrayToEdgeFaceCellArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        AverageVertexArrayToEdgeFaceCellArrayInputValues* inputValues);
  ~AverageVertexArrayToEdgeFaceCellArray() noexcept;

  AverageVertexArrayToEdgeFaceCellArray(const AverageVertexArrayToEdgeFaceCellArray&) = delete;
  AverageVertexArrayToEdgeFaceCellArray(AverageVertexArrayToEdgeFaceCellArray&&) noexcept = delete;
  AverageVertexArrayToEdgeFaceCellArray& operator=(const AverageVertexArrayToEdgeFaceCellArray&) = delete;
  AverageVertexArrayToEdgeFaceCellArray& operator=(AverageVertexArrayToEdgeFaceCellArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AverageVertexArrayToEdgeFaceCellArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
