#pragma once

#include <string>
#include <vector>

#include "Complex/Filtering/AbstractFilter.h"
#include "Complex/Filtering/PipelinePath.h"
#include "Complex/REST/PipelineController.h"

namespace Complex
{

/**
 * class PipelineHandler
 *
 */

class PipelineHandler
{
public:
  /**
   * Default Constructor
   */
  PipelineHandler();

  virtual ~PipelineHandler();

  /**
   * @brief
   * @param position
   * @param filter
   */
  void addFilter(Complex::PipelinePath position, AbstractFilter::IdType filter);

  /**
   * @brief
   * @return std::vector<PipelineController>
   */
  std::vector<PipelineController> getPipelineControllers() const;

  /**
   * @brief
   * @param json
   */
  PipelineController::IdType createPipeline(const std::string& json);

  /**
   * @brief Executes the target pipeline.
   * @param id
   */
  void execute(PipelineController::IdType id);

  /**
   * @brief Preflights the target pipeline.
   * @param  id
   */
  void preflight(PipelineController::IdType id);

  /**
   * @brief Checks for the specified PipelineController ID. Returns true if
   * the PipelineController can be found. Returns false otherwise.
   * @param  id
   * @return bool
   */
  bool containsPipeline(PipelineController::IdType id) const;

  /**
   * @brief Closes the pipeline based on the specified ID.
   * @param id
   */
  void closePipeline(PipelineController::IdType id);

protected:
private:
  std::vector<PipelineController> m_PipelineControllers;
};
} // namespace SIMPL
