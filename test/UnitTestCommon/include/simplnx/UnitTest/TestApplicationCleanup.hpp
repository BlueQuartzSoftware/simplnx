#pragma once

#include "simplnx/Core/Application.hpp"

#include <cstdio>
#include <exception>
#include <memory>

namespace nx::core::UnitTest
{
/**
 * @brief Checks the existing Application save, deletes the singleton, and checks for retained owners.
 * @return True if the checked save and deletion complete without errors and no observed owner remains.
 * @pre Known framework, worker, and UI owners have released Application under the required runtime scopes.
 * @note Does not construct Application or control Qt, Python, backend, or profile lifetimes.
 * @note Caught save errors do not skip deletion. A terminating noexcept destructor cannot be recovered here.
 */
inline bool FinishTestApplication() noexcept
{
  bool successful = true;
  std::weak_ptr<Application> application;
  try
  {
    if(auto app = Application::Instance())
    {
      application = app;
      const auto saveResult = app->savePreferences();
      if(saveResult.invalid())
      {
        successful = false;
        for(const auto& saveError : saveResult.errors())
        {
          std::fprintf(stderr, "Test Application save failed: %s\n", saveError.message.c_str());
        }
      }
    }
  } catch(const std::exception& exception)
  {
    std::fprintf(stderr, "Test Application save failed: %s\n", exception.what());
    successful = false;
  } catch(...)
  {
    std::fputs("Test Application save failed with an unknown exception\n", stderr);
    successful = false;
  }

  // The temporary strong owner leaves scope before deletion and the weak-owner check.
  // DeleteInstance and the destructor report additional save failures through logging only.
  try
  {
    Application::DeleteInstance();
  } catch(const std::exception& exception)
  {
    std::fprintf(stderr, "Test Application deletion failed: %s\n", exception.what());
    successful = false;
  } catch(...)
  {
    std::fputs("Test Application deletion failed with an unknown exception\n", stderr);
    successful = false;
  }
  if(!application.expired())
  {
    std::fputs("Application owner remains after test framework teardown\n", stderr);
    successful = false;
  }
  return successful;
}
} // namespace nx::core::UnitTest
