#pragma once

#include "PartitionGeometry.hpp"

namespace nx::core
{

/**
 * @class PartitionGeometryScanline
 * @brief Writes partition IDs with bounded buffers and checked bulk I/O.
 *
 * Node geometry uses bulk vertex and mask reads. Grid geometry gets each cell
 * coordinate through IGridGeometry and writes output IDs in 65,536-tuple chunks.
 */
class SIMPLNXCORE_EXPORT PartitionGeometryScanline
{
public:
  /**
   * @brief Defines the scalar store used for node coordinates.
   */
  using VertexStore = PartitionGeometry::VertexStore;

  /**
   * @brief Creates a scanline geometry partitioner.
   * @param dataStructure Provides input geometry and partition outputs.
   * @param msgHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops before later output chunks when true.
   * @param inputValues Specifies validated paths and partition settings. The caller
   * must keep this object alive for the partitioner lifetime.
   */
  PartitionGeometryScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues);
  /**
   * @brief Destroys the non-owning partitioner.
   */
  ~PartitionGeometryScanline() noexcept;

  PartitionGeometryScanline(const PartitionGeometryScanline&) = delete;
  PartitionGeometryScanline(PartitionGeometryScanline&&) noexcept = delete;
  PartitionGeometryScanline& operator=(const PartitionGeometryScanline&) = delete;
  PartitionGeometryScanline& operator=(PartitionGeometryScanline&&) noexcept = delete;

  /**
   * @brief Writes partition IDs through bounded buffers.
   * @return First bulk-I/O or unknown-geometry error, or success after cancellation.
   *
   * Cancellation or an I/O error can retain complete output chunks already written.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PartitionGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief Partitions grid cells and writes bounded output chunks.
   * @param inputGeometry Provides cell coordinates.
   * @param partitionIdsStore Receives one partition ID per input cell.
   * @param psImageGeom Defines partition-grid cells.
   * @param outOfBoundsValue Supplies IDs for coordinates outside the grid.
   * @return First output error, or success after completion or cancellation.
   */
  Result<> partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue);

  /**
   * @brief Partitions buffered node coordinates and an optional mask.
   * @param vertexListStore Provides flat XYZ vertex coordinates.
   * @param partitionIdsStore Receives one partition ID per vertex.
   * @param psImageGeom Defines partition-grid cells.
   * @param outOfBoundsValue Supplies IDs for masked or exterior vertices.
   * @param maskArrayOpt Selects vertices when present.
   * @return First input or output error, or success after cancellation.
   */
  Result<> partitionNodeBasedGeometry(const VertexStore& vertexListStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                      const std::optional<const BoolArray>& maskArrayOpt);
};

} // namespace nx::core
