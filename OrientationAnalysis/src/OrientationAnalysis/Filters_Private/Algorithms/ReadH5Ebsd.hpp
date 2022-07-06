#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadH5EbsdInputValues inputValues;

  inputValues.ReadH5Ebsd = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ReadH5Ebsd_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);

  return ReadH5Ebsd(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ReadH5EbsdInputValues
{
  <<<NOT_IMPLEMENTED>>> ReadH5Ebsd;
  /*[x]*/ DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath CellEnsembleAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ReadH5Ebsd
{
public:
  ReadH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5EbsdInputValues* inputValues);
  ~ReadH5Ebsd() noexcept;

  ReadH5Ebsd(const ReadH5Ebsd&) = delete;
  ReadH5Ebsd(ReadH5Ebsd&&) noexcept = delete;
  ReadH5Ebsd& operator=(const ReadH5Ebsd&) = delete;
  ReadH5Ebsd& operator=(ReadH5Ebsd&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadH5EbsdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
