#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeQuaternionConjugateInputValues;

/**
 * @class ComputeQuaternionConjugateDirect
 * @brief Conjugates quaternion arrays through direct access.
 *
 * The dispatcher normally selects this executor for in-memory targets. The
 * executor retains the direct ParallelDataAlgorithm loop. It does not provide
 * a general DataArray or DataStore concurrent-access guarantee.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeQuaternionConjugateDirect
{
public:
  /**
   * @brief Initializes the direct quaternion-conjugation executor.
   * @param dataStructure Provides the selected quaternion arrays.
   * @param mesgHandler Provides the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the input and output arrays.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues remain valid
   *      while this executor runs.
   * @pre The selected arrays contain four components for each tuple.
   */
  ComputeQuaternionConjugateDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   const ComputeQuaternionConjugateInputValues* inputValues);

  /**
   * @brief Destroys the direct quaternion-conjugation executor.
   */
  ~ComputeQuaternionConjugateDirect() noexcept;

  ComputeQuaternionConjugateDirect(const ComputeQuaternionConjugateDirect&) = delete;
  ComputeQuaternionConjugateDirect(ComputeQuaternionConjugateDirect&&) noexcept = delete;
  ComputeQuaternionConjugateDirect& operator=(const ComputeQuaternionConjugateDirect&) = delete;
  ComputeQuaternionConjugateDirect& operator=(ComputeQuaternionConjugateDirect&&) noexcept = delete;

  /**
   * @brief Conjugates all quaternion tuples through direct array access.
   * @return Success after the traversal or cancellation.
   *
   * Cancellation is checked for each tuple. The method returns success and can
   * leave output tuples not recomputed.
   */
  Result<> operator()();

  /**
   * @brief Returns the external cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeQuaternionConjugateInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
