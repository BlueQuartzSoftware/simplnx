#include <optional>

#include "Test2Filter.hpp"
#include "Test2Plugin.hpp"

using namespace complex;

const AbstractPlugin::IdType Test2Plugin::ID = Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854e").value();

IFilter::UniquePointer createTestFilter()
{
  return IFilter::UniquePointer(new Test2Filter());
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
