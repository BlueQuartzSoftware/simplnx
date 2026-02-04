#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ComputeFeaturePhasesBinaryInputValues inputValues;
  inputValues.CellDataAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(cell_data_attribute_matrix_path);
  inputValues.FeatureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_array_path);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(feature_phases_array_name);
  inputValues.MaskArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(mask_array_path);
  return ComputeFeaturePhasesBinary(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeaturePhasesBinaryInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellDataAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsArrayPath;
  DataObjectNameParameter::ValueType FeaturePhasesArrayName;
  ArraySelectionParameter::ValueType MaskArrayPath;
};

/**
 * @class ComputeFeaturePhasesBinary
 * @brief This algorithm implements support code for the ComputeFeaturePhasesBinaryFilter
 */

class SIMPLNXCORE_EXPORT ComputeFeaturePhasesBinary
{
public:
  ComputeFeaturePhasesBinary(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeaturePhasesBinaryInputValues* inputValues);
  ~ComputeFeaturePhasesBinary() noexcept;

  ComputeFeaturePhasesBinary(const ComputeFeaturePhasesBinary&) = delete;
  ComputeFeaturePhasesBinary(ComputeFeaturePhasesBinary&&) noexcept = delete;
  ComputeFeaturePhasesBinary& operator=(const ComputeFeaturePhasesBinary&) = delete;
  ComputeFeaturePhasesBinary& operator=(ComputeFeaturePhasesBinary&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeaturePhasesBinaryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
