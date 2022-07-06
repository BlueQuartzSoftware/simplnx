#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindModulusMismatchInputValues inputValues;

  inputValues.ModuliArrayPath = filterArgs.value<DataPath>(k_ModuliArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshDeltaModulusArrayName = filterArgs.value<DataPath>(k_SurfaceMeshDeltaModulusArrayName_Key);

  return FindModulusMismatch(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT FindModulusMismatchInputValues
{
  DataPath ModuliArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshDeltaModulusArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT FindModulusMismatch
{
public:
  FindModulusMismatch(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindModulusMismatchInputValues* inputValues);
  ~FindModulusMismatch() noexcept;

  FindModulusMismatch(const FindModulusMismatch&) = delete;
  FindModulusMismatch(FindModulusMismatch&&) noexcept = delete;
  FindModulusMismatch& operator=(const FindModulusMismatch&) = delete;
  FindModulusMismatch& operator=(FindModulusMismatch&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindModulusMismatchInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
