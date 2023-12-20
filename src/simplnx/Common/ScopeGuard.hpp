#pragma once

#include <type_traits>
#include <utility>

namespace nx::core
{
/**
 * @brief ScopeGuard executes its callable when it is destructed
 * @tparam FuncT Callable type
 */
template <class FuncT>
class ScopeGuard
{
  static_assert(std::is_nothrow_invocable_v<FuncT>, "ScopeGuard's callable must be nothrow invocable");
  static_assert(std::is_same_v<decltype(std::declval<FuncT>()()), void>, "ScopeGuard's callable must return void");
  static_assert(!std::is_reference_v<FuncT> && !std::is_const_v<FuncT> && !std::is_volatile_v<FuncT>, "ScopeGuard's callable must have value semantics");
  static_assert(std::is_nothrow_move_constructible_v<FuncT>, "ScopeGuard's callable must be nothrow move constructible");
  static_assert(std::is_nothrow_destructible_v<FuncT>, "ScopeGuard's callable must be nothrow destructible");

public:
  ScopeGuard() = delete;

  explicit ScopeGuard(FuncT func) noexcept
  : m_Func(std::move(func))
  {
  }

  ScopeGuard(FuncT func, bool active) noexcept
  : m_Func(std::move(func))
  , m_Active(active)
  {
  }

  ScopeGuard(const ScopeGuard&) = delete;

  ScopeGuard(ScopeGuard&& rhs) noexcept
  : m_Func(std::move(rhs.m_Func))
  , m_Active(std::exchange(rhs.m_Active, false))
  {
  }

  ScopeGuard& operator=(const ScopeGuard&) = delete;

  ScopeGuard& operator=(ScopeGuard&& rhs) noexcept = delete;

  ~ScopeGuard() noexcept
  {
    if(m_Active)
    {
      m_Func();
    }
  }

  bool isActive() const noexcept
  {
    return m_Active;
  }

private:
  FuncT m_Func;
  bool m_Active = true;
};

/**
 * @brief Creates a ScopeGuard from the given callable.
 * @tparam FuncT
 * @param func
 * @return
 */
template <class FuncT>
[[nodiscard]] ScopeGuard<std::remove_cv_t<std::remove_reference_t<FuncT>>> MakeScopeGuard(FuncT&& func) noexcept
{
  return ScopeGuard<std::remove_cv_t<std::remove_reference_t<FuncT>>>(std::forward<FuncT>(func));
}
} // namespace nx::core
