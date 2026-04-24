#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ReadNIfTIFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SimplnxCore/utils/nifti1.h"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <zlib.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
struct SyntheticNiftiParams
{
  std::array<int16_t, 3> dims{1, 1, 1};
  std::array<float, 3> spacing{1.0f, 1.0f, 1.0f};
  std::array<float, 3> origin{0.0f, 0.0f, 0.0f};
  int16_t niftiDatatype{NIFTI_TYPE_UINT8};
  int16_t bitpix{8};
  float sclSlope{0.0f};
  float sclInter{0.0f};
  int16_t rank{3};      // dim[0]; override to 1/2/4 for rejection tests
  int16_t qformCode{1}; // set to 0 to disable qform
  float quaternB{0.0f};
  float quaternC{0.0f};
  float quaternD{0.0f};
  int16_t sformCode{0};                        // >0 enables srow_* population
  std::array<std::array<float, 4>, 3> sform{}; // 3x4 affine when sformCode > 0
};

nifti_1_header MakeHeader(const SyntheticNiftiParams& p)
{
  nifti_1_header hdr{};
  hdr.sizeof_hdr = 348;
  hdr.dim[0] = p.rank;
  hdr.dim[1] = p.dims[0];
  hdr.dim[2] = p.dims[1];
  hdr.dim[3] = p.dims[2];
  hdr.dim[4] = 1;
  hdr.dim[5] = 1;
  hdr.dim[6] = 1;
  hdr.dim[7] = 1;

  hdr.pixdim[0] = 1.0f;
  hdr.pixdim[1] = p.spacing[0];
  hdr.pixdim[2] = p.spacing[1];
  hdr.pixdim[3] = p.spacing[2];
  hdr.pixdim[4] = 0.0f;

  hdr.datatype = p.niftiDatatype;
  hdr.bitpix = p.bitpix;
  hdr.vox_offset = 352.0f;
  hdr.scl_slope = p.sclSlope;
  hdr.scl_inter = p.sclInter;

  hdr.qform_code = p.qformCode;
  hdr.qoffset_x = p.origin[0];
  hdr.qoffset_y = p.origin[1];
  hdr.qoffset_z = p.origin[2];
  hdr.quatern_b = p.quaternB;
  hdr.quatern_c = p.quaternC;
  hdr.quatern_d = p.quaternD;

  hdr.sform_code = p.sformCode;
  if(p.sformCode > 0)
  {
    for(int i = 0; i < 4; i++)
    {
      hdr.srow_x[i] = p.sform[0][i];
      hdr.srow_y[i] = p.sform[1][i];
      hdr.srow_z[i] = p.sform[2][i];
    }
  }

  std::memcpy(hdr.magic, "n+1\0", 4);
  return hdr;
}

template <typename T>
void ByteSwap(T& v)
{
  v = nx::core::byteswap(v);
}

// Mirrors the private ByteSwapHeader in NiftiUtilities.cpp so the test can
// synthesize files in foreign byte order and exercise the endian-detection
// path in ReadNiftiHeader.
void ByteSwapHeader(nifti_1_header& hdr)
{
  ByteSwap(hdr.sizeof_hdr);
  ByteSwap(hdr.extents);
  ByteSwap(hdr.session_error);
  for(auto& v : hdr.dim)
  {
    ByteSwap(v);
  }
  ByteSwap(hdr.intent_p1);
  ByteSwap(hdr.intent_p2);
  ByteSwap(hdr.intent_p3);
  ByteSwap(hdr.intent_code);
  ByteSwap(hdr.datatype);
  ByteSwap(hdr.bitpix);
  ByteSwap(hdr.slice_start);
  for(auto& v : hdr.pixdim)
  {
    ByteSwap(v);
  }
  ByteSwap(hdr.vox_offset);
  ByteSwap(hdr.scl_slope);
  ByteSwap(hdr.scl_inter);
  ByteSwap(hdr.slice_end);
  ByteSwap(hdr.cal_max);
  ByteSwap(hdr.cal_min);
  ByteSwap(hdr.slice_duration);
  ByteSwap(hdr.toffset);
  ByteSwap(hdr.glmax);
  ByteSwap(hdr.glmin);
  ByteSwap(hdr.qform_code);
  ByteSwap(hdr.sform_code);
  ByteSwap(hdr.quatern_b);
  ByteSwap(hdr.quatern_c);
  ByteSwap(hdr.quatern_d);
  ByteSwap(hdr.qoffset_x);
  ByteSwap(hdr.qoffset_y);
  ByteSwap(hdr.qoffset_z);
  for(auto& v : hdr.srow_x)
  {
    ByteSwap(v);
  }
  for(auto& v : hdr.srow_y)
  {
    ByteSwap(v);
  }
  for(auto& v : hdr.srow_z)
  {
    ByteSwap(v);
  }
}

template <typename T>
std::vector<uint8_t> ToBytesByteSwapped(std::vector<T> values)
{
  for(auto& v : values)
  {
    ByteSwap(v);
  }
  return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(values.data()), reinterpret_cast<const uint8_t*>(values.data()) + values.size() * sizeof(T));
}

void WriteNiftiFile(const fs::path& path, const nifti_1_header& hdr, const std::vector<uint8_t>& voxelBytes, bool gzipped)
{
  static constexpr std::array<uint8_t, 4> zeroExtension{0, 0, 0, 0};

  if(gzipped)
  {
    gzFile gz = gzopen(path.string().c_str(), "wb");
    REQUIRE(gz != nullptr);
    REQUIRE(gzwrite(gz, &hdr, static_cast<unsigned int>(sizeof(hdr))) == static_cast<int>(sizeof(hdr)));
    REQUIRE(gzwrite(gz, zeroExtension.data(), 4) == 4);
    if(!voxelBytes.empty())
    {
      REQUIRE(gzwrite(gz, voxelBytes.data(), static_cast<unsigned int>(voxelBytes.size())) == static_cast<int>(voxelBytes.size()));
    }
    gzclose(gz);
  }
  else
  {
    std::ofstream ofs(path, std::ios::binary);
    REQUIRE(ofs.is_open());
    ofs.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    ofs.write(reinterpret_cast<const char*>(zeroExtension.data()), 4);
    if(!voxelBytes.empty())
    {
      ofs.write(reinterpret_cast<const char*>(voxelBytes.data()), static_cast<std::streamsize>(voxelBytes.size()));
    }
    ofs.close();
  }
}

template <typename T>
std::vector<uint8_t> ToBytes(const std::vector<T>& values)
{
  std::vector<uint8_t> out(values.size() * sizeof(T));
  std::memcpy(out.data(), values.data(), out.size());
  return out;
}

Arguments MakeFilterArgs(const fs::path& inputFile, const DataPath& geomPath, const std::string& amName, const std::string& arrName, bool applyScaling, bool useAffine,
                         const CropGeometryParameter::ValueType& crop = CropGeometryParameter::ValueType{})
{
  Arguments args;
  args.insertOrAssign(ReadNIfTIFileFilter::k_InputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(inputFile));
  args.insertOrAssign(ReadNIfTIFileFilter::k_UseAffineIfPresent_Key, std::make_any<bool>(useAffine));
  args.insertOrAssign(ReadNIfTIFileFilter::k_ApplyScalingTransform_Key, std::make_any<bool>(applyScaling));
  args.insertOrAssign(ReadNIfTIFileFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(crop));
  args.insertOrAssign(ReadNIfTIFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ReadNIfTIFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(amName));
  args.insertOrAssign(ReadNIfTIFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>(arrName));
  return args;
}

fs::path OutputDir()
{
  fs::path dir = fs::path(std::string(nx::core::unit_test::k_BinaryTestOutputDir.view())) / "ReadNIfTIFile";
  fs::create_directories(dir);
  return dir;
}

template <typename T>
void RequireArrayEquals(const DataStructure& ds, const DataPath& dataArrayPath, const std::vector<T>& expected)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(dataArrayPath));
  const auto& arr = ds.getDataRefAs<DataArray<T>>(dataArrayPath);
  const auto& store = arr.getDataStoreRef();
  REQUIRE(store.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    const T actual = store[i];
    if(actual != expected[i])
    {
      UNSCOPED_INFO(fmt::format("Mismatch at index {}: expected {}, got {}", i, expected[i], actual));
      REQUIRE(actual == expected[i]);
    }
  }
}

void RequireFloatArrayApproxEquals(const DataStructure& ds, const DataPath& dataArrayPath, const std::vector<float>& expected, float tolerance = 1e-5f)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<float32>>(dataArrayPath));
  const auto& arr = ds.getDataRefAs<DataArray<float32>>(dataArrayPath);
  const auto& store = arr.getDataStoreRef();
  REQUIRE(store.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    const float actual = store[i];
    if(std::fabs(actual - expected[i]) > tolerance)
    {
      UNSCOPED_INFO(fmt::format("Mismatch at index {}: expected {}, got {}", i, expected[i], actual));
      REQUIRE(std::fabs(actual - expected[i]) <= tolerance);
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: uint8 round trip (.nii and .nii.gz)", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {4, 3, 2};
  const std::array<float, 3> spacing = {0.5f, 0.75f, 1.25f};
  const std::array<float, 3> origin = {-1.0f, 2.0f, 3.5f};

  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<uint8_t> voxels(numVoxels);
  std::iota(voxels.begin(), voxels.end(), static_cast<uint8_t>(0));

  SyntheticNiftiParams params;
  params.dims = dims;
  params.spacing = spacing;
  params.origin = origin;
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const DataPath geomPath({"NIfTI Image"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  for(bool gzipped : {false, true})
  {
    DYNAMIC_SECTION("format=" << (gzipped ? ".nii.gz" : ".nii"))
    {
      const fs::path filePath = outDir / (gzipped ? "uint8.nii.gz" : "uint8.nii");
      WriteNiftiFile(filePath, hdr, ToBytes(voxels), gzipped);

      DataStructure dataStructure;
      ReadNIfTIFileFilter filter;
      const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true);

      const auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

      const auto execute = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
      const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
      const auto actualDims = geom.getDimensions();
      const auto actualSpacing = geom.getSpacing();
      const auto actualOrigin = geom.getOrigin();
      constexpr float tol = 1e-5f;
      REQUIRE(actualDims[0] == static_cast<usize>(dims[0]));
      REQUIRE(actualDims[1] == static_cast<usize>(dims[1]));
      REQUIRE(actualDims[2] == static_cast<usize>(dims[2]));
      REQUIRE(std::fabs(actualSpacing[0] - spacing[0]) < tol);
      REQUIRE(std::fabs(actualSpacing[1] - spacing[1]) < tol);
      REQUIRE(std::fabs(actualSpacing[2] - spacing[2]) < tol);
      REQUIRE(std::fabs(actualOrigin[0] - origin[0]) < tol);
      REQUIRE(std::fabs(actualOrigin[1] - origin[1]) < tol);
      REQUIRE(std::fabs(actualOrigin[2] - origin[2]) < tol);

      RequireArrayEquals<uint8>(dataStructure, arrPath, voxels);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: int16 with scaling transform", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {3, 3, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<int16_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<int16_t>(i) - 4;
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_INT16;
  params.bitpix = 16;
  params.sclSlope = 0.25f;
  params.sclInter = 10.0f;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "int16_scaled.nii";
  WriteNiftiFile(filePath, hdr, ToBytes(voxels), /*gzipped=*/false);

  const DataPath geomPath({"NIfTI Image Scaled"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  SECTION("apply_scaling = true → float32 promoted, values transformed")
  {
    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true);

    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

    std::vector<float> expected(numVoxels);
    for(usize i = 0; i < numVoxels; i++)
    {
      expected[i] = static_cast<float>(voxels[i]) * params.sclSlope + params.sclInter;
    }
    RequireFloatArrayApproxEquals(dataStructure, arrPath, expected);
  }

  SECTION("apply_scaling = false → native int16 preserved")
  {
    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/false, /*useAffine=*/true);

    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

    RequireArrayEquals<int16>(dataStructure, arrPath, voxels);
  }
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: float32 round trip", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {2, 2, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<float> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = 0.25f * static_cast<float>(i) - 1.0f;
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_FLOAT32;
  params.bitpix = 32;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "float32.nii";
  WriteNiftiFile(filePath, hdr, ToBytes(voxels), /*gzipped=*/false);

  const DataPath geomPath({"NIfTI Float"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  RequireFloatArrayApproxEquals(dataStructure, arrPath, voxels);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: RGB24 (3-component uint8)", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {3, 2, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<uint8_t> voxels(numVoxels * 3);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i * 3 + 0] = static_cast<uint8_t>(i * 3 + 0);
    voxels[i * 3 + 1] = static_cast<uint8_t>(i * 3 + 1);
    voxels[i * 3 + 2] = static_cast<uint8_t>(i * 3 + 2);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_RGB24;
  params.bitpix = 24;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "rgb24.nii";
  WriteNiftiFile(filePath, hdr, voxels, /*gzipped=*/false);

  const DataPath geomPath({"NIfTI RGB"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<uint8>>(arrPath));
  const auto& arr = dataStructure.getDataRefAs<DataArray<uint8>>(arrPath);
  REQUIRE(arr.getNumberOfComponents() == 3);
  REQUIRE(arr.getDataStoreRef().getSize() == voxels.size());
  RequireArrayEquals<uint8>(dataStructure, arrPath, voxels);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: RGBA32 (4-component uint8)", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {2, 2, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<uint8_t> voxels(numVoxels * 4);
  for(usize i = 0; i < voxels.size(); i++)
  {
    voxels[i] = static_cast<uint8_t>(i);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_RGBA32;
  params.bitpix = 32;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "rgba32.nii";
  WriteNiftiFile(filePath, hdr, voxels, /*gzipped=*/false);

  const DataPath geomPath({"NIfTI RGBA"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<uint8>>(arrPath));
  const auto& arr = dataStructure.getDataRefAs<DataArray<uint8>>(arrPath);
  REQUIRE(arr.getNumberOfComponents() == 4);
  RequireArrayEquals<uint8>(dataStructure, arrPath, voxels);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: VoxelSubvolume crop streams only the selected region", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {6, 5, 4};
  const std::array<float, 3> spacing = {1.0f, 1.0f, 1.0f};
  const std::array<float, 3> origin = {0.0f, 0.0f, 0.0f};

  const usize nx = static_cast<usize>(dims[0]);
  const usize ny = static_cast<usize>(dims[1]);
  const usize nz = static_cast<usize>(dims[2]);
  const usize numVoxels = nx * ny * nz;

  std::vector<uint16_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<uint16_t>(i);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.spacing = spacing;
  params.origin = origin;
  params.niftiDatatype = NIFTI_TYPE_UINT16;
  params.bitpix = 16;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "uint16_crop_voxel.nii";
  WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  crop.cropX = true;
  crop.cropY = true;
  crop.cropZ = true;
  crop.xBoundVoxels = {1, 3};
  crop.yBoundVoxels = {1, 3};
  crop.zBoundVoxels = {1, 2};

  const DataPath geomPath({"NIfTI Cropped"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  const auto outDims = geom.getDimensions();
  const auto outOrigin = geom.getOrigin();
  const auto outSpacing = geom.getSpacing();
  REQUIRE(outDims[0] == 3);
  REQUIRE(outDims[1] == 3);
  REQUIRE(outDims[2] == 2);
  // Origin shifts by start * spacing
  constexpr float tol = 1e-5f;
  REQUIRE(std::fabs(outOrigin[0] - 1.0f) < tol);
  REQUIRE(std::fabs(outOrigin[1] - 1.0f) < tol);
  REQUIRE(std::fabs(outOrigin[2] - 1.0f) < tol);
  REQUIRE(std::fabs(outSpacing[0] - 1.0f) < tol);
  REQUIRE(std::fabs(outSpacing[1] - 1.0f) < tol);
  REQUIRE(std::fabs(outSpacing[2] - 1.0f) < tol);

  std::vector<uint16_t> expected;
  expected.reserve(3 * 3 * 2);
  for(usize dz = 0; dz < 2; dz++)
  {
    for(usize dy = 0; dy < 3; dy++)
    {
      for(usize dx = 0; dx < 3; dx++)
      {
        const usize srcZ = 1 + dz;
        const usize srcY = 1 + dy;
        const usize srcX = 1 + dx;
        const usize srcLinear = srcZ * ny * nx + srcY * nx + srcX;
        expected.push_back(static_cast<uint16_t>(srcLinear));
      }
    }
  }
  RequireArrayEquals<uint16>(dataStructure, arrPath, expected);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: PhysicalSubvolume crop maps physical bounds to voxels", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {6, 5, 4};
  const std::array<float, 3> spacing = {0.5f, 1.0f, 2.0f};
  const std::array<float, 3> origin = {0.0f, 0.0f, 0.0f};

  const usize nx = static_cast<usize>(dims[0]);
  const usize ny = static_cast<usize>(dims[1]);
  const usize nz = static_cast<usize>(dims[2]);
  const usize numVoxels = nx * ny * nz;

  std::vector<uint8_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<uint8_t>(i);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.spacing = spacing;
  params.origin = origin;
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "uint8_crop_physical.nii";
  WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

  // With origin=0, spacing=(0.5, 1.0, 2.0), voxel (xi, yi, zi) has cell-center
  // (xi*0.5+0.25, yi*1.0+0.5, zi*2.0+1.0). Target voxels x=[1..4] y=[1..3] z=[0..2]:
  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  crop.cropX = true;
  crop.cropY = true;
  crop.cropZ = true;
  crop.xBoundPhysical = {0.6f, 2.4f};
  crop.yBoundPhysical = {1.2f, 3.7f};
  crop.zBoundPhysical = {0.5f, 5.5f};

  const DataPath geomPath({"NIfTI Cropped Physical"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  const auto outDims = geom.getDimensions();
  REQUIRE(outDims[0] == 4);
  REQUIRE(outDims[1] == 3);
  REQUIRE(outDims[2] == 3);

  const usize destNx = outDims[0];
  const usize destNy = outDims[1];
  const usize destNz = outDims[2];
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<uint8>>(arrPath));
  const auto& arr = dataStructure.getDataRefAs<DataArray<uint8>>(arrPath);
  const auto& store = arr.getDataStoreRef();
  REQUIRE(store.getSize() == destNx * destNy * destNz);

  std::vector<uint8_t> expected;
  expected.reserve(destNx * destNy * destNz);
  for(usize dz = 0; dz < destNz; dz++)
  {
    for(usize dy = 0; dy < destNy; dy++)
    {
      for(usize dx = 0; dx < destNx; dx++)
      {
        const usize srcZ = 0 + dz;
        const usize srcY = 1 + dy;
        const usize srcX = 1 + dx;
        const usize srcLinear = srcZ * ny * nx + srcY * nx + srcX;
        expected.push_back(static_cast<uint8_t>(srcLinear));
      }
    }
  }
  RequireArrayEquals<uint8>(dataStructure, arrPath, expected);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: VoxelSubvolume crop preserves RGB24 components", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {4, 3, 3};
  const usize nx = static_cast<usize>(dims[0]);
  const usize ny = static_cast<usize>(dims[1]);
  const usize nz = static_cast<usize>(dims[2]);
  const usize numVoxels = nx * ny * nz;

  std::vector<uint8_t> voxels(numVoxels * 3);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i * 3 + 0] = static_cast<uint8_t>((i * 3 + 0) & 0xFF);
    voxels[i * 3 + 1] = static_cast<uint8_t>((i * 3 + 1) & 0xFF);
    voxels[i * 3 + 2] = static_cast<uint8_t>((i * 3 + 2) & 0xFF);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_RGB24;
  params.bitpix = 24;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const fs::path filePath = outDir / "rgb24_crop.nii";
  WriteNiftiFile(filePath, hdr, voxels, false);

  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  crop.cropX = true;
  crop.cropY = true;
  crop.cropZ = true;
  crop.xBoundVoxels = {1, 2};
  crop.yBoundVoxels = {0, 1};
  crop.zBoundVoxels = {1, 2};

  const DataPath geomPath({"NIfTI RGB Cropped"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const Arguments args = MakeFilterArgs(filePath, geomPath, amName, arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<uint8>>(arrPath));
  const auto& arr = dataStructure.getDataRefAs<DataArray<uint8>>(arrPath);
  REQUIRE(arr.getNumberOfComponents() == 3);

  const usize destNx = 2;
  const usize destNy = 2;
  const usize destNz = 2;
  std::vector<uint8_t> expected;
  expected.reserve(destNx * destNy * destNz * 3);
  for(usize dz = 0; dz < destNz; dz++)
  {
    for(usize dy = 0; dy < destNy; dy++)
    {
      for(usize dx = 0; dx < destNx; dx++)
      {
        const usize srcX = 1 + dx;
        const usize srcY = 0 + dy;
        const usize srcZ = 1 + dz;
        const usize srcLinear = srcZ * ny * nx + srcY * nx + srcX;
        expected.push_back(static_cast<uint8_t>((srcLinear * 3 + 0) & 0xFF));
        expected.push_back(static_cast<uint8_t>((srcLinear * 3 + 1) & 0xFF));
        expected.push_back(static_cast<uint8_t>((srcLinear * 3 + 2) & 0xFF));
      }
    }
  }
  RequireArrayEquals<uint8>(dataStructure, arrPath, expected);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: rejects 4D volumes and .hdr/.img pair", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path outDir = OutputDir();
  const DataPath geomPath({"NIfTI Bad"});

  SECTION("dim[0] == 4 is rejected")
  {
    nifti_1_header hdr = MakeHeader({});
    hdr.dim[0] = 4;
    hdr.dim[1] = 2;
    hdr.dim[2] = 2;
    hdr.dim[3] = 2;
    hdr.dim[4] = 2;
    hdr.datatype = NIFTI_TYPE_UINT8;
    hdr.bitpix = 8;
    std::vector<uint8_t> voxels(2 * 2 * 2 * 2, 0);
    const fs::path filePath = outDir / "4d.nii";
    WriteNiftiFile(filePath, hdr, voxels, false);

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);
    const auto preflight = filter.preflight(dataStructure, args);
    REQUIRE(preflight.outputActions.invalid());
  }

  SECTION("ni1 (hdr/img pair magic) is rejected")
  {
    nifti_1_header hdr = MakeHeader({});
    hdr.datatype = NIFTI_TYPE_UINT8;
    hdr.bitpix = 8;
    hdr.dim[1] = 2;
    hdr.dim[2] = 2;
    hdr.dim[3] = 2;
    std::memcpy(hdr.magic, "ni1\0", 4);
    std::vector<uint8_t> voxels(8, 0);
    const fs::path filePath = outDir / "pair.nii";
    WriteNiftiFile(filePath, hdr, voxels, false);

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);
    const auto preflight = filter.preflight(dataStructure, args);
    REQUIRE(preflight.outputActions.invalid());
  }
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: rejects missing input file, 1D/2D volumes, and unsupported datatypes", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path outDir = OutputDir();
  const DataPath geomPath({"NIfTI Bad"});

  SECTION("missing input file")
  {
    const fs::path filePath = outDir / "does_not_exist_123456.nii";
    std::error_code ec;
    fs::remove(filePath, ec);

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);
    const auto preflight = filter.preflight(dataStructure, args);
    REQUIRE(preflight.outputActions.invalid());
  }

  for(int16_t rank : {int16_t{1}, int16_t{2}})
  {
    DYNAMIC_SECTION("rank=" << rank << " is rejected")
    {
      SyntheticNiftiParams params;
      params.dims = {2, 2, 2};
      params.niftiDatatype = NIFTI_TYPE_UINT8;
      params.bitpix = 8;
      params.rank = rank;
      nifti_1_header hdr = MakeHeader(params);

      const fs::path filePath = outDir / fmt::format("rank_{}.nii", rank);
      std::vector<uint8_t> voxels(8, 0);
      WriteNiftiFile(filePath, hdr, voxels, false);

      DataStructure dataStructure;
      ReadNIfTIFileFilter filter;
      const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);
      const auto preflight = filter.preflight(dataStructure, args);
      REQUIRE(preflight.outputActions.invalid());
    }
  }

  SECTION("unsupported datatype (complex64) is rejected")
  {
    SyntheticNiftiParams params;
    params.dims = {2, 2, 2};
    params.niftiDatatype = NIFTI_TYPE_COMPLEX64;
    params.bitpix = 64;
    nifti_1_header hdr = MakeHeader(params);

    const fs::path filePath = outDir / "complex64.nii";
    std::vector<uint8_t> voxels(2 * 2 * 2 * 8, 0);
    WriteNiftiFile(filePath, hdr, voxels, false);

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);
    const auto preflight = filter.preflight(dataStructure, args);
    REQUIRE(preflight.outputActions.invalid());
  }

  SECTION("truncated voxel body fails at execute")
  {
    SyntheticNiftiParams params;
    params.dims = {4, 4, 4};
    params.niftiDatatype = NIFTI_TYPE_UINT8;
    params.bitpix = 8;
    nifti_1_header hdr = MakeHeader(params);

    // Header announces 64 voxels, but we only write 8 bytes of body.
    std::vector<uint8_t> voxels(8, 0);
    const fs::path filePath = outDir / "truncated.nii";
    WriteNiftiFile(filePath, hdr, voxels, false);

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);

    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(dataStructure, args);
    REQUIRE(execute.result.invalid());
  }
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: sform-based origin and spacing extraction", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {3, 3, 3};
  const float sx = 2.0f;
  const float sy = 3.0f;
  const float sz = 4.0f;
  const std::array<float, 3> tOrigin = {7.5f, -1.0f, 2.25f};

  SyntheticNiftiParams params;
  params.dims = dims;
  params.spacing = {1.0f, 1.0f, 1.0f}; // pixdim deliberately different from sform
  params.origin = {0.0f, 0.0f, 0.0f};  // qoffset deliberately different
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  params.qformCode = 0; // force sform path
  params.sformCode = 1; // NIFTI_XFORM_SCANNER_ANAT
  params.sform = {{{sx, 0.0f, 0.0f, tOrigin[0]}, {0.0f, sy, 0.0f, tOrigin[1]}, {0.0f, 0.0f, sz, tOrigin[2]}}};

  nifti_1_header hdr = MakeHeader(params);
  std::vector<uint8_t> voxels(static_cast<usize>(dims[0]) * dims[1] * dims[2], 0);
  const fs::path filePath = OutputDir() / "sform_diag.nii";
  WriteNiftiFile(filePath, hdr, voxels, false);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const DataPath geomPath({"NIfTI Sform"});
  const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  REQUIRE(preflight.outputActions.warnings().empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geomPath));
  const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  const auto outOrigin = geom.getOrigin();
  const auto outSpacing = geom.getSpacing();
  constexpr float tol = 1e-5f;
  REQUIRE(std::fabs(outOrigin[0] - tOrigin[0]) < tol);
  REQUIRE(std::fabs(outOrigin[1] - tOrigin[1]) < tol);
  REQUIRE(std::fabs(outOrigin[2] - tOrigin[2]) < tol);
  REQUIRE(std::fabs(outSpacing[0] - sx) < tol);
  REQUIRE(std::fabs(outSpacing[1] - sy) < tol);
  REQUIRE(std::fabs(outSpacing[2] - sz) < tol);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: sform rotation emits a warning", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  // 45deg rotation about z, unit spacing, zero translation — off-diagonal
  // srow entries should trip the affineHasRotation detector.
  const float c = std::cos(0.7853981633974483f);
  const float s = std::sin(0.7853981633974483f);

  SyntheticNiftiParams params;
  params.dims = {2, 2, 2};
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  params.qformCode = 0;
  params.sformCode = 1;
  params.sform = {{{c, -s, 0.0f, 0.0f}, {s, c, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}}};

  nifti_1_header hdr = MakeHeader(params);
  std::vector<uint8_t> voxels(8, 0);
  const fs::path filePath = OutputDir() / "sform_rotated.nii";
  WriteNiftiFile(filePath, hdr, voxels, false);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const DataPath geomPath({"NIfTI Rotated"});
  const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);

  const auto& warnings = preflight.outputActions.warnings();
  REQUIRE_FALSE(warnings.empty());
  bool foundRotationWarning = false;
  for(const auto& w : warnings)
  {
    if(w.code == -34750)
    {
      foundRotationWarning = true;
      break;
    }
  }
  REQUIRE(foundRotationWarning);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: UseAffineIfPresent=false falls back to pixdim + zero origin", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  // File advertises origin (100, 200, 300) via sform and spacing (5, 5, 5)
  // via sform column magnitudes. pixdim stays at (1, 1, 1). When the user
  // disables the affine, we should fall back to pixdim + zero origin.
  SyntheticNiftiParams params;
  params.dims = {2, 2, 2};
  params.spacing = {1.0f, 1.0f, 1.0f};
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  params.qformCode = 0;
  params.sformCode = 1;
  params.sform = {{{5.0f, 0.0f, 0.0f, 100.0f}, {0.0f, 5.0f, 0.0f, 200.0f}, {0.0f, 0.0f, 5.0f, 300.0f}}};

  nifti_1_header hdr = MakeHeader(params);
  std::vector<uint8_t> voxels(8, 0);
  const fs::path filePath = OutputDir() / "no_affine.nii";
  WriteNiftiFile(filePath, hdr, voxels, false);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const DataPath geomPath({"NIfTI NoAffine"});
  const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/false);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
  const auto outOrigin = geom.getOrigin();
  const auto outSpacing = geom.getSpacing();
  constexpr float tol = 1e-5f;
  REQUIRE(std::fabs(outOrigin[0]) < tol);
  REQUIRE(std::fabs(outOrigin[1]) < tol);
  REQUIRE(std::fabs(outOrigin[2]) < tol);
  REQUIRE(std::fabs(outSpacing[0] - 1.0f) < tol);
  REQUIRE(std::fabs(outSpacing[1] - 1.0f) < tol);
  REQUIRE(std::fabs(outSpacing[2] - 1.0f) < tol);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: big-endian uint16 round trip", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {3, 3, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];

  std::vector<uint16_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<uint16_t>(0x0100 + i); // values whose bytes differ to exercise the swap
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_UINT16;
  params.bitpix = 16;
  nifti_1_header hdr = MakeHeader(params);

  // Swap the header and voxel data so the file is in the opposite byte order
  // from the host. The filter should detect this via sizeof_hdr and swap back.
  ByteSwapHeader(hdr);
  const std::vector<uint8_t> voxelBytes = ToBytesByteSwapped(voxels);

  const fs::path filePath = OutputDir() / "big_endian_uint16.nii";
  WriteNiftiFile(filePath, hdr, voxelBytes, false);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const DataPath geomPath({"NIfTI BE"});
  const std::string arrName = "ImageData";
  const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath(arrName);
  RequireArrayEquals<uint16>(dataStructure, arrPath, voxels);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: round-trips remaining native datatypes", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {3, 2, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];

  auto runCase = [&](const std::string& label, int16_t niftiDatatype, int16_t bitpix, auto sample) {
    using T = decltype(sample);

    DYNAMIC_SECTION(label)
    {
      std::vector<T> voxels(numVoxels);
      for(usize i = 0; i < numVoxels; i++)
      {
        voxels[i] = static_cast<T>(static_cast<int64_t>(i) - 3);
      }

      SyntheticNiftiParams params;
      params.dims = dims;
      params.niftiDatatype = niftiDatatype;
      params.bitpix = bitpix;
      nifti_1_header hdr = MakeHeader(params);

      const fs::path filePath = OutputDir() / fmt::format("type_{}.nii", label);
      WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

      DataStructure dataStructure;
      ReadNIfTIFileFilter filter;
      const DataPath geomPath({fmt::format("NIfTI {}", label)});
      const std::string arrName = "ImageData";
      const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true);

      const auto preflight = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
      const auto execute = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

      const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath(arrName);
      RequireArrayEquals<T>(dataStructure, arrPath, voxels);
    }
  };

  runCase("int8", NIFTI_TYPE_INT8, 8, int8_t{0});
  runCase("uint32", NIFTI_TYPE_UINT32, 32, uint32_t{0});
  runCase("int32", NIFTI_TYPE_INT32, 32, int32_t{0});
  runCase("uint64", NIFTI_TYPE_UINT64, 64, uint64_t{0});
  runCase("int64", NIFTI_TYPE_INT64, 64, int64_t{0});

  DYNAMIC_SECTION("float64")
  {
    std::vector<double> voxels(numVoxels);
    for(usize i = 0; i < numVoxels; i++)
    {
      voxels[i] = 0.125 * static_cast<double>(i) - 1.0;
    }

    SyntheticNiftiParams params;
    params.dims = dims;
    params.niftiDatatype = NIFTI_TYPE_FLOAT64;
    params.bitpix = 64;
    nifti_1_header hdr = MakeHeader(params);

    const fs::path filePath = OutputDir() / "type_float64.nii";
    WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const DataPath geomPath({"NIfTI float64"});
    const std::string arrName = "ImageData";
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true);

    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

    const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath(arrName);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float64>>(arrPath));
    const auto& arr = dataStructure.getDataRefAs<DataArray<float64>>(arrPath);
    const auto& store = arr.getDataStoreRef();
    REQUIRE(store.getSize() == voxels.size());
    for(usize i = 0; i < voxels.size(); i++)
    {
      REQUIRE(std::fabs(static_cast<double>(store[i]) - voxels[i]) < 1e-9);
    }
  }
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: crop edge cases", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {4, 4, 4};
  const usize nx = static_cast<usize>(dims[0]);
  const usize ny = static_cast<usize>(dims[1]);
  const usize nz = static_cast<usize>(dims[2]);
  const usize numVoxels = nx * ny * nz;

  std::vector<uint8_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<uint8_t>(i);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path outDir = OutputDir();
  const DataPath geomPath({"NIfTI CropEdge"});
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath(arrName);

  SECTION("per-axis crop toggles: cropY=false leaves y at full extent")
  {
    const fs::path filePath = outDir / "per_axis_toggle.nii";
    WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

    CropGeometryParameter::ValueType crop;
    crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
    crop.cropX = true;
    crop.cropY = false;
    crop.cropZ = true;
    crop.xBoundVoxels = {1, 2};
    crop.yBoundVoxels = {99, 99}; // deliberately nonsense; should be ignored
    crop.zBoundVoxels = {0, 2};

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

    const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
    const auto outDims = geom.getDimensions();
    REQUIRE(outDims[0] == 2);
    REQUIRE(outDims[1] == 4); // full y extent
    REQUIRE(outDims[2] == 3);

    std::vector<uint8_t> expected;
    expected.reserve(2 * 4 * 3);
    for(usize dz = 0; dz < 3; dz++)
    {
      for(usize dy = 0; dy < 4; dy++)
      {
        for(usize dx = 0; dx < 2; dx++)
        {
          const usize srcLinear = dz * ny * nx + dy * nx + (1 + dx);
          expected.push_back(static_cast<uint8_t>(srcLinear));
        }
      }
    }
    RequireArrayEquals<uint8>(dataStructure, arrPath, expected);
  }

  SECTION("gzipped file combined with VoxelSubvolume crop")
  {
    const fs::path filePath = outDir / "crop.nii.gz";
    WriteNiftiFile(filePath, hdr, ToBytes(voxels), /*gzipped=*/true);

    CropGeometryParameter::ValueType crop;
    crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
    crop.cropX = true;
    crop.cropY = true;
    crop.cropZ = true;
    crop.xBoundVoxels = {1, 3};
    crop.yBoundVoxels = {1, 2};
    crop.zBoundVoxels = {0, 1};

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

    std::vector<uint8_t> expected;
    expected.reserve(3 * 2 * 2);
    for(usize dz = 0; dz < 2; dz++)
    {
      for(usize dy = 0; dy < 2; dy++)
      {
        for(usize dx = 0; dx < 3; dx++)
        {
          const usize srcLinear = (0 + dz) * ny * nx + (1 + dy) * nx + (1 + dx);
          expected.push_back(static_cast<uint8_t>(srcLinear));
        }
      }
    }
    RequireArrayEquals<uint8>(dataStructure, arrPath, expected);
  }

  SECTION("physical bounds entirely outside the volume are rejected")
  {
    const fs::path filePath = outDir / "phys_oob.nii";
    WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

    CropGeometryParameter::ValueType crop;
    crop.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
    crop.cropX = true;
    crop.cropY = true;
    crop.cropZ = true;
    crop.xBoundPhysical = {100.0f, 200.0f};
    crop.yBoundPhysical = {100.0f, 200.0f};
    crop.zBoundPhysical = {100.0f, 200.0f};

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

    const auto preflight = filter.preflight(dataStructure, args);
    REQUIRE(preflight.outputActions.invalid());
  }

  SECTION("reversed voxel bounds are rejected")
  {
    const fs::path filePath = outDir / "reversed_bounds.nii";
    WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

    CropGeometryParameter::ValueType crop;
    crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
    crop.cropX = true;
    crop.cropY = true;
    crop.cropZ = true;
    crop.xBoundVoxels = {3, 1};
    crop.yBoundVoxels = {0, 2};
    crop.zBoundVoxels = {0, 2};

    DataStructure dataStructure;
    ReadNIfTIFileFilter filter;
    const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true, crop);

    const auto preflight = filter.preflight(dataStructure, args);
    REQUIRE(preflight.outputActions.invalid());
  }
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: identity scaling transform is not applied and does not promote", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {2, 2, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<int16_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<int16_t>(i) - 2;
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_INT16;
  params.bitpix = 16;
  params.sclSlope = 1.0f; // identity
  params.sclInter = 0.0f;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path filePath = OutputDir() / "identity_scale.nii";
  WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const DataPath geomPath({"NIfTI IdentityScale"});
  const std::string arrName = "ImageData";
  const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", arrName, /*applyScaling=*/true, /*useAffine=*/true);

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

  // No promotion to float32 — native int16 array should be present.
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath(arrName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int16>>(arrPath));
  RequireArrayEquals<int16>(dataStructure, arrPath, voxels);
}

TEST_CASE("SimplnxCore::ReadNIfTIFileFilter: header cache is reused across preflight calls", "[SimplnxCore][ReadNIfTIFileFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<int16_t, 3> dims = {2, 2, 2};
  const usize numVoxels = static_cast<usize>(dims[0]) * dims[1] * dims[2];
  std::vector<uint8_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<uint8_t>(i);
  }

  SyntheticNiftiParams params;
  params.dims = dims;
  params.niftiDatatype = NIFTI_TYPE_UINT8;
  params.bitpix = 8;
  nifti_1_header hdr = MakeHeader(params);

  const fs::path filePath = OutputDir() / "cache_reuse.nii";
  WriteNiftiFile(filePath, hdr, ToBytes(voxels), false);

  DataStructure dataStructure;
  ReadNIfTIFileFilter filter;
  const DataPath geomPath({"NIfTI Cached"});
  const Arguments args = MakeFilterArgs(filePath, geomPath, "Cell Data", "ImageData", /*applyScaling=*/true, /*useAffine=*/true);

  // First preflight populates the cache.
  const auto firstPreflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(firstPreflight.outputActions);

  // Record the mtime, then overwrite the first 4 bytes of the file with
  // garbage and restore the mtime. A cache miss would read the corrupted
  // sizeof_hdr and fail; a cache hit skips the file read entirely.
  const auto originalMtime = fs::last_write_time(filePath);
  {
    std::fstream f(filePath, std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(f.is_open());
    const std::array<char, 4> garbage = {'X', 'X', 'X', 'X'};
    f.seekp(0);
    f.write(garbage.data(), garbage.size());
    f.close();
  }
  fs::last_write_time(filePath, originalMtime);

  // Second preflight on the SAME filter instance should hit the cache and
  // succeed despite the corrupted header bytes.
  DataStructure dataStructure2;
  const auto secondPreflight = filter.preflight(dataStructure2, args);
  SIMPLNX_RESULT_REQUIRE_VALID(secondPreflight.outputActions);

  // Sanity check: a fresh filter instance (new m_InstanceId → cold cache)
  // should see the corruption and fail.
  DataStructure dataStructure3;
  ReadNIfTIFileFilter freshFilter;
  const auto freshPreflight = freshFilter.preflight(dataStructure3, args);
  REQUIRE(freshPreflight.outputActions.invalid());
}
