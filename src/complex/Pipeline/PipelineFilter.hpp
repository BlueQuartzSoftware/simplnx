#pragma once

#include "nod/nod.hpp"

#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/AbstractPipelineNode.hpp"

namespace complex
{
class FilterHandle;

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
   * @return PipelineFilter*
   */
  static PipelineFilter* Create(const FilterHandle& handle);

  /**
   * @brief Constructs a PipelineFilter with the specified filter.
   *
   * The PipelineFilter takes ownership of the std::unique_ptr<IFilter>.
   * @param filter
   */
  PipelineFilter(IFilter::UniquePointer&& filter);

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
  std::string getName() override;

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
  std::vector<Warning> getWarnings() const;

  /**
   * @brief Returns a collection of errors emitted by the target filter.
   * This collection is cleared when the node is preflighted or executed.
   * @return std::vector<complex::Error>
   */
  std::vector<Error> getErrors() const;

private:
  IFilter::UniquePointer m_Filter;
  Arguments m_Arguments;
  std::vector<Warning> m_Warnings;
  std::vector<Error> m_Errors;
  WarningsChangedSignal m_WarningsSignal;
  ErrorsChangedSignal m_ErrorsSignal;
};
} // namespace complex
