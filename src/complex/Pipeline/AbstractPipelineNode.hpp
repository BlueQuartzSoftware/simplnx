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
  using SignalType = nod::signal<void(AbstractPipelineNode*, const std::shared_ptr<AbstractPipelineMessage>&)>;

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
  enum class Status
  {
    Dirty = 0,
    Executing,
    Completed
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
   * @brief Marks the node and all of its dependents as dirty.
   */
  void markDirty();

  /**
   * @brief Returns true if node is dirty. Returns false otherwise.
   * @return bool
   */
  bool isDirty() const;

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
   */
  void setPreflightStructure(const DataStructure& ds);

  /**
   * @brief Returns a Pipeline containing the parent pipeline up to the current
   * node. This will expand DREAM3D files as their own Pipeline.
   * @return std::unique_ptr<Pipeline>
   */
  std::unique_ptr<Pipeline> getPrecedingPipelineSegment() const;

private:
  Status m_Status = Status::Dirty;
  Pipeline* m_Parent = nullptr;
  DataStructure m_DataStructure;
  DataStructure m_PreflightStructure;
  bool m_IsPreflighted = false;
  SignalType m_Signal;
};
} // namespace complex
