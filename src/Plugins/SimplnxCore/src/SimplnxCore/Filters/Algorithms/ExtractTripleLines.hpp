#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @brief Source and destination paths, optional Node Types stores, and extraction options.
 * NumFeatures must belong to the destination Edge Attribute Matrix. DestinationNodeTypes,
 * when supplied, must belong to the destination Vertex Attribute Matrix.
 */
struct SIMPLNXCORE_EXPORT ExtractTripleLinesInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsPath;
  DataPath TripleLineGeometryPath;
  DataPath NumFeaturesPath;
  const Int8AbstractDataStore* SourceNodeTypes = nullptr;
  Int8AbstractDataStore* DestinationNodeTypes = nullptr;
  bool IncludeExteriorLines = false;
};

/**
 * @brief Extracts mesh edges that border three or more unique Feature Ids into an Edge Geometry.
 *
 * Feature Id 0 is an ordinary Feature. Negative labels represent outside regions and count
 * only when IncludeExteriorLines is true. Node Types do not determine edge selection.
 * Edge selection counts unique Feature Ids, not triangles: at a non-manifold junction several
 * sheets can share an edge while bordering only two Features.
 * NumFeatures saturates at 4, which means four or more unique Features.
 * Output vertices ascend by source index. Edges ascend by (lower, higher) source vertex index.
 */
class SIMPLNXCORE_EXPORT ExtractTripleLines
{
public:
  /**
   * @brief Initializes extraction using pre-created output geometry and arrays.
   * @param dataStructure Contains the source mesh and destination geometry and arrays.
   * @param messageHandler Receives throttled progress messages.
   * @param shouldCancel Signals cancellation during each pass.
   * @param inputValues Supplies paths, paired Node Types stores, and selection options.
   * @pre Arguments and inputValues outlive this executor. Face Labels have two components
   * and one tuple per triangle. Node Types, if supplied, have one tuple per source vertex.
   */
  ExtractTripleLines(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ExtractTripleLinesInputValues* inputValues);
  ~ExtractTripleLines() noexcept;

  ExtractTripleLines(const ExtractTripleLines&) = delete;
  ExtractTripleLines(ExtractTripleLines&&) noexcept = delete;
  ExtractTripleLines& operator=(const ExtractTripleLines&) = delete;
  ExtractTripleLines& operator=(ExtractTripleLines&&) noexcept = delete;

  /**
   * @brief Selects edges, compacts vertices, and resizes both output attribute matrices.
   * @return Errors for excessive source vertices, unpaired Node Types stores, or resize failures.
   * Cancellation returns a valid result and can leave resized, partially written output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ExtractTripleLinesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
