#pragma once

#include <memory>
#include <string>

namespace SIMPL
{
class FilterList;
class PipelineHandler;

/**
 * class RestServer
 *
 */

class RestServer
{
public:
  // Constructors/Destructors
  //

  /**
   * Empty Destructor
   */
  virtual ~RestServer();

  // Static Public attributes
  //

  // Public attributes
  //

  /**
   * @brief
   * @return int32_t
   */
  int32_t getPort() const;

  /**
   * @brief
   * @param json
   */
  void handleMsg(std::string json);

protected:
private:
  SIMPL::FilterList* m_FilterList;
  std::unique_ptr<SIMPL::PipelineHandler> m_PipelineHandler;
};
} // namespace SIMPL
