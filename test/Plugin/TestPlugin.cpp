#include "TestPlugin.hpp"
#include "TestFilter.hpp"

using namespace complex;

const complex::AbstractPlugin::IdType TestPlugin::ID = complex::Uuid::FromString("fb357ceb-3d7b-406e-b4ed-adf371b65160").value();

complex::IFilter* createTestFilter()
{
  return new TestFilter();
}

complex::AbstractPlugin* initPlugin()
{
  return new TestPlugin();
}

TestPlugin::TestPlugin()
: complex::AbstractPlugin(ID, "Test Plugin", "Description")
{
  addFilter(&createTestFilter);
}

TestPlugin::~TestPlugin() = default;
