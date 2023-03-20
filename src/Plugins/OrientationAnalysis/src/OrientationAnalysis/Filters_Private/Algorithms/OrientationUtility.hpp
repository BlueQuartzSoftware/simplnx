#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  OrientationUtilityInputValues inputValues;


  return OrientationUtility(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT OrientationUtilityInputValues
{
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT OrientationUtility
{
public:
  OrientationUtility(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, OrientationUtilityInputValues* inputValues);
  ~OrientationUtility() noexcept;

  OrientationUtility(const OrientationUtility&) = delete;
  OrientationUtility(OrientationUtility&&) noexcept = delete;
  OrientationUtility& operator=(const OrientationUtility&) = delete;
  OrientationUtility& operator=(OrientationUtility&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const OrientationUtilityInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
