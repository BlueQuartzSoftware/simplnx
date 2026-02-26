#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

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
