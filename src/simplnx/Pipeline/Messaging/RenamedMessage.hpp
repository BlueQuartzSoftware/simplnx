#pragma once

#include "simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp"

#include <string>

namespace nx::core
{
class Pipeline;

/**
 * @class RenamedMessage
 * @brief The RenamedMessage class exists to notify Pipeline observers that a
 * target Pipeline has had its name changed, what it was changed from, and what
 * it was changed to.
 */
class SIMPLNX_EXPORT RenamedMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new pipeline message notifying that the specified
   * Pipeline was renamed, what the new name is, and what it was changed from.
   * @param node
   * @param newName
   * @param oldName
   */
  RenamedMessage(Pipeline* pipeline, const std::string& newName, const std::string& oldName);

  ~RenamedMessage() override;

  /**
   * @brief Returns the Pipeline's new name.
   * @return std::string
   */
  std::string getNewName() const;

  /**
   * @brief Returns the Pipeline's old name.
   * @return std::string
   */
  std::string getPreviousName() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  std::string m_NewName;
  std::string m_OldName;
};
} // namespace nx::core
