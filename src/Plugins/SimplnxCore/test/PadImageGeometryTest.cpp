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

TEST_CASE("SimplnxCore::PadImageGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][PadImageGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

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
  geomPtr->setOrigin({0.0F, 0.0F, 0.0F});
  geomPtr->setSpacing({1.0F, 1.0F, 1.0F});
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
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
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
  CHECK(pre.outputActions.warnings()[0].message.find("X") != std::string::npos);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors()[0].code == -86027);
  CHECK(geom->getOrigin()[0] == -3e38f);
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
