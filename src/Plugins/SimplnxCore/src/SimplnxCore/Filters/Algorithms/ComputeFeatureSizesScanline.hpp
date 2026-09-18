#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct ComputeFeatureSizesInputValues;

/**
 * @class ComputeFeatureSizesScanline
 * @brief Computes feature sizes with sequential bulk transfers.
 *
 * Feature IDs use 262,144-tuple chunks. Three matching output buffers require about three MiB.
 * RectGrid execution also reads a matching element-size chunk. Feature accumulators remain
 * feature-sized.
 *
 * The scanline traversal retains global raster order. This preserves the serial Kahan accumulation
 * result. The algorithm returns the first bulk read or write error.
 *
 * @see ComputeFeatureSizesDirect.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureSizesScanline
{
public:
  /**
   * @brief Initializes the scanline feature-size algorithm.
   * @param dataStructure Contains geometry, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between chunks or features.
   * @param inputValues Selects outputs and required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureSizesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureSizesInputValues* inputValues);
  /**
   * @brief Destroys the scanline feature-size algorithm.
   */
  ~ComputeFeatureSizesScanline() noexcept;

  ComputeFeatureSizesScanline(const ComputeFeatureSizesScanline&) = delete;
  ComputeFeatureSizesScanline(ComputeFeatureSizesScanline&&) noexcept = delete;
  ComputeFeatureSizesScanline& operator=(const ComputeFeatureSizesScanline&) = delete;
  ComputeFeatureSizesScanline& operator=(ComputeFeatureSizesScanline&&) noexcept = delete;

  /**
   * @brief Computes feature sizes with sequential bulk transfers.
   * @return Success, or a geometry, Feature ID, feature-count, or storage error.
   *
   * When a checkpoint observes cancellation, the method returns success. Feature output written
   * before that checkpoint remains. Element-size creation or deletion can remain after cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
