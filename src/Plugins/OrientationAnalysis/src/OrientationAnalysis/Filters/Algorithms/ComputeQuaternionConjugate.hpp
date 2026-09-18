#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeQuaternionConjugateInputValues
 * @brief Identifies quaternion-conjugation inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeQuaternionConjugateInputValues
{
  DataPath QuaternionDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
};

/**
 * @class ComputeQuaternionConjugate
 * @brief Dispatches quaternion conjugation.
 *
 * The direct path uses direct array access. The scanline path uses bounded
 * bulk buffers for OOC targets.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeQuaternionConjugate
{
public:
  /**
   * @brief Initializes quaternion-conjugation dispatch.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies input and output arrays.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeQuaternionConjugate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeQuaternionConjugateInputValues* inputValues);
  /**
   * @brief Destroys the quaternion-conjugation dispatcher.
   */
  ~ComputeQuaternionConjugate() noexcept;

  ComputeQuaternionConjugate(const ComputeQuaternionConjugate&) = delete;
  ComputeQuaternionConjugate(ComputeQuaternionConjugate&&) noexcept = delete;
  ComputeQuaternionConjugate& operator=(const ComputeQuaternionConjugate&) = delete;
  ComputeQuaternionConjugate& operator=(ComputeQuaternionConjugate&&) noexcept = delete;

  /**
   * @brief Dispatches quaternion conjugation.
   * @return Result from the selected executor.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
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
