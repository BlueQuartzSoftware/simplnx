#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeLargestCrossSectionsInputValues;

/**
 * @class ComputeLargestCrossSectionsScanline
 * @brief Computes cross sections with bounded plane buffers and bulk store I/O.
 *
 * Disk-backed Feature Ids are never accessed per voxel, and temporary memory is
 * limited to one cross-section rather than the full cell array.
 */
class SIMPLNXCORE_EXPORT ComputeLargestCrossSectionsScanline
{
public:
  /**
   * @brief Creates a bulk-I/O cross-section algorithm.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later planes when true.
   * @param inputValues Specifies validated paths and the plane. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeLargestCrossSectionsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                      const ComputeLargestCrossSectionsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning bulk-I/O algorithm.
   */
  ~ComputeLargestCrossSectionsScanline() noexcept;

  ComputeLargestCrossSectionsScanline(const ComputeLargestCrossSectionsScanline&) = delete;
  ComputeLargestCrossSectionsScanline(ComputeLargestCrossSectionsScanline&&) noexcept = delete;
  ComputeLargestCrossSectionsScanline& operator=(const ComputeLargestCrossSectionsScanline&) = delete;
  ComputeLargestCrossSectionsScanline& operator=(ComputeLargestCrossSectionsScanline&&) noexcept = delete;

  /**
   * @brief Computes the largest area for every feature with bounded plane I/O.
   * @return Error from validation or bulk I/O, or success after cancellation.
   *
   * Cancellation writes maxima from completed planes before the method returns.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeLargestCrossSectionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
