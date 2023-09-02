ITKImageProcessing
==================

.. py:module:: ITKImageProcessing

.. _ITKAbsImage:
.. py:class:: ITKAbsImage

   **UI Display Name:** *ITK Abs Image Filter*

   itk::Math::abs() is used to perform the computation.

   `Link to the full online documentation for ITKAbsImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKAbsImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKAcosImage:
.. py:class:: ITKAcosImage

   **UI Display Name:** *ITK Acos Image Filter*

   This filter is templated over the pixel type of the input image and the pixel type of the output image.

   `Link to the full online documentation for ITKAcosImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKAcosImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKAdaptiveHistogramEqualizationImage:
.. py:class:: ITKAdaptiveHistogramEqualizationImage

   **UI Display Name:** *ITK Adaptive Histogram Equalization Image Filter*

   Histogram equalization modifies the contrast in an image. The AdaptiveHistogramEqualizationImageFilter is a superset of many contrast enhancing filters. By modifying its parameters (alpha, beta, and window), the AdaptiveHistogramEqualizationImageFilter can produce an adaptively equalized histogram or a version of unsharp mask (local mean subtraction). Instead of applying a strict histogram equalization in a window about a pixel, this filter prescribes a mapping function (power law) controlled by the parameters alpha and beta.

   `Link to the full online documentation for ITKAdaptiveHistogramEqualizationImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKAdaptiveHistogramEqualizationImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Alpha                   | alpha                    |
   +-------------------------+--------------------------+
   | Beta                    | beta                     |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Radius                  | radius                   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(alpha, beta, input_image_data_path, output_image_data_path, radius, selected_image_geom_path)

      :param complex.Float32Parameter alpha: Set/Get the value of alpha. Alpha = 0 produces the adaptive histogram equalization (provided beta=0). Alpha = 1 produces an unsharp mask. Default is 0.3.
      :param complex.Float32Parameter beta: Set/Get the value of beta. If beta = 1 (and alpha = 1), then the output image matches the input image. As beta approaches 0, the filter behaves as an unsharp mask. Default is 0.3.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.VectorUInt32Parameter radius: Radius Dimensions XYZ
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKAsinImage:
.. py:class:: ITKAsinImage

   **UI Display Name:** *ITK Asin Image Filter*

   This filter is templated over the pixel type of the input image and the pixel type of the output image.

   `Link to the full online documentation for ITKAsinImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKAsinImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKAtanImage:
.. py:class:: ITKAtanImage

   **UI Display Name:** *ITK Atan Image Filter*

   This filter is templated over the pixel type of the input image and the pixel type of the output image.

   `Link to the full online documentation for ITKAtanImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKAtanImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryContourImage:
.. py:class:: ITKBinaryContourImage

   **UI Display Name:** *ITK Binary Contour Image Filter*

   BinaryContourImageFilter takes a binary image as input, where the pixels in the objects are the pixels with a value equal to ForegroundValue. Only the pixels on the contours of the objects are kept. The pixels not on the border are changed to BackgroundValue.

   `Link to the full online documentation for ITKBinaryContourImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryContourImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, foreground_value, fully_connected, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: Set/Get the background value used to mark the pixels not on the border of the objects.
      :param complex.Float64Parameter foreground_value: Set/Get the foreground value used to identify the objects in the input and output images.
      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryDilateImage:
.. py:class:: ITKBinaryDilateImage

   **UI Display Name:** *ITK Binary Dilate Image Filter*

   BinaryDilateImageFilter is a binary dilation morphologic operation on the foreground of an image. Only the value designated by the intensity value "SetForegroundValue()" (alias as SetDilateValue() ) is considered as foreground, and other intensity values are considered background.

   `Link to the full online documentation for ITKBinaryDilateImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryDilateImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | BoundaryToForeground    | boundary_to_foreground   |
   +-------------------------+--------------------------+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, boundary_to_foreground, foreground_value, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: The background value of the image
      :param complex.BoolParameter boundary_to_foreground: 
      :param complex.Float64Parameter foreground_value: The foreground value of the image
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryErodeImage:
.. py:class:: ITKBinaryErodeImage

   **UI Display Name:** *ITK Binary Erode Image Filter*

   BinaryErodeImageFilter is a binary erosion morphologic operation on the foreground of an image. Only the value designated by the intensity value "SetForegroundValue()" (alias as SetErodeValue() ) is considered as foreground, and other intensity values are considered background.

   `Link to the full online documentation for ITKBinaryErodeImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryErodeImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | BoundaryToForeground    | boundary_to_foreground   |
   +-------------------------+--------------------------+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, boundary_to_foreground, foreground_value, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: The background value of the image
      :param complex.BoolParameter boundary_to_foreground: 
      :param complex.Float64Parameter foreground_value: The foreground value of the image
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryMorphologicalClosingImage:
.. py:class:: ITKBinaryMorphologicalClosingImage

   **UI Display Name:** *ITK Binary Morphological Closing Image Filter*

   This filter removes small (i.e., smaller than the structuring element) holes and tube like structures in the interior or at the boundaries of the image. The morphological closing of an image "f" is defined as: Closing(f) = Erosion(Dilation(f)).

   `Link to the full online documentation for ITKBinaryMorphologicalClosingImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryMorphologicalClosingImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | SafeBorder              | safe_border              |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(foreground_value, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, safe_border, selected_image_geom_path)

      :param complex.Float64Parameter foreground_value: Set the value in the image to consider as 'foreground'. Defaults to maximum value of InputPixelType.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter safe_border: A safe border is added to input image to avoid borders effects and remove it once the closing is done
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryMorphologicalOpeningImage:
.. py:class:: ITKBinaryMorphologicalOpeningImage

   **UI Display Name:** *ITK Binary Morphological Opening Image Filter*

   This filter removes small (i.e., smaller than the structuring element) structures in the interior or at the boundaries of the image. The morphological opening of an image "f" is defined as: Opening(f) = Dilatation(Erosion(f)).

   `Link to the full online documentation for ITKBinaryMorphologicalOpeningImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryMorphologicalOpeningImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, foreground_value, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: Set the value in eroded part of the image. Defaults to zero
      :param complex.Float64Parameter foreground_value: Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryOpeningByReconstructionImage:
.. py:class:: ITKBinaryOpeningByReconstructionImage

   **UI Display Name:** *ITK Binary Opening By Reconstruction Image Filter*

   This filter removes small (i.e., smaller than the structuring element) objects in the image. It is defined as: Opening(f) = ReconstructionByDilatation(Erosion(f)).

   `Link to the full online documentation for ITKBinaryOpeningByReconstructionImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryOpeningByReconstructionImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, foreground_value, fully_connected, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: Set the value in eroded part of the image. Defaults to zero
      :param complex.Float64Parameter foreground_value: Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType.
      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryProjectionImage:
.. py:class:: ITKBinaryProjectionImage

   **UI Display Name:** *ITK Binary Projection Image Filter*

   This class was contributed to the Insight Journal by Gaetan Lehmann. The original paper can be found at https://www.insight-journal.org/browse/publication/71

   `Link to the full online documentation for ITKBinaryProjectionImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryProjectionImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | ForegroundValue         | foreground_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | ProjectionDimension     | projection_dimension     |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, foreground_value, input_image_data_path, output_image_data_path, projection_dimension, selected_image_geom_path)

      :param complex.Float64Parameter background_value: Set the value used as 'background'. Any pixel value which is not DilateValue is considered background. BackgroundValue is used for defining boundary conditions. Defaults to NumericTraits<PixelType>::NonpositiveMin() .
      :param complex.Float64Parameter foreground_value: Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType. Subclasses may alias this to DilateValue or ErodeValue.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.UInt32Parameter projection_dimension: 
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryThinningImage:
.. py:class:: ITKBinaryThinningImage

   **UI Display Name:** *ITK Binary Thinning Image Filter*

   This class is parameterized over the type of the input image and the type of the output image.

   `Link to the full online documentation for ITKBinaryThinningImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryThinningImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBinaryThresholdImage:
.. py:class:: ITKBinaryThresholdImage

   **UI Display Name:** *ITK Binary Threshold Image Filter*

   This filter produces an output image whose pixels are either one of two values ( OutsideValue or InsideValue ), depending on whether the corresponding input image pixels lie between the two thresholds ( LowerThreshold and UpperThreshold ). Values equal to either threshold is considered to be between the thresholds.

   `Link to the full online documentation for ITKBinaryThresholdImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBinaryThresholdImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | InsideValue             | inside_value             |
   +-------------------------+--------------------------+
   | LowerThreshold          | lower_threshold          |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | OutsideValue            | outside_value            |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | UpperThreshold          | upper_threshold          |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, inside_value, lower_threshold, output_image_data_path, outside_value, selected_image_geom_path, upper_threshold)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.UInt8Parameter inside_value: Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max()
      :param complex.Float64Parameter lower_threshold: 
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.UInt8Parameter outside_value: Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() .
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.Float64Parameter upper_threshold: Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is NumericTraits<InputPixelType>::max . An exception is thrown if the lower threshold is greater than the upper threshold.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKBlackTopHatImage:
.. py:class:: ITKBlackTopHatImage

   **UI Display Name:** *ITK Black Top Hat Image Filter*

   Black top hat extracts local minima that are smaller than the structuring element. It subtracts the background from the input image. The output of the filter transforms the black valleys into white peaks.

   `Link to the full online documentation for ITKBlackTopHatImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKBlackTopHatImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | SafeBorder              | safe_border              |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, safe_border, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter safe_border: A safe border is added to input image to avoid borders effects and remove it once the closing is done
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKClosingByReconstructionImage:
.. py:class:: ITKClosingByReconstructionImage

   **UI Display Name:** *ITK Closing By Reconstruction Image Filter*

   This filter is similar to the morphological closing, but contrary to the morphological closing, the closing by reconstruction preserves the shape of the components. The closing by reconstruction of an image "f" is defined as:

   `Link to the full online documentation for ITKClosingByReconstructionImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKClosingByReconstructionImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | PreserveIntensities     | preserve_intensities     |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, preserve_intensities, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter preserve_intensities: Set/Get whether the original intensities of the image retained for those pixels unaffected by the opening by reconstruction. If Off, the output pixel contrast will be reduced.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKCosImage:
.. py:class:: ITKCosImage

   **UI Display Name:** *ITK Cos Image Filter*

   This filter is templated over the pixel type of the input image and the pixel type of the output image.

   `Link to the full online documentation for ITKCosImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKCosImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKDilateObjectMorphologyImage:
.. py:class:: ITKDilateObjectMorphologyImage

   **UI Display Name:** *ITK Dilate Object Morphology Image Filter*

   Dilate an image using binary morphology. Pixel values matching the object value are considered the "foreground" and all other pixels are "background". This is useful in processing mask images containing only one object.

   `Link to the full online documentation for ITKDilateObjectMorphologyImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKDilateObjectMorphologyImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | ObjectValue             | object_value             |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, object_value, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.Float64Parameter object_value: 
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKDiscreteGaussianImage:
.. py:class:: ITKDiscreteGaussianImage

   **UI Display Name:** *ITK Discrete Gaussian Image Filter*

   The Gaussian operator used here was described by Tony Lindeberg (Discrete Scale-Space Theory and the Scale-Space Primal Sketch. Dissertation. Royal Institute of Technology, Stockholm, Sweden. May 1991.) The Gaussian kernel used here was designed so that smoothing and derivative operations commute after discretization.

   `Link to the full online documentation for ITKDiscreteGaussianImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKDiscreteGaussianImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | MaximumError            | maximum_error            |
   +-------------------------+--------------------------+
   | MaximumKernelWidth      | maximum_kernel_width     |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | UseImageSpacing         | use_image_spacing        |
   +-------------------------+--------------------------+
   | Variance                | variance                 |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, maximum_error, maximum_kernel_width, output_image_data_path, selected_image_geom_path, use_image_spacing, variance)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorFloat64Parameter maximum_error: The maximum error for each axis
      :param complex.UInt32Parameter maximum_kernel_width: Set the kernel to be no wider than MaximumKernelWidth pixels, even if MaximumError demands it. The default is 32 pixels.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.BoolParameter use_image_spacing: Set/Get whether or not the filter will use the spacing of the input image in its calculations. Use On to take the image spacing information into account and to specify the Gaussian variance in real world units; use Off to ignore the image spacing and to specify the Gaussian variance in voxel units. Default is On.
      :param complex.VectorFloat64Parameter variance: The value of the input variance for each axis
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKErodeObjectMorphologyImage:
.. py:class:: ITKErodeObjectMorphologyImage

   **UI Display Name:** *ITK Erode Object Morphology Image Filter*

   Erosion of an image using binary morphology. Pixel values matching the object value are considered the "object" and all other pixels are "background". This is useful in processing mask images containing only one object.

   `Link to the full online documentation for ITKErodeObjectMorphologyImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKErodeObjectMorphologyImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | ObjectValue             | object_value             |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, input_image_data_path, kernel_radius, kernel_type, object_value, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: Set the value to be assigned to eroded pixels
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.Float64Parameter object_value: 
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKExpImage:
.. py:class:: ITKExpImage

   **UI Display Name:** *ITK Exp Image Filter*

   The computation is performed using std::exp(x).

   `Link to the full online documentation for ITKExpImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKExpImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKExpNegativeImage:
.. py:class:: ITKExpNegativeImage

   **UI Display Name:** *ITK Exp Negative Image Filter*

   Every output pixel is equal to std::exp(-K.x ). where x is the intensity of the homologous input pixel, and K is a user-provided constant.

   `Link to the full online documentation for ITKExpNegativeImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKExpNegativeImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGradientMagnitudeImage:
.. py:class:: ITKGradientMagnitudeImage

   **UI Display Name:** *ITK Gradient Magnitude Image Filter*

   Computes the gradient magnitude of an image region at each pixel.

   `Link to the full online documentation for ITKGradientMagnitudeImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGradientMagnitudeImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | UseImageSpacing         | use_image_spacing        |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path, use_image_spacing)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.BoolParameter use_image_spacing: Set/Get whether or not the filter will use the spacing of the input image in the computation of the derivatives. Use On to compute the gradient in physical space; use Off to ignore image spacing and to compute the gradient in isotropic voxel space. Default is On.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGrayscaleDilateImage:
.. py:class:: ITKGrayscaleDilateImage

   **UI Display Name:** *ITK Grayscale Dilate Image Filter*

   Dilate an image using grayscale morphology. Dilation takes the maximum of all the pixels identified by the structuring element.

   `Link to the full online documentation for ITKGrayscaleDilateImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGrayscaleDilateImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGrayscaleErodeImage:
.. py:class:: ITKGrayscaleErodeImage

   **UI Display Name:** *ITK Grayscale Erode Image Filter*

   Erode an image using grayscale morphology. Erosion takes the maximum of all the pixels identified by the structuring element.

   `Link to the full online documentation for ITKGrayscaleErodeImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGrayscaleErodeImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGrayscaleFillholeImage:
.. py:class:: ITKGrayscaleFillholeImage

   **UI Display Name:** *ITK Grayscale Fillhole Image Filter*

   GrayscaleFillholeImageFilter fills holes in a grayscale image. Holes are local minima in the grayscale topography that are not connected to boundaries of the image. Gray level values adjacent to a hole are extrapolated across the hole.

   `Link to the full online documentation for ITKGrayscaleFillholeImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGrayscaleFillholeImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGrayscaleGrindPeakImage:
.. py:class:: ITKGrayscaleGrindPeakImage

   **UI Display Name:** *ITK Grayscale Grind Peak Image Filter*

   GrayscaleGrindPeakImageFilter removes peaks in a grayscale image. Peaks are local maxima in the grayscale topography that are not connected to boundaries of the image. Gray level values adjacent to a peak are extrapolated through the peak.

   `Link to the full online documentation for ITKGrayscaleGrindPeakImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGrayscaleGrindPeakImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGrayscaleMorphologicalClosingImage:
.. py:class:: ITKGrayscaleMorphologicalClosingImage

   **UI Display Name:** *ITK Grayscale Morphological Closing Image Filter*

   Close an image using grayscale morphology.

   `Link to the full online documentation for ITKGrayscaleMorphologicalClosingImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGrayscaleMorphologicalClosingImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | SafeBorder              | safe_border              |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, safe_border, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter safe_border: A safe border is added to input image to avoid borders effects and remove it once the closing is done
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKGrayscaleMorphologicalOpeningImage:
.. py:class:: ITKGrayscaleMorphologicalOpeningImage

   **UI Display Name:** *ITK Grayscale Morphological Opening Image Filter*

   Open an image using grayscale morphology.

   `Link to the full online documentation for ITKGrayscaleMorphologicalOpeningImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKGrayscaleMorphologicalOpeningImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | SafeBorder              | safe_border              |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, safe_border, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter safe_border: A safe border is added to input image to avoid borders effects and remove it once the closing is done
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKHConvexImage:
.. py:class:: ITKHConvexImage

   **UI Display Name:** *ITK H Convex Image Filter*

   HConvexImageFilter extract local maxima that are more than h intensity units above the (local) background. This has the effect of extracting objects that are brighter than background by at least h intensity units.

   `Link to the full online documentation for ITKHConvexImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKHConvexImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Height                  | height                   |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, height, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.Float64Parameter height: Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKHMaximaImage:
.. py:class:: ITKHMaximaImage

   **UI Display Name:** *ITK H Maxima Image Filter*

   HMaximaImageFilter suppresses local maxima that are less than h intensity units above the (local) background. This has the effect of smoothing over the "high" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMinimaImageFilter to suppress the local minima whose depth is less than h intensity units below the (local) background.

   `Link to the full online documentation for ITKHMaximaImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKHMaximaImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Height                  | height                   |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(height, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter height: Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKHMinimaImage:
.. py:class:: ITKHMinimaImage

   **UI Display Name:** *ITK H Minima Image Filter*

   HMinimaImageFilter suppresses local minima that are less than h intensity units below the (local) background. This has the effect of smoothing over the "low" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMaximaImageFilter to suppress the local maxima whose height is less than h intensity units above the (local) background.

   `Link to the full online documentation for ITKHMinimaImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKHMinimaImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Height                  | height                   |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, height, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.Float64Parameter height: Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKImageReader:
.. py:class:: ITKImageReader

   **UI Display Name:** *ITK Image Reader*

   Reads images through ITK

   `Link to the full online documentation for ITKImageReader <http://www.dream3d.io/nx_reference_manual/Filters/ITKImageReader>`_ 

   Mapping of UI display to python named argument

   +------------------------+-----------------------+
   | UI Display             | Python Named Argument |
   +========================+=======================+
   | Cell Data Name         | cell_data_name        |
   +------------------------+-----------------------+
   | File                   | file_name             |
   +------------------------+-----------------------+
   | Created Image Geometry | geometry_path         |
   +------------------------+-----------------------+
   | Created Image Data     | image_data_array_path |
   +------------------------+-----------------------+

   .. py:method:: Execute(cell_data_name, file_name, geometry_path, image_data_array_path)

      :param complex.DataObjectNameParameter cell_data_name: The name of the created cell attribute matrix
      :param complex.FileSystemPathParameter file_name: Input image file
      :param complex.DataGroupCreationParameter geometry_path: The path to the created Image Geometry
      :param complex.ArrayCreationParameter image_data_array_path: The path to the created image data array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKImageWriter:
.. py:class:: ITKImageWriter

   **UI Display Name:** *ITK Image Export*

   This **Filter** will save images based on an array that represents grayscale, RGB or ARGB color values. If the input array represents a 3D volume, the **Filter** will output a series of slices along one of the orthogonal axes.  The options are to produce XY slices along the Z axis, XZ slices along the Y axis or YZ slices along the X axis. The user has the option to save in one of 3 standard image formats: TIF, BMP, or PNG. The output files will be numbered sequentially starting at zero (0) and ending at the total dimensions for the chosen axis. For example, if the Z axis has 117 dimensions, 117 XY image files will be produced and numbered 0 to 116. Unless the data is a single slice then only a single image will be produced using the name given in the Output File parameter.

   `Link to the full online documentation for ITKImageWriter <http://www.dream3d.io/nx_reference_manual/Filters/ITKImageWriter>`_ 

   Mapping of UI display to python named argument

   +------------------------+-----------------------+
   | UI Display             | Python Named Argument |
   +========================+=======================+
   | Output File            | file_name             |
   +------------------------+-----------------------+
   | Input Image Data Array | image_array_path      |
   +------------------------+-----------------------+
   | Image Geometry         | image_geom_path       |
   +------------------------+-----------------------+
   | Index Offset           | index_offset          |
   +------------------------+-----------------------+
   | Plane                  | plane                 |
   +------------------------+-----------------------+

   .. py:method:: Execute(file_name, image_array_path, image_geom_path, index_offset, plane)

      :param complex.FileSystemPathParameter file_name: Path to the output file to write.
      :param complex.ArraySelectionParameter image_array_path: The image data that will be processed by this filter.
      :param complex.GeometrySelectionParameter image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.UInt64Parameter index_offset: This is the starting index when writing mulitple images
      :param complex.ChoicesParameter plane: Selection for plane normal for writing the images (XY, XZ, or YZ)
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKImportFijiMontageFilter:
.. py:class:: ITKImportFijiMontageFilter

   **UI Display Name:** *ITK Import Fiji Montage*

   Imports multiple images for the purpose of montage assembly. Each image is stored in it's own *DataContaner/AttributeMatrix/AttributeArray* where the name of the *DataContainer* is based off the row & column index of the montage. The filter assumes that the Configuration File is in the same folder as the images. The created *AttributeMatrix* and *AttributeArray* will have the same name. The image files **MUST** be located in the same directory as the Fiji Configuration File.

   `Link to the full online documentation for ITKImportFijiMontageFilter <http://www.dream3d.io/nx_reference_manual/Filters/ITKImportFijiMontageFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------------+----------------------------+
   | UI Display                               | Python Named Argument      |
   +==========================================+============================+
   | Cell Attribute Matrix Name               | cell_attribute_matrix_name |
   +------------------------------------------+----------------------------+
   | Change Origin                            | change_origin              |
   +------------------------------------------+----------------------------+
   | Color Weighting                          | color_weights              |
   +------------------------------------------+----------------------------+
   | Convert To GrayScale                     | convert_to_gray_scale      |
   +------------------------------------------+----------------------------+
   | Image Geometry Prefix                    | data_container_path        |
   +------------------------------------------+----------------------------+
   | Name of Created DataGroup                | data_group_name            |
   +------------------------------------------+----------------------------+
   | Image DataArray Name                     | image_data_array_name      |
   +------------------------------------------+----------------------------+
   | Fiji Configuration File                  | input_file                 |
   +------------------------------------------+----------------------------+
   | Length Unit                              | length_unit                |
   +------------------------------------------+----------------------------+
   | Origin                                   | origin                     |
   +------------------------------------------+----------------------------+
   | Parent Imported Images Under a DataGroup | parent_data_group          |
   +------------------------------------------+----------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, change_origin, color_weights, convert_to_gray_scale, data_container_path, data_group_name, image_data_array_name, input_file, length_unit, origin, parent_data_group)

      :param complex.StringParameter cell_attribute_matrix_name: The name of the Cell Attribute Matrix
      :param complex.BoolParameter change_origin: Set the origin of the mosaic to a user defined value
      :param complex.VectorFloat32Parameter color_weights: The luminosity values for the conversion
      :param complex.BoolParameter convert_to_gray_scale: The filter will show an error if the images are already in grayscale format
      :param complex.StringParameter data_container_path: A prefix that can be used for each Image Geometry
      :param complex.StringParameter data_group_name: Name of the overarching parent DataGroup
      :param complex.StringParameter image_data_array_name: The name of the import image data
      :param complex.FileSystemPathParameter input_file: This is the configuration file in the same directory as all of the identified geometries
      :param complex.ChoicesParameter length_unit: The length unit that will be set into the created image geometry
      :param complex.VectorFloat32Parameter origin: The new origin of the mosaic
      :param complex.BoolParameter parent_data_group: Create a new DataGroup to hold the  imported images
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKImportImageStack:
.. py:class:: ITKImportImageStack

   **UI Display Name:** *ITK Import Images (3D Stack)*

   Read in a stack of 2D images into a 3D volume with ITK. Supports most commonscalar pixel types and the many file formats supported by ITK.Images are read in as an **Image Geomotry**. The user must specify the originin physical space and resolution (uniform physical size of the resulting **Cells**).

   `Link to the full online documentation for ITKImportImageStack <http://www.dream3d.io/nx_reference_manual/Filters/ITKImportImageStack>`_ 

   Mapping of UI display to python named argument

   +---------------------------+------------------------+
   | UI Display                | Python Named Argument  |
   +===========================+========================+
   | Cell Data Name            | cell_data_name         |
   +---------------------------+------------------------+
   | Created Image Data        | image_data_array_path  |
   +---------------------------+------------------------+
   | Created Image Geometry    | image_geometry_path    |
   +---------------------------+------------------------+
   | Optional Slice Operations | image_transform_choice |
   +---------------------------+------------------------+
   | Input File List           | input_file_list_info   |
   +---------------------------+------------------------+
   | Origin                    | origin                 |
   +---------------------------+------------------------+
   | Spacing                   | spacing                |
   +---------------------------+------------------------+

   .. py:method:: Execute(cell_data_name, image_data_array_path, image_geometry_path, image_transform_choice, input_file_list_info, origin, spacing)

      :param complex.DataObjectNameParameter cell_data_name: The name of the created cell attribute matrix
      :param complex.DataObjectNameParameter image_data_array_path: The path to the created image data array
      :param complex.DataGroupCreationParameter image_geometry_path: The path to the created Image Geometry
      :param complex.ChoicesParameter image_transform_choice: Operation that is performed on each slice. 0=None, 1=Flip on X, 2=Flip on Y
      :param complex.GeneratedFileListParameter input_file_list_info: The list of 2D image files to be read in to a 3D volume
      :param complex.VectorFloat32Parameter origin: The origin of the 3D volume
      :param complex.VectorFloat32Parameter spacing: The spacing of the 3D volume
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKIntensityWindowingImage:
.. py:class:: ITKIntensityWindowingImage

   **UI Display Name:** *ITK Intensity Windowing Image Filter*

   IntensityWindowingImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should have and the lower and upper limits of the intensity window of the input image. This operation is very common in visualization, and can also be applied as a convenient preprocessing operation for image segmentation.

   `Link to the full online documentation for ITKIntensityWindowingImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKIntensityWindowingImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | OutputMaximum           | output_maximum           |
   +-------------------------+--------------------------+
   | OutputMinimum           | output_minimum           |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | WindowMaximum           | window_maximum           |
   +-------------------------+--------------------------+
   | WindowMinimum           | window_minimum           |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, output_maximum, output_minimum, selected_image_geom_path, window_maximum, window_minimum)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.Float64Parameter output_maximum: Set/Get the values of the maximum and minimum intensities of the output image.
      :param complex.Float64Parameter output_minimum: Set/Get the values of the maximum and minimum intensities of the output image.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.Float64Parameter window_maximum: Set/Get the values of the maximum and minimum intensities of the input intensity window.
      :param complex.Float64Parameter window_minimum: Set/Get the values of the maximum and minimum intensities of the input intensity window.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKInvertIntensityImage:
.. py:class:: ITKInvertIntensityImage

   **UI Display Name:** *ITK Invert Intensity Image Filter*

   InvertIntensityImageFilter inverts intensity of pixels by subtracting pixel value to a maximum value. The maximum value can be set with SetMaximum and defaults the maximum of input pixel type. This filter can be used to invert, for example, a binary image, a distance map, etc.

   `Link to the full online documentation for ITKInvertIntensityImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKInvertIntensityImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Maximum                 | maximum                  |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, maximum, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.Float64Parameter maximum: Set/Get the maximum intensity value for the inversion.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKLabelContourImage:
.. py:class:: ITKLabelContourImage

   **UI Display Name:** *ITK Label Contour Image Filter*

   LabelContourImageFilter takes a labeled image as input, where the pixels in the objects are the pixels with a value different of the BackgroundValue. Only the pixels on the contours of the objects are kept. The pixels not on the border are changed to BackgroundValue. The labels of the object are the same in the input and in the output image.

   `Link to the full online documentation for ITKLabelContourImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKLabelContourImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, fully_connected, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.Float64Parameter background_value: Set/Get the background value used to identify the objects and mark the pixels not on the border of the objects.
      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. note For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKLog10Image:
.. py:class:: ITKLog10Image

   **UI Display Name:** *ITK Log10 Image Filter*

   The computation is performed using std::log10(x).

   `Link to the full online documentation for ITKLog10Image <http://www.dream3d.io/nx_reference_manual/Filters/ITKLog10Image>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKLogImage:
.. py:class:: ITKLogImage

   **UI Display Name:** *ITK Log Image Filter*

   

   `Link to the full online documentation for ITKLogImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKLogImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKMaskImage:
.. py:class:: ITKMaskImage

   **UI Display Name:** *ITK Mask Image Filter*

   Mask an image with a mask.

   `Link to the full online documentation for ITKMaskImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKMaskImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | MaskImage               | mask_image_data_path     |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | OutsideValue            | outside_value            |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, mask_image_data_path, output_image_data_path, outside_value, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.ArraySelectionParameter mask_image_data_path: The path to the image data to be used as the mask (should be the same size as the input image)
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.Float64Parameter outside_value: Method to explicitly set the outside value of the mask.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKMedianImage:
.. py:class:: ITKMedianImage

   **UI Display Name:** *ITK Median Image Filter*

   Computes an image where a given pixel is the median value of the the pixels in a neighborhood about the corresponding input pixel.

   `Link to the full online documentation for ITKMedianImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKMedianImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Radius                  | radius                   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, radius, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.VectorUInt32Parameter radius: Radius Dimensions XYZ
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKMhaFileReader:
.. py:class:: ITKMhaFileReader

   **UI Display Name:** *ITK MHA File Reader*

   Reads MHA images and their transformation matrices using ITK

   `Link to the full online documentation for ITKMhaFileReader <http://www.dream3d.io/nx_reference_manual/Filters/ITKMhaFileReader>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------+---------------------------------------+
   | UI Display                             | Python Named Argument                 |
   +========================================+=======================================+
   | Apply Image Transformation To Geometry | apply_image_transformation            |
   +----------------------------------------+---------------------------------------+
   | Cell Data Name                         | cell_data_name                        |
   +----------------------------------------+---------------------------------------+
   | Input MHA File                         | file_name                             |
   +----------------------------------------+---------------------------------------+
   | Created Image Geometry                 | geometry_path                         |
   +----------------------------------------+---------------------------------------+
   | Created Image Data                     | image_data_array_path                 |
   +----------------------------------------+---------------------------------------+
   | Interpolation Type                     | interpolation_type                    |
   +----------------------------------------+---------------------------------------+
   | Save Image Transformation As Array     | save_image_transformation             |
   +----------------------------------------+---------------------------------------+
   | Transformation Matrix                  | transformation_matrix_data_array_path |
   +----------------------------------------+---------------------------------------+

   .. py:method:: Execute(apply_image_transformation, cell_data_name, file_name, geometry_path, image_data_array_path, interpolation_type, save_image_transformation, transformation_matrix_data_array_path)

      :param complex.BoolParameter apply_image_transformation: When true, the transformation matrix found in the image's header metadata will be applied to the created image geometry.
      :param complex.DataObjectNameParameter cell_data_name: The name of the created cell attribute matrix
      :param complex.FileSystemPathParameter file_name: The input .mha file that will be read.
      :param complex.DataGroupCreationParameter geometry_path: The path to the created Image Geometry
      :param complex.ArrayCreationParameter image_data_array_path: The path to the created image data array
      :param complex.ChoicesParameter interpolation_type: The type of interpolation algorithm that is used. 0=NearestNeighbor, 1=Linear
      :param complex.BoolParameter save_image_transformation: When true, the transformation matrix found in the image's header metadata will be saved as a data array in the created image geometry.
      :param complex.ArrayCreationParameter transformation_matrix_data_array_path: The path to the transformation matrix data array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKMorphologicalGradientImage:
.. py:class:: ITKMorphologicalGradientImage

   **UI Display Name:** *ITK Morphological Gradient Image Filter*

   The structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring element having values > 0 are candidates for affecting the center pixel.* MorphologyImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

   `Link to the full online documentation for ITKMorphologicalGradientImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKMorphologicalGradientImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKMorphologicalWatershedImage:
.. py:class:: ITKMorphologicalWatershedImage

   **UI Display Name:** *ITK Morphological Watershed Image Filter*

   Watershed pixel are labeled 0. TOutputImage should be an integer type. Labels of output image are in no particular order. You can reorder the labels such that object labels are consecutive and sorted based on object size by passing the output of this filter to a RelabelComponentImageFilter .

   `Link to the full online documentation for ITKMorphologicalWatershedImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKMorphologicalWatershedImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Level                   | level                    |
   +-------------------------+--------------------------+
   | MarkWatershedLine       | mark_watershed_line      |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, level, mark_watershed_line, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.Float64Parameter level: 
      :param complex.BoolParameter mark_watershed_line: Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false do not only avoid writing watershed pixels, it also decrease algorithm complexity.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKNormalizeImage:
.. py:class:: ITKNormalizeImage

   **UI Display Name:** *ITK Normalize Image Filter*

   NormalizeImageFilter shifts and scales an image so that the pixels in the image have a zero mean and unit variance. This filter uses StatisticsImageFilter to compute the mean and variance of the input and then applies ShiftScaleImageFilter to shift and scale the pixels.

   `Link to the full online documentation for ITKNormalizeImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKNormalizeImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKNotImage:
.. py:class:: ITKNotImage

   **UI Display Name:** *ITK Not Image Filter*

   This class is templated over the type of an input image and the type of the output image. Numeric conversions (castings) are done by the C++ defaults.

   `Link to the full online documentation for ITKNotImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKNotImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKOpeningByReconstructionImage:
.. py:class:: ITKOpeningByReconstructionImage

   **UI Display Name:** *ITK Opening By Reconstruction Image Filter*

   This filter preserves regions, in the foreground, that can completely contain the structuring element. At the same time, this filter eliminates all other regions of foreground pixels. Contrary to the morphological opening, the opening by reconstruction preserves the shape of the components that are not removed by erosion. The opening by reconstruction of an image "f" is defined as:

   `Link to the full online documentation for ITKOpeningByReconstructionImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKOpeningByReconstructionImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | PreserveIntensities     | preserve_intensities     |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, kernel_radius, kernel_type, output_image_data_path, preserve_intensities, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter preserve_intensities: Set/Get whether the original intensities of the image retained for those pixels unaffected by the opening by reconstruction. If Off, the output pixel contrast will be reduced.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKOtsuMultipleThresholdsImage:
.. py:class:: ITKOtsuMultipleThresholdsImage

   **UI Display Name:** *ITK Otsu Multiple Thresholds Image Filter*

   This filter creates a labeled image that separates the input image into various classes. The filter computes the thresholds using the OtsuMultipleThresholdsCalculator and applies those thresholds to the input image using the ThresholdLabelerImageFilter . The NumberOfHistogramBins and NumberOfThresholds can be set for the Calculator. The LabelOffset can be set for the ThresholdLabelerImageFilter .

   `Link to the full online documentation for ITKOtsuMultipleThresholdsImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKOtsuMultipleThresholdsImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | LabelOffset             | label_offset             |
   +-------------------------+--------------------------+
   | NumberOfHistogramBins   | number_of_histogram_bins |
   +-------------------------+--------------------------+
   | NumberOfThresholds      | number_of_thresholds     |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | ReturnBinMidpoint       | return_bin_midpoint      |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | ValleyEmphasis          | valley_emphasis          |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, label_offset, number_of_histogram_bins, number_of_thresholds, output_image_data_path, return_bin_midpoint, selected_image_geom_path, valley_emphasis)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.UInt8Parameter label_offset: Set/Get the offset which labels have to start from. Default is 0.
      :param complex.UInt32Parameter number_of_histogram_bins: Set/Get the number of histogram bins. Default is 128.
      :param complex.UInt8Parameter number_of_thresholds: Set/Get the number of thresholds. Default is 1.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter return_bin_midpoint: Should the threshold value be mid-point of the bin or the maximum? Default is to return bin maximum.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.BoolParameter valley_emphasis: Set/Get the use of valley emphasis. Default is false.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKRelabelComponentImage:
.. py:class:: ITKRelabelComponentImage

   **UI Display Name:** *ITK Relabel Component Image Filter*

   RelabelComponentImageFilter remaps the labels associated with the objects in an image (as from the output of ConnectedComponentImageFilter ) such that the label numbers are consecutive with no gaps between the label numbers used. By default, the relabeling will also sort the labels based on the size of the object: the largest object will have label #1, the second largest will have label #2, etc. If two labels have the same size their initial order is kept. The sorting by size can be disabled using SetSortByObjectSize.

   `Link to the full online documentation for ITKRelabelComponentImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKRelabelComponentImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | MinimumObjectSize       | minimum_object_size      |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | SortByObjectSize        | sort_by_object_size      |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, minimum_object_size, output_image_data_path, selected_image_geom_path, sort_by_object_size)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.UInt64Parameter minimum_object_size: Set the minimum size in pixels for an object. All objects smaller than this size will be discarded and will not appear in the output label map. NumberOfObjects will count only the objects whose pixel counts are greater than or equal to the minimum size. Call GetOriginalNumberOfObjects to find out how many objects were present in the original label map.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.BoolParameter sort_by_object_size: Controls whether the object labels are sorted by size. If false, initial order of labels is kept.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKRescaleIntensityImage:
.. py:class:: ITKRescaleIntensityImage

   **UI Display Name:** *ITK Rescale Intensity Image Filter*

   RescaleIntensityImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should have.

   `Link to the full online documentation for ITKRescaleIntensityImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKRescaleIntensityImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | OutputMaximum           | output_maximum           |
   +-------------------------+--------------------------+
   | OutputMinimum           | output_minimum           |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, output_maximum, output_minimum, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.Float64Parameter output_maximum: The maximum output value that is used.
      :param complex.Float64Parameter output_minimum: The minimum output value that is used.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKSigmoidImage:
.. py:class:: ITKSigmoidImage

   **UI Display Name:** *ITK Sigmoid Image Filter*

   A linear transformation is applied first on the argument of the sigmoid function. The resulting total transform is given by

   `Link to the full online documentation for ITKSigmoidImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKSigmoidImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Alpha                   | alpha                    |
   +-------------------------+--------------------------+
   | Beta                    | beta                     |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | OutputMaximum           | output_maximum           |
   +-------------------------+--------------------------+
   | OutputMinimum           | output_minimum           |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(alpha, beta, input_image_data_path, output_image_data_path, output_maximum, output_minimum, selected_image_geom_path)

      :param complex.Float64Parameter alpha: The Alpha value from the Sigmoid equation. 
      :param complex.Float64Parameter beta: The Beta value from teh sigmoid equation
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.Float64Parameter output_maximum: The maximum output value
      :param complex.Float64Parameter output_minimum: The minimum output value
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKSignedMaurerDistanceMapImage:
.. py:class:: ITKSignedMaurerDistanceMapImage

   **UI Display Name:** *ITK Signed Maurer Distance Map Image Filter*

   

   `Link to the full online documentation for ITKSignedMaurerDistanceMapImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKSignedMaurerDistanceMapImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | BackgroundValue         | background_value         |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | InsideIsPositive        | inside_is_positive       |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | SquaredDistance         | squared_distance         |
   +-------------------------+--------------------------+
   | UseImageSpacing         | use_image_spacing        |
   +-------------------------+--------------------------+

   .. py:method:: Execute(background_value, input_image_data_path, inside_is_positive, output_image_data_path, selected_image_geom_path, squared_distance, use_image_spacing)

      :param complex.Float64Parameter background_value: Set the background value which defines the object. Usually this value is = 0.
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.BoolParameter inside_is_positive: Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.BoolParameter squared_distance: Set if the distance should be squared.
      :param complex.BoolParameter use_image_spacing: Set if image spacing should be used in computing distances.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKSinImage:
.. py:class:: ITKSinImage

   **UI Display Name:** *ITK Sin Image Filter*

   The computations are performed using std::sin(x).

   `Link to the full online documentation for ITKSinImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKSinImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKSqrtImage:
.. py:class:: ITKSqrtImage

   **UI Display Name:** *ITK Sqrt Image Filter*

   The computations are performed using std::sqrt(x).

   `Link to the full online documentation for ITKSqrtImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKSqrtImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKSquareImage:
.. py:class:: ITKSquareImage

   **UI Display Name:** *ITK Square Image Filter*

   

   `Link to the full online documentation for ITKSquareImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKSquareImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKTanImage:
.. py:class:: ITKTanImage

   **UI Display Name:** *ITK Tan Image Filter*

   The computations are performed using std::tan(x).

   `Link to the full online documentation for ITKTanImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKTanImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKThresholdImage:
.. py:class:: ITKThresholdImage

   **UI Display Name:** *ITK Threshold Image Filter*

   ThresholdImageFilter sets image values to a user-specified "outside" value (by default, "black") if the image values are below, above, or between simple threshold values.

   `Link to the full online documentation for ITKThresholdImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKThresholdImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Lower                   | lower                    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | OutsideValue            | outside_value            |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+
   | Upper                   | upper                    |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, lower, output_image_data_path, outside_value, selected_image_geom_path, upper)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.Float64Parameter lower: Set/Get methods to set the lower threshold.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.Float64Parameter outside_value: The pixel type must support comparison operators. Set the 'outside' pixel value. The default value NumericTraits<PixelType>::ZeroValue() .
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :param complex.Float64Parameter upper: Set/Get methods to set the upper threshold.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKValuedRegionalMaximaImage:
.. py:class:: ITKValuedRegionalMaximaImage

   **UI Display Name:** *ITK Valued Regional Maxima Image Filter*

   Regional maxima are flat zones surrounded by pixels of lower value. A completely flat image will be marked as a regional maxima by this filter.

   `Link to the full online documentation for ITKValuedRegionalMaximaImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKValuedRegionalMaximaImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: 
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKValuedRegionalMinimaImage:
.. py:class:: ITKValuedRegionalMinimaImage

   **UI Display Name:** *ITK Valued Regional Minima Image Filter*

   Regional minima are flat zones surrounded by pixels of higher value. A completely flat image will be marked as a regional minima by this filter.

   `Link to the full online documentation for ITKValuedRegionalMinimaImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKValuedRegionalMinimaImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | FullyConnected          | fully_connected          |
   +-------------------------+--------------------------+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(fully_connected, input_image_data_path, output_image_data_path, selected_image_geom_path)

      :param complex.BoolParameter fully_connected: 
      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ITKWhiteTopHatImage:
.. py:class:: ITKWhiteTopHatImage

   **UI Display Name:** *ITK White Top Hat Image Filter*

   Top-hats are described in Chapter 4.5 of Pierre Soille's book "Morphological Image Analysis: Principles and Applications", Second Edition, Springer, 2003.

   `Link to the full online documentation for ITKWhiteTopHatImage <http://www.dream3d.io/nx_reference_manual/Filters/ITKWhiteTopHatImage>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------+
   | UI Display              | Python Named Argument    |
   +=========================+==========================+
   | Input Image Data Array  | input_image_data_path    |
   +-------------------------+--------------------------+
   | KernelRadius            | kernel_radius            |
   +-------------------------+--------------------------+
   | KernelType              | kernel_type              |
   +-------------------------+--------------------------+
   | Output Image Data Array | output_image_data_path   |
   +-------------------------+--------------------------+
   | SafeBorder              | safe_border              |
   +-------------------------+--------------------------+
   | Image Geometry          | selected_image_geom_path |
   +-------------------------+--------------------------+

   .. py:method:: Execute(input_image_data_path, kernel_radius, kernel_type, output_image_data_path, safe_border, selected_image_geom_path)

      :param complex.ArraySelectionParameter input_image_data_path: The image data that will be processed by this filter.
      :param complex.VectorUInt32Parameter kernel_radius: The radius of the kernel structuring element.
      :param complex.ChoicesParameter kernel_type: The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross
      :param complex.DataObjectNameParameter output_image_data_path: The result of the processing will be stored in this Data Array.
      :param complex.BoolParameter safe_border: A safe border is added to input image to avoid borders effects and remove it once the closing is done
      :param complex.GeometrySelectionParameter selected_image_geom_path: Select the Image Geometry Group from the DataStructure.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


