#pragma once

#include "Complex/Filtering/Pipeline.h"

namespace Complex
{

/**
 * class PipelineController
 *
 */

class PipelineController
{
public:
  using IdType = size_t;

  /**
   * @brief
   * @param  pipeline
   */
  PipelineController(Complex::Pipeline* pipeline);

  virtual ~PipelineController();

  /**
   * @brief Returns the ID for looking up the managed pipeline.
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief Returns a pointer to the managed pipeline.
   * @return Complex::Pipeline*
   */
  Complex::Pipeline* getPipeline() const;

  /**
   * @brief Begins to preflight the managed pipeline.
   */
  void preflight();

  /**
   * @brief Begins to execute the managed pipeline.
   */
  void execute();

  /**
   * @brief Cancels the execution or preflighting of the managed pipeline.
   */
  void cancel();

protected:
private:
  std::unique_ptr<Complex::Pipeline> m_Pipeline;
  IdType m_Id;
};
} // namespace Complex
