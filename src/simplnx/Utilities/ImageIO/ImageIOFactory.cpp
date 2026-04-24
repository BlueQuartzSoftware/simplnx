#include "ImageIOFactory.hpp"

#include "simplnx/Utilities/ImageIO/StbImageIO.hpp"
#include "simplnx/Utilities/ImageIO/TiffImageIO.hpp"

#include <fmt/format.h>

#include <algorithm>

using namespace nx::core;

Result<std::unique_ptr<IImageIO>> nx::core::CreateImageIO(const std::filesystem::path& filePath)
{
  std::string ext = filePath.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

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
