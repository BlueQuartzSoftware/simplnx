#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct DBSCANInputValues;

/**
 * @class DBSCANScanline
 * @brief Computes grid-based DBSCAN with bounded external records.
 *
 * Genuine OOC execution externally sorts selected points into grid order. It
 * stores grid, core, axis, and label records in temporary files. Fixed caches
 * and tiles bound RAM even for dense cells. Temporary disk use scales with
 * selected points and occupied grids.
 *
 * A forced scanline call on resident arrays uses the resident fallback. It does
 * not create the external record pipeline.
 */
class SIMPLNXCORE_EXPORT DBSCANScanline
{
public:
  /**
   * @brief Initializes scanline DBSCAN clustering.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Supplies the common interface. This path emits no messages.
   * @param shouldCancel Signals cancellation between bounded operations.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  DBSCANScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues);
  ~DBSCANScanline() noexcept;

  DBSCANScanline(const DBSCANScanline&) = delete;
  DBSCANScanline(DBSCANScanline&&) noexcept = delete;
  DBSCANScanline& operator=(const DBSCANScanline&) = delete;
  DBSCANScanline& operator=(DBSCANScanline&&) noexcept = delete;

  /**
   * @brief Builds grids, merges clusters, and labels tuples.
   * @return Success or a no-cluster warning, or a validation, mask, storage, or record-I/O error.
   *
   * Cancellation returns success. Completed label windows remain, and the
   * feature AttributeMatrix keeps its previous size after cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace nx::core
