#pragma once

#include "simplnx/Common/FileReplaceGuard.hpp"
#include "simplnx/Common/Result.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace nx::core::UnitTest
{
/**
 * @class RecordingFileReplaceHandler
 * @brief Test-only file-replace handler that observes and answers BeforeFileReplace() calls.
 *
 * Writer tests need to see which final paths a writer passes to BeforeFileReplace() and to inject a hook result.
 * RegisterRecorder() registers a recorder as a handler; ProcessRecorder() returns one recorder that is registered for the
 * whole test executable.
 *
 * The recorder is inert until arm() is called. An armed recorder handles only calls whose target equals the armed
 * target: it calls the armed callback and returns the armed result. Every other call returns a valid result. A one-shot
 * arming disarms itself on its first matching call, so exactly one matching call sees the callback and the armed
 * result. Each test calls disarm() in a scope guard, because a registered recorder outlives the test.
 *
 * arm() must happen before the writer call that the test observes and must not overlap a hook call. A call that
 * started before disarm() can still use the armed callback. The callback runs inside BeforeFileReplace(), so it must
 * not call a writer: the nested call returns k_FileReplaceNestedError.
 */
class RecordingFileReplaceHandler
{
public:
  /**
   * @brief Names the callback that observes one matching hook call.
   *
   * The callback receives the target path and the hook mode of the call.
   */
  using HookCallback = std::function<void(const std::filesystem::path& target, FileWriteHookMode mode)>;

  RecordingFileReplaceHandler() = default;
  ~RecordingFileReplaceHandler() noexcept = default;

  RecordingFileReplaceHandler(const RecordingFileReplaceHandler&) = delete;
  RecordingFileReplaceHandler(RecordingFileReplaceHandler&&) = delete;
  RecordingFileReplaceHandler& operator=(const RecordingFileReplaceHandler&) = delete;
  RecordingFileReplaceHandler& operator=(RecordingFileReplaceHandler&&) = delete;

  /**
   * @brief Arms the recorder for one target path.
   * @param target Target path whose hook calls the recorder handles. Other targets are ignored.
   * @param callback Callback for each matching call. An empty callback only returns result.
   * @param result Result returned from each matching call.
   * @param isOneShot True to disarm on the first matching call.
   * @pre No hook call runs on this recorder.
   */
  void arm(std::filesystem::path target, HookCallback callback, Result<> result, bool isOneShot)
  {
    m_Target = std::move(target);
    m_Callback = std::move(callback);
    m_Result = std::move(result);
    m_IsOneShot = isOneShot;
    m_IsArmed.store(true, std::memory_order_release);
  }

  /**
   * @brief Makes the recorder inert.
   *
   * The armed callback and result stay in place, so a hook call that already passed the armed check can finish safely.
   */
  void disarm()
  {
    m_IsArmed.store(false, std::memory_order_release);
  }

  /**
   * @brief Reports whether the recorder handles matching calls.
   * @return True between arm() and disarm(), or until the first matching call of a one-shot arming.
   */
  bool isArmed() const
  {
    return m_IsArmed.load(std::memory_order_acquire);
  }

  /**
   * @brief Calls the armed callback and returns the armed result for a matching target.
   * @param target Target path of the hook call.
   * @param mode Hook mode of the hook call.
   * @return The armed result for a matching call; otherwise a valid result.
   */
  Result<> handle(const std::filesystem::path& target, FileWriteHookMode mode)
  {
    if(!m_IsArmed.load(std::memory_order_acquire) || target != m_Target)
    {
      return {};
    }
    if(m_IsOneShot)
    {
      // Only the call that clears the flag handles the one-shot arming.
      bool wasArmed = true;
      if(!m_IsArmed.compare_exchange_strong(wasArmed, false, std::memory_order_acq_rel))
      {
        return {};
      }
    }
    if(m_Callback)
    {
      m_Callback(target, mode);
    }
    return m_Result;
  }

private:
  std::atomic_bool m_IsArmed = false;
  std::filesystem::path m_Target;
  HookCallback m_Callback;
  Result<> m_Result;
  bool m_IsOneShot = false;
};

/**
 * @brief Registers a recorder as a file-replace handler.
 * @param recorder Recorder to register. The registration shares its ownership.
 * @return Registration token; the recorder stays registered while a copy of the token exists.
 *
 * Handlers run in registration order, so a recorder observes the state after every handler that registered before it.
 */
inline std::shared_ptr<void> RegisterRecorder(const std::shared_ptr<RecordingFileReplaceHandler>& recorder)
{
  return RegisterFileReplaceHandler([recorder](const std::filesystem::path& target, FileWriteHookMode mode) { return recorder->handle(target, mode); });
}

/**
 * @brief Returns the recorder that is registered for the whole test executable.
 * @return The one process recorder.
 *
 * The first call registers the recorder; it stays registered until the process ends. A handler that registers later
 * runs after the recorder. The recorder is inert until a test arms it.
 */
inline RecordingFileReplaceHandler& ProcessRecorder()
{
  static const std::shared_ptr<RecordingFileReplaceHandler> s_Recorder = std::make_shared<RecordingFileReplaceHandler>();
  static const std::shared_ptr<void> s_Registration = RegisterRecorder(s_Recorder);
  return *s_Recorder;
}

/**
 * @class RecorderDisarmGuard
 * @brief Disarms recorders when a test ends, also after a failed assertion.
 *
 * The guard does not own the recorders. Each recorder must outlive the guard.
 */
class RecorderDisarmGuard
{
public:
  /**
   * @brief Creates the guard.
   * @param recorders Recorders to disarm at destruction.
   */
  explicit RecorderDisarmGuard(std::initializer_list<RecordingFileReplaceHandler*> recorders)
  : m_Recorders(recorders)
  {
  }

  ~RecorderDisarmGuard() noexcept
  {
    for(RecordingFileReplaceHandler* recorder : m_Recorders)
    {
      recorder->disarm();
    }
  }

  RecorderDisarmGuard(const RecorderDisarmGuard&) = delete;
  RecorderDisarmGuard(RecorderDisarmGuard&&) = delete;
  RecorderDisarmGuard& operator=(const RecorderDisarmGuard&) = delete;
  RecorderDisarmGuard& operator=(RecorderDisarmGuard&&) = delete;

private:
  std::vector<RecordingFileReplaceHandler*> m_Recorders;
};
} // namespace nx::core::UnitTest
