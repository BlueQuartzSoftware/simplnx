#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Pipeline/AbstractPipelineNode.hpp"
#include "simplnx/Pipeline/Messaging/PipelineNodeObserver.hpp"

#include <vector>

namespace nx::core
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
class SIMPLNX_EXPORT Pipeline : public AbstractPipelineNode, protected PipelineNodeObserver
{
  using node_type = std::shared_ptr<AbstractPipelineNode>;
  using collection_type = std::vector<node_type>;

public:
  using index_type = uint64;
  using iterator = collection_type::iterator;
  using const_iterator = collection_type::const_iterator;

  static constexpr StringLiteral k_Extension = ".d3dpipeline";

  /**
   * @brief Constructs a Pipeline from json.
   * @param json
   * @return
   */
  static Result<Pipeline> FromJson(const nlohmann::json& json);

  /**
   * @brief Constructs a Pipeline from json and the given filter list.
   * @param json
   * @param filterList
   * @return
   */
  static Result<Pipeline> FromJson(const nlohmann::json& json, FilterList* filterList);

  /**
   * @brief Constructs a Pipeline from a JSON file.
   * @param path
   * @return Result<Pipeline>
   */
  static Result<Pipeline> FromFile(const std::filesystem::path& path);

  /**
   * @brief Constructs a Pipeline from a JSON file with the given FilterList.
   * @param path
   * @param filterList
   * @return Result<Pipeline>
   */
  static Result<Pipeline> FromFile(const std::filesystem::path& path, FilterList* filterList);

  /**
   * @brief Attempts to read a SIMPL json pipeline and convert to a simplnx Pipeline.
   * @param json
   * @param filterList
   * @return
   */
  static Result<Pipeline> FromSIMPLJson(const nlohmann::json& json, FilterList* filterList);

  /**
   * @brief Attempts to read a SIMPL json pipeline and convert to a simplnx Pipeline.
   * @param json
   * @return
   */
  static Result<Pipeline> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Constructs a Pipeline from a SIMPL JSON file with the given FilterList.
   * @param path
   * @param filterList
   * @return Result<Pipeline>
   */
  static Result<Pipeline> FromSIMPLFile(const std::filesystem::path& path, FilterList* filterList);

  /**
   * @brief Constructs a Pipeline from a SIMPL JSON file.
   * @param path
   * @return Result<Pipeline>
   */
  static Result<Pipeline> FromSIMPLFile(const std::filesystem::path& path);

  /**
   * @brief Constructs a pipeline with the specified name. If no name is
   * provided, a default name of "Unnamed Pipeline" will be used.
   * @param name = "Unnamed Pipeline"
   */
  Pipeline(const std::string& name = "Untitled Pipeline", FilterList* filterList = nullptr);

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
  std::string getName() const override;

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
  bool preflight(const std::atomic_bool& shouldCancel = false, bool allowRenaming = false);

  /**
   * @brief Preflights the pipeline segment using the provided DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @param dataStructure
   * @param shouldCancel
   * @return bool
   */
  bool preflight(DataStructure& dataStructure, const std::atomic_bool& shouldCancel) override;

  /**
   * @brief Preflights the pipeline segment using the provided DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @param dataStructure
   * @param shouldCancel
   * @param allowRenaming
   * @return bool
   */
  bool preflight(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, bool allowRenaming);

  /**
   * @brief Executes the pipeline segment using an empty DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @return bool
   */
  bool execute(const std::atomic_bool& shouldCancel = false);

  /**
   * @brief Preflights the pipeline segment using the provided DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @param dataStructure
   * @param renamedPaths
   * @param shouldCancel
   * @param allowRenaming = false
   * @return bool
   */
  bool preflight(DataStructure& dataStructure, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming = false) override;

  /**
   * @brief Executes the pipeline segment using the provided DataStructure.
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise.
   * @param dataStructure
   * @return bool
   */
  bool execute(DataStructure& dataStructure, const std::atomic_bool& shouldCancel) override;

  /**
   * @brief Preflights the pipeline segment from a target position using the
   * provided DataStructure.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @param dataStructure
   * @return bool
   */
  bool preflightFrom(index_type index, DataStructure& dataStructure, const std::atomic_bool& shouldCancel = false, bool allowRenaming = false);

  /**
   * @brief Preflights the pipeline segment from a target position using the
   * provided DataStructure.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @param dataStructure
   * @param renamedPaths
   * @return bool
   */
  bool preflightFrom(index_type index, DataStructure& dataStructure, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel = false, bool allowRenaming = false);

  /**
   * @brief Preflights the pipeline segment from a target position using the
   * DataStructure at the previous index. Uses an empty DataStructure if index is 0.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @return bool
   */
  bool preflightFrom(index_type index, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel = false);

  /**
   * @brief Preflights the pipeline segment from a target position using the
   * DataStructure at the previous index. Uses an empty DataStructure if index is 0.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @param renamedPaths
   * @return bool
   */
  bool preflightFrom(index_type index, const std::atomic_bool& shouldCancel = false);

  /**
   * @brief Checks if the pipeline can be preflighted at the target index.
   *
   * Returns true if the preceeding node has preflight data or the target
   * index is 0. Returns false otherwise.
   * @param index
   * @return bool
   */
  bool canPreflightFrom(index_type index) const;

  /**
   * @brief Checks if the pipeline can be executed at the target index.
   *
   * Returns true if the preceeding node has execution data or the target
   * index is 0. Returns false otherwise.
   * @param index
   * @return bool
   */
  bool canExecuteFrom(index_type index) const;

  /**
   * @brief Executes the pipeline segment from a target position using the
   * provided DataStructure.
   *
   * Returns true if the pipeline segment completes without errors. Returns
   * false otherwise. Returns false if the starting index is out of bounds.
   * @param index
   * @param dataStructure
   * @return bool
   */
  bool executeFrom(index_type index, DataStructure& dataStructure, const std::atomic_bool& shouldCancel = false);

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
  bool executeFrom(index_type index, const std::atomic_bool& shouldCancel = false);

  /**
   * @brief Returns the getSize of the pipeline segment.
   * @return usize
   */
  usize size() const;

  /**
   * @brief Returns true if the pipeline is empty. Returns false otherwise.
   * @return bool
   */
  bool isEmpty() const;

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
  bool insertAt(index_type index, std::shared_ptr<AbstractPipelineNode> node);

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
   * @param node
   * @return bool
   */
  bool insertAt(iterator pos, std::shared_ptr<AbstractPipelineNode> node);

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
   * @brief Attempts to move a pipeline node at the specified index to a new
   * index in the pipeline. Returns true if the node was moved. Returns false
   * otherwise.
   * @param fromIndex
   * @param toIndex
   * @return bool
   */
  bool move(index_type fromIndex, index_type toIndex);

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
   * @brief Returns true if the pipeline contains a filter with the given uuid.
   * @param id
   * @return bool
   */
  bool contains(const Uuid& id) const;

  /**
   * @brief Inserts the specified pipeline node to the front of the pipeline
   * segment.
   * @param node
   */
  bool push_front(std::shared_ptr<AbstractPipelineNode> node);

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
  bool push_back(std::shared_ptr<AbstractPipelineNode> node);

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
  const_iterator find(const AbstractPipelineNode* node) const;

  /**
   * @brief Creates and returns a copy of the Pipeline.
   * @return std::unique_ptr<AbstractPipelineNode>
   */
  std::unique_ptr<AbstractPipelineNode> deepCopy() const override;

  /**
   * @brief Creates and returns a pipeline copying the nodes between two iterators.
   * @param start
   * @param end
   * @return std::unique_ptr<Pipeline>
   */
  std::unique_ptr<Pipeline> copySegment(const iterator& start, const iterator& end);

  /**
   * @brief Creates and returns a pipeline copying the nodes between two iterators.
   * @param start
   * @param end
   * @return std::unique_ptr<Pipeline>
   */
  std::unique_ptr<Pipeline> copySegment(const const_iterator& start, const const_iterator& end) const;

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
   * @return nx::core::FilterList*
   */
  nx::core::FilterList* getFilterList() const;

  /**
   * @brief Assigns a new FilterList for use when adding filters by
   * FilterHandle. Passing nullptr will clear the assigned FilterList and use
   * the current Application instance's FilterList instead.
   * @param filterList
   */
  void setFilterList(nx::core::FilterList* filterList);

  /**
   * @brief Preflights the pipeline and checks the maximum amount of memory required to run the pipeline.
   * @return Memory size in Bytes
   */
  uint64 checkMemoryRequired();

protected:
  /**
   * @brief Returns implementation-specific json value for the node.
   * This method should only be called from toJson().
   * @return
   */
  nlohmann::json toJsonImpl() const override;

  /**
   * @brief Called when the specified pipeline node emits a message.
   * @param node
   * @param msg
   */
  void onNotify(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg) override;

private:
  /**
   * @brief Returns a pointer to the active FilterList that should be used for
   * creating IFilters. Returns the assigned FilterList pointer if one was
   * provided. Otherwise, this returns the current Application's FilterList.
   * @return nx::core::FilterList*
   */
  nx::core::FilterList* getActiveFilterList() const;

  /**
   * @brief Resets all collection nodes' parent pipeline.
   */
  void resetCollectionParent();

  /**
   * @brief Returns true if the pipeline has encountered warnings before the
   * specified index. Returns false otherwise.
   * @param index
   * @return bool
   */
  bool hasWarningsBeforeIndex(index_type index) const;

  /**
   * @brief Returns true if the pipeline has encountered errors before the
   * specified index. Returns false otherwise.
   * @param index
   * @return bool
   */
  bool hasErrorsBeforeIndex(index_type index) const;

  ////////////
  // Variables
  std::string m_Name;
  collection_type m_Collection;
  FilterList* m_FilterList = nullptr;
  uint64 m_MemoryRequired = 0;
};
} // namespace nx::core
