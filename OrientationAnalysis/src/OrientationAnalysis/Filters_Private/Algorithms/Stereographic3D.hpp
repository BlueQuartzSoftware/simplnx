#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Stereographic3DInputValues inputValues;

  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CoordinatesArrayName = filterArgs.value<DataPath>(k_CoordinatesArrayName_Key);

  return Stereographic3D(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT Stereographic3DInputValues
{
  DataPath QuatsArrayPath;
  DataPath CoordinatesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT Stereographic3D
{
public:
  Stereographic3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Stereographic3DInputValues* inputValues);
  ~Stereographic3D() noexcept;

  Stereographic3D(const Stereographic3D&) = delete;
  Stereographic3D(Stereographic3D&&) noexcept = delete;
  Stereographic3D& operator=(const Stereographic3D&) = delete;
  Stereographic3D& operator=(Stereographic3D&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Stereographic3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
