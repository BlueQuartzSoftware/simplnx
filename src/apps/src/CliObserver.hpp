#pragma once

#include "simplnx/Pipeline/Messaging/PipelineNodeObserver.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"

namespace nx::core::CLI
{
/**
 * @class PipelineObserver
 * @brief The PipelineObserver class writes pipeline messages to the standard output.
 */
class PipelineObserver : public PipelineNodeObserver
{
public:
  PipelineObserver(Pipeline* pipeline = nullptr);
  ~PipelineObserver() override;

protected:
  /**
   * @brief Called when the specified pipeline node emits a message.
   * @param node
   * @param msg
   */
  void onNotify(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg) override;

  static void OnCancelled();

  static void OnFaultStateChanged(AbstractPipelineNode* node, FaultState state);

private:
  std::vector<nod::scoped_connection> m_SignalConnections;
};
} // namespace nx::core::CLI
