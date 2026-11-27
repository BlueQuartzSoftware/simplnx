#include "ImageFilterIntegrationTestUtils.hpp"

#include "NXImageProcessing/Filters/ConnectedComponentImageFilter.hpp"
#include "SimplnxCore/Filters/ConvertDataFilter.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <string>

namespace image_integration
{
// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx result assertion macros.
void ExecuteChecked(const IFilter& filter, DataStructure& dataStructure, const Arguments& args)
{
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

DataPath ConvertToFloat32(DataStructure& dataStructure, const DataPath& input)
{
  const auto filter = Application::Instance()->getFilterList()->createFilter(FilterTraits<ConvertDataFilter>::uuid);
  REQUIRE(filter != nullptr);
  auto args = filter->getDefaultArguments();
  args.insertOrAssign(ConvertDataFilter::k_ScalarType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(DataType::float32)));
  args.insertOrAssign(ConvertDataFilter::k_ArrayToConvertPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ConvertDataFilter::k_ConvertedArrayName_Key, std::make_any<std::string>("Converted"));
  args.insertOrAssign(ConvertDataFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
  ExecuteChecked(*filter, dataStructure, args);
  return input.replaceName("Converted");
}

DataPath Components(DataStructure& dataStructure, const DataPath& input, const std::string& name)
{
  const ConnectedComponentImageFilter filter;
  auto args = ImageArguments(filter, input, name);
  args.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  ExecuteChecked(filter, dataStructure, args);
  return input.replaceName(name);
}

void SetGeometry(DataStructure& dataStructure)
{
  ImageGeom* geometryPtr = nullptr;
  REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"})));
  const auto spacingResult = geometryPtr->setSpacing({1, 2, 3});
  SIMPLNX_RESULT_REQUIRE_VALID(spacingResult);
  const auto originResult = geometryPtr->setOrigin({4, 5, 6});
  SIMPLNX_RESULT_REQUIRE_VALID(originResult);
  geometryPtr->setUnits(IGeometry::LengthUnit::Millimeter);
}

void CheckGeometry(DataStructure& dataStructure, const SizeVec3Type& dimensions)
{
  const ImageGeom* geometryPtr = nullptr;
  REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"})));
  REQUIRE(geometryPtr->getDimensions() == dimensions);
  REQUIRE(geometryPtr->getSpacing() == FloatVec3Type{1, 2, 3});
  REQUIRE(geometryPtr->getOrigin() == FloatVec3Type{4, 5, 6});
  REQUIRE(geometryPtr->getUnits() == IGeometry::LengthUnit::Millimeter);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
} // namespace image_integration
