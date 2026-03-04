#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/ExecutionContext.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json_fwd.hpp>

#include <atomic>
#include <memory>
#include <vector>

namespace nx::core
{
class AbstractPipelineNode;
class Pipeline;

/**
 * @class IPipelineNode
 * @brief Pure interface defining the public contract for all pipeline nodes.
 * Provides the core identity, operations, and state query methods that every
 * pipeline node must implement. Signal/observer mechanisms are provided by
 * the AbstractPipelineNode base class.
 */
class SIMPLNX_EXPORT IPipelineNode
{
public:
  /**
   * @brief Specific types of pipeline node for quick type checking.
   */
  enum class NodeType
  {
    Pipeline,
    Filter
  };

  using RenamedPath = std::pair<DataPath, DataPath>;
  using RenamedPaths = std::vector<RenamedPath>;

  virtual ~IPipelineNode() noexcept;

  IPipelineNode(const IPipelineNode&) = delete;
  IPipelineNode(IPipelineNode&&) noexcept = delete;
  IPipelineNode& operator=(const IPipelineNode&) = delete;
  IPipelineNode& operator=(IPipelineNode&&) noexcept = delete;

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
  virtual Pipeline* getParentPipeline() const = 0;

  /**
   * @brief Sets the parent Pipeline pointer.
   * @param parent
   */
  virtual void setParentPipeline(Pipeline* parent) = 0;

  /**
   * @brief Returns true if the node has a parent pipeline. Returns false otherwise.
   * @return bool
   */
  virtual bool hasParentPipeline() const = 0;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param dataStructure
   * @param shouldCancel
   * @return bool
   */
  virtual bool preflight(DataStructure& dataStructure, const std::atomic_bool& shouldCancel) = 0;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param dataStructure
   * @param renamedPaths Collection of renamed output paths.
   * @param shouldCancel
   * @param allowRenaming
   * @return bool
   */
  virtual bool preflight(DataStructure& dataStructure, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming) = 0;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param dataStructure
   * @param shouldCancel
   * @return bool
   */
  virtual bool execute(DataStructure& dataStructure, const std::atomic_bool& shouldCancel) = 0;

  /**
   * @brief Creates and returns a unique pointer to a copy of the node.
   * @return std::unique_ptr<AbstractPipelineNode>
   */
  virtual std::unique_ptr<AbstractPipelineNode> deepCopy() const = 0;

  /**
   * @brief Returns the fault state of the node.
   * @return FaultState
   */
  virtual FaultState getFaultState() const = 0;

  /**
   * @brief Returns true if the node has errors. Otherwise, this method returns false.
   * @return bool
   */
  virtual bool hasErrors() const = 0;

  /**
   * @brief Returns true if the node has warnings. Otherwise, this method returns false.
   * @return bool
   */
  virtual bool hasWarnings() const = 0;

  /**
   * @brief Returns true if the node is disabled. Otherwise, this method returns false.
   * @return bool
   */
  virtual bool isDisabled() const = 0;

  /**
   * @brief Returns true if the node is enabled. Otherwise, this method returns false.
   * @return bool
   */
  virtual bool isEnabled() const = 0;

  /**
   * @brief Sets whether the node is disabled.
   * @param disabled = true
   */
  virtual void setDisabled(bool disabled = true) = 0;

  /**
   * @brief Sets whether the node is disabled.
   * @param enabled = true
   */
  virtual void setEnabled(bool enabled = true) = 0;

  /**
   * @brief Returns a const reference to the executed DataStructure.
   * @return const DataStructure&
   */
  virtual const DataStructure& getDataStructure() const = 0;

  /**
   * @brief Returns a const reference to the preflight DataStructure.
   * @return const DataStructure&
   */
  virtual const DataStructure& getPreflightStructure() const = 0;

  /**
   * @brief Clears the stored DataStructure and marks the node as dirty.
   */
  virtual void clearDataStructure() = 0;

  /**
   * @brief Clears the stored preflight and execute DataStructures, marks the
   * node as dirty, and clears the preflighted flag.
   */
  virtual void clearPreflightStructure() = 0;

  /**
   * @brief Returns true if the node has been preflighted and contains the
   * resulting DataStructure. Returns false otherwise.
   * @return bool
   */
  virtual bool isPreflighted() const = 0;

  /**
   * @brief Converts the current node to json.
   * @return nlohmann::json
   */
  virtual nlohmann::json toJson() const = 0;

  /**
   * @brief Returns a Pipeline containing the entire pipeline up to the current
   * node. This will expand DREAM3D files as their own Pipeline.
   * @return std::unique_ptr<Pipeline>
   */
  virtual std::unique_ptr<Pipeline> getPrecedingPipeline() const = 0;

  /**
   * @brief Gets the executionContext for the pipeline.
   * @return ExecutionContext
   */
  virtual ExecutionContext getPipelineExecutionContext() const = 0;

protected:
  IPipelineNode() = default;
};
} // namespace nx::core
