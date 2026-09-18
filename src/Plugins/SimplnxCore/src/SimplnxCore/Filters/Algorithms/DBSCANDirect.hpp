#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct DBSCANInputValues;

/**
 * @class DBSCANDirect
 * @brief Computes grid-based DBSCAN with direct element access.
 *
 * Point membership, occupied grids, neighborhoods, and cluster state remain in
 * RAM. Direct pairwise point reads are efficient for resident arrays. A forced
 * direct path can repeatedly load and evict out-of-core chunks.
 */
class SIMPLNXCORE_EXPORT DBSCANDirect
{
public:
  /**
   * @brief Initializes direct DBSCAN clustering.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives phase messages.
   * @param shouldCancel Signals cancellation between algorithm passes.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  DBSCANDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues);
  ~DBSCANDirect() noexcept;

  DBSCANDirect(const DBSCANDirect&) = delete;
  DBSCANDirect(DBSCANDirect&&) noexcept = delete;
  DBSCANDirect& operator=(const DBSCANDirect&) = delete;
  DBSCANDirect& operator=(DBSCANDirect&&) noexcept = delete;

  /**
   * @brief Builds grids, merges clusters, and labels tuples.
   * @return Success or a no-cluster warning, or an input or mask error.
   *
   * Cancellation returns success. Completed labels remain, and the feature
   * AttributeMatrix keeps its previous size after cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
