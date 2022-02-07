#pragma once

#include "nod/nod.hpp"

#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/AbstractPipelineNode.hpp"

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
   * @brief Attempts to preflight the node using the provided DataStructure.
   * Returns true if preflighting succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  bool preflight(DataStructure& data) override;

  /**
   * @brief Attempts to execute the node using the provided DataStructure.
   * Returns true if execution succeeded. Otherwise, this returns false.
   * @param data
   * @return bool
   */
  bool execute(DataStructure& data) override;

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
   * @brief Converts the current node to json.
   * @return nlohmann::json
   */
  nlohmann::json toJson() const override;

protected:
  /**
   * @brief Notifies observers to an IFilter::Message emitted by the IFilter
   * during preflight or execute.
   * @param message
   */
  void notifyFilterMessage(const IFilter::Message& message);

private:
  IFilter::UniquePointer m_Filter;
  Arguments m_Arguments;
  int32 m_Index = 0;

  std::vector<complex::Warning> m_Warnings;
  std::vector<complex::Error> m_Errors;
  std::vector<IFilter::PreflightValue> m_PreflightValues;
};
} // namespace complex
