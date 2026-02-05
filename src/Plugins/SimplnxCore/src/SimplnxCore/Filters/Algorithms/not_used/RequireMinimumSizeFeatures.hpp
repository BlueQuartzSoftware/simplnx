#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RequireMinimumSizeFeaturesInputValues inputValues;
  inputValues.ApplySinglePhase = filterArgs.value<BoolParameter::ValueType>(apply_single_phase);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_path);
  inputValues.FeaturePhasesPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_phases_path);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_image_geometry_path);
  inputValues.MinAllowedFeaturesSize = filterArgs.value<Int64Parameter::ValueType>(min_allowed_features_size);
  inputValues.NumCellsPath = filterArgs.value<ArraySelectionParameter::ValueType>(num_cells_path);
  inputValues.PhaseNumber = filterArgs.value<Int64Parameter::ValueType>(phase_number);
  return RequireMinimumSizeFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RequireMinimumSizeFeaturesInputValues
{
  BoolParameter::ValueType ApplySinglePhase;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  ArraySelectionParameter::ValueType FeaturePhasesPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  Int64Parameter::ValueType MinAllowedFeaturesSize;
  ArraySelectionParameter::ValueType NumCellsPath;
  Int64Parameter::ValueType PhaseNumber;
};

/**
 * @class RequireMinimumSizeFeatures
 * @brief This algorithm implements support code for the RequireMinimumSizeFeaturesFilter
 */

class SIMPLNXCORE_EXPORT RequireMinimumSizeFeatures
{
public:
  RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequireMinimumSizeFeaturesInputValues* inputValues);
  ~RequireMinimumSizeFeatures() noexcept;

  RequireMinimumSizeFeatures(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures(RequireMinimumSizeFeatures&&) noexcept = delete;
  RequireMinimumSizeFeatures& operator=(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures& operator=(RequireMinimumSizeFeatures&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RequireMinimumSizeFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
