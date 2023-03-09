// Catch2 recommends placing these lines by themselves in a translation unit
// which will help reduce unnecessary recompilations of the expensive Catch main
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "ITKImageProcessing/ITKImageProcessingPlugin.hpp"

namespace
{
/**
 * @brief Listens for the beginning of the test run and
 * registers the ITK ImageIO factories once.
 */
struct ITKTestRunListener : Catch::TestEventListenerBase
{
  using TestEventListenerBase::TestEventListenerBase;

  void testRunStarting([[maybe_unused]] const Catch::TestRunInfo& testInfo) override
  {
    ITKImageProcessingPlugin::RegisterITKImageIO();
  }
};
} // namespace

CATCH_REGISTER_LISTENER(ITKTestRunListener)
