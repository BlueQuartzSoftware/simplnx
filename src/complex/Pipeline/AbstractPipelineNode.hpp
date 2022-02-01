#pragma once

#include <memory>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "nod/nod.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class AbstractPipelineMessage;
class Pipeline;

/**
 * @class AbstractPipelineNode
 * @brief The AbstractPipelineNode class serves as the abstract base class for
 * all items that can be contained within a pipeline. Shared API is declared
 * for implementation in derived classes.
 */
class COMPLEX_EXPORT AbstractPipelineNode
{
public:
  // Making Pipeline a friend class allows Pipelines to set flags of the nodes
  // they contain.
  friend class Pipeline;

  using SignalType = nod::signal<void(AbstractPipelineNode*, const std::shared_ptr<AbstractPipelineMessage>&)>;

  enum class RunState
  {
    Idle = 0,
    Preflighting = 1,
    Executing = 2
  };
  using PipelineRunStateSignalType = nod::signal<void(AbstractPipelineNode*, RunState)>;
  PipelineRunStateSignalType& getPipelineRunStateSignal();
  void sendPipelineRunStateMessage(RunState value);

  using FilterRunStateSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, RunState)>;
  FilterRunStateSignalType& getFilterRunStateSignal();
  void sendFilterRunStateMessage(int32_t filterIndex, RunState value);

  using FilterProgressSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, int32_t, const std::string&)>;
  FilterProgressSignalType& getFilterProgressSignal();
  void sendFilterProgressMessage(int32_t filterIndex, int32_t progress, const std::string& message);

  using FilterUpdateSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, const std::string&)>;
  FilterUpdateSignalType& getFilterUpdateSignal();
  void sendFilterUpdateMessage(int32_t filterIndex, const std::string& message);

  enum class FaultState
  {
    None = 0,
    Warnings = 1,
    Errors = 2
  };
  using PipelineFaultSignalType = nod::signal<void(AbstractPipelineNode*, FaultState)>;
  PipelineFaultSignalType& getPipelineFaultSignal();
  void sendPipelineFaultMessage(FaultState state);

  using FilterFaultSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, FaultState)>;
  FilterFaultSignalType& getFilterFaultSignal();
  void sendFilterFaultMessage(int32_t filterIndex, FaultState state);

  using FilterFaultDetailSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, WarningCollection, ErrorCollection)>;
  FilterFaultDetailSignalType& getFilterFaultDetailSignal();
  void sendFilterFaultDetailMessage(int32_t filterIndex, const WarningCollection& warnings, const ErrorCollection& errors);

  /**
   * @brief Specific types of pipeline node for quick type checking.
   */
  enum class NodeType
  {
    Pipeline,
    Filter
  };

  /**
   * @brief Specific states of pipeline and filter execution.
   */
  enum Status : uint8
  {
    None = 0,
    Executing = 1,
    Executed = 2,
    Error = 4,
    Warning = 8,
    Disabled = 16
  };

  virtual ~AbstractPipelineNode();

  /**
   * @brief Returns the node type for quick type checking.
   * @return NodeType
   */
  virtual NodeType getType() const = 0;

  /**
   * @brief Returns the pipeline node's name.
   * @return std::string
   */
  virtual std::string getName() const = 0;

  /**
   * @brief Returns a pointer to the parent Pipeline. Returns nullptr if no
   * parent could be found.
   * @return Pipeline*
   */
  Pipeline* getParentPipeline() const;

  /**
   * @brief Sets the parent Pipeline pointer.
   * @param parent
   */
  void setParentPipeline(Pipeline* parent);

  /**
   * @brief Returns true if the node has a parent pipeline. Returns false otherwise.
   * @return bool
   */
  bool hasParentPipeline() const;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  virtual bool preflight(DataStructure& data) = 0;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  virtual bool execute(DataStructure& data) = 0;

  /**
   * @brief Creates and returns a unique pointer to a copy of the node.
   * @return std::unique_ptr<AbstractPipelineNode>
   */
  virtual std::unique_ptr<AbstractPipelineNode> deepCopy() const = 0;

  /**
   * @brief Returns true if the node is currently being executed. Otherwise,
   * this method returns false.
   * @return bool
   */
  bool isExecuting() const;

  /**
   * @brief Returns true if the node is has been executed. Otherwise,
   * this method returns false.
   * @return bool
   */
  bool hasBeenExecuted() const;

  /**
   * @brief Returns true if the node has errors. Otherwise, this method returns
   * false.
   * @return bool
   */
  bool hasErrors() const;

  /**
   * @brief Returns true if the node has warnings. Otherwise, this method
   * returns false.
   * @return bool
   */
  bool hasWarnings() const;

  /**
   * @brief Returns true if the node is disabled. Otherwise, this method
   * returns false.
   * @return bool
   */
  bool isDisabled() const;

  /**
   * @brief Returns true if the node is enabled. Otherwise, this method
   * returns false.
   * @return bool
   */
  bool isEnabled() const;

  /**
   * @brief Sets whether the node is disabled.
   * @param disabled = true
   */
  void setDisabled(bool disabled = true);

  /**
   * @brief Sets whether the node is disabled.
   * @param enabled = true
   */
  void setEnabled(bool enabled = true);

  /**
   * @brief Returns the pipeline node status.
   * @return Status
   */
  Status getStatus() const;

  /**
   * @brief Returns a const reference to the executed DataStructure.
   * @return const DataStructure&
   */
  const DataStructure& getDataStructure() const;

  /**
   * @brief Returns a const reference to the preflight DataStructure.
   * @return const DataStructure&
   */
  const DataStructure& getPreflightStructure() const;

  /**
   * @brief Clears the stored DataStructure and marks the node as dirty. The
   * dirty status does not propogate to dependent nodes.
   */
  void clearDataStructure();

  /**
   * @brief Clears the stored preflight and execute DataStructures, marks the
   * node as dirty, and clears the preflighted flag.
   */
  void clearPreflightStructure();

  /**
   * @brief Returns true if the node has been preflighted and contains the
   * resulting DataStructure. Returns false otherwise.
   * @return bool
   */
  bool isPreflighted() const;

  /**
   * @brief Returns a reference to the signal used for messaging.
   * @return SignalType&
   */
  SignalType& getSignal();

  /**
   * @brief Converts the current node to json.
   * @return
   */
  virtual nlohmann::json toJson() const = 0;

  /**
   * @brief Returns a Pipeline containing the entire pipeline up to the current
   * node. This will expand DREAM3D files as their own Pipeline.
   * @return std::unique_ptr<Pipeline>
   */
  std::unique_ptr<Pipeline> getPrecedingPipeline() const;

protected:
  /**
   * @brief Sets the current node status.
   * @param status
   */
  void setStatus(Status status);

  /**
   * @brief Sets or clears the Warning flag.
   * @param value = true
   */
  void setHasWarnings(bool value = true);

  /**
   * @brief Sets or clears the Error flag.
   * @param value = true
   */
  void setHasErrors(bool value = true);

  /**
   * @brief Sets or clears the Executing flag.
   * @param value = true
   */
  void setIsExecuting(bool value = true);

  /**
   * @brief Sets or clears the Executed flag.
   * @param value = true
   */
  virtual void setHasBeenExecuted(bool value = true);

  /**
   * @brief Notifies known observers of the provided message.
   * @param msg
   */
  void notify(const std::shared_ptr<AbstractPipelineMessage>& msg);

  /**
   * @brief Constructs a new AbstractPipelineNode. If no parent pipeline is specified,
   * then the node is created without one.
   * @param parent = nullptr
   */
  AbstractPipelineNode(Pipeline* parent = nullptr);

  /**
   * @brief Updates the stored DataStructure. This should only be called from
   * within the execute(DataStructure&) method.
   * @param ds
   */
  void setDataStructure(const DataStructure& ds);

  /**
   * @brief Updates the stored DataStructure from preflighting the node. This
   * should only be called from within the preflight(DataStructure&) method.
   * @param ds
   * @param success = true
   */
  void setPreflightStructure(const DataStructure& ds, bool success = true);

  /**
   * @brief Called when ending pipeline node execution.
   * Sets the DataStructure and clears the Executing flag.
   * If there is a parent node, sets the Executed flag.
   */
  virtual void endExecution(DataStructure& dataStructure);

  /**
   * @brief Returns a Pipeline containing the parent pipeline up to the current
   * node. This will expand DREAM3D files as their own Pipeline.
   * @return std::unique_ptr<Pipeline>
   */
  std::unique_ptr<Pipeline> getPrecedingPipelineSegment() const;

private:
  Status m_Status = Status::None;
  Pipeline* m_Parent = nullptr;
  DataStructure m_DataStructure;
  DataStructure m_PreflightStructure;
  bool m_IsPreflighted = false;
  SignalType m_Signal;

  PipelineRunStateSignalType m_PipelineRunStateSignal;
  FilterRunStateSignalType m_FilterRunStateSignal;

  FilterProgressSignalType m_FilterProgressSignal;
  FilterUpdateSignalType m_FilterUpdateSignal;

  PipelineFaultSignalType m_PipelineFaultSignal;
  FilterFaultSignalType m_FilterFaultSignal;

  FilterFaultDetailSignalType m_FilterFaultDetailSignal;
};
} // namespace complex
