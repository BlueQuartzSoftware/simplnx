#pragma once

#include "simplnx/Pipeline/Messaging/PipelineNodeObserver.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"

namespace nx::core
{
namespace CLI
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

  void onCancelled() const;

  void onFaultStateChanged(AbstractPipelineNode* node, FaultState state) const;

private:
  std::vector<nod::scoped_connection> m_SignalConnections;
};
} // namespace CLI
} // namespace nx::core
