#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeQuaternionConjugateInputValues;

/**
 * @class ComputeQuaternionConjugateScanline
 * @brief Conjugates quaternion arrays through bounded bulk I/O.
 *
 * The executor reads 65,536 tuples, conjugates a local 1 MiB buffer, and writes
 * the same range before it reads another chunk. This design bounds memory and
 * avoids per-value access to disk-backed stores. The executor does not perform
 * concurrent DataStore access.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeQuaternionConjugateScanline
{
public:
  /**
   * @brief Initializes the scanline quaternion-conjugation executor.
   * @param dataStructure Provides the selected quaternion arrays.
   * @param mesgHandler Provides the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the input and output arrays.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues remain valid
   *      while this executor runs.
   * @pre The selected arrays contain four components for each tuple.
   */
  ComputeQuaternionConjugateScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeQuaternionConjugateInputValues* inputValues);

  /**
   * @brief Destroys the scanline quaternion-conjugation executor.
   */
  ~ComputeQuaternionConjugateScanline() noexcept;

  ComputeQuaternionConjugateScanline(const ComputeQuaternionConjugateScanline&) = delete;
  ComputeQuaternionConjugateScanline(ComputeQuaternionConjugateScanline&&) noexcept = delete;
  ComputeQuaternionConjugateScanline& operator=(const ComputeQuaternionConjugateScanline&) = delete;
  ComputeQuaternionConjugateScanline& operator=(ComputeQuaternionConjugateScanline&&) noexcept = delete;

  /**
   * @brief Streams input quaternion chunks and writes their conjugates.
   * @return Success, or an input or output bulk-I/O error.
   *
   * Cancellation is checked before each chunk. The method returns success and
   * preserves output from completed chunks.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeQuaternionConjugateInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
