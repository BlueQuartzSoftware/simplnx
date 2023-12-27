from ITKImageProcessing.Plugin import ITKImageProcessingPlugin
from ITKImageProcessing.ITKGrayscaleFillholeImage import ITKGrayscaleFillholeImage

def get_plugin():
  return ITKImageProcessingPlugin()

__all__ = ['ITKImageProcessingPlugin', 'ITKGrayscaleFillholeImage', 'get_plugin']
