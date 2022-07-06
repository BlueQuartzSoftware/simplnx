#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RemoveFlaggedVerticesInputValues inputValues;

  inputValues.VertexGeometry = filterArgs.value<DataPath>(k_VertexGeometry_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ReducedVertexGeometry = filterArgs.value<StringParameter::ValueType>(k_ReducedVertexGeometry_Key);

  return RemoveFlaggedVertices(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT RemoveFlaggedVerticesInputValues
{
  DataPath VertexGeometry;
  DataPath MaskArrayPath;
  StringParameter::ValueType ReducedVertexGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT RemoveFlaggedVertices
{
public:
  RemoveFlaggedVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedVerticesInputValues* inputValues);
  ~RemoveFlaggedVertices() noexcept;

  RemoveFlaggedVertices(const RemoveFlaggedVertices&) = delete;
  RemoveFlaggedVertices(RemoveFlaggedVertices&&) noexcept = delete;
  RemoveFlaggedVertices& operator=(const RemoveFlaggedVertices&) = delete;
  RemoveFlaggedVertices& operator=(RemoveFlaggedVertices&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedVerticesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
