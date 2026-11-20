#pragma once

#include "simplnx/Common/Result.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

// NOLINTNEXTLINE(readability-identifier-naming) Existing image-engine namespace.
namespace nx::core::ImageProcessing
{
/**
 * @brief Appends diagnostics from one image transfer in execution order.
 * @details Keep the existing warnings vector through successful transfers so later pages do not repeatedly copy earlier warnings.
 * The caller stops on the first failed transfer.
 * @param accumulated Results from earlier transfers.
 * @param transfer Result of the current transfer.
 */
inline void AccumulateTransferResult(Result<>& accumulated, Result<>&& transfer)
{
  if(transfer.invalid())
  {
    accumulated = MergeResults(std::move(accumulated), std::move(transfer));
    return;
  }
  std::ranges::move(transfer.warnings(), std::back_inserter(accumulated.warnings()));
}
} // namespace nx::core::ImageProcessing
