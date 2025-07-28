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
  ComputeFeaturePhasesInputValues inputValues;
  inputValues.CellFeaturesAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(cell_features_attribute_matrix_path);
  inputValues.CellPhasesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(cell_phases_array_path);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_path);
  inputValues.FeaturePhasesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(feature_phases_array_name);
  return ComputeFeaturePhases(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeaturePhasesInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeaturesAttributeMatrixPath;
  ArraySelectionParameter::ValueType CellPhasesArrayPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  DataObjectNameParameter::ValueType FeaturePhasesArrayName;
};

/**
 * @class ComputeFeaturePhases
 * @brief This algorithm implements support code for the ComputeFeaturePhasesFilter
 */

class SIMPLNXCORE_EXPORT ComputeFeaturePhases
{
public:
  ComputeFeaturePhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeaturePhasesInputValues* inputValues);
  ~ComputeFeaturePhases() noexcept;

  ComputeFeaturePhases(const ComputeFeaturePhases&) = delete;
  ComputeFeaturePhases(ComputeFeaturePhases&&) noexcept = delete;
  ComputeFeaturePhases& operator=(const ComputeFeaturePhases&) = delete;
  ComputeFeaturePhases& operator=(ComputeFeaturePhases&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeaturePhasesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
