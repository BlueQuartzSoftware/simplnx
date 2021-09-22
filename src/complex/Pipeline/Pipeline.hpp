#pragma once

#include <vector>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/IPipelineNode.hpp"
#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"

namespace complex
{
class FilterHandle;

/**
 * @class Pipeline
 * @brief The Pipeline class is a type of IPipelineNode that contains a list of
 * IPipelineNodes in a specific order that can be chained together and executed as
 * a single entity.
 * As a subclass of IPipelineNode, Pipelines can be nested within other Pipelines.
 */
class COMPLEX_EXPORT Pipeline : public IPipelineNode
{
  using node_type = std::shared_ptr<IPipelineNode>;
  using collection_type = std::vector<node_type>;

public:
  using index_type = uint64_t;
  using iterator = collection_type::iterator;
  using const_iterator = collection_type::const_iterator;

  /**
   * @brief Constructs a pipeline with the specified name. If no name is
   * provided, a default name of "Unnamed Pipeline" will be used.
   * @param name
   */
  Pipeline(const std::string& name = "Unnamed Pipeline");

  /**
   * @brief Copy constructor
   * @param other
   */
  Pipeline(const Pipeline& other);

  /**
   * @brief Move constructor
   * @param other
   */
  Pipeline(Pipeline&& other) noexcept;

  virtual ~Pipeline();

  /**
   * @brief Returns the node type for quick type checking.
   * @return NodeType
   */
  NodeType getType() const override;

  /**
   * @brief Returns the pipeline's current name.
   * @return std::string
   */
  std::string getName() override;

  /**
   * @brief Sets the pipeline's name.
   * @param name
   */
  void setName(const std::string& name);

  /**
   * @brief Preflights the pipeline segment using an empty DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @return bool
   */
  bool preflight();

  /**
   * @brief Executes the pipeline segment using an empty DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @return bool
   */
  bool execute();

  /**
   * @brief Preflights the pipeline segment using the provided DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @param ds
   * @return bool
   */
  bool preflight(DataStructure& ds) override;

  /**
   * @brief Executes the pipeline segment using the provided DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @param ds
   * @return bool
   */
  bool execute(DataStructure& ds) override;

  /**
   * @brief Preflights the pipeline segment from a target position using the
   * provided DataStructure.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @param ds
   * @return bool
   */
  bool preflightFrom(const index_type& index, DataStructure& ds);

  /**
   * @brief Preflights the pipeline segment from a target position using the
   * DataStructure at the previous index. Uses an empty DataStructure if index is 0.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @return
   */
  bool preflightFrom(const index_type& index);

  /**
   * @brief Checks if the pipeline can be preflighted at the target index.
   *
   * Returns true if the preceeding node has preflight data or the target
   * index is 0. Returns false otherwise.
   * @param index
   * @return bool
   */
  bool canPreflightFrom(const index_type& index) const;

  /**
   * @brief Checks if the pipeline can be executed at the target index.
   *
   * Returns true if the preceeding node has execution data or the target
   * index is 0. Returns false otherwise.
   * @param index
   * @return bool
   */
  bool canExecuteFrom(const index_type& index) const;

  /**
   * @brief Executes the pipeline segment from a target position using the
   * provided DataStructure.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @param ds
   * @return bool
   */
  bool executeFrom(const index_type& index, DataStructure& ds);

  /**
   * @brief Executes the pipeline segment from the target position using the
   * previous node's DataStructure. Starts with an empty DataStructure if
   * index is 0.
   *
   * Returns true if the pipeline execution completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @return
   */
  bool executeFrom(const index_type& index);

  /**
   * @brief Returns the size of the pipeline segment.
   * @return size_t
   */
  size_t size() const;

  /**
   * @brief Returns a pointer to the pipeline node at the given index.
   * This function does not include bounds checking.
   * @param index
   * @return IPipelineNode*
   */
  IPipelineNode* operator[](index_type index);

  /**
   * @brief Returns a pointer to the pipeline node at the given index.
   * Returns nullptr if the specified index is greater than or equal to the
   * size of the pipeline.
   * @param index
   * @return const IPipelineNode*
   */
  IPipelineNode* at(index_type index);

  /**
   * @brief Returns a const pointer to the pipeline node at the given index.
   * Returns nullptr if the specified index is greater than or equal to the
   * size of the pipeline.
   * @param index
   * @return const IPipelineNode*
   */
  const IPipelineNode* at(index_type index) const;

  /**
   * @brief Inserts the provided pipeline node at the specified index. Returns
   * true if the process succeeds. Returns false otherwise.
   * @param index
   * @param node
   * @return bool
   */
  bool insertAt(index_type index, IPipelineNode* node);

  /**
   * @brief Inserts the provided filter at the specified index. Returns true if
   * the process succeeds. Returns false otherwise.
   *
   * This will always fail if the provided filter is null.
   * @param index
   * @param filter
   * @return bool
   */
  bool insertAt(index_type index, IFilter::UniquePointer&& filter);

  /**
   * @brief Inserts a filter using the provided FilterHandle at the specified
   * index. Returns true if the process succeeds. Returns false otherwise.
   * @param index
   * @param handle
   * @return bool
   */
  bool insertAt(index_type index, const FilterHandle& handle);

  /**
   * @brief Inserts a pipeline node at the specified iterator position. Returns
   * true if the process succeeds. Returns false otherwise.
   *
   * This will always fail if the provided IPipelineNode is null.
   * @param pos
   * @param ptr
   * @return bool
   */
  bool insertAt(iterator pos, const std::shared_ptr<IPipelineNode>& ptr);

  /**
   * @brief Removes the specified pipeline node from the pipeline segment.
   * Returns true if the node is removed. Returns false otherwise.
   * @param node
   * @return bool
   */
  bool remove(IPipelineNode* node);

  /**
   * @brief Removes the pipeline node specified by the provided iterator.
   * Returns true if the node is removed. Returns false otherwise.
   * @param iter
   * @return bool
   */
  bool remove(iterator iter);

  /**
   * @brief Removes the pipeline node specified by the provided iterator.
   * Returns true if the node is removed. Returns false otherwise.
   * @param iter
   * @return bool
   */
  bool remove(const_iterator iter);

  /**
   * @brief Attempts to remove the pipeline node at the specified index.
   * Returns true if the node was removed. Returns false otherwise.
   * @param pos
   * @return bool
   */
  bool removeAt(index_type pos);

  /**
   * @brief Clears the pipeline of all nodes.
   */
  void clear();

  /**
   * @brief Returns true if the pipeline segment contains the specified node.
   * Otherwise, returns false.
   * @param node
   * @return bool
   */
  bool contains(IPipelineNode* node) const;

  /**
   * @brief Returns true if the pipeline segment contains the specified filter.
   * Otherwise, returns false.
   * @param filter
   * @return bool
   */
  bool contains(IFilter* filter) const;

  /**
   * @brief Inserts the specified pipeline node to the front of the pipeline
   * segment.
   * @param node
   */
  bool push_front(IPipelineNode* node);

  /**
   * @brief Inserts a pipeline node at the front of the pipeline segment based
   * on the target FilterHandle.
   * @param handle
   */
  bool push_front(const FilterHandle& handle);

  /**
   * @brief Inserts the specified pipeline node at the end of the pipeline segment.
   * @param node
   */
  bool push_back(IPipelineNode* node);

  /**
   * @brief Inserts a pipeline node at the back of the pipeline segment based
   * on the target FilterHandle.
   * @param handle
   */
  bool push_back(const FilterHandle& handle);

  /**
   * @brief Returns an iterator to the pipeline node with the specified ID.
   * Returns an iterator to the end of the pipeline segment if the node is not
   * found.
   * @param id
   * @return iterator
   */
  iterator find(const IdType& id);

  /**
   * @brief Returns a const_iterator to the pipeline node with the specified ID.
   * Returns a const_iterator to the end of the pipeline segment if the node is
   * not found.
   * @param id
   * @return const_iterator
   */
  const_iterator find(const IdType& id) const;

  /**
   * @brief Returns an iterator to the pipeline node. Returns an iterator to
   * the end of the pipeline segment if the node is not found
   * @param node
   * @return iterator
   */
  iterator find(IPipelineNode* node);

  /**
   * @brief Returns a const_iterator to the pipeline node. Returns a
   * const_iterator to the end of the pipeline segment if the node is not found.
   * @param node
   * @return const_iterator
   */
  const_iterator find(IPipelineNode* node) const;

  /**
   * @brief Returns an iterator to the beginning of the collection.
   * @return iterator
   */
  iterator begin();

  /**
   * @brief Returns an interator to the end of the collection.
   * @return iterator
   */
  iterator end();

  /**
   * @brief Returns a const_iterator to the beginning of the collection.
   * @return const_iterator
   */
  const_iterator begin() const;

  /**
   * @brief Returns a const_iterator to the end of the collection.
   * @return const_iterator
   */
  const_iterator end() const;

  Pipeline& operator=(const Pipeline& rhs) noexcept;
  Pipeline& operator=(Pipeline&& rhs) noexcept;

private:
  std::string m_Name;
  collection_type m_Collection;
};
} // namespace complex
