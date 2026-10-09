#pragma once

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/UnitTest/CatchSessionRunner.hpp"
#include "simplnx/UnitTest/ProcessTestProfile.hpp"
#include "simplnx/UnitTest/TestApplicationCleanup.hpp"

#include <cstdio>
#include <exception>
#include <iostream>
#include <string>

namespace nx::core::UnitTest
{
/**
 * @brief Runs Catch with profile routing installed before listener construction.
 * @param argc Original command-line argument count.
 * @param argv Original command-line arguments.
 * @return Catch status, or a nonzero setup or observable teardown failure.
 */
inline int RunOwnedCatchSession(int argc, char* argv[])
{
  ProcessTestProfile profile;
  std::string error;
  CatchSessionResult result;
  try
  {
    if(Application::Instance() != nullptr)
    {
      std::cerr << "Application exists before test profile installation\n";
      return 1;
    }
    if(!profile.install(error))
    {
      std::cerr << "Test profile installation failed: " << error << '\n';
      return 1;
    }
    if(Preferences::DefaultFilePath("DREAM3DNX") != profile.corePreferencesFile())
    {
      std::cerr << "Core preferences do not resolve to the installed test profile\n";
    }
    else
    {
      result = RunCatchSession(argc, argv);
    }
  } catch(const std::exception& exception)
  {
    std::cerr << "Test runner failed: " << exception.what() << '\n';
    result = {255, false};
  } catch(...)
  {
    std::fputs("Test runner failed with an unknown exception\n", stderr);
    result = {255, false};
  }

  // RunCatchSession destroys Session and its listeners before checked Application teardown.
  if(!FinishTestApplication())
  {
    result = {1, false};
  }
  const int exitCode = profile.finishExitCode(result.exitCode, result.successful, error);
  if(!error.empty())
  {
    std::cerr << error << '\n';
  }
  return exitCode;
}
} // namespace nx::core::UnitTest
