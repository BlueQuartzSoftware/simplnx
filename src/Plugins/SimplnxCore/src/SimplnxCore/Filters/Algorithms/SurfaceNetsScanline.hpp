#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace nx::core
{
struct SurfaceNetsInputValues;

/**
 * @class SurfaceNetsScanline
 * @brief Out-of-core algorithm for SurfaceNets. Selected by DispatchAlgorithm
 * when any input array is backed by chunked (OOC) storage.
 *
 * Phase 1 performs slice-by-slice cell classification, reading FeatureIds
 * via copyIntoBuffer in Z-slices. Surface cells are stored in O(surface)
 * data structures rather than the O(n) Cell array used by MMCellMap.
 */
class SIMPLNXCORE_EXPORT SurfaceNetsScanline
{
public:
  SurfaceNetsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues);
  ~SurfaceNetsScanline() noexcept;

  SurfaceNetsScanline(const SurfaceNetsScanline&) = delete;
  SurfaceNetsScanline(SurfaceNetsScanline&&) noexcept = delete;
  SurfaceNetsScanline& operator=(const SurfaceNetsScanline&) = delete;
  SurfaceNetsScanline& operator=(SurfaceNetsScanline&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Per-vertex information stored only for surface cells.
   */
  struct VertexInfo
  {
    std::array<int32, 3> cellIndex; // (i,j,k) in padded coordinates
    MMCellFlag flag;
  };

private:
  DataStructure& m_DataStructure;
  const SurfaceNetsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // O(surface) data structures — populated during cell classification
  std::vector<VertexInfo> m_Vertices;
  std::unordered_map<uint64, usize> m_CellToVertex;
};

} // namespace nx::core
