#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AlignGeometriesInputValues inputValues;

  inputValues.MovingGeometry = filterArgs.value<DataPath>(k_MovingGeometry_Key);
  inputValues.TargetGeometry = filterArgs.value<DataPath>(k_TargetGeometry_Key);
  inputValues.AlignmentType = filterArgs.value<ChoicesParameter::ValueType>(k_AlignmentType_Key);

  return AlignGeometries(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT AlignGeometriesInputValues
{
  DataPath MovingGeometry;
  DataPath TargetGeometry;
  ChoicesParameter::ValueType AlignmentType;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT AlignGeometries
{
public:
  AlignGeometries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignGeometriesInputValues* inputValues);
  ~AlignGeometries() noexcept;

  AlignGeometries(const AlignGeometries&) = delete;
  AlignGeometries(AlignGeometries&&) noexcept = delete;
  AlignGeometries& operator=(const AlignGeometries&) = delete;
  AlignGeometries& operator=(AlignGeometries&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AlignGeometriesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
