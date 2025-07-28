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

struct SIMPLNXCORE_EXPORT ComputeBoundaryElementFractionsInputValues
{
  DataObjectNameParameter::ValueType BoundaryCellFractionsArrayName;
  ArraySelectionParameter::ValueType BoundaryCellsArrayPath;
  AttributeMatrixSelectionParameter::ValueType FeatureDataAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsArrayPath;
};

/**
 * @class ComputeBoundaryElementFractions
 * @brief This algorithm implements support code for the ComputeBoundaryElementFractionsFilter
 */

class SIMPLNXCORE_EXPORT ComputeBoundaryElementFractions
{
public:
  ComputeBoundaryElementFractions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  ComputeBoundaryElementFractionsInputValues* inputValues);
  ~ComputeBoundaryElementFractions() noexcept;

  ComputeBoundaryElementFractions(const ComputeBoundaryElementFractions&) = delete;
  ComputeBoundaryElementFractions(ComputeBoundaryElementFractions&&) noexcept = delete;
  ComputeBoundaryElementFractions& operator=(const ComputeBoundaryElementFractions&) = delete;
  ComputeBoundaryElementFractions& operator=(ComputeBoundaryElementFractions&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryElementFractionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
