#pragma once

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <memory>
#include <utility>

namespace nx::core::unit_test
{
/**
 * @brief Temporarily replaces the process-wide storage-format resolver.
 * @param resolver Replacement policy. A null pointer leaves the current policy active.
 * @return Guard that restores the previous shared owner on normal exit or exception unwinding.
 * @pre No concurrent default-resolver access or mutation occurs during the guarded scope.
 */
[[nodiscard]] inline auto SetDefaultFormatResolverForScope(std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  auto previousResolver = DataStructure::GetDefaultFormatResolver();
  auto restoreResolver = MakeScopeGuard([savedResolver = std::move(previousResolver)]() noexcept { DataStructure::SetDefaultFormatResolver(savedResolver); });
  DataStructure::SetDefaultFormatResolver(std::move(resolver));
  return restoreResolver;
}
} // namespace nx::core::unit_test
