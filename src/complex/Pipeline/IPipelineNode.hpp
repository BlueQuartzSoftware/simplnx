#pragma once

#include <memory>
#include <vector>

#include "complex/Common/Uuid.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class IPipelineMessage;
class Pipeline;
class PipelineNodeObserver;

class COMPLEX_EXPORT IPipelineNode
{
  friend class PipelineNodeObserver;

public:
  using IdType = Uuid;

  enum class Status
  {
    Dirty = 0,
    Executing,
    Completed
  };

  virtual ~IPipelineNode();

  /**
   * @brief Returns the pipeline node's ID.
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief Returns the PipelineNode's name.
   * @return std::string
   */
  virtual std::string getName() = 0;

  /**
   * @brief Returns the parent node.
   * @return Pipeline*
   */
  Pipeline* getParentNode() const;

  /**
   * @brief Sets a new parent node.
   * @param parent
   */
  void setParentNode(Pipeline* parent);

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  virtual bool preflight(DataStructure& data) const = 0;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  virtual bool execute(DataStructure& data) = 0;

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
   * @brief Returns the DataStructure
   * @return const DataStructure&
   */
  const DataStructure& getDataStructure() const;

  /**
   * @brief Clears the stored DataStructure and marks the node as dirty. The
   * dirty status does not propogate to dependent nodes.
   */
  void clearDataStructure();

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
  void notify(const std::shared_ptr<IPipelineMessage>& msg);

  /**
   * @brief Adds the specified observer to the list of known observers.
   * @param obs
   */
  void addObserver(PipelineNodeObserver* obs);

  /**
   * @brief Removes the specified observer from the list of known observers.
   * @param obs
   */
  void removeObserver(PipelineNodeObserver* obs);

  /**
   * @brief Default constructor
   */
  IPipelineNode(Pipeline* parent = nullptr);

  /**
   * @brief Updates the stored DataStructure. This should only be called from
   * within the execute(DataStructure&) method.
   * @param ds
   */
  void setDataStructure(const DataStructure& ds);

private:
  /**
   * @brief Creates and returns a new pipeline node ID.
   * @return IdType
   */
  static IdType CreateId();

  ////////////
  // Variables
  IdType m_Id;
  Status m_Status = Status::Dirty;
  Pipeline* m_Parent;
  DataStructure m_DataStructure;
  std::vector<PipelineNodeObserver*> m_Observers;
};
} // namespace complex
