#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AverageEdgeFaceCellArrayToVertexArrayInputValues inputValues;

  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.AverageVertexArrayPath = filterArgs.value<DataPath>(k_AverageVertexArrayPath_Key);

  return AverageEdgeFaceCellArrayToVertexArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT AverageEdgeFaceCellArrayToVertexArrayInputValues
{
  DataPath SelectedArrayPath;
  DataPath AverageVertexArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT AverageEdgeFaceCellArrayToVertexArray
{
public:
  AverageEdgeFaceCellArrayToVertexArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        AverageEdgeFaceCellArrayToVertexArrayInputValues* inputValues);
  ~AverageEdgeFaceCellArrayToVertexArray() noexcept;

  AverageEdgeFaceCellArrayToVertexArray(const AverageEdgeFaceCellArrayToVertexArray&) = delete;
  AverageEdgeFaceCellArrayToVertexArray(AverageEdgeFaceCellArrayToVertexArray&&) noexcept = delete;
  AverageEdgeFaceCellArrayToVertexArray& operator=(const AverageEdgeFaceCellArrayToVertexArray&) = delete;
  AverageEdgeFaceCellArrayToVertexArray& operator=(AverageEdgeFaceCellArrayToVertexArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AverageEdgeFaceCellArrayToVertexArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
