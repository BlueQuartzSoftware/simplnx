#include "ProgWorkshopPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{52ce1c70-74b1-581a-af32-0212c8396739}").value(), Uuid::FromString("52ce1c70-74b1-581a-af32-0212c8396739").value()}, /* Lesson1 */
    {Uuid::FromString("{f5295d39-4563-5a71-927d-3c0f5e36702a}").value(), Uuid::FromString("f5295d39-4563-5a71-927d-3c0f5e36702a").value()}, /* Lesson2 */
    {Uuid::FromString("{c712cd5a-5223-58a8-a0bd-14c06009b2ed}").value(), Uuid::FromString("c712cd5a-5223-58a8-a0bd-14c06009b2ed").value()}, /* Lesson3 */
    {Uuid::FromString("{43bceb39-849d-5c9a-a75c-5ef958908007}").value(), Uuid::FromString("43bceb39-849d-5c9a-a75c-5ef958908007").value()}, /* Lesson4 */
    {Uuid::FromString("{169f34c2-62b2-57cf-ac79-9067dbac0d73}").value(), Uuid::FromString("169f34c2-62b2-57cf-ac79-9067dbac0d73").value()}, /* Lesson5 */
    {Uuid::FromString("{b2eb2376-9c34-5744-bbb3-05c8874a3a31}").value(), Uuid::FromString("b2eb2376-9c34-5744-bbb3-05c8874a3a31").value()}, /* Lesson6 */
    {Uuid::FromString("{a4b86d5f-d812-50d9-8d95-3f3f5fcb0597}").value(), Uuid::FromString("a4b86d5f-d812-50d9-8d95-3f3f5fcb0597").value()}, /* Lesson7 */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("35442d72-ad89-5db3-9c45-2eb8d8a787c9");
} // namespace

ProgWorkshopPlugin::ProgWorkshopPlugin()
: AbstractPlugin(k_ID, "ProgWorkshop", "<<--Description was not read-->>", "Open-Source")
{
  registerFilters();
}

ProgWorkshopPlugin::~ProgWorkshopPlugin() = default;

std::vector<complex::H5::IDataFactory*> ProgWorkshopPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ProgWorkshopPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ProgWorkshop/plugin_filter_registration.h"
