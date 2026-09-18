#pragma once

#include "PartitionGeometry.hpp"

namespace nx::core
{

/**
 * @class PartitionGeometryDirect
 * @brief Partitions resident geometry data with parallel direct access.
 *
 * ImageGeom input precomputes one partition index per axis. Other grid geometry
 * uses getCoords() per cell. Node geometry uses direct vertex and mask access.
 */
class SIMPLNXCORE_EXPORT PartitionGeometryDirect
{
public:
  /**
   * @brief Defines the scalar store used for node coordinates.
   */
  using VertexStore = PartitionGeometry::VertexStore;

  /**
   * @brief Creates a direct geometry partitioner.
   * @param dataStructure Provides input geometry and partition outputs.
   * @param msgHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops active parallel ranges when true.
   * @param inputValues Specifies validated paths and partition settings. The caller
   * must keep this object alive for the partitioner lifetime.
   */
  PartitionGeometryDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues);
  /**
   * @brief Destroys the non-owning partitioner.
   */
  ~PartitionGeometryDirect() noexcept;

  PartitionGeometryDirect(const PartitionGeometryDirect&) = delete;
  PartitionGeometryDirect(PartitionGeometryDirect&&) noexcept = delete;
  PartitionGeometryDirect& operator=(const PartitionGeometryDirect&) = delete;
  PartitionGeometryDirect& operator=(PartitionGeometryDirect&&) noexcept = delete;

  /**
   * @brief Writes partition IDs through parallel direct access.
   * @return Error for an unknown geometry, or success after cancellation.
   *
   * Cancellation can retain partial partition-grid Feature IDs and partition IDs.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PartitionGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief Partitions grid cells from their spatial coordinates.
   * @param inputGeometry Provides cell coordinates and dimensions.
   * @param partitionIdsStore Receives one partition ID per input cell.
   * @param psImageGeom Defines partition-grid cells.
   * @param outOfBoundsValue Supplies IDs for coordinates outside the grid.
   * @return Success after completion or cancellation.
   */
  Result<> partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue);
  /**
   * @brief Partitions nodes from vertex coordinates and an optional mask.
   * @param vertexListStore Provides flat XYZ vertex coordinates.
   * @param partitionIdsStore Receives one partition ID per vertex.
   * @param psImageGeom Defines partition-grid cells.
   * @param outOfBoundsValue Supplies IDs for masked or exterior vertices.
   * @param maskArrayOpt Selects vertices when present.
   * @return Success after completion or cancellation.
   */
  Result<> partitionNodeBasedGeometry(const VertexStore& vertexListStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                      const std::optional<const BoolArray>& maskArrayOpt);
};

} // namespace nx::core
