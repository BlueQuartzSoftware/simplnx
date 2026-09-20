#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"

#include <array>
#include <limits>
#include <memory>
#include <type_traits>

namespace nx::core
{
struct SurfaceNetsInputValues;

/**
 * @class SurfaceNetsScanline
 * @brief Builds Surface Nets output with sequential I/O and external records.
 *
 * Cell classification needs two adjacent Feature ID Z slices. Each padded cell
 * has one fixed record for its flag, label, local position, vertex ID, and node
 * type. Genuine out-of-core execution stores O(padded volume) records on disk
 * and retains eight bounded cache pages. A forced scanline run on resident data
 * permits an O(padded volume) in-memory record-store fallback.
 *
 * Optional relaxation updates records in padded raster order. Later phases read
 * the records instead of materializing a surface-sized vertex map. Vertices,
 * faces, labels, and selected tuple data write in 4,096-item batches. An error
 * can occur after an earlier output in the same batch was written.
 *
 * Out-of-core winding repair requires external sorting and temporary-record
 * capabilities. A forced resident scanline run can use resident connectivity.
 * Cancellation is checked across classification, smoothing, counting, and
 * output scans. It returns success at explicit checkpoints without rollback.
 *
 * @see SurfaceNetsDirect for the resident MMCellMap implementation.
 */
class SIMPLNXCORE_EXPORT SurfaceNetsScanline
{
public:
  /**
   * @brief Initializes the scanline Surface Nets implementation.
   * @param dataStructure Contains input and output objects.
   * @param mesgHandler Receives phase and winding messages.
   * @param shouldCancel Signals cancellation across record and output scans.
   * @param inputValues Selects smoothing, winding, transfers, and paths.
   * @pre All arguments outlive this executor.
   */
  SurfaceNetsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues);
  /**
   * @brief Destroys the scanline Surface Nets implementation.
   */
  ~SurfaceNetsScanline() noexcept;

  SurfaceNetsScanline(const SurfaceNetsScanline&) = delete;
  SurfaceNetsScanline(SurfaceNetsScanline&&) noexcept = delete;
  SurfaceNetsScanline& operator=(const SurfaceNetsScanline&) = delete;
  SurfaceNetsScanline& operator=(SurfaceNetsScanline&&) noexcept = delete;

  /**
   * @brief Classifies padded cells, writes mesh output, and optionally repairs winding.
   * @return Dimension, allocation, record, bulk-I/O, transfer, or winding result.
   *
   * Cancellation returns success at explicit checkpoints. Resized or written
   * output is not restored.
   */
  Result<> operator()();

  /**
   * @struct SurfaceCellRecord
   * @brief Stores fixed state for one padded cell.
   *
   * An invalid VertexId identifies a cell without a surface vertex. The record
   * store allocates one entry per padded cell so flat neighbor lookup remains
   * deterministic and does not require a resident cell-to-record map.
   */
  struct SurfaceCellRecord
  {
    MMCellFlag Flag;
    uint64 VertexId = std::numeric_limits<uint64>::max();
    int32 Label = 0;
    std::array<float32, 3> LocalPosition = {0.5f, 0.5f, 0.5f};
    int8 NodeType = 0;
    bool IsReferenced = false;
  };
  static_assert(std::is_trivially_copyable_v<SurfaceCellRecord>);

private:
  DataStructure& m_DataStructure;
  const SurfaceNetsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::unique_ptr<ITemporaryRecordStore> m_SurfaceCells;
  std::unique_ptr<BoundedRecordPageCache<SurfaceCellRecord>> m_SurfaceCellCache;
};

} // namespace nx::core
