#pragma once

#include <string>

#include "complex/Pipeline/Messaging/IPipelineMessage.hpp"

namespace complex
{
class Pipeline;

/**
 * @class RenamedMessage
 * @brief
 */
class COMPLEX_EXPORT RenamedMessage : public IPipelineMessage
{
public:
  /**
   * @brief Constructs a new pipeline message notifying that the specified node was renamed, what the new name is, and what it was changed from.
   * @param node
   * @param newName
   * @param oldName
   */
  RenamedMessage(Pipeline* pipeline, const std::string& newName, const std::string& oldName);

  virtual ~RenamedMessage();

  /**
   * @brief Returns the node's new name.
   * @return std::string
   */
  std::string getNewName() const;

  /**
   * @brief Returns the node's old name.
   * @return std::string
   */
  std::string getOldName() const;

private:
  std::string m_NewName;
  std::string m_OldName;
};
} // namespace complex
