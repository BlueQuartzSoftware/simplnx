#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ExtractVertexGeometryInputValues
 * @brief Collects mask, array-handling, and geometry settings.
 *
 * ArrayHandling is consumed by filter actions. It does not change the executor's
 * copy logic.
 */
struct SIMPLNXCORE_EXPORT ExtractVertexGeometryInputValues
{
  ChoicesParameter::ValueType ArrayHandling;
  bool UseMask;
  DataPath MaskArrayPath;
  DataPath InputGeometryPath;
  MultiArraySelectionParameter::ValueType IncludedDataArrayPaths;
  DataPath VertexGeometryPath;
  std::string VertexAttrMatrixName;
  std::string SharedVertexListName;
};

/**
 * @class ExtractVertexGeometry
 * @brief Creates vertices at selected grid-cell centers.
 *
 * An optional Bool or UInt8 mask compacts selected cells in source order.
 * Included cell arrays use the same compaction. The mask is streamed again for
 * counting, coordinates, and each included array. This avoids a cell-sized
 * keep bitmap at the cost of repeated mask reads.
 *
 * Transfer scratch is bounded by 65,536 tuples and component count. Vertex
 * coordinates use direct setTuple() writes. An OOC vertex store can therefore
 * incur one store access for each selected cell.
 */
class SIMPLNXCORE_EXPORT ExtractVertexGeometry
{
public:
  /**
   * @brief Initializes vertex-geometry extraction.
   * @param dataStructure Contains the grid, arrays, and output VertexGeom.
   * @param mesgHandler Receives extraction phase messages.
   * @param shouldCancel Signals cancellation between bounded passes.
   * @param inputValues Selects mask, included arrays, and output paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ExtractVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractVertexGeometryInputValues* inputValues);
  ~ExtractVertexGeometry() noexcept;

  ExtractVertexGeometry(const ExtractVertexGeometry&) = delete;
  ExtractVertexGeometry(ExtractVertexGeometry&&) noexcept = delete;
  ExtractVertexGeometry& operator=(const ExtractVertexGeometry&) = delete;
  ExtractVertexGeometry& operator=(ExtractVertexGeometry&&) noexcept = delete;

  /**
   * @brief Creates selected vertices and copies included cell arrays.
   * @return Success, or an invalid-mask error.
   * @pre Grid cell count matches the mask and every included array.
   * @pre Included source and destination arrays have matching types and components.
   *
   * Masked counting and coordinate passes check cancellation between chunks.
   * The unmasked coordinate pass and each array copy do not check cancellation.
   *
   * All bulk-transfer Result values are ignored. Storage failures can produce
   * invalid output while this function returns success. Cancellation can leave
   * resized and partially written vertex or attribute arrays.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
