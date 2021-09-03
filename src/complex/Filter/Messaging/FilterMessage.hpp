#pragma once

#include <string>

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class FilterMessage
 * @brief The FilterMessage class serves as the message emitted by IFilter to
 * its known FilterObservers.
 */
class COMPLEX_EXPORT FilterMessage
{
public:
  enum class Type : uint8_t
  {
    Info = 0,
    Debug,
    Warning,
    Error
  };

  /**
   * @brief Constructs a new FilterMessage with the given type and message.
   * @param type
   * @param msg
   */
  FilterMessage(Type type, const std::string& msg);

  virtual ~FilterMessage();

  /**
   * @brief Returns the message Type value.
   * @return Type
   */
  Type getType() const;

  /**
   * @brief Returns the filter message string.
   * @return std::string
   */
  std::string getMessage() const;

private:
  std::string m_Message;
  Type m_Type = Type::Info;
};
} // namespace complex
