#include "ImageIOFactory.hpp"

#include "simplnx/Utilities/ImageIO/StbImageIO.hpp"
#include "simplnx/Utilities/ImageIO/TiffImageIO.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cctype>

using namespace nx::core;

Result<std::unique_ptr<IImageIO>> nx::core::CreateImageIO(const std::filesystem::path& filePath)
{
  std::string ext = filePath.extension().string();
  // Pass through unsigned char — std::tolower(int) is UB on negative input, which a signed
  // char with the high bit set produces.
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
  {
    return {std::make_unique<StbImageIO>()};
  }

  if(ext == ".tif" || ext == ".tiff")
  {
    return {std::make_unique<TiffImageIO>()};
  }

  return MakeErrorResult<std::unique_ptr<IImageIO>>(-20200, fmt::format("Unsupported image format '{}'. Supported: .png, .jpg, .jpeg, .bmp, .tif, .tiff", ext));
}
