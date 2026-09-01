#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeLargestCrossSectionsInputValues;

/**
 * @class ComputeLargestCrossSectionsDirect
 * @brief Computes cross sections directly from contiguous in-memory Feature Ids.
 *
 * XY traverses contiguous slices. XZ workers use private feature scratch and a
 * serial maximum reduction. The YZ path batches at most 16 X planes to bound
 * scratch memory while preserving contiguous row reads.
 */
class SIMPLNXCORE_EXPORT ComputeLargestCrossSectionsDirect
{
public:
  /**
   * @brief Creates an in-memory cross-section algorithm.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later planes when true.
   * @param inputValues Specifies validated paths and the plane. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeLargestCrossSectionsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                    const ComputeLargestCrossSectionsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning in-memory algorithm.
   */
  ~ComputeLargestCrossSectionsDirect() noexcept;

  ComputeLargestCrossSectionsDirect(const ComputeLargestCrossSectionsDirect&) = delete;
  ComputeLargestCrossSectionsDirect(ComputeLargestCrossSectionsDirect&&) noexcept = delete;
  ComputeLargestCrossSectionsDirect& operator=(const ComputeLargestCrossSectionsDirect&) = delete;
  ComputeLargestCrossSectionsDirect& operator=(ComputeLargestCrossSectionsDirect&&) noexcept = delete;

  /**
   * @brief Computes the largest area for every feature.
   * @return Error from Feature Id validation, or success after cancellation.
   *
   * Cancellation can retain maxima from planes that finished before cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeLargestCrossSectionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
