#include "TestPlugin.h"
#include "TestFilter.h"

using namespace Complex;

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
: AbstractPlugin("Test Plugin", "Description", ID)
{
  addFilter(&createTestFilter);
}

TestPlugin::~TestPlugin() = default;
