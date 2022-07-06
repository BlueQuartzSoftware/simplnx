#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DiscretizeDDDomainInputValues inputValues;

  inputValues.CellSize = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  inputValues.EdgeDataContainerName = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  inputValues.OutputDataContainerName = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  inputValues.OutputAttributeMatrixName = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  inputValues.OutputArrayName = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  return DiscretizeDDDomain(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT DiscretizeDDDomainInputValues
{
  VectorFloat32Parameter::ValueType CellSize;
  DataPath EdgeDataContainerName;
  DataPath OutputDataContainerName;
  DataPath OutputAttributeMatrixName;
  DataPath OutputArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT DiscretizeDDDomain
{
public:
  DiscretizeDDDomain(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DiscretizeDDDomainInputValues* inputValues);
  ~DiscretizeDDDomain() noexcept;

  DiscretizeDDDomain(const DiscretizeDDDomain&) = delete;
  DiscretizeDDDomain(DiscretizeDDDomain&&) noexcept = delete;
  DiscretizeDDDomain& operator=(const DiscretizeDDDomain&) = delete;
  DiscretizeDDDomain& operator=(DiscretizeDDDomain&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DiscretizeDDDomainInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
