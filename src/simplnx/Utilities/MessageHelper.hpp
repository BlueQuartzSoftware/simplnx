#pragma once

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <type_traits>

namespace nx::core
{
/**
 * @brief Calculates the perecent complete as an int32
 * @param currentProgress
 * @param max
 * @return
 */
inline constexpr int32 CalculatePercentCompleteAsInt(usize currentProgress, usize max)
{
  return static_cast<int32>(static_cast<float32>(currentProgress) / static_cast<float32>(max) * 100.0f);
}

/**
 * @brief The Messenger class is responsible for the backend for
 * processing messages. User code does not need to create this
 * class manually.
 */
class SIMPLNX_EXPORT Messenger
{
public:
  Messenger() = delete;

  /**
   * @brief Constructs a Messenger using IFilter::MessageHandler
   * which must remain valid for the lifetime of this instance.
   * User code does not need to call this constructor.
   * @param messageHandler
   */
  Messenger(const IFilter::MessageHandler& messageHandler);

  ~Messenger() noexcept;

  Messenger(const Messenger&) = delete;
  Messenger(Messenger&&) noexcept;

  Messenger& operator=(const Messenger&) = delete;
  Messenger& operator=(Messenger&&) noexcept;

  /**
   * @brief Sends a message that is guarenteed to be output
   * @param message
   */
  void sendMessage(std::string message);

  /**
   * @brief Attempts to send a message but depending on
   * how many other messages are being processed it be
   * discarded.
   * @param message
   */
  void trySendMessage(std::string message);

private:
  struct Impl;
  std::unique_ptr<Impl> m_Impl;
};

/**
 * @brief Functor of the form std::string func()
 */
template <class CallableT>
concept ThrottledMessageFunctor = std::is_invocable_r_v<std::string, CallableT>;

/**
 * @brief ThrottledMessenger limits the rate at messages are sent
 * based on a user provided interval. Works on a per instance basis.
 */
class ThrottledMessenger
{
public:
  ThrottledMessenger() = delete;

  /**
   * @brief Constructs a ThrottledMessenger using a Messenger for the actual sending and
   * limits the messages to the given interval. User code does not need to call this constructor.
   * @param messenger
   * @param interval
   */
  ThrottledMessenger(std::shared_ptr<Messenger> messenger, std::chrono::milliseconds interval)
  : m_Messenger(std::move(messenger))
  , m_LastTime(std::chrono::steady_clock::now())
  , m_Interval(interval)
  {
  }

  /**
   * @brief Checks if a message can be sent and if so calls the given
   * functor to construct the message.
   * @param functor
   */
  void sendThrottledMessage(ThrottledMessageFunctor auto functor)
  {
    auto now = std::chrono::steady_clock::now();
    m_LastTimeDiff = now - m_LastTime;
    if(m_LastTimeDiff >= m_Interval)
    {
      m_LastTime = now;
      m_Messenger->trySendMessage(functor());
    }
  }

  /**
   * @brief Returns the last time point at which a message was sent.
   * @return
   */
  std::chrono::steady_clock::time_point getLastTime() const
  {
    return m_LastTime;
  }

  /**
   * @brief Returns the last time difference between sent messages.
   * @return
   */
  std::chrono::steady_clock::duration getLastTimeDiff() const
  {
    return m_LastTimeDiff;
  }

private:
  std::shared_ptr<Messenger> m_Messenger = nullptr;
  std::chrono::steady_clock::time_point m_LastTime = {};
  std::chrono::milliseconds m_Interval = {};
  std::chrono::steady_clock::duration m_LastTimeDiff = {};
};

/**
 * @brief Functor of the form std::string func(usize currentProgress, usize maxProgress)
 */
template <class CallableT>
concept ProgressMessageFunctor = std::is_invocable_r_v<std::string, CallableT, usize, usize>;

/**
 * @brief ProgressMessageData is the shared data to track total progress between threads.
 * Stores the max progress which should be set before sending updates.
 * Stores the current progress as an atomic variable to be thread safe.
 * Stores an optional message template which will be used to construct messages.
 * It should accept two arguments in the fmt format.
 */
struct ProgressMessageData
{
  usize m_MaxProgress = 0;
  std::atomic_size_t m_CurrentProgress = 0;
  std::string m_MessageTemplate;
};

/**
 * @brief ProgressMessenger can send messages while tracking the total progress among all threads.
 * Uses a ThrottledMessenger to actually send the messages.
 */
class ProgressMessenger
{
public:
  ProgressMessenger() = delete;

  /**
   * @brief Constructs a ProgressMessenger using the shared ProgressMessageData, Messenger, and interval.
   * User code does not need to call this constructor.
   * @param progressMessageData
   * @param messenger
   * @param interval
   */
  ProgressMessenger(std::shared_ptr<ProgressMessageData> progressMessageData, std::shared_ptr<Messenger> messenger, std::chrono::milliseconds interval)
  : m_ThrottledMessenger(std::move(messenger), interval)
  , m_ProgressMessageData(std::move(progressMessageData))
  {
  }

  /**
   * @brief Sends a throttled message. Delegates to ThrottledMessenger
   * @param functor
   */
  void sendThrottledMessage(ThrottledMessageFunctor auto functor)
  {
    m_ThrottledMessenger.sendThrottledMessage(functor);
  }

  /**
   * @brief Sends a progress message after incrementing by the given amount.
   * The functor takes the current progress and max progress as arguments and
   * is only called if a message can be sent according to the ThrottledMessenger.
   * @param increment
   * @param functor
   */
  void sendProgressMessage(usize increment, ProgressMessageFunctor auto functor)
  {
    m_ProgressMessageData->m_CurrentProgress += increment;
    auto nestedFunctor = [this, functor]() { return functor(m_ProgressMessageData->m_CurrentProgress.load(), m_ProgressMessageData->m_MaxProgress); };
    sendThrottledMessage(nestedFunctor);
  }

  /**
   * @brief Sends a progress message after incrementing by the given amount.
   * The message is constructed using the message template.
   * @param increment
   */
  void sendProgressMessage(usize increment)
  {
    auto func = [this](usize currentProgress, usize maxProgress) {
      int32 percentComplete = CalculatePercentCompleteAsInt(currentProgress, maxProgress);
      return fmt::format(fmt::runtime(m_ProgressMessageData->m_MessageTemplate), percentComplete);
    };

    sendProgressMessage(increment, func);
  }

  /**
   * @brief Returns the shared progress data.
   * @return
   */
  const ProgressMessageData& getProgressMessageData() const
  {
    return *m_ProgressMessageData;
  }

  /**
   * @brief Returns the underlying ThrottledMessenger.
   * @return
   */
  const ThrottledMessenger& getThrottledMessenger() const
  {
    return m_ThrottledMessenger;
  }

private:
  ThrottledMessenger m_ThrottledMessenger;
  std::shared_ptr<ProgressMessageData> m_ProgressMessageData = nullptr;
};

/**
 * @brief The ProgressMessageHelper class manages the shared progress data
 * and is used to create individual ProgressMessenger for each thread.
 * Stores a shared Messenger.
 */
class ProgressMessageHelper
{
public:
  /**
   * @brief Constructs a ProgressMessageHelper using a Messenger.
   * User code does not need to call this constructor.
   * @param messenger
   */
  ProgressMessageHelper(std::shared_ptr<Messenger> messenger)
  : m_Messenger(std::move(messenger))
  , m_ProgressMessageData(std::make_shared<ProgressMessageData>())
  {
  }

  /**
   * @brief Creates a ProgressMessenger with the given interval.
   * @param interval
   * @return
   */
  ProgressMessenger createProgressMessenger(std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  {
    return ProgressMessenger(m_ProgressMessageData, m_Messenger, interval);
  }

  /**
   * @brief Sets the progress message template.
   * E.g. "Completed {}/{}"
   * @param messageTemplate
   */
  void setProgressMessageTemplate(std::string messageTemplate)
  {
    m_ProgressMessageData->m_MessageTemplate = std::move(messageTemplate);
  }

  /**
   * @brief Sets the maximum progress.
   * @param maxProgress
   */
  void setMaxProgresss(usize maxProgress)
  {
    m_ProgressMessageData->m_MaxProgress = maxProgress;
  }

  /**
   * @brief Resets the current progress to 0. Should only be called
   * when there are no active ProgressMessengers.k
   */
  void resetProgress()
  {
    m_ProgressMessageData->m_CurrentProgress = 0;
  }

private:
  std::shared_ptr<Messenger> m_Messenger = nullptr;
  std::shared_ptr<ProgressMessageData> m_ProgressMessageData = nullptr;
};

/**
 * @brief The MessageHelper class manages the lifetime of the Messenger class
 * for itself and all child classes. Used to create progress and throttled messengers.
 */
class MessageHelper
{
public:
  MessageHelper() = delete;

  /**
   * @brief Constructs a MessageHelper using a MessageHandler.
   * @param messageHandler
   */
  MessageHelper(const IFilter::MessageHandler& messageHandler)
  : m_Messenger(std::make_shared<Messenger>(messageHandler))
  {
  }

  ~MessageHelper() noexcept = default;

  MessageHelper(const MessageHelper&) = delete;
  MessageHelper(MessageHelper&&) noexcept = default;

  MessageHelper& operator=(const MessageHelper&) = delete;
  MessageHelper& operator=(MessageHelper&&) noexcept = default;

  /**
   * @brief Sends a message that is guarenteed to be output.
   * Delegated to the underlying Messenger.
   * @param message
   */
  void sendMessage(std::string message)
  {
    m_Messenger->sendMessage(std::move(message));
  }

  /**
   * @brief Attempts to send a message but depending on
   * how many other messages are being processed it be
   * discarded. Delegated to the underlying Messenger.
   * @param message
   */
  void trySendMessage(std::string message)
  {
    m_Messenger->trySendMessage(std::move(message));
  }

  /**
   * @brief Creates a ThrottledMessenger with the given interval.
   * @param interval
   * @return
   */
  ThrottledMessenger createThrottledMessenger(std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  {
    return ThrottledMessenger(m_Messenger, interval);
  }

  /**
   * @brief Creates a ProgresssMessageHelper.
   * @return
   */
  ProgressMessageHelper createProgressMessageHelper()
  {
    return ProgressMessageHelper(m_Messenger);
  }

private:
  std::shared_ptr<Messenger> m_Messenger = nullptr;
};
} // namespace nx::core
