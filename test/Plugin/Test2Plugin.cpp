#include "Test2Plugin.h"
#include "Test2Filter.h"

using namespace SIMPL;

const AbstractPlugin::IdType Test2Plugin::ID = "05cc618b-781f-4ac0-b9ac-43f26ce1854e";

AbstractFilter* createTestFilter()
{
  return new Test2Filter();
}

AbstractPlugin* initPlugin()
{
  return new Test2Plugin();
}

Test2Plugin::Test2Plugin()
: AbstractPlugin(ID, "Test 2 Plugin", "Description")
{
  addFilter(&createTestFilter);
}

Test2Plugin::~Test2Plugin() = default;
