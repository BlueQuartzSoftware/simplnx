#pragma once

#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/AbstractPipelineNode.hpp"

#include <nod/nod.hpp>

namespace complex
{
class FilterHandle;
class FilterList;

/**
 * @class PipelineFilter
 * @brief The PipelineFilter class is a pipeline node specialized for wrapping an
 * IFilter object. The node keeps track of the resulting DataStructure as well
 * as error and warning messages.
 */
class COMPLEX_EXPORT PipelineFilter : public AbstractPipelineNode
{
public:
  using WarningsChangedSignal = nod::signal<void(std::vector<complex::Warning>)>;
  using ErrorsChangedSignal = nod::signal<void(std::vector<complex::Error>)>;

  /**
   * @brief Attempts to construct a PipelineFilter based on the specified
   * FilterHandle. Returns nullptr if the corresponding filter could not be
   * created. Otherwise, this method returns a pointer to the created
   * PipelineFilter.
   * @param handle
   * @param args = {}
   * @param filterList = nullptr
   * @return std::unique_ptr<PipelineFilter>
   */
  static std::unique_ptr<PipelineFilter> Create(const FilterHandle& handle, const Arguments& args = {}, FilterList* filterList = nullptr);

  /**
   * @brief Constructs a PipelineFilter from json. Returns nullptr on failure.
   * @param json
   * @return std::unique_ptr<PipelineFilter>
   */
  static Result<std::unique_ptr<PipelineFilter>> FromJson(const nlohmann::json& json);

  /**
   * @brief Constructs a PipelineFilter from json using the given filter list. Returns nullptr on failure.
   * @param json
   * @param filterList
   * @return std::unique_ptr<PipelineFilter>
   */
  static Result<std::unique_ptr<PipelineFilter>> FromJson(const nlohmann::json& json, const FilterList& filterList);

  /**
   * @brief Constructs a PipelineFilter from SIMPL json using the given filter list. Returns nullptr on failure.
   * @param json
   * @param filterList
   * @return std::unique_ptr<PipelineFilter>
   */
  static Result<std::unique_ptr<PipelineFilter>> FromSIMPLJson(const nlohmann::json& json, const FilterList& filterList);

  /**
   * @brief Creates a comment string from the errors collection
   * @param errors The errors to convert to strings for storing in a comment
   * @param prefix The comment string prefix for each of the errors
   * @return The converted comment string containing all of the errors
   */
  static std::string CreateErrorComments(const complex::ErrorCollection& errors, const std::string& prefix);

  /**
   * @brief Constructs a PipelineFilter with the provided filter and arguments.
   * If no Arguments are provided, a default empty value will be used instead.
   *
   * The PipelineFilter takes ownership of the std::unique_ptr<IFilter>.
   * @param filter
   * @param args = {}
   */
  PipelineFilter(IFilter::UniquePointer&& filter, const Arguments& args = {});

  ~PipelineFilter() override;

  /**
   * @brief Returns the node type for quick type checking.
   * @return NodeType
   */
  NodeType getType() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  std::string getName() const override;

  /**
   * @brief Returns a pointer to the wrapped filter.
   * @return IFilter*
   */
  IFilter* getFilter() const;

  /**
   * @brief Returns a collection of the filter's parameters.
   * @return Parameters
   */
  Parameters getParameters() const;

  /**
   * @brief Returns a collection of the node's filter arguments.
   * @return Arguments
   */
  Arguments getArguments() const;

  /**
   * @brief Sets the node's filter arguments and marks the node as dirty.
   * @param args
   */
  void setArguments(const Arguments& args);

  /**
   * @brief Sets the index of this filter in an executing pipeline
   * @param index
   */
  void setIndex(int32 index);

  /**
   * @brief Gets the node's filter comments.
   */
  const std::string& getComments() const;

  /**
   * @brief Sets the node's filter comments.
   * @param comments
   */
  void setComments(const std::string& comments);

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @param renamedPaths Collection of renamed output paths.
   * @return bool
   */
  bool preflight(DataStructure& data, const std::atomic_bool& shouldCancel) override;

  /**
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @param renamedPaths Collection of renamed output paths.
   * @return bool
   */
  bool preflight(DataStructure& data, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming) override;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  bool execute(DataStructure& data, const std::atomic_bool& shouldCancel) override;

  /**
   * @brief Returns a vector of DataPaths created when preflighting the node.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getCreatedPaths() const;

  /**
   * @brief Returns a vector of DataPaths that would be modified when executing the node
   * @return std::vector<DataPath>
   */
  std::vector<DataObjectModification> getDataObjectModificationNotifications() const;

  /**
   * @brief Returns a collection of warnings returned by the target filter.
   * This collection is cleared when the node is preflighted or executed.
   * @return std::vector<complex::Warning>
   */
  std::vector<complex::Warning> getWarnings() const;

  /**
   * @brief Returns a collection of errors emitted by the target filter.
   * This collection is cleared when the node is preflighted or executed.
   * @return std::vector<complex::Error>
   */
  std::vector<complex::Error> getErrors() const;

  /**
   * @brief Returns a collection of preflight values emitted by the target filter.
   * This collection is cleared when the node is preflighted or executed.
   * @return std::vector<complex::Error>
   */
  const std::vector<IFilter::PreflightValue>& getPreflightValues() const;

  /**
   * @brief Creates and returns a unique pointer to a copy of the node.
   * @return std::unique_ptr<AbstractPipelineNode>
   */
  std::unique_ptr<AbstractPipelineNode> deepCopy() const override;

  /**
   * @brief Adjusts arguments for renamed DataPaths.
   * @param renamedPaths
   */
  void renamePathArgs(const RenamedPaths& renamedPaths);

protected:
  /**
   * @brief Returns implementation-specific json value for the node.
   * This method should only be called from toJson().
   * @return
   */
  nlohmann::json toJsonImpl() const override;

  /**
   * @brief Notifies observers to an IFilter::Message emitted by the IFilter
   * during preflight or execute.
   * @param message
   */
  void notifyFilterMessage(const IFilter::Message& message);

  /**
   * @brief Notifies observers to a set of renamed DataPaths.
   * @param renamedPathPairs
   */
  void notifyRenamedPaths(const RenamedPaths& renamedPathPairs);

  /**
   * @brief Checks for the renaming of created DataPaths.
   * Emits notifications when a renaming is detected.
   *
   * This method does nothing if the number of created paths does not match
   * the current count.
   * @param oldCreatedPaths
   * @return RenamedTypes
   */
  RenamedPaths checkForRenamedPaths(std::vector<DataPath> oldCreatedPaths) const;

private:
  IFilter::UniquePointer m_Filter;
  Arguments m_Arguments;
  int32 m_Index = 0;
  std::string m_Comments;
  std::vector<complex::Warning> m_Warnings;
  std::vector<complex::Error> m_Errors;
  std::vector<IFilter::PreflightValue> m_PreflightValues;
  std::vector<DataPath> m_CreatedPaths;
  std::vector<DataObjectModification> m_DataModifiedActions;
};
} // namespace complex
