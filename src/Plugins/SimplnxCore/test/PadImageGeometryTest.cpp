
#include <catch2/catch.hpp>

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/PadImageGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace pad_image_geometry
{
const DataPath k_InputGeometryPath({"ImageGeometry"});
const DataPath k_OutputGeometryPath({"OutputGeometry"});
const DataPath k_AttributeMatrixPath = k_InputGeometryPath.createChildPath("Cell Data");
const std::string k_OutputGeometryPathStr = "Padded Image Geometry";
#define True true
#define False false
} // namespace pad_image_geometry

/* **************************************************************************** */
/** @brief TEST CASE 0 */
Arguments Args_0()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 0"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 1 */
Arguments Args_1()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 1"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 2 */
Arguments Args_2()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 2"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 3 */
Arguments Args_3()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 3"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 4 */
Arguments Args_4()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 4"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 5 */
Arguments Args_5()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 5"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 6 */
Arguments Args_6()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 6"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 7 */
Arguments Args_7()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 7"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 8 */
Arguments Args_8()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 8"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 9 */
Arguments Args_9()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 9"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 10 */
Arguments Args_10()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 10"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 11 */
Arguments Args_11()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 11"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 12 */
Arguments Args_12()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 12"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 13 */
Arguments Args_13()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 13"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 0}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{0, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 14 */
Arguments Args_14()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(False));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 14"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
  return args;
}

/* **************************************************************************** */
/** @brief TEST CASE 15 */
Arguments Args_15()
{
  Arguments args;
  args.insertOrAssign(PadImageGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(pad_image_geometry::k_AttributeMatrixPath));
  args.insertOrAssign(PadImageGeometryFilter::k_PadXDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadYDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_PadZDim_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_DefaultFillValue_Key, std::make_any<int32>(0));
  args.insertOrAssign(PadImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(pad_image_geometry::k_InputGeometryPath));
  args.insertOrAssign(PadImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Padded Image Geometry 15"})));
  args.insertOrAssign(PadImageGeometryFilter::k_PerformInPlace_Key, std::make_any<bool>(True));
  args.insertOrAssign(PadImageGeometryFilter::k_XMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_YMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_ZMinMax_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{10, 10}));
  args.insertOrAssign(PadImageGeometryFilter::k_UpdateOrigin_Key, std::make_any<bool>(True));
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
