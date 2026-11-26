#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/PadImageGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace pad_image_geometry
{
const DataPath k_InputGeometryPath({"ImageGeometry"});
const DataPath k_OutputGeometryPath({"OutputGeometry"});
const DataPath k_AttributeMatrixPath = k_InputGeometryPath.createChildPath("Cell Data");
const std::string k_OutputGeometryPathStr = "Padded Image Geometry";
} // namespace pad_image_geometry

/* **************************************************************************** */
/** @brief TEST CASE 0 */
static Arguments Args_0()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 0"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 1 */
static Arguments Args_1()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 1"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 2 */
static Arguments Args_2()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 2"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 3 */
static Arguments Args_3()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 3"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 4 */
static Arguments Args_4()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 4"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 5 */
static Arguments Args_5()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 5"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 6 */
static Arguments Args_6()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 6"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 7 */
static Arguments Args_7()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 7"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 8 */
static Arguments Args_8()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 8"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 9 */
static Arguments Args_9()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 9"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 10 */
static Arguments Args_10()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 10"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 11 */
static Arguments Args_11()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 11"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 12 */
static Arguments Args_12()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 12"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 13 */
static Arguments Args_13()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 13"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 14 */
static Arguments Args_14()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 14"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 15 */
static Arguments Args_15()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 15"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(true));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(true));
  return args;
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Valid Filter Execution", "[SimplnxCore][PadImageGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_2_PadImageGeometry.tar.gz", "7_2_PadImageGeometry");

  std::vector<Arguments> args = {
      Args_0(), Args_1(), Args_2(), Args_3(), Args_4(), Args_5(), Args_6(), Args_7(), Args_8(), Args_9(), Args_10(), Args_11(), Args_12(), Args_13(), Args_14(), Args_15(),
  };

  for(size_t idx = 0; idx < args.size(); idx++)
  {
    INFO(fmt::format("Test Case {}", idx));
    DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/7_2_PadImageGeometry/Input_Geometry.dream3d", unit_test::k_TestFilesDir)));

    DataStructure exemplarDataStructure = LoadDataStructure(fs::path(fmt::format("{}/7_2_PadImageGeometry/Test_Case_{}.dream3d", unit_test::k_TestFilesDir, idx)));

    // Instantiate the filter, a DataStructure object and an Arguments Object
    PadImageGeometryFilter filter;

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args[idx]);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args[idx]);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, pad_image_geometry::k_AttributeMatrixPath, dataStructure, pad_image_geometry::k_AttributeMatrixPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][PadImageGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "PadImageGeometryFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<PadImageGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(PadImageGeometryFilter::k_AttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<int32>(PadImageGeometryFilter::k_DefaultFillValue_Key) == 5);
      CHECK(args.value<bool>(PadImageGeometryFilter::k_UpdateOrigin_Key) == true);
    }
  }
}

TEST_CASE("SimplnxCore::PadImageGeometry: Child names containing the geometry name are kept", "[SimplnxCore][PadImageGeometry]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  const DataPath destGeomPath({"Geom Out"});
  auto* geomPtr = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  REQUIRE(geomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setCellData(*elementAmPtr);
  auto* dataPtr =
      Int32Array::Create(dataStructure, "Data", DataStoreUtilities::CreateDataStore<int32>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                         elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr = Int32Array::Create(dataStructure, "Geom Values",
                                       DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(42);
  PadImageGeometryFilter filter;
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(destGeomPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(srcGeomPath.createChildPath("Cell Data")));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<std::vector<int32>>(std::vector<int32>{1, 1}));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.outputActions.value().actions)
    {
      const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
      if(candidatePtr != nullptr && candidatePtr->path() == srcGeomPath.createChildPath("Geom Feature Data"))
      {
        copyActionPtr = candidatePtr;
        break;
      }
    }
    REQUIRE(copyActionPtr != nullptr);
    const auto createdPaths = copyActionPtr->getAllCreatedPaths();
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedValuesPath) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), renamedAmPath.createChildPath("Geom Out Values")) == createdPaths.end());
  }
  SECTION("Copied objects keep child names")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataAs<AttributeMatrix>(copiedAmPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Int32Array>(copiedValuesPath) != nullptr);
    REQUIRE_FALSE(dataStructure.containsData(renamedAmPath));
    REQUIRE(dataStructure.getDataAs<Int32Array>(destGeomPath.createChildPath("Cell Data").createChildPath("Data")) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
ImageGeom* MakeValidationImage(DataStructure& ds, const SizeVec3& dims = {2, 2, 2}, const FloatVec3& spacing = {1, 1, 1}, const FloatVec3& origin = {0, 0, 0})
{
  auto* geom = ImageGeom::Create(ds, "ValidationImage");
  REQUIRE(geom != nullptr);
  geom->setDimensions(dims);
  REQUIRE(geom->setSpacing(spacing).valid());
  REQUIRE(geom->setOrigin(origin).valid());
  auto* cells = AttributeMatrix::Create(ds, "Cells", {dims[2], dims[1], dims[0]}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  return geom;
}
} // namespace

namespace
{
Arguments ValidationPaddingArgs()
{
  PadImageGeometryFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, DataPath({"ValidationImage", "Cells"}));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, false);
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, DataPath({"Padded"}));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, true);
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, false);
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, false);
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, VectorInt32Parameter::ValueType{1, 0});
  return args;
}
} // namespace
TEST_CASE("SimplnxCore::PadImageGeometry: Padding signs regression", "[SimplnxCore][PadImageGeometry][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5);
  CAPTURE(row);
  DataStructure ds;
  MakeValidationImage(ds);
  auto args = ValidationPaddingArgs();
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, row / 2 == 0);
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, row / 2 == 1);
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, row / 2 == 2);
  args.insertOrAssign(row / 2 == 0 ? PadImageGeometryFilter::k_XMinMax_Key :
                      row / 2 == 1 ? PadImageGeometryFilter::k_YMinMax_Key :
                                     PadImageGeometryFilter::k_ZMinMax_Key,
                      row % 2 == 0 ? VectorInt32Parameter::ValueType{-1, 0} : VectorInt32Parameter::ValueType{0, -1});
  PadImageGeometryFilter filter;
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors()[0].code == -86019 - row);
  CHECK(result.outputActions.errors()[0].message.find(row / 2 == 0 ? "X" : row / 2 == 1 ? "Y" : "Z") != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(row % 2 == 0 ? "Min" : "Max") != std::string::npos);
}
TEST_CASE("SimplnxCore::PadImageGeometry: Origin result regression", "[SimplnxCore][PadImageGeometry][NumericDomainRed]")
{
  DataStructure ds;
  auto* geom = MakeValidationImage(ds, {2, 2, 2}, {3e38f, 1, 1}, {-3e38f, 0, 0});
  auto args = ValidationPaddingArgs();
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, true);
  PadImageGeometryFilter filter;
  auto pre = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  REQUIRE_FALSE(pre.outputActions.warnings().empty());
  CHECK(pre.outputActions.warnings()[0].code == -86032);
  CHECK(pre.outputActions.warnings()[0].message.find("X Min & Max Padding' Min value 1 with origin -3e+38 and spacing 3e+38 produces origin result -inf") != std::string::npos);
  DataStructure preflightDs = ds;
  const auto preflightActionResult = pre.outputActions.value().applyRegular(preflightDs, IDataAction::Mode::Preflight);
  const auto& preflightGeom = preflightDs.getDataRefAs<ImageGeom>(DataPath({"ValidationImage"}));
  CHECK(preflightGeom.getOrigin() == FloatVec3{-3e38f, 0, 0});
  CHECK(preflightGeom.getSpacing() == FloatVec3{3e38f, 1, 1});
  CHECK_FALSE(preflightDs.containsData(DataPath({"Padded"})));
  auto result = filter.execute(ds, args);
  for(const auto* failure : std::array<const Result<>*, 2>{&preflightActionResult, &result.result})
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(*failure);
    REQUIRE(failure->errors().size() == 1);
    CHECK(failure->errors().front().code == -1557);
    CHECK(failure->errors().front().message.find("'Padded'") != std::string::npos);
    CHECK(failure->errors().front().message.find("Origin on the X axis must be finite. Received -inf.") != std::string::npos);
  }
  CHECK(geom->getOrigin() == FloatVec3{-3e38f, 0, 0});
  CHECK(geom->getSpacing() == FloatVec3{3e38f, 1, 1});
  CHECK_FALSE(ds.containsData(DataPath({"Padded"})));
}

TEST_CASE("SimplnxCore::PadImageGeometry: Representability and inactive padding", "[SimplnxCore][PadImageGeometry]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6);
  DataStructure ds;
  auto* geom = MakeValidationImage(ds, {1, 1, 1});
  auto args = ValidationPaddingArgs();
  const int32 max = std::numeric_limits<int32>::max();
  if(row == 0)
  {
    args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, true);
    args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, true);
    for(auto key : {PadImageGeometryFilter::k_XMinMax_Key, PadImageGeometryFilter::k_YMinMax_Key, PadImageGeometryFilter::k_ZMinMax_Key})
    {
      args.insertOrAssign(key, VectorInt32Parameter::ValueType{max, max});
    }
  }
  else if(row == 1 || row == 2)
  {
    args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, true);
    args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, VectorInt32Parameter::ValueType{1073741824, 1073741823});
    args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, VectorInt32Parameter::ValueType{1073741824, 1073741823});
    auto* cells = geom->getCellData();
    REQUIRE(cells != nullptr);
    if(row == 1)
    {
      REQUIRE(Int32Array::CreateWithStore<Int32DataStore>(ds, "Values", {1, 1, 1}, {1}, cells->getId()) != nullptr);
    }
    else
    {
      REQUIRE(UInt8Array::CreateWithStore<UInt8DataStore>(ds, "Values", {1, 1, 1}, {4}, cells->getId()) != nullptr);
    }
  }
  else if(row == 3)
  {
    args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, VectorInt32Parameter::ValueType{-1, -1});
  }
  else if(row == 4)
  {
    args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, VectorInt32Parameter::ValueType{1});
  }
  else if(row == 5)
  {
    args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, VectorInt32Parameter::ValueType{0, 0});
  }
  else
  {
    args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, VectorInt32Parameter::ValueType{1, 1});
    args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, true);
    args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, VectorInt32Parameter::ValueType{0, 0});
  }
  PadImageGeometryFilter filter;
  auto pre = filter.preflight(ds, args);
  if(row == 4)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(pre.outputActions);
    CHECK(pre.outputActions.errors()[0].code == -1);
    return;
  }
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  if(row == 0)
  {
    REQUIRE(pre.outputActions.warnings().size() == 1);
    CHECK(pre.outputActions.warnings()[0].code == -86031);
    auto applied = pre.outputActions.value().applyRegular(ds, IDataAction::Mode::Preflight);
    SIMPLNX_RESULT_REQUIRE_VALID(applied);
    auto* output = ds.getDataAs<ImageGeom>(DataPath({"Padded"}));
    REQUIRE(output != nullptr);
    CHECK(output->getDimensions() == SizeVec3{1, 1, 1});
    DataStructure executeDs;
    auto* source = MakeValidationImage(executeDs, {1, 1, 1});
    auto result = filter.execute(executeDs, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    CHECK(result.result.errors()[0].code == -86026);
    CHECK(source->getDimensions() == SizeVec3{1, 1, 1});
  }
  else if(row == 1 || row == 2)
  {
    auto applied = pre.outputActions.value().applyRegular(ds, IDataAction::Mode::Preflight);
    SIMPLNX_RESULT_REQUIRE_INVALID(applied);
    REQUIRE(applied.errors()[0].code == -270);
    CHECK(applied.errors()[0].message.find(row == 1 ? "byte" : "value") != std::string::npos);
  }
}
TEST_CASE("SimplnxCore::PadImageGeometry: Execute active vector validation", "[SimplnxCore][PadImageGeometry]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4);
  DataStructure ds;
  auto* geom = MakeValidationImage(ds, {2, 2, 1});
  auto* cells = geom->getCellData();
  REQUIRE(cells != nullptr);
  auto* marker = Int32Array::CreateWithStore<Int32DataStore>(ds, "Marker", {1, 2, 2}, {1}, cells->getId());
  REQUIRE(marker != nullptr);
  for(usize idx = 0; idx < 4; ++idx)
  {
    (*marker)[idx] = static_cast<int32>(idx + 1);
  }
  auto args = ValidationPaddingArgs();
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, VectorInt32Parameter::ValueType{1, 1});
  PadImageGeometryFilter filter;
  auto pre = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  auto applied = pre.outputActions.value().applyRegular(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
  auto* dest = ds.getDataAs<Int32Array>(DataPath({"Padded", "Cells", "Marker"}));
  REQUIRE(dest != nullptr);
  dest->fill(99);
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, row == 0 ? VectorInt32Parameter::ValueType{-1, 0} : row == 1 ? VectorInt32Parameter::ValueType{1} : VectorInt32Parameter::ValueType{1, 1});
  if(row >= 3)
  {
    args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, row == 3);
    args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, row == 4);
    args.insertOrAssign(row == 3 ? PadImageGeometryFilter::k_YMinMax_Key : PadImageGeometryFilter::k_ZMinMax_Key, VectorInt32Parameter::ValueType{});
  }
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, DataPath({"FilteredPadded"}));
  auto result = filter.execute(ds, args);
  if(row != 2)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    CHECK(result.result.errors()[0].code == (row == 0 ? -86019 : -1));
    CHECK_FALSE(ds.containsData(DataPath({"FilteredPadded"})));
    for(usize idx = 0; idx < 8; ++idx)
    {
      CHECK((*dest)[idx] == 99);
    }
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    dest = ds.getDataAs<Int32Array>(DataPath({"FilteredPadded", "Cells", "Marker"}));
    REQUIRE(dest != nullptr);
    const std::array<int32, 8> expected = {0, 1, 2, 0, 0, 3, 4, 0};
    for(usize idx = 0; idx < 8; ++idx)
    {
      CHECK((*dest)[idx] == expected[idx]);
    }
  }
  for(usize idx = 0; idx < 4; ++idx)
  {
    CHECK((*marker)[idx] == idx + 1);
  }
}

namespace
{
DataStructure CreatePadFeatureFixture(bool addFeatureAm, bool addEnsembleAm)
{
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom"});
  auto* geomPtr = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  REQUIRE(geomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(cellAmPtr != nullptr);
  geomPtr->setCellData(*cellAmPtr);
  auto* featureIdsPtr =
      Int32Array::Create(dataStructure, "Feature Ids",
                         DataStoreUtilities::CreateDataStore<int32>(dataStructure, srcGeomPath.createChildPath("Cell Data").createChildPath("Feature Ids"), {2, 2, 2}, {1}), cellAmPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  featureIdsPtr->fill(1);
  if(addFeatureAm)
  {
    REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", {2}, geomPtr->getId()) != nullptr);
  }
  if(addEnsembleAm)
  {
    REQUIRE(AttributeMatrix::Create(dataStructure, "Ensemble Data", {2}, geomPtr->getId()) != nullptr);
  }
  return dataStructure;
}

Arguments MakePadArgs(bool padX, bool padY, bool padZ, const std::vector<int32>& xMinMax, const std::vector<int32>& yMinMax, const std::vector<int32>& zMinMax, int32 fillValue, bool updateOrigin)
{
  const DataPath srcGeomPath({"Geom"});
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Geom"})));
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(srcGeomPath.createChildPath("Cell Data")));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(false));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(padX));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(padY));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(padZ));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(xMinMax));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(yMinMax));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(zMinMax));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(fillValue));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(updateOrigin));
  return args;
}

usize CountWarnings(const std::vector<Warning>& warnings, int32 code)
{
  return static_cast<usize>(std::count_if(warnings.begin(), warnings.end(), [code](const Warning& warning) { return warning.code == code; }));
}
} // namespace

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Default settings", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(true, false);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(true, true, true, {0, 0}, {0, 0}, {0, 0}, 0, GENERATE(false, true));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Nonzero default padding value", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(false, false);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(true, true, true, {0, 1}, {0, 0}, {0, 0}, 5, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 1);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  const auto warningIter = std::find_if(warnings.begin(), warnings.end(), [](const Warning& warning) { return warning.code == -67401; });
  REQUIRE(warningIter != warnings.end());
  REQUIRE(warningIter->message.find('5') != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Nonzero fill without added cells", "[SimplnxCore][PadImageGeometryFilter][L21]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(true, true);

  const bool padX = GENERATE(false, true);
  CAPTURE(padX);
  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(padX, true, true, padX ? VectorInt32Parameter::ValueType{0, 0} : VectorInt32Parameter::ValueType{2, 1}, {0, 0}, {0, 0}, 5, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Minimum padding with copied Attribute Matrices", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(true, true);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(true, true, true, {2, 0}, {0, 0}, {0, 0}, 0, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Minimum padding with Update Origin on", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(true, false);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(true, true, true, {2, 0}, {0, 0}, {0, 0}, 0, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Minimum padding with only Cell Data", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(false, false);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(true, true, true, {2, 0}, {0, 0}, {0, 0}, 0, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Maximum padding only", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(true, false);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(true, true, true, {0, 2}, {0, 0}, {0, 0}, 0, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Preflight Disabled X minimum padding", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreatePadFeatureFixture(true, false);

  PadImageGeometryFilter filter;
  Arguments args = MakePadArgs(false, true, true, {2, 0}, {0, 2}, {0, 0}, 0, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(CountWarnings(warnings, -67401) == 0);
  REQUIRE(CountWarnings(warnings, -67402) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometry: Zero padding on enabled axes preserves Cell Data", "[SimplnxCore][PadImageGeometry][L21]")
{
  UnitTest::LoadPlugins();
  const bool inPlace = GENERATE(false, true);
  const usize paddedAxis = GENERATE(0, 1, 2, 3); // 3 means all enabled axes have zero padding.
  const bool padZ = GENERATE(false, true);
  CAPTURE(inPlace, paddedAxis, padZ);
  if(paddedAxis == 2 && !padZ)
  {
    return;
  }

  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Geom");
  REQUIRE(geom != nullptr);
  geom->setDimensions({2, 2, 1});
  REQUIRE(geom->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(geom->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellAm = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 2, 2}, geom->getId());
  REQUIRE(cellAm != nullptr);
  geom->setCellData(*cellAm);
  const DataPath cellPath({"Geom", "Cell Data"});
  auto* scalar = Int32Array::Create(dataStructure, "Scalar", DataStoreUtilities::CreateDataStore<int32>(dataStructure, cellPath.createChildPath("Scalar"), {1, 2, 2}, {1}), cellAm->getId());
  auto* vector = Int32Array::Create(dataStructure, "Vector", DataStoreUtilities::CreateDataStore<int32>(dataStructure, cellPath.createChildPath("Vector"), {1, 2, 2}, {2}), cellAm->getId());
  REQUIRE(scalar != nullptr);
  REQUIRE(vector != nullptr);
  const std::vector<int32> inputScalar = {1, 2, 3, 4};
  const std::vector<int32> inputVector = {11, 12, 21, 22, 31, 32, 41, 42};
  for(usize i = 0; i < inputScalar.size(); ++i)
  {
    (*scalar)[i] = inputScalar[i];
  }
  for(usize i = 0; i < inputVector.size(); ++i)
  {
    (*vector)[i] = inputVector[i];
  }

  // X is fastest. A minimum-side cell shifts every source tuple by one on the selected axis.
  const std::vector<std::vector<int32>> expectedScalars = {{9, 1, 2, 9, 3, 4}, {9, 9, 1, 2, 3, 4}, {9, 9, 9, 9, 1, 2, 3, 4}, {1, 2, 3, 4}};
  const std::vector<std::vector<int32>> expectedVectors = {
      {9, 9, 11, 12, 21, 22, 9, 9, 31, 32, 41, 42}, {9, 9, 9, 9, 11, 12, 21, 22, 31, 32, 41, 42}, {9, 9, 9, 9, 9, 9, 9, 9, 11, 12, 21, 22, 31, 32, 41, 42}, {11, 12, 21, 22, 31, 32, 41, 42}};
  Arguments args = MakePadArgs(true, true, padZ, {paddedAxis == 0 ? 1 : 0, 0}, {paddedAxis == 1 ? 1 : 0, 0}, {paddedAxis == 2 ? 1 : 0, 0}, 9, false);
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(inPlace));
  PadImageGeometryFilter filter;
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath outputPath = inPlace ? cellPath : DataPath({"Padded Geom", "Cell Data"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputPath.createChildPath("Scalar")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(outputPath.createChildPath("Vector")));
  const auto& outputScalar = dataStructure.getDataRefAs<Int32Array>(outputPath.createChildPath("Scalar"));
  const auto& outputVector = dataStructure.getDataRefAs<Int32Array>(outputPath.createChildPath("Vector"));
  REQUIRE(outputScalar.getSize() == expectedScalars[paddedAxis].size());
  REQUIRE(outputVector.getSize() == expectedVectors[paddedAxis].size());
  for(usize i = 0; i < outputScalar.getSize(); ++i)
  {
    CHECK(outputScalar[i] == expectedScalars[paddedAxis][i]);
  }
  for(usize i = 0; i < outputVector.getSize(); ++i)
  {
    CHECK(outputVector[i] == expectedVectors[paddedAxis][i]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PadImageGeometryFilter: Nested Feature AM padding warning", "[SimplnxCore][PadImageGeometryFilter]")
{
  UnitTest::LoadPlugins();
  auto ds = CreatePadFeatureFixture(false, false);
  auto* geom = ds.getDataAs<ImageGeom>(DataPath({"Geom"}));
  REQUIRE(geom != nullptr);
  auto* group = DataGroup::Create(ds, "Nested", geom->getId());
  REQUIRE(group != nullptr);
  auto* features = AttributeMatrix::Create(ds, "Feature Data", {2}, group->getId());
  REQUIRE(features != nullptr);
  REQUIRE(UnitTest::CreateTestDataArray<float32>(ds, "Centroids", {2}, {3}, features->getId()) != nullptr);
  PadImageGeometryFilter filter;
  const bool updateOrigin = GENERATE(false, true);
  auto args = MakePadArgs(true, true, true, {2, 0}, {0, 0}, {0, 0}, 0, updateOrigin);
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(CountWarnings(result.outputActions.warnings(), -67402) == (updateOrigin ? 0 : 1));
}
