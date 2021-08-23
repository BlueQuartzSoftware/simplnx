#pragma once

#include <memory>
#include <vector>

#include "complex/Common/Uuid.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT IPipelineNode
{
public:
  using IdType = Uuid;

  virtual ~IPipelineNode();

  /**
   * @brief Returns the pipeline node's ID.
   * @return IdType
   */
  IdType getId() const;

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
  IPipelineNode();

  /**
   * @brief Clears the node's dirty flag.
   * 
   * This is a protected method and should only be called from within the
   * execute(DataStructure&) method.
   */
  void markNotDirty();

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
  bool m_IsDirty = true;
  DataStructure m_DataStructure;
};
} // namespace complex
