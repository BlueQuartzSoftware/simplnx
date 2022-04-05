#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "fmt/format.h"

#include "nlohmann/json.hpp"

#include "PRObserver.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Pipeline/Pipeline.hpp"

namespace fs = std::filesystem;
using namespace complex;

#if 0
#define TEST_PIPELINE
#else
#undef TEST_PIPELINE
#endif

void loadApp(complex::Application& app)
{
#if(__APPLE__)
  {
    fs::path appPath = app.getCurrentDir();
    appPath = appPath.parent_path();

    // Check if there is a Plugins Folder inside the app package
    if(fs::exists(appPath / "Plugins"))
    {
      appPath = appPath / "Plugins";
    }
    else // Climb out of the app package and look in the build directory
    {
      appPath = appPath.parent_path().parent_path();
    }
    app.loadPlugins(appPath, true);
  }
#else
  app.loadPlugins(app.getCurrentDir(), true);
#endif
}

#ifdef TEST_PIPELINE
Pipeline createTestPipeline()
{
  Uuid pluginId = Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value();

  FilterHandle createGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), pluginId);
  FilterHandle createArrayHandle(Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d").value(), pluginId);

  Arguments groupArgs;
  groupArgs.insert("Data_Object_Path", DataPath({"Foo"}));

  Arguments arrayArgs;
  arrayArgs.insert("numeric_type", NumericType::float32);
  arrayArgs.insert("component_count", std::make_any<uint64>(3));
  arrayArgs.insert("tuple_count", std::make_any<uint64>(5));
  arrayArgs.insert("output_data_array", DataPath({"Foo", "Bar"}));

  Pipeline pipeline;
  pipeline.push_back(createGroupHandle, groupArgs);
  pipeline.push_back(createArrayHandle, arrayArgs);

  return pipeline;
}
#endif

int main(int argc, char* argv[])
{
  complex::Application app;
  loadApp(app);

#ifdef TEST_PIPELINE
  Pipeline pipeline = createTestPipeline();
#else
  if(argc < 2)
  {
    std::cout << "PipelineRunner requires a filepath to run" << std::endl;
    return 0;
  }

  fs::path targetPath = argv[1];
  if(!fs::exists(targetPath))
  {
    std::cout << fmt::format("Path '{}' does not exist", targetPath.string()) << std::endl;
    return -1;
  }

  auto result = Pipeline::FromFile(targetPath);
  if(result.invalid())
  {
    std::cout << fmt::format("Could not load pipeline at path: '{}'", targetPath.string()) << std::endl;
    return -1;
  }

  std::cout << fmt::format("Executing pipeline at path: '{}'\n", targetPath.string()) << std::endl;

  Pipeline pipeline = result.value();
#endif
  PipelineRunner::PipelineObserver obs(&pipeline);
  pipeline.execute();

  std::cout << "\n---------------------------" << std::endl;
  std::cout << "Finished executing pipeline" << std::endl;

  return 0;
}
