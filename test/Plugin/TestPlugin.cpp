#include "TestPlugin.hpp"
#include "TestFilter.hpp"

using namespace complex;

const AbstractPlugin::IdType TestPlugin::ID = "05cc618b-781f-4ac0-b9ac-43f26ce1854f";

AbstractFilter* createTestFilter()
{
  return new TestFilter();
}

AbstractPlugin* initPlugin()
{
  return new TestPlugin();
}

TestPlugin::TestPlugin()
: AbstractPlugin(ID, "Test Plugin", "Description")
{
  addFilter(&createTestFilter);
}

TestPlugin::~TestPlugin() = default;
