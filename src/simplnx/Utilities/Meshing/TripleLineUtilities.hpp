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
 * @brief Options controlling triple line extraction.
 */
struct SIMPLNX_EXPORT TripleLineOptions
{
  /**
   * @brief When true, the "outside the volume" region (Feature Id -1) is counted as a
   * distinct region. This makes grain boundaries that reach the free surface of the
   * volume register as triple lines. When false (the default) only interior triple
   * lines are produced.
   */
  bool IncludeExteriorLines = false;
};

/**
 * @brief Extracts the triple lines of a multi-material triangle mesh into an Edge Geometry.
 *
 * A mesh edge is a triple line segment if and only if the set of unique Feature Ids across
 * the FaceLabels of every triangle sharing that edge has 3 or more members. Feature Id -1
 * ("outside the volume") is counted only when options.IncludeExteriorLines is true.
 *
 * NOTE: NodeTypes is deliberately NOT consulted as part of the rule, and the number of
 * triangles sharing an edge is deliberately not used as the criterion. See
 * docs/superpowers/specs/2026-08-19-triple-line-generation-design.md for why.
 *
 * sourceNodeTypesStore is COPIED THROUGH onto the output vertices and is never read to decide
 * whether an edge is a triple line. Do not be tempted to gate the rule on it - that alternative
 * was considered and rejected because NodeTypes is produced by three different code paths while
 * FaceLabels is a single harmonized convention.
 *
 * The output Edge Geometry is self-contained: its vertex list is a compacted copy holding
 * only the vertices used by triple line edges, so it shares no DataObject with triangleGeom.
 *
 * NOTE: Edge and vertex ordering follows std::unordered_map iteration order, which is
 * deterministic for a given build but is NOT guaranteed stable across standard-library
 * implementations. Anyone building an exemplar .dream3d comparison against this Edge
 * Geometry should sort the edges/vertices first rather than relying on ordering.
 *
 * @param triangleGeom The source mesh. Not modified.
 * @param faceLabelsRef The per-triangle Feature Id pairs. 2 components, int32.
 * @param sourceNodeTypesRef The source mesh's per-vertex NodeTypes. Copied through only.
 * @param tripleLineGeom The Edge Geometry to populate. Resized by this function.
 * @param numFeaturesRef Per-edge unique Feature Id count. Resized by this function.
 *        Saturates at 4, matching the NodeTypes convention.
 * @param tripleLineNodeTypesRef Per-vertex NodeTypes for the output, copied from
 *        sourceNodeTypesRef. Resized by this function. Lets the resulting Edge Geometry be
 *        consumed directly by filters that require a Node Type array, such as Laplacian
 *        Smoothing.
 * @param options See TripleLineOptions.
 * @param shouldCancel Checked periodically; on cancel the function returns early.
 * @param messageHandler Progress destination.
 * @return Result<> Invalid if the mesh has too many vertices for the edge key packing.
 */
SIMPLNX_EXPORT Result<> GenerateTripleLines(const TriangleGeom& triangleGeom, const Int32AbstractDataStore& faceLabelsRef, const Int8AbstractDataStore& sourceNodeTypesRef, EdgeGeom& tripleLineGeom,
                                            Int8AbstractDataStore& numFeaturesRef, Int8AbstractDataStore& tripleLineNodeTypesRef, const TripleLineOptions& options,
                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);
} // namespace nx::core::MeshingUtilities
