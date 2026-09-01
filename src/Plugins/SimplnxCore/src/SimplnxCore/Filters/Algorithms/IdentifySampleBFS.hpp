#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct IdentifySampleInputValues;

/**
 * @class IdentifySampleBFS
 * @brief Uses breadth-first search (BFS) for resident sample identification.
 *
 * A full-volume run retains the largest face-connected true component. It can
 * then fill false components that do not touch an image boundary. Equal-sized
 * components favor the component that the scan finds last.
 *
 * The full-volume path keeps two N-bit vectors and a queue that can grow to the
 * component size. Its random neighbor access is efficient for resident data but
 * can repeatedly load disk-backed chunks. The dispatcher therefore normally
 * selects this path only for resident storage. A storage override can force it.
 *
 * The initial seed is enqueued before it is marked as visited. A back edge can
 * enqueue a multi-cell component's seed a second time. This affects the recorded
 * component count but does not add another output voxel.
 *
 * Cancellation is checked between image rows, not within a component search or
 * the satellite-removal pass. The operation does not roll back mask changes.
 * Slice mode uses the buffered BFS implementation in IdentifySampleCommon.hpp.
 */
class SIMPLNXCORE_EXPORT IdentifySampleBFS
{
public:
  /**
   * @brief Initializes the resident BFS implementation.
   * @param dataStructure Contains the ImageGeom and mask.
   * @param mesgHandler Receives phase and slice messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects hole and slice behavior.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  IdentifySampleBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues);
  ~IdentifySampleBFS() noexcept;

  IdentifySampleBFS(const IdentifySampleBFS&) = delete;
  IdentifySampleBFS(IdentifySampleBFS&&) noexcept = delete;
  IdentifySampleBFS& operator=(const IdentifySampleBFS&) = delete;
  IdentifySampleBFS& operator=(IdentifySampleBFS&&) noexcept = delete;

  /**
   * @brief Retains the largest component and optionally fills holes.
   * @return A success result after the selected BFS path returns.
   * @pre The mask is scalar Bool or UInt8 and matches ImageGeom cell dimensions.
   * @pre SliceBySlicePlaneIndex identifies XY, XZ, or YZ.
   *
   * Cancellation can leave prior mask changes in place. The slice path discards
   * bulk-I/O results, so an I/O failure is not returned by this interface.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IdentifySampleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
