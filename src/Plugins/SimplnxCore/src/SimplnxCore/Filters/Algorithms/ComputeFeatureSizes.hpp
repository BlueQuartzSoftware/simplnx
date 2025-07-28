#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeatureSizesInputValues
{
  DataObjectNameParameter::ValueType EquivalentDiametersName;
  AttributeMatrixSelectionParameter::ValueType FeatureAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataObjectNameParameter::ValueType NumElementsName;
  BoolParameter::ValueType SaveElementSizes;
  DataObjectNameParameter::ValueType VolumesName;
};

/**
 * @class ComputeFeatureSizes
 * @brief This algorithm implements support code for the ComputeFeatureSizesFilter
 */

class SIMPLNXCORE_EXPORT ComputeFeatureSizes
{
public:
  ComputeFeatureSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureSizesInputValues* inputValues);
  ~ComputeFeatureSizes() noexcept;

  ComputeFeatureSizes(const ComputeFeatureSizes&) = delete;
  ComputeFeatureSizes(ComputeFeatureSizes&&) noexcept = delete;
  ComputeFeatureSizes& operator=(const ComputeFeatureSizes&) = delete;
  ComputeFeatureSizes& operator=(ComputeFeatureSizes&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
