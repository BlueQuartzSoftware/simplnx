#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EbsdToH5EbsdInputValues inputValues;

  inputValues.OrientationData = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OrientationData_Key);

  return EbsdToH5Ebsd(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT EbsdToH5EbsdInputValues
{
  <<<NOT_IMPLEMENTED>>> OrientationData;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT EbsdToH5Ebsd
{
public:
  EbsdToH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EbsdToH5EbsdInputValues* inputValues);
  ~EbsdToH5Ebsd() noexcept;

  EbsdToH5Ebsd(const EbsdToH5Ebsd&) = delete;
  EbsdToH5Ebsd(EbsdToH5Ebsd&&) noexcept = delete;
  EbsdToH5Ebsd& operator=(const EbsdToH5Ebsd&) = delete;
  EbsdToH5Ebsd& operator=(EbsdToH5Ebsd&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EbsdToH5EbsdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
