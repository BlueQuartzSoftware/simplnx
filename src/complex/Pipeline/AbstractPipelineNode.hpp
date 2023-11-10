#pragma once

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

#include <nlohmann/json_fwd.hpp>
#include <nod/nod.hpp>

#include <atomic>
#include <memory>
#include <vector>

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

  using RenamedPath = std::pair<DataPath, DataPath>;
  using RenamedPaths = std::vector<RenamedPath>;

  using SignalType = nod::signal<void(AbstractPipelineNode*, const std::shared_ptr<AbstractPipelineMessage>&)>;

  using PipelineRunStateSignalType = nod::signal<void(AbstractPipelineNode*, RunState)>;
  const PipelineRunStateSignalType& getPipelineRunStateSignal() const;
  PipelineRunStateSignalType& getPipelineRunStateSignal();
  void sendPipelineRunStateMessage(RunState value);

  using FilterRunStateSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, RunState)>;
  const FilterRunStateSignalType& getFilterRunStateSignal() const;
  FilterRunStateSignalType& getFilterRunStateSignal();
  void sendFilterRunStateMessage(int32_t filterIndex, RunState value);

  using FilterProgressSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, int32_t, const std::string&)>;
  const FilterProgressSignalType& getFilterProgressSignal() const;
  FilterProgressSignalType& getFilterProgressSignal();
  void sendFilterProgressMessage(int32_t filterIndex, int32_t progress, const std::string& message);

  using FilterUpdateSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, const std::string&)>;
  const FilterUpdateSignalType& getFilterUpdateSignal() const;
  FilterUpdateSignalType& getFilterUpdateSignal();
  void sendFilterUpdateMessage(int32_t filterIndex, const std::string& message);

  using PipelineFaultSignalType = nod::signal<void(AbstractPipelineNode*, FaultState)>;
  const PipelineFaultSignalType& getPipelineFaultSignal() const;
  PipelineFaultSignalType& getPipelineFaultSignal();
  void sendPipelineFaultMessage(FaultState state);

  using FilterFaultSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, FaultState)>;
  const FilterFaultSignalType& getFilterFaultSignal() const;
  FilterFaultSignalType& getFilterFaultSignal();
  void sendFilterFaultMessage(int32_t filterIndex, FaultState state);

  using FilterFaultDetailSignalType = nod::signal<void(AbstractPipelineNode*, int32_t, WarningCollection, ErrorCollection)>;
  const FilterFaultDetailSignalType& getFilterFaultDetailSignal() const;
  FilterFaultDetailSignalType& getFilterFaultDetailSignal();
  void sendFilterFaultDetailMessage(int32_t filterIndex, const WarningCollection& warnings, const ErrorCollection& errors);

  using CancelledSignalType = nod::signal<void()>;
  const CancelledSignalType& getCancelledSignal() const;
  CancelledSignalType& getCancelledSignal();
  void sendCancelledMessage();

  /**
   * @brief Specific types of pipeline node for quick type checking.
   */
  enum class NodeType
  {
    Pipeline,
    Filter
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
  virtual bool preflight(DataStructure& data, const std::atomic_bool& shouldCancel) = 0;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @param RenamedPaths& renamedPaths = {} Collection of renamed output paths.
   * Only used for PipelineFilters.
   * @param shouldCancel
   * @param allowRenaming
   * @return bool
   */
  virtual bool preflight(DataStructure& data, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming) = 0;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  virtual bool execute(DataStructure& data, const std::atomic_bool& shouldCancel) = 0;

  /**
   * @brief Creates and returns a unique pointer to a copy of the node.
   * @return std::unique_ptr<AbstractPipelineNode>
   */
  virtual std::unique_ptr<AbstractPipelineNode> deepCopy() const = 0;

  /**
   * @brief Returns the fault state of the node.
   * @return bool
   */
  FaultState getFaultState() const;

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
  nlohmann::json toJson() const;

  /**
   * @brief Returns a Pipeline containing the entire pipeline up to the current
   * node. This will expand DREAM3D files as their own Pipeline.
   * @return std::unique_ptr<Pipeline>
   */
  std::unique_ptr<Pipeline> getPrecedingPipeline() const;

protected:
  /**
   * @brief Attempts to read the disabled state from the provided node's json.
   * If the disabled key is not found, this method returns false.
   * @param json
   * @return
   */
  static bool ReadDisabledState(const nlohmann::json& json);

  /**
   * @brief Returns implementation-specific json value for the node.
   * This method should only be called from toJson().
   * @return
   */
  virtual nlohmann::json toJsonImpl() const = 0;

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
   * @param dataStructure
   */
  void setDataStructure(const DataStructure& dataStructure);

  /**
   * @brief Checks the DataStructure memory size and moves DataArrays to out-of-core if required.
   * @param dataStructure
   */
  void checkDataStructureSize(DataStructure& dataStructure);

  /**
   * @brief Updates the stored DataStructure from preflighting the node. This
   * should only be called from within the preflight(DataStructure&) method.
   * @param dataStructure
   * @param success = true
   */
  void setPreflightStructure(const DataStructure& dataStructure, bool success = true);

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
   * @brief Sets the fault state back to None
   */
  void clearFaultState();

private:
  Pipeline* m_Parent = nullptr;
  DataStructure m_DataStructure;
  DataStructure m_PreflightStructure;
  bool m_IsPreflighted = false;
  SignalType m_Signal;
  FaultState m_FaultState = FaultState::None;
  bool m_IsDisabled = false;

  PipelineRunStateSignalType m_PipelineRunStateSignal;
  FilterRunStateSignalType m_FilterRunStateSignal;

  FilterProgressSignalType m_FilterProgressSignal;
  FilterUpdateSignalType m_FilterUpdateSignal;

  PipelineFaultSignalType m_PipelineFaultSignal;
  FilterFaultSignalType m_FilterFaultSignal;

  FilterFaultDetailSignalType m_FilterFaultDetailSignal;

  CancelledSignalType m_CancelledSignal;
};
} // namespace complex
