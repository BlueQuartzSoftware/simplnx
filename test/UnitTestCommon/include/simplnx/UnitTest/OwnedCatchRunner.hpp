#pragma once

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/UnitTest/ProcessTestProfile.hpp"

#include <catch2/catch.hpp>

#include <cstdio>
#include <iostream>
#include <memory>
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
  int result = 1;
  bool successful = false;
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
      // Session destruction releases listeners before Application saves and profile cleanup.
      Catch::Session session;
      result = session.applyCommandLine(argc, argv);
      if(result == 0)
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
          result = static_cast<int>(*Catch::list(config));
          if((data.waitForKeypress & Catch::WaitForKeypress::BeforeExit) != 0)
          {
            Catch::cout() << "...waiting for enter/ return before exiting, with code: " << result << std::endl;
            static_cast<void>(std::getchar());
          }
          successful = true;
        }
        else
        {
          result = session.run();
          successful = result == 0;
        }
      }
    }

    std::weak_ptr<Application> application;
    if(auto app = Application::Instance())
    {
      application = app;
      const auto saveResult = app->savePreferences();
      if(saveResult.invalid())
      {
        result = 1;
        successful = false;
        for(const auto& saveError : saveResult.errors())
        {
          std::cerr << "Test Application save failed: " << saveError.message << '\n';
        }
      }
    }
    // DeleteInstance and the destructor report additional save failures through logging only.
    Application::DeleteInstance();
    if(!application.expired())
    {
      std::cerr << "Application owner remains after test framework teardown\n";
      result = 1;
      successful = false;
    }
  } catch(const std::exception& exception)
  {
    std::cerr << "Test runner failed: " << exception.what() << '\n';
    result = 255;
    successful = false;
  }
  result = profile.finishExitCode(result, successful, error);
  if(!error.empty())
  {
    std::cerr << error << '\n';
  }
  return result;
}
} // namespace nx::core::UnitTest
