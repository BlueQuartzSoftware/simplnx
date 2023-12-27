from ITKImageProcessing.ITKGrayscaleFillholeImage import ITKGrayscaleFillholeImage
import complex as cx

class ITKImageProcessingPlugin:
  def __init__(self) -> None:
    pass

  def id(self) -> cx.Uuid:
    return cx.Uuid('3efe91e4-a962-465a-8d5a-bd7e5269b0ce')

  def name(self) -> str:
    return 'ITKImageProcessingPlugin'

  def description(self) -> str:
    return 'Python filters that wrap the ITK Software library. ITK is located at https://github.com/InsightSoftwareConsortium/ITK'

  def vendor(self) -> str:
    return 'BlueQuartz Software'

  def get_filters(self):
    return [ITKGrayscaleFillholeImage]
