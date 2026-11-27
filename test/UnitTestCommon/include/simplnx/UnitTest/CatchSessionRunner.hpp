#pragma once

#ifndef CATCH_CONFIG_RUNNER
#error "CatchSessionRunner.hpp requires a CATCH_CONFIG_RUNNER translation unit"
#endif

#include <catch2/catch.hpp>

#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>

namespace nx::core::unit_test
{
/**
 * @struct CatchSessionResult
 * @brief Keeps successful listing counts distinct from Catch failures with the same integer status.
 */
struct CatchSessionResult
{
  int exit_code = 1;
  bool successful = false;
};

/**
 * @brief Runs Catch and destroys its Session before returning the outcome.
 * @param argc Original command-line argument count.
 * @param argv Original command-line arguments.
 * @return Catch status and success, or an unsuccessful status of 255 after a caught exception.
 * @pre No Catch Session has been constructed in this process.
 * @note The caller owns profile installation and Application teardown.
 */
inline CatchSessionResult RunCatchSession(int argc, char* argv[])
{
  CatchSessionResult result;
  try
  {
    Catch::Session session;
    result.exit_code = session.applyCommandLine(argc, argv);
    if(result.exit_code == 0)
    {
      const auto& data = session.configData();
      const bool listing = data.listTests || data.listTestNamesOnly || data.listTags || data.listReporters;
      if(listing && !data.showHelp && !data.libIdentify)
      {
        // Catch::Session maps listing exceptions and a successful count of 255 to the same status.
        // Use Catch's listing API inside this exception boundary to distinguish those outcomes.
        if((data.waitForKeypress & Catch::WaitForKeypress::BeforeStart) != 0)
        {
          Catch::cout() << "...waiting for enter/ return before starting" << '\n';
          static_cast<void>(std::getchar());
        }
        auto config = std::make_shared<Catch::Config>(data);
        Catch::seedRng(*config);
        if(data.filenamesAsTags)
        {
          Catch::applyFilenamesAsTags(*config);
        }
        result.exit_code = static_cast<int>(*Catch::list(config));
        if((data.waitForKeypress & Catch::WaitForKeypress::BeforeExit) != 0)
        {
          Catch::cout() << "...waiting for enter/ return before exiting, with code: " << result.exit_code << '\n';
          static_cast<void>(std::getchar());
        }
        result.successful = true;
      }
      else
      {
        result.exit_code = session.run();
        result.successful = result.exit_code == 0;
      }
    }
  } catch(const std::exception& exception)
  {
    std::fprintf(stderr, "Test runner failed: %s\n", exception.what());
    result = {.exit_code = 255, .successful = false};
  } catch(...)
  {
    std::fputs("Test runner failed with an unknown exception\n", stderr);
    result = {.exit_code = 255, .successful = false};
  }
  return result;
}
} // namespace nx::core::UnitTest
