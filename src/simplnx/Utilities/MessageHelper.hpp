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
inline constexpr int32 CalculatePercentCompleteAsInt(usize currentProgress, usize max)
{
  return static_cast<int32>(static_cast<float32>(currentProgress) / static_cast<float32>(max) * 100.0f);
}

class SIMPLNX_EXPORT Messenger
{
public:
  Messenger() = delete;

  Messenger(const IFilter::MessageHandler& messageHandler);

  ~Messenger() noexcept;

  Messenger(const Messenger&) = delete;
  Messenger(Messenger&&) noexcept;

  Messenger& operator=(const Messenger&) = delete;
  Messenger& operator=(Messenger&&) noexcept;

  void sendMessage(std::string message);
  void trySendMessage(std::string message);

private:
  struct Impl;
  std::unique_ptr<Impl> m_Impl;
};

template <class CallableT>
concept ThrottledMessageFunctor = std::is_invocable_r_v<std::string, CallableT>;

class ThrottledMessenger
{
public:
  ThrottledMessenger() = delete;

  ThrottledMessenger(std::shared_ptr<Messenger> messenger, std::chrono::milliseconds interval)
  : m_Messenger(std::move(messenger))
  , m_LastTime(std::chrono::steady_clock::now())
  , m_Interval(interval)
  {
  }

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

  std::chrono::steady_clock::time_point getLastTime() const
  {
    return m_LastTime;
  }

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

template <class CallableT>
concept ProgressMessageFunctor = std::is_invocable_r_v<std::string, CallableT, usize, usize>;

struct ProgressMessageData
{
  usize m_MaxProgress = 0;
  std::atomic_size_t m_CurrentProgress = 0;
  std::string m_MessageTemplate;
};

class ProgressMessenger
{
public:
  ProgressMessenger() = delete;

  ProgressMessenger(std::shared_ptr<ProgressMessageData> progressMessageData, std::shared_ptr<Messenger> messenger, std::chrono::milliseconds interval)
  : m_ThrottledMessenger(std::move(messenger), interval)
  , m_ProgressMessageData(std::move(progressMessageData))
  {
  }

  void sendThrottledMessage(ThrottledMessageFunctor auto functor)
  {
    m_ThrottledMessenger.sendThrottledMessage(functor);
  }

  void sendProgressMessage(usize increment, ProgressMessageFunctor auto functor)
  {
    m_ProgressMessageData->m_CurrentProgress += increment;
    auto nestedFunctor = [this, functor]() { return functor(m_ProgressMessageData->m_CurrentProgress.load(), m_ProgressMessageData->m_MaxProgress); };
    sendThrottledMessage(nestedFunctor);
  }

  void sendProgressMessage(usize increment)
  {
    auto func = [this](usize currentProgress, usize maxProgress) {
      int32 percentComplete = CalculatePercentCompleteAsInt(currentProgress, maxProgress);
      return fmt::format(fmt::runtime(m_ProgressMessageData->m_MessageTemplate), percentComplete);
    };

    sendProgressMessage(increment, func);
  }

  const ProgressMessageData& getProgressMessageData() const
  {
    return *m_ProgressMessageData;
  }

  const ThrottledMessenger& getThrottledMessenger() const
  {
    return m_ThrottledMessenger;
  }

private:
  ThrottledMessenger m_ThrottledMessenger;
  std::shared_ptr<ProgressMessageData> m_ProgressMessageData = nullptr;
};

class ProgressMessageHelper
{
public:
  ProgressMessageHelper(std::shared_ptr<Messenger> messenger)
  : m_Messenger(std::move(messenger))
  , m_ProgressMessageData(std::make_shared<ProgressMessageData>())
  {
  }

  ProgressMessenger createProgressMessenger(std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  {
    return ProgressMessenger(m_ProgressMessageData, m_Messenger, interval);
  }

  void setProgressMessageTemplate(std::string messageTemplate)
  {
    m_ProgressMessageData->m_MessageTemplate = std::move(messageTemplate);
  }

  void setMaxProgresss(usize maxProgress)
  {
    m_ProgressMessageData->m_MaxProgress = maxProgress;
  }

  void resetProgress()
  {
    m_ProgressMessageData->m_CurrentProgress = 0;
  }

private:
  std::shared_ptr<Messenger> m_Messenger = nullptr;
  std::shared_ptr<ProgressMessageData> m_ProgressMessageData = nullptr;
};

class MessageHelper
{
public:
  MessageHelper() = delete;

  MessageHelper(const IFilter::MessageHandler& messageHandler)
  : m_Messenger(std::make_shared<Messenger>(messageHandler))
  {
  }

  ~MessageHelper() noexcept = default;

  MessageHelper(const MessageHelper&) = delete;
  MessageHelper(MessageHelper&&) noexcept = default;

  MessageHelper& operator=(const MessageHelper&) = delete;
  MessageHelper& operator=(MessageHelper&&) noexcept = default;

  void sendMessage(std::string message)
  {
    m_Messenger->sendMessage(std::move(message));
  }

  void trySendMessage(std::string message)
  {
    m_Messenger->trySendMessage(std::move(message));
  }

  ThrottledMessenger createThrottledMessenger(std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  {
    return ThrottledMessenger(m_Messenger, interval);
  }

  ProgressMessageHelper createProgressMessageHelper()
  {
    return ProgressMessageHelper(m_Messenger);
  }

private:
  std::shared_ptr<Messenger> m_Messenger = nullptr;
};
} // namespace nx::core
