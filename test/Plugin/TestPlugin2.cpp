#include "Test2Plugin.h"
#include "Test2Filter.h"

AbstractFilter* createTestFilter()
{
  return new TestFilter2();
}

AbstractPlugin* initPlugin()
{
  return new TestPlugin2();
}

TestPlugin2::TestPlugin2()
: AbstractPlugin("Test Plugin", "Description", "05cc618b-781f-4ac0-b9ac-43f26ce1854e")
{
}

TestPlugin2::~TestPlugin2() = default;

void TestPlugin2::initialize()
{
  addFilter(&createTestFilter);
  setLoaded();
}
