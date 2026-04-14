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
 * @brief Input parameter bundle for the MultiThresholdObjects algorithm.
 *
 * Aggregates the threshold configuration, output mask settings, and custom
 * true/false values needed by both the in-core (Direct) and out-of-core
 * (Scanline) variants of multi-threshold filtering.
 */
struct SIMPLNXCORE_EXPORT MultiThresholdObjectsInputValues
{
  ArrayThresholdsParameter::ValueType ArrayThresholdsObject; ///< Tree of threshold comparisons (sets and individual conditions)
  DataTypeParameter::ValueType CreatedMaskType;              ///< DataType for the output mask array (e.g., uint8, bool)
  Float64Parameter::ValueType CustomFalseValue;              ///< Custom value to write for FALSE elements (default: 0.0)
  Float64Parameter::ValueType CustomTrueValue;               ///< Custom value to write for TRUE elements (default: 1.0)
  DataObjectNameParameter::ValueType OutputDataArrayName;    ///< Name of the output mask array
  BoolParameter::ValueType UseCustomFalseValue;              ///< Whether to use CustomFalseValue instead of 0
  BoolParameter::ValueType UseCustomTrueValue;               ///< Whether to use CustomTrueValue instead of 1
};

/**
 * @class MultiThresholdObjects
 * @brief Dispatcher algorithm for applying multiple threshold conditions to arrays
 * and producing a boolean mask.
 *
 * This class acts as a thin dispatcher that selects between two concrete algorithm
 * implementations at runtime:
 *
 * - **MultiThresholdObjectsDirect** (in-core): Uses per-element operator[] / getComponentValue()
 *   access with an O(n) tempResultVector for intermediate results. Optimal when all
 *   arrays reside in memory.
 *
 * - **MultiThresholdObjectsScanline** (out-of-core / OOC): Processes data in fixed-size
 *   64K-tuple chunks using copyIntoBuffer()/copyFromBuffer() bulk I/O. Eliminates the
 *   O(n) tempResultVector by working chunk-by-chunk, and avoids per-element OOC access.
 *
 * The dispatch decision is made by DispatchAlgorithm<Direct, Scanline>() in
 * AlgorithmDispatch.hpp, which checks whether any input IDataArray uses OOC storage.
 *
 * **Why two variants exist**: Each threshold condition requires reading an input array
 * and comparing every element. When input arrays are stored out-of-core, per-element
 * getComponentValue() calls trigger chunk load/evict cycles. The Scanline variant reads
 * input data in 64K-tuple chunks, performs all comparisons for that chunk in memory,
 * then writes the results back — converting N random accesses per threshold into
 * N/65536 sequential bulk reads.
 *
 * @see MultiThresholdObjectsDirect
 * @see MultiThresholdObjectsScanline
 * @see AlgorithmDispatch.hpp
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjects
{
public:
  /**
   * @brief Constructs the dispatcher with all resources needed by either algorithm variant.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  MultiThresholdObjects(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjects() noexcept;

  MultiThresholdObjects(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects(MultiThresholdObjects&&) noexcept = delete;
  MultiThresholdObjects& operator=(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects& operator=(MultiThresholdObjects&&) noexcept = delete;

  /**
   * @brief Dispatches to the Direct or Scanline algorithm based on storage type.
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                  ///< Reference to the DataStructure containing all arrays
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;                          ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;                 ///< Message handler for progress updates
};

} // namespace nx::core
