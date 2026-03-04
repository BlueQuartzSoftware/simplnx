#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct IdentifySampleInputValues;

/**
 * @class IdentifySampleCCL
 * @brief Chunk-sequential CCL algorithm for identifying the largest sample region.
 *
 * This is the out-of-core-optimized implementation. It uses scanline Connected
 * Component Labeling (CCL) with a union-find structure, processing data in chunk
 * order to minimize disk I/O. The algorithm only accesses backward neighbors
 * (-X, -Y, -Z) during labeling, ensuring sequential chunk access.
 *
 * Trade-off: Uses a std::vector<int64> label array (8 bytes per voxel) which is
 * more memory than the BFS approach (1 bit per voxel), but avoids the random
 * access pattern that causes chunk thrashing in OOC mode.
 *
 * @see IdentifySampleBFS for the in-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT IdentifySampleCCL
{
public:
  IdentifySampleCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues);
  ~IdentifySampleCCL() noexcept;

  IdentifySampleCCL(const IdentifySampleCCL&) = delete;
  IdentifySampleCCL(IdentifySampleCCL&&) noexcept = delete;
  IdentifySampleCCL& operator=(const IdentifySampleCCL&) = delete;
  IdentifySampleCCL& operator=(IdentifySampleCCL&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IdentifySampleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
