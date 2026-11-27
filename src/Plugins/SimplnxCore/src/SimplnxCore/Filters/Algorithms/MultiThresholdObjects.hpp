#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayThresholdsParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DataTypeParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

/**
 * @struct MultiThresholdObjectsInputValues
 * @brief Stores threshold tree, mask type, and output values.
 */
struct SIMPLNXCORE_EXPORT MultiThresholdObjectsInputValues
{
  ArrayThresholdsParameter::ValueType array_thresholds_object;
  DataTypeParameter::ValueType created_mask_type;
  Float64ParameterType::ValueType custom_false_value = 0.0;
  Float64ParameterType::ValueType custom_true_value = 1.0;
  DataObjectNameParameter::ValueType output_data_array_name;
  BoolParameter::ValueType use_custom_false_value = false;
  BoolParameter::ValueType use_custom_true_value = false;
};

/**
 * @class MultiThresholdObjects
 * @brief Evaluates a threshold tree into a mask array.
 *
 * The dispatcher checks each threshold input and the output mask. Resident arrays use
 * direct access. Disk-backed arrays use bounded bulk I/O to avoid repeated chunk loads.
 *
 * @see MultiThresholdObjectsDirect
 * @see MultiThresholdObjectsScanline
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjects
{
public:
  /**
   * @brief Creates a threshold dispatcher.
   * @param dataStructure Provides threshold inputs and the output mask.
   * @param mesgHandler Receives progress messages from the selected algorithm.
   * @param shouldCancel Stops later evaluation work when true.
   * @param inputValues Specifies validated threshold settings. The caller must keep
   * this object alive for the dispatcher lifetime.
   */
  MultiThresholdObjects(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjectsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~MultiThresholdObjects() noexcept = default;

  MultiThresholdObjects(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects(MultiThresholdObjects&&) noexcept = delete;
  MultiThresholdObjects& operator=(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects& operator=(MultiThresholdObjects&&) noexcept = delete;

  /**
   * @brief Selects direct or scanline evaluation from participating storage.
   * @return Bulk-I/O error from scanline evaluation, or success after cancellation.
   *
   * Direct cancellation can retain a partially written mask. Scanline cancellation
   * can retain complete output chunks written before the current chunk.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
