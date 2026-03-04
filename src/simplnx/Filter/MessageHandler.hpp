#pragma once

#include "simplnx/Common/Types.hpp"

#include <functional>
#include <string>

namespace nx::core
{
/**
 * @brief Represents a message from a filter during execution.
 */
struct Message
{
  /**
   * @brief Message type enumeration.
   */
  enum class Type : uint8
  {
    Info = 0,
    Debug,
    Progress,
    Warning,
    Error
  };

  Type type = Type::Info;
  std::string message;
};

/**
 * @brief Extends Message to include progress information.
 */
struct ProgressMessage : public Message
{
  int32 progress = 0;
};

/**
 * @brief Handler for processing filter messages during execution.
 */
struct MessageHandler
{
  using Callback = std::function<void(const Message&)>;

  /**
   * @brief Invokes the callback with a message.
   * @param message The message to send
   */
  void operator()(const Message& message) const
  {
    if(m_Callback)
    {
      m_Callback(message);
    }
  }

  /**
   * @brief Invokes the callback with an info message.
   * @param message The message text
   */
  void operator()(std::string message) const
  {
    operator()(Message{Message::Type::Info, std::move(message)});
  }

  /**
   * @brief Invokes the callback with a typed message.
   * @param type The message type
   * @param message The message text
   */
  void operator()(Message::Type type, std::string message) const
  {
    operator()(Message{type, std::move(message)});
  }

  /**
   * @brief Invokes the callback with a progress message.
   * @param type The message type
   * @param message The message text
   * @param progress The progress value
   */
  void operator()(Message::Type type, std::string message, int32 progress) const
  {
    operator()(ProgressMessage{type, std::move(message), progress});
  }

  Callback m_Callback;
};
} // namespace nx::core
