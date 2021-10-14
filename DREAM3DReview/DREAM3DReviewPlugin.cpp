#include "DREAM3DReviewPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("fb840d2c-6cdc-53a6-b472-b596dcb4693c");
} // namespace

DREAM3DReviewPlugin::DREAM3DReviewPlugin()
: AbstractPlugin(k_ID, "DREAM3DReview", "Description", "BlueQuartz Software")
{
  registerFilters();
}

DREAM3DReviewPlugin::~DREAM3DReviewPlugin() = default;

std::vector<complex::H5::IDataFactory*> DREAM3DReviewPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(DREAM3DReviewPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "DREAM3DReview/plugin_filter_registration.h"
