#pragma once

#ifndef CATCH_CONFIG_RUNNER
#error "CatchSessionRunner.hpp requires a CATCH_CONFIG_RUNNER translation unit"
#endif

#include <catch2/catch.hpp>

#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>

namespace nx::core::UnitTest
{
/**
 * @struct CatchSessionResult
 * @brief Keeps successful listing counts distinct from Catch failures with the same integer status.
 */
struct CatchSessionResult
{
  int exitCode = 1;
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
    result.exitCode = session.applyCommandLine(argc, argv);
    if(result.exitCode == 0)
    {
      const auto& data = session.configData();
      const bool listing = data.listTests || data.listTestNamesOnly || data.listTags || data.listReporters;
      if(listing && !data.showHelp && !data.libIdentify)
      {
        // Catch::Session maps listing exceptions and a successful count of 255 to the same status.
        // Use Catch's listing API inside this exception boundary to distinguish those outcomes.
        if((data.waitForKeypress & Catch::WaitForKeypress::BeforeStart) != 0)
        {
          Catch::cout() << "...waiting for enter/ return before starting" << std::endl;
          static_cast<void>(std::getchar());
        }
        auto config = std::make_shared<Catch::Config>(data);
        Catch::seedRng(*config);
        if(data.filenamesAsTags)
        {
          Catch::applyFilenamesAsTags(*config);
        }
        result.exitCode = static_cast<int>(*Catch::list(config));
        if((data.waitForKeypress & Catch::WaitForKeypress::BeforeExit) != 0)
        {
          Catch::cout() << "...waiting for enter/ return before exiting, with code: " << result.exitCode << std::endl;
          static_cast<void>(std::getchar());
        }
        result.successful = true;
      }
      else
      {
        result.exitCode = session.run();
        result.successful = result.exitCode == 0;
      }
    }
  } catch(const std::exception& exception)
  {
    std::fprintf(stderr, "Test runner failed: %s\n", exception.what());
    result = {255, false};
  } catch(...)
  {
    std::fputs("Test runner failed with an unknown exception\n", stderr);
    result = {255, false};
  }
  return result;
}
} // namespace nx::core::UnitTest
