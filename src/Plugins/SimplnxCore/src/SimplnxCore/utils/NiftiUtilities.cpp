#include "NiftiUtilities.hpp"

#include "simplnx/Common/Bit.hpp"

#include <fmt/format.h>
#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nx::core::nifti
{
static_assert(sizeof(nifti_1_header) == k_HeaderSize, "nifti_1_header must be exactly 348 bytes");
static_assert(sizeof(int) == 4, "NIfTI-1 assumes sizeof(int) == 4");
static_assert(sizeof(short) == 2, "NIfTI-1 assumes sizeof(short) == 2");
static_assert(sizeof(float) == 4, "NIfTI-1 assumes sizeof(float) == 4");

namespace
{
template <class T>
void SwapInPlace(T& value)
{
  value = nx::core::byteswap(value);
}

void ByteSwapHeader(nifti_1_header& hdr)
{
  SwapInPlace(hdr.sizeof_hdr);
  SwapInPlace(hdr.extents);
  SwapInPlace(hdr.session_error);
  for(auto& v : hdr.dim)
  {
    SwapInPlace(v);
  }
  SwapInPlace(hdr.intent_p1);
  SwapInPlace(hdr.intent_p2);
  SwapInPlace(hdr.intent_p3);
  SwapInPlace(hdr.intent_code);
  SwapInPlace(hdr.datatype);
  SwapInPlace(hdr.bitpix);
  SwapInPlace(hdr.slice_start);
  for(auto& v : hdr.pixdim)
  {
    SwapInPlace(v);
  }
  SwapInPlace(hdr.vox_offset);
  SwapInPlace(hdr.scl_slope);
  SwapInPlace(hdr.scl_inter);
  SwapInPlace(hdr.slice_end);
  SwapInPlace(hdr.cal_max);
  SwapInPlace(hdr.cal_min);
  SwapInPlace(hdr.slice_duration);
  SwapInPlace(hdr.toffset);
  SwapInPlace(hdr.glmax);
  SwapInPlace(hdr.glmin);
  SwapInPlace(hdr.qform_code);
  SwapInPlace(hdr.sform_code);
  SwapInPlace(hdr.quatern_b);
  SwapInPlace(hdr.quatern_c);
  SwapInPlace(hdr.quatern_d);
  SwapInPlace(hdr.qoffset_x);
  SwapInPlace(hdr.qoffset_y);
  SwapInPlace(hdr.qoffset_z);
  for(auto& v : hdr.srow_x)
  {
    SwapInPlace(v);
  }
  for(auto& v : hdr.srow_y)
  {
    SwapInPlace(v);
  }
  for(auto& v : hdr.srow_z)
  {
    SwapInPlace(v);
  }
}
} // namespace

std::optional<NiftiTypeInfo> NiftiDatatypeToSimplnx(int16 niftiDatatype)
{
  switch(niftiDatatype)
  {
  case NIFTI_TYPE_UINT8:
    return NiftiTypeInfo{DataType::uint8, 1, 8};
  case NIFTI_TYPE_INT8:
    return NiftiTypeInfo{DataType::int8, 1, 8};
  case NIFTI_TYPE_UINT16:
    return NiftiTypeInfo{DataType::uint16, 1, 16};
  case NIFTI_TYPE_INT16:
    return NiftiTypeInfo{DataType::int16, 1, 16};
  case NIFTI_TYPE_UINT32:
    return NiftiTypeInfo{DataType::uint32, 1, 32};
  case NIFTI_TYPE_INT32:
    return NiftiTypeInfo{DataType::int32, 1, 32};
  case NIFTI_TYPE_UINT64:
    return NiftiTypeInfo{DataType::uint64, 1, 64};
  case NIFTI_TYPE_INT64:
    return NiftiTypeInfo{DataType::int64, 1, 64};
  case NIFTI_TYPE_FLOAT32:
    return NiftiTypeInfo{DataType::float32, 1, 32};
  case NIFTI_TYPE_FLOAT64:
    return NiftiTypeInfo{DataType::float64, 1, 64};
  case NIFTI_TYPE_RGB24:
    return NiftiTypeInfo{DataType::uint8, 3, 24};
  case NIFTI_TYPE_RGBA32:
    return NiftiTypeInfo{DataType::uint8, 4, 32};
  default:
    return std::nullopt;
  }
}

Result<NiftiMetadata> ReadNiftiHeader(const std::filesystem::path& filePath, bool useAffineIfPresent)
{
  const std::string pathStr = filePath.string();
  gzFile gz = gzopen(pathStr.c_str(), "rb");
  if(gz == nullptr)
  {
    return MakeErrorResult<NiftiMetadata>(-34700, fmt::format("Could not open NIfTI file for reading: '{}'", pathStr));
  }

  nifti_1_header hdr{};
  int bytesRead = gzread(gz, &hdr, static_cast<unsigned int>(k_HeaderSize));
  gzclose(gz);

  if(bytesRead != static_cast<int>(k_HeaderSize))
  {
    return MakeErrorResult<NiftiMetadata>(-34701, fmt::format("Failed to read 348-byte NIfTI-1 header from '{}' (read {} bytes)", pathStr, bytesRead));
  }

  bool byteSwap = false;
  if(hdr.sizeof_hdr != static_cast<int>(k_HeaderSize))
  {
    int swapped = nx::core::byteswap(hdr.sizeof_hdr);
    if(swapped == static_cast<int>(k_HeaderSize))
    {
      byteSwap = true;
      ByteSwapHeader(hdr);
    }
    else
    {
      return MakeErrorResult<NiftiMetadata>(
          -34702, fmt::format("Invalid NIfTI-1 header in '{}': sizeof_hdr={} (expected 348 in either byte order). File does not appear to be a valid NIfTI-1 file.", pathStr, hdr.sizeof_hdr));
    }
  }

  if(std::memcmp(hdr.magic, k_SingleFileMagic.data(), 4) != 0)
  {
    if(std::memcmp(hdr.magic, k_PairFileMagic.data(), 4) == 0)
    {
      return MakeErrorResult<NiftiMetadata>(
          -34703, fmt::format("NIfTI file '{}' uses the .hdr/.img pair format (magic='ni1'). This filter only supports the single-file format (.nii / .nii.gz, magic='n+1').", pathStr));
    }
    const std::string magicDisplay(hdr.magic, hdr.magic + 4);
    return MakeErrorResult<NiftiMetadata>(-34704, fmt::format("NIfTI file '{}' has unrecognized magic value '{}'. Expected 'n+1' for a single-file NIfTI-1.", pathStr, magicDisplay));
  }

  const int16 rank = hdr.dim[0];
  if(rank != 3)
  {
    return MakeErrorResult<NiftiMetadata>(-34705, fmt::format("NIfTI file '{}' is {}-dimensional (dim[0]={}). This filter currently supports 3D NIfTI files only.", pathStr, rank, rank));
  }

  if(hdr.dim[1] <= 0 || hdr.dim[2] <= 0 || hdr.dim[3] <= 0)
  {
    return MakeErrorResult<NiftiMetadata>(-34706, fmt::format("NIfTI file '{}' has invalid dimensions: ({}, {}, {})", pathStr, hdr.dim[1], hdr.dim[2], hdr.dim[3]));
  }

  auto typeInfo = NiftiDatatypeToSimplnx(hdr.datatype);
  if(!typeInfo.has_value())
  {
    return MakeErrorResult<NiftiMetadata>(-34707, fmt::format("NIfTI file '{}' has unsupported datatype code {} (bitpix={}). Supported: uint8/int8, uint16/int16, uint32/int32, uint64/int64, "
                                                              "float32, float64, RGB24, RGBA32.",
                                                              pathStr, hdr.datatype, hdr.bitpix));
  }

  if(hdr.bitpix != 0 && hdr.bitpix != typeInfo->bitpix)
  {
    return MakeErrorResult<NiftiMetadata>(-34708,
                                          fmt::format("NIfTI file '{}': datatype code {} implies bitpix={}, but header reports bitpix={}.", pathStr, hdr.datatype, typeInfo->bitpix, hdr.bitpix));
  }

  NiftiMetadata md;
  md.filePath = pathStr;
  md.magic = std::string(hdr.magic, hdr.magic + 3);
  for(int i = 0; i < 8; i++)
  {
    md.dim[i] = hdr.dim[i];
    md.pixdim[i] = hdr.pixdim[i];
  }
  md.dimensions = {static_cast<usize>(hdr.dim[1]), static_cast<usize>(hdr.dim[2]), static_cast<usize>(hdr.dim[3])};

  // Default spacing from pixdim, fallback to 1.0 for zero values
  md.spacing = {std::fabs(hdr.pixdim[1]), std::fabs(hdr.pixdim[2]), std::fabs(hdr.pixdim[3])};
  for(auto& s : md.spacing)
  {
    if(s == 0.0f)
    {
      s = 1.0f;
    }
  }

  md.niftiDatatype = hdr.datatype;
  md.bitpix = typeInfo->bitpix;
  md.dataType = typeInfo->dataType;
  md.componentCount = typeInfo->componentCount;

  usize vo = (hdr.vox_offset <= 0.0f) ? k_MinVoxOffset : static_cast<usize>(hdr.vox_offset);
  if(vo < k_MinVoxOffset)
  {
    vo = k_MinVoxOffset;
  }
  md.voxOffset = vo;
  md.byteSwapRequired = byteSwap;

  md.sclSlope = hdr.scl_slope;
  md.sclInter = hdr.scl_inter;
  md.hasNontrivialScaling = (hdr.scl_slope != 0.0f) && (hdr.scl_slope != 1.0f || hdr.scl_inter != 0.0f);
  if(md.niftiDatatype == NIFTI_TYPE_RGB24 || md.niftiDatatype == NIFTI_TYPE_RGBA32)
  {
    md.hasNontrivialScaling = false;
  }

  md.qformCode = hdr.qform_code;
  md.sformCode = hdr.sform_code;
  for(int k = 0; k < 4; k++)
  {
    md.sformMatrix[0][k] = hdr.srow_x[k];
    md.sformMatrix[1][k] = hdr.srow_y[k];
    md.sformMatrix[2][k] = hdr.srow_z[k];
  }

  const float32 rotEps = 1.0e-5f;
  if(useAffineIfPresent && hdr.sform_code > 0)
  {
    md.origin = {hdr.srow_x[3], hdr.srow_y[3], hdr.srow_z[3]};
    for(int col = 0; col < 3; col++)
    {
      const float32 cx = hdr.srow_x[col];
      const float32 cy = hdr.srow_y[col];
      const float32 cz = hdr.srow_z[col];
      const float32 mag = std::sqrt(cx * cx + cy * cy + cz * cz);
      if(mag > 0.0f)
      {
        md.spacing[col] = mag;
      }
      for(int row = 0; row < 3; row++)
      {
        if(row == col)
        {
          continue;
        }
        const float32 comp = (row == 0) ? cx : (row == 1) ? cy : cz;
        if(std::fabs(comp) > rotEps * std::max(mag, 1.0e-6f))
        {
          md.affineHasRotation = true;
        }
      }
    }
    md.affineApplied = true;
  }
  else if(useAffineIfPresent && hdr.qform_code > 0)
  {
    md.origin = {hdr.qoffset_x, hdr.qoffset_y, hdr.qoffset_z};
    const float32 bcdSq = hdr.quatern_b * hdr.quatern_b + hdr.quatern_c * hdr.quatern_c + hdr.quatern_d * hdr.quatern_d;
    if(bcdSq > rotEps)
    {
      md.affineHasRotation = true;
    }
    md.affineApplied = true;
  }
  else
  {
    md.origin = {0.0f, 0.0f, 0.0f};
  }

  md.intentCode = hdr.intent_code;
  auto fixedCharsToString = [](const char* src, usize maxLen) {
    usize len = 0;
    while(len < maxLen && src[len] != '\0')
    {
      len++;
    }
    return std::string(src, len);
  };
  md.intentName = fixedCharsToString(hdr.intent_name, sizeof(hdr.intent_name));
  md.descrip = fixedCharsToString(hdr.descrip, sizeof(hdr.descrip));

  return {md};
}

} // namespace nx::core::nifti
