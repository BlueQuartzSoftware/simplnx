#include "Test2Plugin.hpp"
#include "Test2Filter.hpp"

using namespace complex;

const AbstractPlugin::IdType Test2Plugin::ID = "05cc618b-781f-4ac0-b9ac-43f26ce1854e";

IFilter* createTestFilter()
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

std::vector<complex::IH5DataFactory*> Test2Plugin::getDataFactories() const
{
  return {};
}
