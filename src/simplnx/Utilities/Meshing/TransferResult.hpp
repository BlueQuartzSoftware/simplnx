#pragma once

#include "simplnx/Common/Result.hpp"

#include <iterator>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace nx::core::MeshingUtilities::detail
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
SIMPLNX_EXPORT void SetTransferEnrichmentFailureCountdown(usize allocationsBeforeFailure);
SIMPLNX_EXPORT bool TransferEnrichmentFailureConsumed();
SIMPLNX_EXPORT void ResetTransferEnrichmentFailure();
SIMPLNX_EXPORT void CheckTransferEnrichmentAllocation();
#endif

// The installed nonstd::expected move constructor is not declared noexcept. Its
// void specialization moves only the error vector, without allocating. Vector
// moves and the element moves used after reserve must not throw.
static_assert(std::is_nothrow_move_assignable_v<Result<>>);
static_assert(std::is_nothrow_move_constructible_v<ErrorCollection>);
static_assert(std::is_nothrow_move_constructible_v<WarningCollection>);
static_assert(std::is_nothrow_move_constructible_v<Warning> && std::is_nothrow_move_assignable_v<Warning>);
static_assert(std::is_nothrow_move_constructible_v<Error> && std::is_nothrow_move_assignable_v<Error>);

// previous must be valid and failure must be invalid. The returned result retains the failure.
inline Result<> PreserveTransferFailure(Result<>& previous, Result<>&& failure)
{
  Result<> protectedResult = std::move(failure);
  try
  {
    auto& earlier = previous.warnings();
    auto& warnings = protectedResult.warnings();
    if(!earlier.empty())
    {
      if(earlier.size() > warnings.max_size() - warnings.size())
      {
        return protectedResult;
      }
      const auto required = earlier.size() + warnings.size();
      if(required > warnings.capacity())
      {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
        CheckTransferEnrichmentAllocation();
#endif
        warnings.reserve(required);
      }
      warnings.insert(warnings.begin(), std::make_move_iterator(earlier.begin()), std::make_move_iterator(earlier.end()));
    }
  } catch(const std::bad_alloc&)
  {
  } catch(const std::length_error&)
  {
  }
  return protectedResult;
}

// previous must be valid; next can be valid or invalid.
inline Result<> MergeTransferResult(Result<>& previous, Result<> next)
{
  if(next.invalid())
  {
    return PreserveTransferFailure(previous, std::move(next));
  }
  return MergeResults(std::move(previous), std::move(next));
}

// previous must be valid; next can be valid or invalid.
inline Result<> MergeTransferResultCopy(Result<>& previous, Result<> next)
{
  if(next.invalid())
  {
    return PreserveTransferFailure(previous, std::move(next));
  }
  return MergeResults(previous, std::move(next));
}
// failure must be invalid; makeContext must return an invalid result.
template <class MakeContext>
void AppendTransferFailureContext(Result<>& failure, MakeContext&& makeContext)
{
  try
  {
    auto context = makeContext();
    auto& errors = failure.errors();
    auto& extra = context.errors();
    if(extra.size() > errors.max_size() - errors.size())
    {
      return;
    }
    const auto required = errors.size() + extra.size();
    if(required > errors.capacity())
    {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
      CheckTransferEnrichmentAllocation();
#endif
      errors.reserve(required);
    }
    errors.insert(errors.end(), std::make_move_iterator(extra.begin()), std::make_move_iterator(extra.end()));
  } catch(const std::bad_alloc&)
  {
  } catch(const std::length_error&)
  {
  }
}
template <class MakeContext>
Result<> WithTransferFailureContext(Result<>&& failure, MakeContext&& makeContext)
{
  AppendTransferFailureContext(failure, std::forward<MakeContext>(makeContext));
  return std::move(failure);
}
} // namespace nx::core::MeshingUtilities::detail
