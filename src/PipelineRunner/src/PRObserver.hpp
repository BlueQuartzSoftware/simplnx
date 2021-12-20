#pragma once

#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"
#include "complex/Pipeline/Pipeline.hpp"

namespace complex
{
namespace PipelineRunner
{
/**
 * @class PipelineObserver
 * @brief The PipelineObserver class writes pipeline messages to the standard output.
 */
class PipelineObserver : public PipelineNodeObserver
{
public:
  PipelineObserver(Pipeline* pipeline = nullptr);
  virtual ~PipelineObserver();

protected:
  /**
   * @brief Called when the specified pipeline node emits a message.
   * @param node
   * @param msg
   */
  void onNotify(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg) override;
};
} // namespace PipelineRunner
} // namespace complex
