#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

#include <fmt/core.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace nx::core::UnitTest
{
/**
 * @class RecordingFileWriteManager
 * @brief Test-only I/O manager that observes and answers the file-write hook.
 *
 * Writer tests need to see which final paths a writer passes to DataIOCollection::beforeFileWrite() and to inject
 * a hook result. The manager registers no store factories, so DataIOCollection::getFormatNames() never lists it and
 * it never creates a store.
 *
 * The manager is inert until arm() is called. An armed manager handles only calls whose target equals the armed
 * target: it calls the armed callback and returns the armed result. Every other call returns a valid result. A
 * one-shot arming disarms itself on its first matching call, so exactly one matching call sees the callback and the
 * armed result. Each test calls disarm() in a scope guard, because the application recorder outlives the test.
 *
 * arm() must happen before the writer call that the test observes and must not overlap a hook call. A call that
 * started before disarm() can still use the armed callback.
 */
class RecordingFileWriteManager : public IDataIOManager
{
public:
  /**
   * @brief Names the callback that observes one matching hook call.
   *
   * The callback receives the final target path and the hook mode of the call.
   */
  using HookCallback = std::function<void(const std::filesystem::path& target, FileWriteHookMode mode)>;

  /**
   * @brief Creates an inert recorder.
   * @param formatName Format name used as the DataIOCollection key. Use a name that no other manager in the collection uses, because
   * DataIOCollection::addIOManager() replaces a manager with the same name.
   */
  explicit RecordingFileWriteManager(std::string formatName)
  : m_FormatName(std::move(formatName))
  {
  }

  ~RecordingFileWriteManager() noexcept override = default;

  RecordingFileWriteManager(const RecordingFileWriteManager&) = delete;
  RecordingFileWriteManager(RecordingFileWriteManager&&) = delete;
  RecordingFileWriteManager& operator=(const RecordingFileWriteManager&) = delete;
  RecordingFileWriteManager& operator=(RecordingFileWriteManager&&) = delete;

  /**
   * @brief Returns the format name given to the constructor.
   * @return Collection key of this recorder.
   */
  std::string formatName() const override
  {
    return m_FormatName;
  }

  /**
   * @brief Arms the recorder for one target path.
   * @param target Final path whose hook calls the recorder handles. Other targets are ignored.
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
   * @param dataStructure Structure that the writer writes. The recorder does not read it.
   * @param target Final path that the writer changes.
   * @param mode Hook mode that the writer requests.
   * @return The armed result for a matching call; otherwise a valid result.
   */
  Result<> onBeforeFileWrite([[maybe_unused]] const DataStructure& dataStructure, const std::filesystem::path& target, FileWriteHookMode mode) override
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
  std::string m_FormatName;
  std::atomic_bool m_IsArmed = false;
  std::filesystem::path m_Target;
  HookCallback m_Callback;
  Result<> m_Result;
  bool m_IsOneShot = false;
};

/**
 * @brief Format name of the recorder that ApplicationRecorder() registers.
 *
 * The name sorts after `HDF5-OOC`, so the collection calls the recorder after the out-of-core manager.
 */
inline constexpr const char* k_ApplicationRecorderName = "zz-test-recorder";

/**
 * @brief Returns the recorder registered in the I/O collection of the current application.
 * @return The one recorder of this executable, named k_ApplicationRecorderName.
 * @throws std::runtime_error if the registration fails.
 *
 * The recorder is a function-local static, so it is the same object for the whole executable. Tests can delete and
 * recreate the Application, and each new Application has a new DataIOCollection. Thus each call checks the collection of
 * the current application and registers the recorder again when that collection does not hold this instance. The
 * check and the registration run under a mutex, so two callers cannot both register. DataIOCollection does not
 * synchronize registration with its lookups, so do not call this function while a writer runs on another thread. The
 * recorder is inert until a test arms it.
 */
inline RecordingFileWriteManager& ApplicationRecorder()
{
  static const std::shared_ptr<RecordingFileWriteManager> recorder = std::make_shared<RecordingFileWriteManager>(k_ApplicationRecorderName);
  static std::mutex registrationMutex;
  const std::lock_guard<std::mutex> registrationLock(registrationMutex);
  DataIOCollection& ioCollection = Application::GetOrCreateInstance()->getIOCollection();
  if(ioCollection.getManager(k_ApplicationRecorderName) != recorder)
  {
    Result<> registrationResult = ioCollection.addIOManager(recorder);
    if(registrationResult.invalid())
    {
      const std::string reason = registrationResult.errors().empty() ? std::string("no error message") : registrationResult.errors().front().message;
      throw std::runtime_error(fmt::format("Cannot register the test recorder '{}': {}", k_ApplicationRecorderName, reason));
    }
  }
  return *recorder;
}
} // namespace nx::core::UnitTest
