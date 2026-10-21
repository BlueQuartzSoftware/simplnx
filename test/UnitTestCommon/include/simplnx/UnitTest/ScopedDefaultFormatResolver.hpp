#pragma once

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <memory>
#include <utility>

namespace nx::core::UnitTest
{
/**
 * @brief Temporarily replaces the process-wide storage-format resolver.
 * @param resolver Replacement policy. A null pointer leaves the current policy active.
 * @return Guard that restores the previous shared owner on normal exit or exception unwinding.
 * @pre No concurrent default-resolver access or mutation occurs during the guarded scope.
 */
[[nodiscard]] inline auto SetDefaultFormatResolverForScope(std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  auto previousResolver = DataStructure::getDefaultFormatResolver();
  auto restoreResolver = MakeScopeGuard([previousResolver = std::move(previousResolver)]() noexcept { DataStructure::setDefaultFormatResolver(previousResolver); });
  DataStructure::setDefaultFormatResolver(std::move(resolver));
  return restoreResolver;
}
} // namespace nx::core::UnitTest
