#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct FillBadDataInputValues;

/**
 * @class FillBadDataBFS
 * @brief BFS flood-fill algorithm for filling bad data regions.
 *
 * This is the in-core-optimized implementation. It uses BFS (breadth-first search)
 * to identify connected components of bad data, then iteratively fills small regions
 * by voting among face neighbors. Uses O(N) temporary buffers (neighbors, alreadyChecked)
 * which is efficient when data fits in RAM.
 *
 * @see FillBadDataCCL for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT FillBadDataBFS
{
public:
  FillBadDataBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataBFS() noexcept;

  FillBadDataBFS(const FillBadDataBFS&) = delete;
  FillBadDataBFS(FillBadDataBFS&&) noexcept = delete;
  FillBadDataBFS& operator=(const FillBadDataBFS&) = delete;
  FillBadDataBFS& operator=(FillBadDataBFS&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
