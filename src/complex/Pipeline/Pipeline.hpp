#pragma once

#include <vector>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/AbstractPipelineNode.hpp"
#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"

namespace complex
{
class FilterHandle;
class FilterList;

/**
 * @class Pipeline
 * @brief The Pipeline class is a type of AbstractPipelineNode that contains a
 * list of AbstractPipelineNodes in a specific order that can be chained
 * together and executed as a single entity.
 *
 * As a subclass of AbstractPipelineNode, Pipelines can be nested within other
 * Pipelines.
 */
class COMPLEX_EXPORT Pipeline : public AbstractPipelineNode
{
  using node_type = std::shared_ptr<AbstractPipelineNode>;
  using collection_type = std::vector<node_type>;

public:
  using index_type = uint64;
  using iterator = collection_type::iterator;
  using const_iterator = collection_type::const_iterator;

  /**
   * @brief Constructs a pipeline with the specified name. If no name is
   * provided, a default name of "Unnamed Pipeline" will be used.
   * @param name = "Unnamed Pipeline"
   */
  Pipeline(const std::string& name = "Unnamed Pipeline", FilterList* filterList = nullptr);

  /**
   * @brief Creates a copy of the specified Pipeline.
   * @param other
   */
  Pipeline(const Pipeline& other);

  /**
   * @brief Creates a new Pipeline and moves values from the specified Pipeline.
   * @param other
   */
  Pipeline(Pipeline&& other) noexcept;

  ~Pipeline() override;

  /**
   * @brief Returns the node type for quick type checking.
   * @return NodeType
   */
  NodeType getType() const override;

  /**
   * @brief Returns the pipeline's name.
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
   * @brief Returns the getSize of the pipeline segment.
   * @return usize
   */
  usize size() const;

  /**
   * @brief Returns a pointer to the pipeline node at the given index.
   * This function does not include bounds checking.
   * @param index
   * @return AbstractPipelineNode*
   */
  AbstractPipelineNode* operator[](index_type index);

  /**
   * @brief Returns a pointer to the pipeline node at the given index.
   * Returns nullptr if the specified index is greater than or equal to the
   * getSize of the pipeline.
   * @param index
   * @return AbstractPipelineNode*
   */
  AbstractPipelineNode* at(index_type index);

  /**
   * @brief Returns a const pointer to the pipeline node at the given index.
   * Returns nullptr if the specified index is greater than or equal to the
   * getSize of the pipeline.
   * @param index
   * @return const AbstractPipelineNode*
   */
  const AbstractPipelineNode* at(index_type index) const;

  /**
   * @brief Inserts the provided pipeline node at the specified index. Returns
   * true if the process succeeds. Returns false otherwise.
   * @param index
   * @param node
   * @return bool
   */
  bool insertAt(index_type index, AbstractPipelineNode* node);

  /**
   * @brief Moves and inserts the provided filter at the specified index.
   * Returns true if the process succeeds. Returns false otherwise.
   *
   * This will always fail if the provided filter is null.
   * @param index
   * @param filter
   * @param args = {}
   * @return bool
   */
  bool insertAt(index_type index, IFilter::UniquePointer&& filter, const Arguments& args = {});

  /**
   * @brief Inserts a filter using the provided FilterHandle and Arguments at
   * the specified index. Returns true if the process succeeds. Returns false
   * otherwise.
   *
   * This method will fail if the FilterHandle does not point to a known filter.
   * @param index
   * @param handle
   * @param args = {}
   * @return bool
   */
  bool insertAt(index_type index, const FilterHandle& handle, const Arguments& args = {});

  /**
   * @brief Inserts a pipeline node at the specified iterator position. Returns
   * true if the process succeeds. Returns false otherwise.
   *
   * This will always fail if the provided AbstractPipelineNode is null.
   * @param pos
   * @param ptr
   * @return bool
   */
  bool insertAt(iterator pos, const std::shared_ptr<AbstractPipelineNode>& ptr);

  /**
   * @brief Removes the specified pipeline node from the pipeline segment.
   * Returns true if the node is removed. Returns false otherwise.
   * @param node
   * @return bool
   */
  bool remove(AbstractPipelineNode* node);

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
  bool contains(AbstractPipelineNode* node) const;

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
  bool push_front(AbstractPipelineNode* node);

  /**
   * @brief Inserts a PipelineFilter at the front of the pipeline segment based
   * on the target FilterHandle and Arguments. Returns true if the process
   * succeeds. Returns false otherwise. This will always fail if no matching
   * IFilter can be constructed from the FilterHandle.
   * @param handle
   * @param args = {}
   * @return bool
   */
  bool push_front(const FilterHandle& handle, const Arguments& args = {});

  /**
   * @brief Inserts a new filter node at the front of the pipeline using the
   * specified values. Returns true if the operation succeeded. Returns false
   * otherwise.
   * @param filter
   * @param args = {}
   * @return bool
   */
  bool push_front(IFilter::UniquePointer&& filter, const Arguments& args = {});

  /**
   * @brief Inserts the specified pipeline node at the end of the pipeline.
   * Returns true if the operation succeeded. Returns false otherwise. This
   * will always fail if the given node is null.
   * @param node
   * @return bool
   */
  bool push_back(AbstractPipelineNode* node);

  /**
   * @brief Inserts a pipeline node at the back of the pipeline segment based
   * on the target FilterHandle and Arguments. Returns true if the operation
   * succeeded. Returns false otherwise. This will always fail if an IFilter
   * could not be constructed for the given FilterHandle.
   * @param handle
   * @param args = {}
   * @return bool
   */
  bool push_back(const FilterHandle& handle, const Arguments& args = {});

  /**
   * @brief Inserts a new filter node at the back of the pipeline using the
   * specified values. Returns true if the operation succeeded. Returns false
   * otherwise. This will always fail if the given filter is null.
   * @param filter
   * @param args = {}
   * @return bool
   */
  bool push_back(IFilter::UniquePointer&& filter, const Arguments& args = {});

  /**
   * @brief Returns an iterator to the pipeline node. Returns an iterator to
   * the end of the pipeline segment if the node is not found
   * @param node
   * @return iterator
   */
  iterator find(AbstractPipelineNode* node);

  /**
   * @brief Returns a const_iterator to the pipeline node. Returns a
   * const_iterator to the end of the pipeline segment if the node is not found.
   * @param node
   * @return const_iterator
   */
  const_iterator find(AbstractPipelineNode* node) const;

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

  /**
   * @brief Copies the target Pipeline.
   * @param rhs
   * @return Pipeline&
   */
  Pipeline& operator=(const Pipeline& rhs) noexcept;

  /**
   * @brief Moves values from the target Pipeline.
   * @param rhs
   * @return Pipeline&
   */
  Pipeline& operator=(Pipeline&& rhs) noexcept;

  /**
   * @brief Returns true if the Pipeline has had a FilterList assigned to it.
   * Returns false otherwise.
   * @return bool
   */
  bool hasFilterList() const;

  /**
   * @brief Returns a pointer to the assigned FilterList. Returns nullptr if
   * a FilterList has not been assigned.
   * @return complex::FilterList*
   */
  complex::FilterList* getFilterList() const;

  /**
   * @brief Assigns a new FilterList for use when adding filters by
   * FilterHandle. Passing nullptr will clear the assigned FilterList and use
   * the current Application instance's FilterList instead.
   * @param filterList
   */
  void setFilterList(complex::FilterList* filterList);

private:
  /**
   * @brief Returns a pointer to the active FilterList that should be used for
   * creating IFilters. Returns the assigned FilterList pointer if one was
   * provided. Otherwise, this returns the current Application's FilterList.
   * @return complex::FilterList*
   */
  complex::FilterList* getActiveFilterList() const;

  ////////////
  // Variables
  std::string m_Name;
  collection_type m_Collection;
  FilterList* m_FilterList = nullptr;
};
} // namespace complex
