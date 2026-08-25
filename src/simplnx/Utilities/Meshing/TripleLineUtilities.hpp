#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <atomic>

namespace nx::core::MeshingUtilities
{
/**
 * @struct TripleLineInputs
 * @brief Source mesh and selection options for triple line extraction.
 */
struct SIMPLNX_EXPORT TripleLineInputs
{
  const TriangleGeom& TriangleGeometry;
  const Int32AbstractDataStore& FaceLabels;
  const Int8AbstractDataStore* NodeTypes = nullptr;
  bool IncludeExteriorLines = false;
};

/**
 * @struct TripleLineOutputs
 * @brief Destination geometry and attribute stores for triple line extraction.
 *
 * NumFeatures must belong to the geometry's Edge Attribute Matrix.
 * If supplied, NodeTypes must belong to its Vertex Attribute Matrix.
 * The function resizes both matrices and their child arrays.
 */
struct SIMPLNX_EXPORT TripleLineOutputs
{
  EdgeGeom& TripleLineGeometry;
  Int8AbstractDataStore& NumFeatures;
  Int8AbstractDataStore* NodeTypes = nullptr;
};

/**
 * @brief Extracts mesh edges that border three or more unique Feature Ids into an Edge Geometry.
 *
 * The rule uses FaceLabels from all triangles that share each edge. Feature Id 0 is an ordinary Feature.
 * Negative labels represent outside regions and count only when IncludeExteriorLines is true.
 * All three meshers use the same FaceLabels convention: -1 means outside.
 * NodeTypes has three different producer code paths, one per mesher, so it does not determine edge selection.
 * Triangle count per edge is also incorrect for non-manifold junctions, where multiple sheets can share the same Feature Ids.
 *
 * NumFeatures stores the unique Feature Id count, saturated at 4: a value of 4 means four or more.
 * NodeTypes is copied only when both input and output pointers are non-null. Both null pointers disable the copy.
 *
 * The output owns a compact copy of the source vertices. Ordering is deterministic:
 * vertices ascend by source index; edges ascend by (lower, higher) source vertex index.
 *
 * @param inputs Source triangle geometry, two-component FaceLabels, optional per-vertex NodeTypes, and exterior selection option.
 * @param outputs Destination geometry, per-edge NumFeatures, and optional per-vertex NodeTypes.
 * @param shouldCancel Checked during each pass. Cancellation returns a valid result; output can be partially written.
 * @param messageHandler Destination for throttled percentage progress.
 * @return Errors for too many source vertices, mismatched NodeTypes pointers, or failed output resizing.
 */
SIMPLNX_EXPORT Result<> GenerateTripleLines(const TripleLineInputs& inputs, const TripleLineOutputs& outputs, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);
} // namespace nx::core::MeshingUtilities
