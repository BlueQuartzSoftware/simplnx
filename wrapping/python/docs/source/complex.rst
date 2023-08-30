complex
=======

.. py:module:: complex

.. _AbaqusHexahedronWriterFilter:
.. py:class:: AbaqusHexahedronWriterFilter

   **UI Display Name:** *Abaqus Hexahedron Exporter*

   This **Filter** produces the basic five Abaqus .inp files for input into the Abaqus analysis tool. The files created are: xxx.inp (the master file), xxx_nodes.inp, xxx_elems.inp, xxx_elset.inp and xxx_sects.inp. This **Filter** is based on a Python script developed by Matthew W. Priddy (Ga. Tech., early 2015).

   `Link to the full online documentation for AbaqusHexahedronWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/AbaqusHexahedronWriterFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+------------------------+
   | UI Display                | Python Named Argument  |
   +===========================+========================+
   | Feature Ids               | feature_ids_array_path |
   +---------------------------+------------------------+
   | Output File Prefix        | file_prefix            |
   +---------------------------+------------------------+
   | Hourglass Stiffness Value | hourglass_stiffness    |
   +---------------------------+------------------------+
   | Selected Image Geometry   | image_geometry_path    |
   +---------------------------+------------------------+
   | Job Name                  | job_name               |
   +---------------------------+------------------------+
   | Output Path               | output_path            |
   +---------------------------+------------------------+

   .. py:method:: Execute(feature_ids_array_path, file_prefix, hourglass_stiffness, image_geometry_path, job_name, output_path)

      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.StringParameter file_prefix: The prefix to use for each output file.
      :param complex.Int32Parameter hourglass_stiffness: The value to use for the Hourglass Stiffness
      :param complex.GeometrySelectionParameter image_geometry_path: The input Image Geometry that will be written.
      :param complex.StringParameter job_name: The name of the job
      :param complex.FileSystemPathParameter output_path: The output file path
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AddBadDataFilter:
.. py:class:: AddBadDataFilter

   **UI Display Name:** *Add Bad Data*

   This **Filter** adds "bad" data to an **Image Geometry**.  This **Filter** is intended to add "realism" (i.e., more representative of an experimental dataset) to synthetic structures that don not have any "bad" **Cells**.  The user can choose to add "random noise" and/or "noise" along **Feature** boundaries. For a given type of noise, the user must then set the volume fraction of **Cells** to set as "bad".  The volume fractions entered apply to only the set of **Cells** that the noise would affect.  For example, if the user chose *0.2* for the volume fraction of boundary "noise", then each boundary **Cell** would have a *20%* chance of being changed to a "bad" **Cell** and all other **Cells** would have a *0%* chance of being changed. In order to compute noise over the **Feature** boundaries, the **Filter** needs the Manhattan distances for each **Cell** from the **Feature** boundaries. Note that the computed Manhattan distances are floating point values, but this **Filter** requires an integer array. To create the necessary integer array, use the Convert Attributer Data Type **Filter** to cast the Manhattan distance array to an integer array.

   `Link to the full online documentation for AddBadDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/AddBadDataFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------+-----------------------------------+
   | UI Display                        | Python Named Argument             |
   +===================================+===================================+
   | Add Boundary Noise                | boundary_noise                    |
   +-----------------------------------+-----------------------------------+
   | Volume Fraction of Boundary Noise | boundary_vol_fraction             |
   +-----------------------------------+-----------------------------------+
   | Boundary Euclidean Distances      | gb_euclidean_distances_array_path |
   +-----------------------------------+-----------------------------------+
   | Image Geometry                    | image_geometry_path               |
   +-----------------------------------+-----------------------------------+
   | Add Random Noise                  | poisson_noise                     |
   +-----------------------------------+-----------------------------------+
   | Volume Fraction of Random Noise   | poisson_vol_fraction              |
   +-----------------------------------+-----------------------------------+
   | Seed                              | seed_value                        |
   +-----------------------------------+-----------------------------------+
   | Use Seed for Random Generation    | use_seed                          |
   +-----------------------------------+-----------------------------------+

   .. py:method:: Execute(boundary_noise, boundary_vol_fraction, gb_euclidean_distances_array_path, image_geometry_path, poisson_noise, poisson_vol_fraction, seed_value, use_seed)

      :param complex.BoolParameter boundary_noise: If true the user may set the boundary volume fraction
      :param complex.Float32Parameter boundary_vol_fraction: A value between 0 and 1 inclusive that is compared against random generation
      :param complex.ArraySelectionParameter gb_euclidean_distances_array_path: This is the GB Manhattan Distances array
      :param complex.GeometrySelectionParameter image_geometry_path: The selected image geometry
      :param complex.BoolParameter poisson_noise: If true the user may set the poisson volume fraction
      :param complex.Float32Parameter poisson_vol_fraction: A value between 0 and 1 inclusive that is compared against random generation
      :param complex.UInt64Parameter seed_value: The seed fed into the random generator
      :param complex.BoolParameter use_seed: When true the user will be able to put in a seed for random generation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AlignGeometries:
.. py:class:: AlignGeometries

   **UI Display Name:** *Align Geometries*

   This **Filter** will align 2 Geometry objects using 1 of several alignment methods:

   `Link to the full online documentation for AlignGeometries <http://www.dream3d.io/nx_reference_manual/Filters/AlignGeometries>`_ 

   Mapping of UI display to python named argument

   +-----------------+-----------------------+
   | UI Display      | Python Named Argument |
   +=================+=======================+
   | Alignment Type  | alignment_type        |
   +-----------------+-----------------------+
   | Moving Geometry | moving_geometry       |
   +-----------------+-----------------------+
   | Fixed Geometry  | target_geometry       |
   +-----------------+-----------------------+

   .. py:method:: Execute(alignment_type, moving_geometry, target_geometry)

      :param complex.ChoicesParameter alignment_type: The type of alignment to perform (Origin or Centroid.
      :param complex.GeometrySelectionParameter moving_geometry: The geometry that will be moved.
      :param complex.GeometrySelectionParameter target_geometry: The geometry that does *not* move.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AlignSectionsFeatureCentroidFilter:
.. py:class:: AlignSectionsFeatureCentroidFilter

   **UI Display Name:** *Align Sections (Feature Centroid)*

   This **Filter** attempts to align 'sections' of the sample perpendicular to the Z-direction by determining the position that closest aligns the centroid(s) of previously defined "regions".  The "regions" are defined by a boolean array where the **Cells** have been flagged by another **Filter**. Typically, during reading/processing of the data, each **Cell** is subject to a "quality metric" (or threshold) that defines if the **Cell** is *good*.  This threshold can be used to define areas of each slice that are bad, either due to actual features in the microstructure or external references inserted by the user/experimentalist.  If these "regions" of *bad* **Cells** are believed to be consistent through sections, then this **Filter** will preserve that by aligning those "regions" on top of one another on consecutive sections. The algorithm of this **Filter** is as follows:

   `Link to the full online documentation for AlignSectionsFeatureCentroidFilter <http://www.dream3d.io/nx_reference_manual/Filters/AlignSectionsFeatureCentroidFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+------------------------------+
   | UI Display                 | Python Named Argument        |
   +============================+==============================+
   | Alignment File Path        | alignment_shift_file_name    |
   +----------------------------+------------------------------+
   | Mask                       | good_voxels_array_path       |
   +----------------------------+------------------------------+
   | Reference Slice            | reference_slice              |
   +----------------------------+------------------------------+
   | Cell Data Attribute Matrix | selected_cell_data_path      |
   +----------------------------+------------------------------+
   | Selected Image Geometry    | selected_image_geometry_path |
   +----------------------------+------------------------------+
   | Use Reference Slice        | use_reference_slice          |
   +----------------------------+------------------------------+
   | Write Alignment Shift File | write_alignment_shifts       |
   +----------------------------+------------------------------+

   .. py:method:: Execute(alignment_shift_file_name, good_voxels_array_path, reference_slice, selected_cell_data_path, selected_image_geometry_path, use_reference_slice, write_alignment_shifts)

      :param complex.FileSystemPathParameter alignment_shift_file_name: The output file path where the user would like the shifts applied to the section to be written.
      :param complex.ArraySelectionParameter good_voxels_array_path: Path to the DataArray Mask
      :param complex.Int32Parameter reference_slice: Slice number to use as reference
      :param complex.AttributeMatrixSelectionParameter selected_cell_data_path: Cell Data Attribute Matrix
      :param complex.GeometrySelectionParameter selected_image_geometry_path: The target geometry on which to perform the alignment
      :param complex.BoolParameter use_reference_slice: Whether the centroids of each section should be compared to a reference slice instead of their neighboring section
      :param complex.BoolParameter write_alignment_shifts: Whether to write the shifts applied to each section to a file
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AlignSectionsListFilter:
.. py:class:: AlignSectionsListFilter

   **UI Display Name:** *Align Sections (List)*

   This **Filter** will apply the cell shifts from a user specified txt file to each section of an Image Geometry. It accepts an alignment .txt file that has full **Cells** shifts that have already been calculated in it.

   `Link to the full online documentation for AlignSectionsListFilter <http://www.dream3d.io/nx_reference_manual/Filters/AlignSectionsListFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------+
   | UI Display                    | Python Named Argument   |
   +===============================+=========================+
   | DREAM3D Alignment File Format | dream3d_alignment_file  |
   +-------------------------------+-------------------------+
   | Input File                    | input_file              |
   +-------------------------------+-------------------------+
   | Selected Image Geometry       | selected_image_geometry |
   +-------------------------------+-------------------------+

   .. py:method:: Execute(dream3d_alignment_file, input_file, selected_image_geometry)

      :param complex.BoolParameter dream3d_alignment_file: Turn this ON if the alignment file was generated by another DREAM.3D Alignment filter
      :param complex.FileSystemPathParameter input_file: The input .txt file path containing the shifts to apply to the sections
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry on which to perform the alignment
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AppendImageGeometryZSliceFilter:
.. py:class:: AppendImageGeometryZSliceFilter

   **UI Display Name:** *Append Z Slice (Image Geometry)*

   This filter allows the user to append an Image Geometry onto the "end" of another Image Geometry. The input anddestination **ImageGeometry** objects must have the same X&Y dimensions. Optional Checks for equal **Resolution** valuescan also be performed.

   `Link to the full online documentation for AppendImageGeometryZSliceFilter <http://www.dream3d.io/nx_reference_manual/Filters/AppendImageGeometryZSliceFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-----------------------+
   | UI Display                 | Python Named Argument |
   +============================+=======================+
   | Check Spacing              | check_resolution      |
   +----------------------------+-----------------------+
   | Destination Image Geometry | destination_geometry  |
   +----------------------------+-----------------------+
   | Input Image Geometry       | input_geometry        |
   +----------------------------+-----------------------+
   | New Image Geometry         | new_geometry          |
   +----------------------------+-----------------------+
   | Save as new geometry       | save_as_new_geometry  |
   +----------------------------+-----------------------+

   .. py:method:: Execute(check_resolution, destination_geometry, input_geometry, new_geometry, save_as_new_geometry)

      :param complex.BoolParameter check_resolution: Checks to make sure the spacing for the input geometry and destination geometry match
      :param complex.GeometrySelectionParameter destination_geometry: The destination image geometry (cell data) that is the final location for the appended data.
      :param complex.GeometrySelectionParameter input_geometry: The incoming image geometry (cell data) that is to be appended.
      :param complex.DataGroupCreationParameter new_geometry: The path to the new geometry with the combined data from the input & destination geometry
      :param complex.BoolParameter save_as_new_geometry: Save the combined data as a new geometry instead of appending the input data to the destination geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ApplyTransformationToGeometryFilter:
.. py:class:: ApplyTransformationToGeometryFilter

   **UI Display Name:** *Apply Transformation to Geometry*

   This **Filter** applies a spatial transformation to either and unstructured **Geometry** or an ImageGeometry. An "unstructured" **Geometry** is any geometry that requires explicit definition of **Vertex** positions. Specifically, **Vertex**, **Edge**, **Triangle**, **Quadrilateral**, and **Tetrahedral** **Geometries** may be transformed by this **Filter**. The transformation is applied in place, so the input **Geometry** will be modified.

   `Link to the full online documentation for ApplyTransformationToGeometryFilter <http://www.dream3d.io/nx_reference_manual/Filters/ApplyTransformationToGeometryFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------------------------------+-------------------------------------+
   | UI Display                                                | Python Named Argument               |
   +===========================================================+=====================================+
   | Cell Attribute Matrix                                     | cell_attribute_matrix_path          |
   +-----------------------------------------------------------+-------------------------------------+
   | Precomputed Transformation Matrix Path                    | computed_transformation_matrix      |
   +-----------------------------------------------------------+-------------------------------------+
   | Resampling or Interpolation                               | interpolation_type                  |
   +-----------------------------------------------------------+-------------------------------------+
   | Transformation Matrix                                     | manual_transformation_matrix        |
   +-----------------------------------------------------------+-------------------------------------+
   | Rotation Axis-Angle                                       | rotation                            |
   +-----------------------------------------------------------+-------------------------------------+
   | Scale                                                     | scale                               |
   +-----------------------------------------------------------+-------------------------------------+
   | Selected Geometry                                         | selected_image_geometry             |
   +-----------------------------------------------------------+-------------------------------------+
   | Transformation Type                                       | transformation_type                 |
   +-----------------------------------------------------------+-------------------------------------+
   | Translate Geometry To Global Origin Before Transformation | translate_geometry_to_global_origin |
   +-----------------------------------------------------------+-------------------------------------+
   | Translation                                               | translation                         |
   +-----------------------------------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_path, computed_transformation_matrix, interpolation_type, manual_transformation_matrix, rotation, scale, selected_image_geometry, transformation_type, translate_geometry_to_global_origin, translation)

      :param complex.AttributeMatrixSelectionParameter cell_attribute_matrix_path: The path to the Cell level data that should be interpolated
      :param complex.ArraySelectionParameter computed_transformation_matrix: A precomputed 4x4 transformation matrix
      :param complex.ChoicesParameter interpolation_type: Select the type of interpolation algorithm
      :param complex.DynamicTableParameter manual_transformation_matrix: The 4x4 Transformation Matrix
      :param complex.VectorFloat32Parameter rotation: <xyz> w (w in degrees)
      :param complex.VectorFloat32Parameter scale: 0>= value < 1: Shrink, value = 1: No transform, value > 1.0 enlarge
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry on which to perform the transformation
      :param complex.ChoicesParameter transformation_type: The type of transformation to perform.
      :param complex.BoolParameter translate_geometry_to_global_origin: Specifies whether to translate the geometry to (0, 0, 0), apply the transformation, and then translate the geometry back to its original origin.
      :param complex.VectorFloat32Parameter translation: A pure translation vector
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ApproximatePointCloudHull:
.. py:class:: ApproximatePointCloudHull

   **UI Display Name:** *Approximate Point Cloud Hull*

   This **Filter** determines a set of points that approximates the surface (or *hull*) or a 3D point cloud represented by a **Vertex Geometry**.  The hull is approximate in that the surface points are not guaranteed to hve belonged to the original point cloud; instead, the determined set of points is meant to represent a sampling of where the 3D point cloud surface occurs. To following steps are used to approximate the hull:

   `Link to the full online documentation for ApproximatePointCloudHull <http://www.dream3d.io/nx_reference_manual/Filters/ApproximatePointCloudHull>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------+-----------------------+
   | UI Display                        | Python Named Argument |
   +===================================+=======================+
   | Grid Resolution                   | grid_resolution       |
   +-----------------------------------+-----------------------+
   | Hull Vertex Geometry              | hull_vertex_geometry  |
   +-----------------------------------+-----------------------+
   | Minimum Number of Empty Neighbors | min_empty_neighbors   |
   +-----------------------------------+-----------------------+
   | Vertex Geometry                   | vertex_geometry       |
   +-----------------------------------+-----------------------+

   .. py:method:: Execute(grid_resolution, hull_vertex_geometry, min_empty_neighbors, vertex_geometry)

      :param complex.VectorFloat32Parameter grid_resolution: Geometry resolution
      :param complex.DataGroupCreationParameter hull_vertex_geometry: Path to create the hull Vertex geometry
      :param complex.UInt64Parameter min_empty_neighbors: Minimum number of empty neighbors
      :param complex.DataPathSelectionParameter vertex_geometry: Path to the target Vertex geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ArrayCalculatorFilter:
.. py:class:: ArrayCalculatorFilter

   **UI Display Name:** *Attribute Array Calculator*

   This **Filter** performs calculations on **Attribute Arrays** using the mathematical expression entered by the user, referred to as the *infix expression*. Calculations follow standard mathematical order of operations rules. Parentheses may be used to influence priority. The output of the entered equation is stored as a new **Attribute Array** of type double in an **Attribute Matrix** chosen by the user.

   `Link to the full online documentation for ArrayCalculatorFilter <http://www.dream3d.io/nx_reference_manual/Filters/ArrayCalculatorFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-----------------------+
   | UI Display              | Python Named Argument |
   +=========================+=======================+
   | Output Calculated Array | calculated_array      |
   +-------------------------+-----------------------+
   | Infix Expression        | infix_equation        |
   +-------------------------+-----------------------+
   | Output Scalar Type      | scalar_type           |
   +-------------------------+-----------------------+

   .. py:method:: Execute(calculated_array, infix_equation, scalar_type)

      :param complex.ArrayCreationParameter calculated_array: The path to the calculated array
      :param complex.CalculatorParameter infix_equation: The mathematical expression used to calculate the output array
      :param complex.NumericTypeParameter scalar_type: The data type of the calculated array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AvizoRectilinearCoordinateWriterFilter:
.. py:class:: AvizoRectilinearCoordinateWriterFilter

   **UI Display Name:** *Avizo Rectilinear Coordinate Exporter*

   This filter writes out a native Avizo Rectilinear Coordinate data file. Values should be present from segmentation of experimental data or synthetic generation and cannot be determined by this filter. Not having these values will result in the filter to fail/not execute.

   `Link to the full online documentation for AvizoRectilinearCoordinateWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/AvizoRectilinearCoordinateWriterFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------+------------------------+
   | UI Display        | Python Named Argument  |
   +===================+========================+
   | Feature Ids       | feature_ids_array_path |
   +-------------------+------------------------+
   | Image Geometry    | geometry_path          |
   +-------------------+------------------------+
   | Output File       | output_file            |
   +-------------------+------------------------+
   | Units             | units                  |
   +-------------------+------------------------+
   | Write Binary File | write_binary_file      |
   +-------------------+------------------------+

   .. py:method:: Execute(feature_ids_array_path, geometry_path, output_file, units, write_binary_file)

      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.GeometrySelectionParameter geometry_path: The path to the input image geometry
      :param complex.FileSystemPathParameter output_file: Amira Mesh .am file created
      :param complex.StringParameter units: The units of the data
      :param complex.BoolParameter write_binary_file: Whether or not to write the output file as binary
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AvizoUniformCoordinateWriterFilter:
.. py:class:: AvizoUniformCoordinateWriterFilter

   **UI Display Name:** *Avizo Uniform Coordinate Exporter*

   This filter writes out a native Avizo Uniform Coordinate data file. Values should be present from segmentation of experimental data or synthetic generation and cannot be determined by this filter. Not having these values will result in the filter to fail/not execute.

   `Link to the full online documentation for AvizoUniformCoordinateWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/AvizoUniformCoordinateWriterFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------+------------------------+
   | UI Display        | Python Named Argument  |
   +===================+========================+
   | Feature Ids       | feature_ids_array_path |
   +-------------------+------------------------+
   | Image Geometry    | geometry_path          |
   +-------------------+------------------------+
   | Output File       | output_file            |
   +-------------------+------------------------+
   | Units             | units                  |
   +-------------------+------------------------+
   | Write Binary File | write_binary_file      |
   +-------------------+------------------------+

   .. py:method:: Execute(feature_ids_array_path, geometry_path, output_file, units, write_binary_file)

      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.GeometrySelectionParameter geometry_path: The path to the input image geometry
      :param complex.FileSystemPathParameter output_file: Amira Mesh .am file created
      :param complex.StringParameter units: The units of the data
      :param complex.BoolParameter write_binary_file: Whether or not to write the output file as binary
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CalculateArrayHistogramFilter:
.. py:class:: CalculateArrayHistogramFilter

   **UI Display Name:** *Calculate Frequency Histogram*

   This **Filter** accepts **DataArray(s)** as input, creates histogram **DataArray(s)** in specified **DataGroup** from input **DataArray(s)**, then calculates histogram values according to user parameters and stores values in created histogram **DataArray(s)**.

   `Link to the full online documentation for CalculateArrayHistogramFilter <http://www.dream3d.io/nx_reference_manual/Filters/CalculateArrayHistogramFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------+-----------------------+
   | UI Display                          | Python Named Argument |
   +=====================================+=======================+
   | Output DataGroup Path               | data_group_name       |
   +-------------------------------------+-----------------------+
   | Suffix for created Histograms       | histogram_suffix      |
   +-------------------------------------+-----------------------+
   | Max Value                           | max_range             |
   +-------------------------------------+-----------------------+
   | Min Value                           | min_range             |
   +-------------------------------------+-----------------------+
   | Create New DataGroup for Histograms | new_data_group        |
   +-------------------------------------+-----------------------+
   | New DataGroup Path                  | new_data_group_name   |
   +-------------------------------------+-----------------------+
   | Number of Bins                      | number_of_bins        |
   +-------------------------------------+-----------------------+
   | Input Data Arrays                   | selected_array_paths  |
   +-------------------------------------+-----------------------+
   | Use Custom Min & Max Range          | user_defined_range    |
   +-------------------------------------+-----------------------+

   .. py:method:: Execute(data_group_name, histogram_suffix, max_range, min_range, new_data_group, new_data_group_name, number_of_bins, selected_array_paths, user_defined_range)

      :param complex.DataGroupSelectionParameter data_group_name: The complete path to the DataGroup in which to store the calculated histogram(s)
      :param complex.StringParameter histogram_suffix: String appended to the end of the histogram array names
      :param complex.Float64Parameter max_range: Specifies the upper bound of the histogram.
      :param complex.Float64Parameter min_range: Specifies the lower bound of the histogram.
      :param complex.BoolParameter new_data_group: Whether or not to store the calculated histogram(s) in a new DataGroup
      :param complex.DataGroupCreationParameter new_data_group_name: The path to the new DataGroup in which to store the calculated histogram(s)
      :param complex.Int32Parameter number_of_bins: Specifies number of histogram bins (greater than zero)
      :param complex.MultiArraySelectionParameter selected_array_paths: The list of arrays to calculate histogram(s) for
      :param complex.BoolParameter user_defined_range: Whether the user can set the min and max values to consider for the histogram
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CalculateFeatureSizesFilter:
.. py:class:: CalculateFeatureSizesFilter

   **UI Display Name:** *Find Feature Sizes*

   This **Filter** calculates the sizes of all **Features**.  The **Filter** simply iterates through all **Elements** querying for the **Feature** that owns them and keeping a tally for each **Feature**.  The tally is then stored as *NumElements* and the *Volume* and *EquivalentDiameter* are also calculated (and stored) by knowing the volume of each **Element**.

   `Link to the full online documentation for CalculateFeatureSizesFilter <http://www.dream3d.io/nx_reference_manual/Filters/CalculateFeatureSizesFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+---------------------------+
   | UI Display                     | Python Named Argument     |
   +================================+===========================+
   | Equivalent Diameters           | equivalent_diameters_path |
   +--------------------------------+---------------------------+
   | Feature Attribute Matrix       | feature_attribute_matrix  |
   +--------------------------------+---------------------------+
   | Cell Feature Ids               | feature_ids_path          |
   +--------------------------------+---------------------------+
   | Target Geometry                | geometry_path             |
   +--------------------------------+---------------------------+
   | Number of Elements             | num_elements_path         |
   +--------------------------------+---------------------------+
   | Generate Missing Element Sizes | save_element_sizes        |
   +--------------------------------+---------------------------+
   | Volumes                        | volumes_path              |
   +--------------------------------+---------------------------+

   .. py:method:: Execute(equivalent_diameters_path, feature_attribute_matrix, feature_ids_path, geometry_path, num_elements_path, save_element_sizes, volumes_path)

      :param complex.DataObjectNameParameter equivalent_diameters_path: DataPath to equivalent diameters array
      :param complex.AttributeMatrixSelectionParameter feature_attribute_matrix: Feature Attribute Matrix of the selected Feature Ids
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Element belongs
      :param complex.DataPathSelectionParameter geometry_path: DataPath to target geometry
      :param complex.DataObjectNameParameter num_elements_path: DataPath to Num Elements array
      :param complex.BoolParameter save_element_sizes: If checked this will generate and store the element sizes ONLY if the geometry does not already contain them.
      :param complex.DataObjectNameParameter volumes_path: DataPath to volumes array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CalculateTriangleAreasFilter:
.. py:class:: CalculateTriangleAreasFilter

   **UI Display Name:** *Calculate Triangle Areas*

   This **Filter** computes the area of each **Triangle** in a **Triangle Geometry** by calculating the following:

   `Link to the full online documentation for CalculateTriangleAreasFilter <http://www.dream3d.io/nx_reference_manual/Filters/CalculateTriangleAreasFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------+-----------------------------+
   | UI Display         | Python Named Argument       |
   +====================+=============================+
   | Created Face Areas | triangle_areas_array_path   |
   +--------------------+-----------------------------+
   | Triangle Geometry  | triangle_geometry_data_path |
   +--------------------+-----------------------------+

   .. py:method:: Execute(triangle_areas_array_path, triangle_geometry_data_path)

      :param complex.DataObjectNameParameter triangle_areas_array_path: The complete path to the array storing the calculated face areas
      :param complex.GeometrySelectionParameter triangle_geometry_data_path: The complete path to the Geometry for which to calculate the face areas
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ChangeAngleRepresentation:
.. py:class:: ChangeAngleRepresentation

   **UI Display Name:** *Convert Angles to Degrees or Radians*

   This **Filter** will multiply the values of every **Element** by a factor to convert *degrees to radians* or *radians to degrees*.  The user needs to know the units of their data in order to use this **Filter** properly.

   `Link to the full online documentation for ChangeAngleRepresentation <http://www.dream3d.io/nx_reference_manual/Filters/ChangeAngleRepresentation>`_ 

   Mapping of UI display to python named argument

   +-----------------+-----------------------+
   | UI Display      | Python Named Argument |
   +=================+=======================+
   | Angles          | angles_array_path     |
   +-----------------+-----------------------+
   | Conversion Type | conversion_type       |
   +-----------------+-----------------------+

   .. py:method:: Execute(angles_array_path, conversion_type)

      :param complex.ArraySelectionParameter angles_array_path: The DataArray containing the angles to be converted.
      :param complex.ChoicesParameter conversion_type: Tells the Filter which conversion is being made
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CombineAttributeArraysFilter:
.. py:class:: CombineAttributeArraysFilter

   **UI Display Name:** *Combine Attribute Arrays*

   This **Filter** will "stack" any number of user-chosen **Attribute Arrays** into a single attribute array and allows the option to remove the original **Attribute Arrays** once this operation is completed. The arrays must all share the same primitive type and number of tuples, but may have differing component dimensions. The resulting combined array will have a total number of components equal to the sum of the number of components for each stacked array. The order in which the components are placed in the combined array is the same as the ordering chosen by the user when selecting the arrays. For example, consider two arrays, one that is a 3-vector and one that is a scalar. The values in memory appear as follows:

   `Link to the full online documentation for CombineAttributeArraysFilter <http://www.dream3d.io/nx_reference_manual/Filters/CombineAttributeArraysFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------+---------------------------+
   | UI Display                  | Python Named Argument     |
   +=============================+===========================+
   | Move Data                   | move_values               |
   +-----------------------------+---------------------------+
   | Normalize Data              | normalize_data            |
   +-----------------------------+---------------------------+
   | Attribute Arrays to Combine | selected_data_array_paths |
   +-----------------------------+---------------------------+
   | Created Data Array          | stacked_data_array_name   |
   +-----------------------------+---------------------------+

   .. py:method:: Execute(move_values, normalize_data, selected_data_array_paths, stacked_data_array_name)

      :param complex.BoolParameter move_values: Whether to remove the original arrays after combining the data
      :param complex.BoolParameter normalize_data: Whether to normalize the combine data on the interval [0, 1]
      :param complex.MultiArraySelectionParameter selected_data_array_paths: The complete path to each of the Attribute Arrays to combine
      :param complex.DataObjectNameParameter stacked_data_array_name: This is the name of the created output array of the combined attribute arrays.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CombineStlFilesFilter:
.. py:class:: CombineStlFilesFilter

   **UI Display Name:** *Combine STL Files*

   This **Filter** combines all of the STL files from a given directory into a single triangle geometry. This filter will make use of the **Import STL File Filter** to read in each stl file in the given directory and then will proceed to combine each of the imported files into a single triangle geometry.

   `Link to the full online documentation for CombineStlFilesFilter <http://www.dream3d.io/nx_reference_manual/Filters/CombineStlFilesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+------------------------------+
   | UI Display              | Python Named Argument        |
   +=========================+==============================+
   | Face Attribute Matrix   | face_attribute_matrix_name   |
   +-------------------------+------------------------------+
   | Face Normals            | face_normals_array_name      |
   +-------------------------+------------------------------+
   | Path to STL Files       | stl_files_path               |
   +-------------------------+------------------------------+
   | Triangle Geometry       | triangle_data_container_name |
   +-------------------------+------------------------------+
   | Vertex Attribute Matrix | vertex_attribute_matrix_name |
   +-------------------------+------------------------------+

   .. py:method:: Execute(face_attribute_matrix_name, face_normals_array_name, stl_files_path, triangle_data_container_name, vertex_attribute_matrix_name)

      :param complex.DataObjectNameParameter face_attribute_matrix_name: The name of the face level attribute matrix to be created with the geometry
      :param complex.DataObjectNameParameter face_normals_array_name: The name of the data array in which to store the face normals for the created triangle geometry
      :param complex.FileSystemPathParameter stl_files_path: The path to the folder containing all the STL files to be combined
      :param complex.DataGroupCreationParameter triangle_data_container_name: The path to the triangle geometry to be created from the combined STL files
      :param complex.DataObjectNameParameter vertex_attribute_matrix_name: The name of the vertex level attribute matrix to be created with the geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ComputeFeatureRectFilter:
.. py:class:: ComputeFeatureRectFilter

   **UI Display Name:** *Compute Feature Corners*

   This **Filter** computes the XYZ minimum and maximum coordinates for each **Feature** in a segmentation. This data can be important for finding the smallest encompassing volume. This values are given in **Pixel** coordinates.

   `Link to the full online documentation for ComputeFeatureRectFilter <http://www.dream3d.io/nx_reference_manual/Filters/ComputeFeatureRectFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+------------------------------------+
   | UI Display                    | Python Named Argument              |
   +===============================+====================================+
   | Feature Data Attribute Matrix | feature_data_attribute_matrix_path |
   +-------------------------------+------------------------------------+
   | Feature Ids                   | feature_ids_array_path             |
   +-------------------------------+------------------------------------+
   | Feature Rect                  | feature_rect_array_path            |
   +-------------------------------+------------------------------------+

   .. py:method:: Execute(feature_data_attribute_matrix_path, feature_ids_array_path, feature_rect_array_path)

      :param complex.DataGroupSelectionParameter feature_data_attribute_matrix_path: The path to the feature data attribute matrix associated with the input feature ids array
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.DataObjectNameParameter feature_rect_array_path: The feature rect calculated from the feature ids
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ComputeMomentInvariants2DFilter:
.. py:class:: ComputeMomentInvariants2DFilter

   **UI Display Name:** *Compute Moment Invariants (2D)*

   This **Filter** computes the 2D Omega-1 and Omega 2 values from the *Central Moments* matrix and optionally will normalize the values to a unit circle and also optionally save the *Central Moments* matrix as a DataArray to the *Cell Feature Attribute Matrix*. Based off the paper by MacSleyne et. al [1], the algorithm will calculate the 2D central moments for each feature starting at *feature id = 1*. Because *feature id 0* is of special significance and typically is a matrix or background it is ignored in this filter. If any feature id has a Z Delta of > 1, the feature will be skipped. This algorithm works strictly in the XY plane and is meant to be applied to a 2D image. Using the research from the cited paper certain shapes can be detected using the Omega-1 and Omega-2 values. An example usage is finding elliptical shapes in an image:

   `Link to the full online documentation for ComputeMomentInvariants2DFilter <http://www.dream3d.io/nx_reference_manual/Filters/ComputeMomentInvariants2DFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Central Moments               | central_moments_array_path    |
   +-------------------------------+-------------------------------+
   | Cell Feature Attribute Matrix | feature_attribute_matrix_path |
   +-------------------------------+-------------------------------+
   | Feature Ids                   | feature_ids_array_path        |
   +-------------------------------+-------------------------------+
   | Feature Rect                  | feature_rect_array_path       |
   +-------------------------------+-------------------------------+
   | 2D Image Geometry             | image_geometry_path           |
   +-------------------------------+-------------------------------+
   | Normalize Moment Invariants   | normalize_moment_invariants   |
   +-------------------------------+-------------------------------+
   | Omega 1                       | omega1_array_path             |
   +-------------------------------+-------------------------------+
   | Omega 2                       | omega2_array_path             |
   +-------------------------------+-------------------------------+
   | Save Central Moments          | save_central_moments          |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(central_moments_array_path, feature_attribute_matrix_path, feature_ids_array_path, feature_rect_array_path, image_geometry_path, normalize_moment_invariants, omega1_array_path, omega2_array_path, save_central_moments)

      :param complex.DataObjectNameParameter central_moments_array_path: Central Moments value
      :param complex.AttributeMatrixSelectionParameter feature_attribute_matrix_path: The path to the cell feature attribute matrix where the created data arrays will be stored
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.ArraySelectionParameter feature_rect_array_path: Array holding the min xy and max xy pixel coordinates of each feature id
      :param complex.GeometrySelectionParameter image_geometry_path: The path to the 2D image geometry to be used as input
      :param complex.BoolParameter normalize_moment_invariants: Should the algorithm normalize the results to unit circle.
      :param complex.DataObjectNameParameter omega1_array_path: Omega1 value
      :param complex.DataObjectNameParameter omega2_array_path: Omega2 value
      :param complex.BoolParameter save_central_moments: Write the Central Moments to a new Data Array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ConditionalSetValue:
.. py:class:: ConditionalSetValue

   **UI Display Name:** *Replace Value in Array (Conditional)*

   This **Filter** replaces values in a user specified **Attribute Array** with a user specified value a second boolean **Attribute Array** specifies, but only when **Use Conditional Mask** is *true*. For example, if the user entered a *Replace Value* of *5.5*, then for every occurence of *true* in the conditional boolean array, the selected **Attribute Array** would be changed to 5.5. If **Use Conditional Mask** is *false*, then **Value to Replace** will be searched for in the provided **Attribute Array** and all instances will be replaced. Below are the ranges for the values that can be entered for the different primitive types of arrays (for user reference). The selected **Attribute Array** must be a scalar array.

   `Link to the full online documentation for ConditionalSetValue <http://www.dream3d.io/nx_reference_manual/Filters/ConditionalSetValue>`_ 

   Mapping of UI display to python named argument

   +----------------------+------------------------+
   | UI Display           | Python Named Argument  |
   +======================+========================+
   | Conditional Array    | conditional_array_path |
   +----------------------+------------------------+
   | Invert Mask          | invert_mask            |
   +----------------------+------------------------+
   | Value To Replace     | remove_value           |
   +----------------------+------------------------+
   | New Value            | replace_value          |
   +----------------------+------------------------+
   | Attribute Array      | selected_array_path    |
   +----------------------+------------------------+
   | Use Conditional Mask | use_conditional        |
   +----------------------+------------------------+

   .. py:method:: Execute(conditional_array_path, invert_mask, remove_value, replace_value, selected_array_path, use_conditional)

      :param complex.ArraySelectionParameter conditional_array_path: The complete path to the conditional array that will determine which values/entries will be replaced if index is TRUE
      :param complex.BoolParameter invert_mask: This makes the filter replace values that are marked FALSE in the conditional array
      :param complex.StringParameter remove_value: The numerical value that will be replaced in the array
      :param complex.StringParameter replace_value: The value that will be used as the replacement value
      :param complex.ArraySelectionParameter selected_array_path: The complete path to array that will have values replaced
      :param complex.BoolParameter use_conditional: Whether to use a boolean mask array to replace values marked TRUE
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ConvertColorToGrayScaleFilter:
.. py:class:: ConvertColorToGrayScaleFilter

   **UI Display Name:** *Color to GrayScale*

   This **Filter** allows the user to select a *flattening* method for turning an array of RGB or RGBa values into grayscale values.

   `Link to the full online documentation for ConvertColorToGrayScaleFilter <http://www.dream3d.io/nx_reference_manual/Filters/ConvertColorToGrayScaleFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------+-------------------------+
   | UI Display               | Python Named Argument   |
   +==========================+=========================+
   | Color Channel            | color_channel           |
   +--------------------------+-------------------------+
   | Color Weighting          | color_weights           |
   +--------------------------+-------------------------+
   | Conversion Algorithm     | conversion_algorithm    |
   +--------------------------+-------------------------+
   | Input Data Arrays        | input_data_array_vector |
   +--------------------------+-------------------------+
   | Output Data Array Prefix | output_array_prefix     |
   +--------------------------+-------------------------+

   .. py:method:: Execute(color_channel, color_weights, conversion_algorithm, input_data_array_vector, output_array_prefix)

      :param complex.Int32Parameter color_channel: The specific R|G|B channel to use as the GrayScale values
      :param complex.VectorFloat32Parameter color_weights: The weightings for each R|G|B component when using the luminosity conversion algorithm
      :param complex.ChoicesParameter conversion_algorithm: Which method to use when flattening the RGB array
      :param complex.MultiArraySelectionParameter input_data_array_vector: Select all DataArrays that need to be converted to GrayScale
      :param complex.StringParameter output_array_prefix: This prefix will be added to each array name that is selected for conversion to form the new array name
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ConvertDataFilter:
.. py:class:: ConvertDataFilter

   **UI Display Name:** *Convert AttributeArray DataType*

   This **Filter** converts attribute data from one primitive type to another by using the built in translation of the compiler. This **Filter** can be used if the user needs to convert an array into a type that is accepted by another **Filter**. For example, a **Filter** may need an input array to be of type _int32_t_ but the array that the user would like to use is _uint16_t_. The user may use this **Filter** to create a new array that has the proper target type (_int32_t_).

   `Link to the full online documentation for ConvertDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ConvertDataFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------+-----------------------+
   | UI Display            | Python Named Argument |
   +=======================+=======================+
   | Data Array to Convert | array_to_convert      |
   +-----------------------+-----------------------+
   | Converted Data Array  | converted_array       |
   +-----------------------+-----------------------+
   | Scalar Type           | scalar_type           |
   +-----------------------+-----------------------+

   .. py:method:: Execute(array_to_convert, converted_array, scalar_type)

      :param complex.ArraySelectionParameter array_to_convert: The complete path to the Data Array to Convert
      :param complex.DataObjectNameParameter converted_array: The name of the converted Data Array
      :param complex.ChoicesParameter scalar_type: Convert to this data type
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CopyDataObjectFilter:
.. py:class:: CopyDataObjectFilter

   **UI Display Name:** *Copy Data Object*

   This **Filter** deep copies one or more DataObjects.

   `Link to the full online documentation for CopyDataObjectFilter <http://www.dream3d.io/nx_reference_manual/Filters/CopyDataObjectFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-----------------------+
   | UI Display              | Python Named Argument |
   +=========================+=======================+
   | Objects to copy         | existing_data_path    |
   +-------------------------+-----------------------+
   | Copied Parent Group     | new_data_path         |
   +-------------------------+-----------------------+
   | Copied Object(s) Suffix | new_path_suffix       |
   +-------------------------+-----------------------+
   | Copy to New Parent      | use_new_parent        |
   +-------------------------+-----------------------+

   .. py:method:: Execute(existing_data_path, new_data_path, new_path_suffix, use_new_parent)

      :param complex.MultiPathSelectionParameter existing_data_path: A list of DataPaths to the DataObjects to be copied
      :param complex.DataGroupSelectionParameter new_data_path: DataPath to parent BaseGroup in which to store the copied DataObject(s)
      :param complex.StringParameter new_path_suffix: Suffix string to be appended to each copied DataObject
      :param complex.BoolParameter use_new_parent: Copy all the DataObjects to a new BaseGroup
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CopyFeatureArrayToElementArray:
.. py:class:: CopyFeatureArrayToElementArray

   **UI Display Name:** *Create Element Array from Feature Array*

   This **Filter** copies the values associated with a **Feature** to all the **Elements** that belong to that **Feature**.  Xmdf visualization files write only the **Element** attributes, so if the user wants to display a spatial map of a **Feature** level attribute, this **Filter** will transfer that information down to the **Element** level.

   `Link to the full online documentation for CopyFeatureArrayToElementArray <http://www.dream3d.io/nx_reference_manual/Filters/CopyFeatureArrayToElementArray>`_ 

   Mapping of UI display to python named argument

   +--------------------------------------+-----------------------------+
   | UI Display                           | Python Named Argument       |
   +======================================+=============================+
   | Created Array Suffix                 | created_array_suffix        |
   +--------------------------------------+-----------------------------+
   | Feature Ids                          | feature_ids_path            |
   +--------------------------------------+-----------------------------+
   | Feature Data to Copy to Element Data | selected_feature_array_path |
   +--------------------------------------+-----------------------------+

   .. py:method:: Execute(created_array_suffix, feature_ids_path, selected_feature_array_path)

      :param complex.StringParameter created_array_suffix: The suffix to add to the input attribute array name when creating the copied array
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Element belongs
      :param complex.MultiArraySelectionParameter selected_feature_array_path: The DataPath to the feature data that should be copied to the cell level
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateAttributeMatrixFilter:
.. py:class:: CreateAttributeMatrixFilter

   **UI Display Name:** *Create Attribute Matrix*

   This **Filter** creates a new **Attribute Matrix**.

   `Link to the full online documentation for CreateAttributeMatrixFilter <http://www.dream3d.io/nx_reference_manual/Filters/CreateAttributeMatrixFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------------------------------+-----------------------+
   | UI Display                                                  | Python Named Argument |
   +=============================================================+=======================+
   | DataObject Path                                             | Data_Object_Path      |
   +-------------------------------------------------------------+-----------------------+
   | Attribute Matrix Dimensions (Slowest to Fastest Dimensions) | tuple_dimensions      |
   +-------------------------------------------------------------+-----------------------+

   .. py:method:: Execute(Data_Object_Path, tuple_dimensions)

      :param complex.DataGroupCreationParameter Data_Object_Path: The complete path to the Attribute Matrix being created
      :param complex.DynamicTableParameter tuple_dimensions: Slowest to Fastest Dimensions
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateDataArray:
.. py:class:: CreateDataArray

   **UI Display Name:** *Create Data Array*

   This **Filter** creates an **Data Array** of any primitive type with any number of components along a *single component dimension*. For example, a scalar as (1) or a 3-vector as (3), but *not* a matrix as (3, 3). The array is initialized to a user define value or with random values within a specified range.

   `Link to the full online documentation for CreateDataArray <http://www.dream3d.io/nx_reference_manual/Filters/CreateDataArray>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------------------------------------------+-----------------------+
   | UI Display                                                                 | Python Named Argument |
   +============================================================================+=======================+
   | Set Tuple Dimensions [not required if creating inside an Attribute Matrix] | advanced_options      |
   +----------------------------------------------------------------------------+-----------------------+
   | Number of Components                                                       | component_count       |
   +----------------------------------------------------------------------------+-----------------------+
   | Data Format                                                                | data_format           |
   +----------------------------------------------------------------------------+-----------------------+
   | Initialization Value                                                       | initialization_value  |
   +----------------------------------------------------------------------------+-----------------------+
   | Numeric Type                                                               | numeric_type          |
   +----------------------------------------------------------------------------+-----------------------+
   | Created Array                                                              | output_data_array     |
   +----------------------------------------------------------------------------+-----------------------+
   | Data Array Dimensions (Slowest to Fastest Dimensions)                      | tuple_dimensions      |
   +----------------------------------------------------------------------------+-----------------------+

   .. py:method:: Execute(advanced_options, component_count, data_format, initialization_value, numeric_type, output_data_array, tuple_dimensions)

      :param complex.BoolParameter advanced_options: This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix
      :param complex.UInt64Parameter component_count: Number of components
      :param complex.DataStoreFormatParameter data_format: This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.
      :param complex.StringParameter initialization_value: This value will be used to fill the new array
      :param complex.NumericTypeParameter numeric_type: Numeric Type of data to create
      :param complex.ArrayCreationParameter output_data_array: Array storing the data
      :param complex.DynamicTableParameter tuple_dimensions: Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateDataGroup:
.. py:class:: CreateDataGroup

   **UI Display Name:** *Create Data Group*

   This **Filter** creates a new **DataGroup**.

   `Link to the full online documentation for CreateDataGroup <http://www.dream3d.io/nx_reference_manual/Filters/CreateDataGroup>`_ 

   Mapping of UI display to python named argument

   +-----------------+-----------------------+
   | UI Display      | Python Named Argument |
   +=================+=======================+
   | DataObject Path | Data_Object_Path      |
   +-----------------+-----------------------+

   .. py:method:: Execute(Data_Object_Path)

      :param complex.DataGroupCreationParameter Data_Object_Path: The complete path to the DataObject being created
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateFeatureArrayFromElementArray:
.. py:class:: CreateFeatureArrayFromElementArray

   **UI Display Name:** *Create Feature Array from Element Array*

   This **Filter** copies all the associated **Element** data of a selected **Element Array** to the **Feature** to which the **Elements** belong. The value stored for each **Feature** will be the value of the *last element copied*.

   `Link to the full online documentation for CreateFeatureArrayFromElementArray <http://www.dream3d.io/nx_reference_manual/Filters/CreateFeatureArrayFromElementArray>`_ 

   Mapping of UI display to python named argument

   +--------------------------------------+------------------------------------+
   | UI Display                           | Python Named Argument              |
   +======================================+====================================+
   | Cell Feature Attribute Matrix        | cell_feature_attribute_matrix_path |
   +--------------------------------------+------------------------------------+
   | Created Feature Attribute Array      | created_array_name                 |
   +--------------------------------------+------------------------------------+
   | Feature Ids                          | feature_ids_path                   |
   +--------------------------------------+------------------------------------+
   | Element Data to Copy to Feature Data | selected_cell_array_path           |
   +--------------------------------------+------------------------------------+

   .. py:method:: Execute(cell_feature_attribute_matrix_path, created_array_name, feature_ids_path, selected_cell_array_path)

      :param complex.DataGroupSelectionParameter cell_feature_attribute_matrix_path: The path to the cell feature attribute matrix where the converted output feature array will be stored
      :param complex.DataObjectNameParameter created_array_name: The path to the copied AttributeArray
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Element belongs
      :param complex.ArraySelectionParameter selected_cell_array_path: Element Data to Copy to Feature Data
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateGeometryFilter:
.. py:class:: CreateGeometryFilter

   **UI Display Name:** *Create Geometry*

   This **Filter** creates a **Geometry** object and the necessary **Element Attribute Matrices** on which to store **Attribute Arrays** and **Element Attribute Arrays** which define the geometry.  The type of **Attribute Matrices** and **Attribute Arrays** created depends on the kind of **Geometry** being created:

   `Link to the full online documentation for CreateGeometryFilter <http://www.dream3d.io/nx_reference_manual/Filters/CreateGeometryFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------+------------------------------+
   | UI Display                        | Python Named Argument        |
   +===================================+==============================+
   | Array Handling                    | array_handling               |
   +-----------------------------------+------------------------------+
   | Cell Attribute Matrix             | cell_attribute_matrix_name   |
   +-----------------------------------+------------------------------+
   | Dimensions                        | dimensions                   |
   +-----------------------------------+------------------------------+
   | Edge Attribute Matrix             | edge_attribute_matrix_name   |
   +-----------------------------------+------------------------------+
   | Edge List                         | edge_list_name               |
   +-----------------------------------+------------------------------+
   | Face Attribute Matrix             | face_attribute_matrix_name   |
   +-----------------------------------+------------------------------+
   | Geometry Name                     | geometry_name                |
   +-----------------------------------+------------------------------+
   | Geometry Type                     | geometry_type                |
   +-----------------------------------+------------------------------+
   | Hexahedral List                   | hexahedral_list_name         |
   +-----------------------------------+------------------------------+
   | Length Unit                       | length_unit_type             |
   +-----------------------------------+------------------------------+
   | Origin                            | origin                       |
   +-----------------------------------+------------------------------+
   | Quadrilateral List                | quadrilateral_list_name      |
   +-----------------------------------+------------------------------+
   | Spacing                           | spacing                      |
   +-----------------------------------+------------------------------+
   | Tetrahedral List                  | tetrahedral_list_name        |
   +-----------------------------------+------------------------------+
   | Triangle List                     | triangle_list_name           |
   +-----------------------------------+------------------------------+
   | Vertex Attribute Matrix           | vertex_attribute_matrix_name |
   +-----------------------------------+------------------------------+
   | Shared Vertex List                | vertex_list_name             |
   +-----------------------------------+------------------------------+
   | Treat Geometry Warnings as Errors | warnings_as_errors           |
   +-----------------------------------+------------------------------+
   | X Bounds                          | x_bounds                     |
   +-----------------------------------+------------------------------+
   | Y Bounds                          | y_bounds                     |
   +-----------------------------------+------------------------------+
   | Z Bounds                          | z_bounds                     |
   +-----------------------------------+------------------------------+

   .. py:method:: Execute(array_handling, cell_attribute_matrix_name, dimensions, edge_attribute_matrix_name, edge_list_name, face_attribute_matrix_name, geometry_name, geometry_type, hexahedral_list_name, length_unit_type, origin, quadrilateral_list_name, spacing, tetrahedral_list_name, triangle_list_name, vertex_attribute_matrix_name, vertex_list_name, warnings_as_errors, x_bounds, y_bounds, z_bounds)

      :param complex.ChoicesParameter array_handling: Determines if the arrays that make up the geometry primitives should be Moved or Copied to the created Geometry object.
      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name of the cell attribute matrix to be created with the geometry
      :param complex.VectorUInt64Parameter dimensions: The number of cells in each of the X, Y, Z directions
      :param complex.DataObjectNameParameter edge_attribute_matrix_name: The name of the edge attribute matrix to be created with the geometry
      :param complex.ArraySelectionParameter edge_list_name: The complete path to the data array defining the edges for the geometry
      :param complex.DataObjectNameParameter face_attribute_matrix_name: The name of the face attribute matrix to be created with the geometry
      :param complex.DataGroupCreationParameter geometry_name: The complete path to the geometry to be created
      :param complex.ChoicesParameter geometry_type: The type of Geometry to create
      :param complex.ArraySelectionParameter hexahedral_list_name: The complete path to the data array defining the hexahedral elements for the geometry
      :param complex.ChoicesParameter length_unit_type: The length unit to be used in the geometry
      :param complex.VectorFloat32Parameter origin: The origin of each of the axes in X, Y, Z order
      :param complex.ArraySelectionParameter quadrilateral_list_name: The complete path to the data array defining the (quad) faces for the geometry
      :param complex.VectorFloat32Parameter spacing: The length scale of each voxel/pixel
      :param complex.ArraySelectionParameter tetrahedral_list_name: The complete path to the data array defining the tetrahedral elements for the geometry
      :param complex.ArraySelectionParameter triangle_list_name: The complete path to the data array defining the (triangular) faces for the geometry
      :param complex.DataObjectNameParameter vertex_attribute_matrix_name: The name of the vertex attribute matrix to be created with the geometry
      :param complex.ArraySelectionParameter vertex_list_name: The complete path to the data array defining the point coordinates for the geometry
      :param complex.BoolParameter warnings_as_errors: Whether run time warnings for Geometries should be treated as errors
      :param complex.ArraySelectionParameter x_bounds: The spatial locations of the planes along the x direction
      :param complex.ArraySelectionParameter y_bounds: The spatial locations of the planes along the y direction
      :param complex.ArraySelectionParameter z_bounds: The spatial locations of the planes along the z direction
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateImageGeometry:
.. py:class:: CreateImageGeometry

   **UI Display Name:** *Create Geometry (Image)*

   This **Filter** creates an **Image Geometry** specifically for the representation of a 3D rectilinear grid of voxels (3D) or pixels(2D). Each axis can have its starting point (origin), resolution, and length defined for the **Geometry**. The **Data Container** in which to place the **Image Geometry** must be specified.

   `Link to the full online documentation for CreateImageGeometry <http://www.dream3d.io/nx_reference_manual/Filters/CreateImageGeometry>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-----------------------+
   | UI Display                 | Python Named Argument |
   +============================+=======================+
   | Cell Data Name             | cell_data_name        |
   +----------------------------+-----------------------+
   | Dimensions                 | dimensions            |
   +----------------------------+-----------------------+
   | Geometry Name [Data Group] | geometry_data_path    |
   +----------------------------+-----------------------+
   | Origin                     | origin                |
   +----------------------------+-----------------------+
   | Spacing                    | spacing               |
   +----------------------------+-----------------------+

   .. py:method:: Execute(cell_data_name, dimensions, geometry_data_path, origin, spacing)

      :param complex.DataObjectNameParameter cell_data_name: The name of the cell Attribute Matrix to be created
      :param complex.VectorUInt64Parameter dimensions: The number of cells in each of the X, Y, Z directions
      :param complex.DataGroupCreationParameter geometry_data_path: The complete path to the Geometry being created
      :param complex.VectorFloat32Parameter origin: The origin of each of the axes in X, Y, Z order
      :param complex.VectorFloat32Parameter spacing: The length scale of each voxel/pixel
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CropImageGeometry:
.. py:class:: CropImageGeometry

   **UI Display Name:** *Crop Geometry (Image)*

   This **Filter** allows the user to crop an **Image Geometry** of interest.  The input parameters are in units of **Cells**.  For example, if a **Image Geometry** was 100x100x100 **Cells** and each **Cell** was 0.25 x 0.25 x 0.25 units of resolution, then if the user wanted to crop the last 5 microns in the X direction, then the user would enter the following:

   `Link to the full online documentation for CropImageGeometry <http://www.dream3d.io/nx_reference_manual/Filters/CropImageGeometry>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Cell Feature Attribute Matrix | cell_feature_attribute_matrix |
   +-------------------------------+-------------------------------+
   | Created Image Geometry        | created_image_geometry        |
   +-------------------------------+-------------------------------+
   | Feature IDs                   | feature_ids                   |
   +-------------------------------+-------------------------------+
   | Max Voxel [Inclusive]         | max_voxel                     |
   +-------------------------------+-------------------------------+
   | Min Voxel                     | min_voxel                     |
   +-------------------------------+-------------------------------+
   | Perform In Place              | remove_original_geometry      |
   +-------------------------------+-------------------------------+
   | Renumber Features             | renumber_features             |
   +-------------------------------+-------------------------------+
   | Selected Image Geometry       | selected_image_geometry       |
   +-------------------------------+-------------------------------+
   | Update Origin                 | update_origin                 |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(cell_feature_attribute_matrix, created_image_geometry, feature_ids, max_voxel, min_voxel, remove_original_geometry, renumber_features, selected_image_geometry, update_origin)

      :param complex.AttributeMatrixSelectionParameter cell_feature_attribute_matrix: DataPath to the feature Attribute Matrix
      :param complex.DataGroupCreationParameter created_image_geometry: The location of the cropped geometry
      :param complex.ArraySelectionParameter feature_ids: DataPath to Cell Feature IDs array
      :param complex.VectorUInt64Parameter max_voxel: Upper bound of the volume to crop out
      :param complex.VectorUInt64Parameter min_voxel: Lower bound of the volume to crop out
      :param complex.BoolParameter remove_original_geometry: Removes the original Image Geometry after filter is completed
      :param complex.BoolParameter renumber_features: Specifies if the feature IDs should be renumbered
      :param complex.GeometrySelectionParameter selected_image_geometry: DataPath to the target ImageGeom
      :param complex.BoolParameter update_origin: Specifies if the origin should be updated
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CropVertexGeometry:
.. py:class:: CropVertexGeometry

   **UI Display Name:** *Crop Geometry (Vertex)*

   This **Filter** crops a **Vertex Geometry** by the given bounding box.  Unlike the cropping of an Image, it is unknown until run time how the **Geometry** will be changed by the cropping operation.  Therefore, this **Filter** requires that a new **Data Container** be created to contain the cropped **Vertex Geometry**.  This new **Data Container** will contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Data Container**.  Additionally, all **Vertex** data will be copied, with tuples *removed* for any **Vertices** outside the bounding box.  The user must supply a name for the cropped **Data Container**, but all other copied objects (**Attribute Matrices** and **Data Arrays**) will retain the same names as the original source.

   `Link to the full online documentation for CropVertexGeometry <http://www.dream3d.io/nx_reference_manual/Filters/CropVertexGeometry>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-----------------------+
   | UI Display                 | Python Named Argument |
   +============================+=======================+
   | Cropped Vertex Geometry    | cropped_geom          |
   +----------------------------+-----------------------+
   | Max Pos                    | max_pos               |
   +----------------------------+-----------------------+
   | Min Pos                    | min_pos               |
   +----------------------------+-----------------------+
   | Vertex Data Arrays to crop | target_arrays         |
   +----------------------------+-----------------------+
   | Vertex Data Name           | vertex_data_name      |
   +----------------------------+-----------------------+
   | Vertex Geometry to Crop    | vertex_geom           |
   +----------------------------+-----------------------+

   .. py:method:: Execute(cropped_geom, max_pos, min_pos, target_arrays, vertex_data_name, vertex_geom)

      :param complex.DataGroupCreationParameter cropped_geom: Created VertexGeom path
      :param complex.VectorFloat32Parameter max_pos: Maximum vertex position
      :param complex.VectorFloat32Parameter min_pos: Minimum vertex position
      :param complex.MultiArraySelectionParameter target_arrays: The complete path to all the vertex data arrays to crop
      :param complex.DataObjectNameParameter vertex_data_name: Name of the vertex data AttributeMatrix
      :param complex.GeometrySelectionParameter vertex_geom: DataPath to target VertexGeom
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _DeleteData:
.. py:class:: DeleteData

   **UI Display Name:** *Delete Data*

   This **Filter** allows the user to remove specified objects from the existing structure. This can be helpful if the user has operations that need as much memory as possible and there are extra objects that are not needed residing in memory. Alternatively, this **Filter** allows the user to remove objects that may share a name with another object further in the **Pipeline** that another **Filter** tries to create, since DREAM.3D generally does not allows objects at the same hierarchy to share the same name.

   `Link to the full online documentation for DeleteData <http://www.dream3d.io/nx_reference_manual/Filters/DeleteData>`_ 

   Mapping of UI display to python named argument

   +---------------------+-----------------------+
   | UI Display          | Python Named Argument |
   +=====================+=======================+
   | DataPaths to remove | removed_data_path     |
   +---------------------+-----------------------+

   .. py:method:: Execute(removed_data_path)

      :param complex.MultiPathSelectionParameter removed_data_path: The complete path to the DataObjects to be removed
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ErodeDilateBadDataFilter:
.. py:class:: ErodeDilateBadDataFilter

   **UI Display Name:** *Erode/Dilate Bad Data*

   Bad data refers to a **Cell** that has a *Feature Id* of *0*, which means the **Cell** has failed some sort of test andbeen marked as a *bad* **Cell**.

   `Link to the full online documentation for ErodeDilateBadDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ErodeDilateBadDataFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+--------------------------+
   | UI Display                 | Python Named Argument    |
   +============================+==========================+
   | Cell Feature Ids           | feature_ids_path         |
   +----------------------------+--------------------------+
   | Attribute Arrays to Ignore | ignored_data_array_paths |
   +----------------------------+--------------------------+
   | Number of Iterations       | num_iterations           |
   +----------------------------+--------------------------+
   | Operation                  | operation                |
   +----------------------------+--------------------------+
   | Selected Image Geometry    | selected_image_geometry  |
   +----------------------------+--------------------------+
   | X Direction                | x_dir_on                 |
   +----------------------------+--------------------------+
   | Y Direction                | y_dir_on                 |
   +----------------------------+--------------------------+
   | Z Direction                | z_dir_on                 |
   +----------------------------+--------------------------+

   .. py:method:: Execute(feature_ids_path, ignored_data_array_paths, num_iterations, operation, selected_image_geometry, x_dir_on, y_dir_on, z_dir_on)

      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Cell belongs
      :param complex.MultiArraySelectionParameter ignored_data_array_paths: The list of arrays to ignore when performing the algorithm
      :param complex.Int32Parameter num_iterations: The number of iterations to use for erosion/dilation
      :param complex.ChoicesParameter operation: Whether to dilate or erode
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.BoolParameter x_dir_on: Whether to erode/dilate in the X direction
      :param complex.BoolParameter y_dir_on: Whether to erode/dilate in the Y direction
      :param complex.BoolParameter z_dir_on: Whether to erode/dilate in the Z direction
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ErodeDilateCoordinationNumberFilter:
.. py:class:: ErodeDilateCoordinationNumberFilter

   **UI Display Name:** *Erode/Dilate Coordination Number*

   This **Filter** will smooth the interface between *good* and *bad* data. The user can specify a *coordination number*,which is the number of neighboring **Cells** of opposite type (i.e., *good* or *bad*) compared to a given **Cell** thatis acceptable. For example, a single *bad* **Cell** surrounded by *good* **Cells** would have a *coordination number* of*6*. The number entered by the user is actually the maximum tolerated *coordination number*. If the user entered a valueof *4*, then all *good* **Cells** with 5 or more *bad* neighbors and *bad* **Cells** with 5 or more *good* neighborswould be removed. After **Cells** with unacceptable *coordination number* are removed, then the neighboring **Cells**are *coarsened* to fill the removed **Cells**.

   `Link to the full online documentation for ErodeDilateCoordinationNumberFilter <http://www.dream3d.io/nx_reference_manual/Filters/ErodeDilateCoordinationNumberFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------------+--------------------------+
   | UI Display                      | Python Named Argument    |
   +=================================+==========================+
   | Coordination Number to Consider | coordination_number      |
   +---------------------------------+--------------------------+
   | Cell Feature Ids                | feature_ids_path         |
   +---------------------------------+--------------------------+
   | Attribute Arrays to Ignore      | ignored_data_array_paths |
   +---------------------------------+--------------------------+
   | Loop Until Gone                 | loop                     |
   +---------------------------------+--------------------------+
   | Selected Image Geometry         | selected_image_geometry  |
   +---------------------------------+--------------------------+

   .. py:method:: Execute(coordination_number, feature_ids_path, ignored_data_array_paths, loop, selected_image_geometry)

      :param complex.Int32Parameter coordination_number:  Number of neighboring **Cells** that can be of opposite classification before a **Cell** will be removed
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Cell belongs
      :param complex.MultiArraySelectionParameter ignored_data_array_paths: The list of arrays to ignore when performing the algorithm
      :param complex.BoolParameter loop: Keep looping until all criteria is met
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ErodeDilateMaskFilter:
.. py:class:: ErodeDilateMaskFilter

   **UI Display Name:** *Erode/Dilate Mask*

   If the mask is _dilated_, the **Filter** grows the *true* regions by one **Cell** in an iterative sequence for a userdefined number of iterations. During the *dilate* process, the classification of any **Cell** neighboring a *false* **Cell** will be changed to *true*. If the mask is _eroded_, the **Filter** shrinks the *true* regions by one **Cell** inan iterative sequence for a user defined number of iterations. During the *erode* process, the classification of the*false* **Cells** is changed to *true* if one of its neighbors is *true*. The **Filter** also offers the option(s) toturn on/off the erosion or dilation in specific directions (X, Y or Z).

   `Link to the full online documentation for ErodeDilateMaskFilter <http://www.dream3d.io/nx_reference_manual/Filters/ErodeDilateMaskFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-------------------------+
   | UI Display              | Python Named Argument   |
   +=========================+=========================+
   | Mask Array Path         | mask_array_path         |
   +-------------------------+-------------------------+
   | Number of Iterations    | num_iterations          |
   +-------------------------+-------------------------+
   | Operation               | operation               |
   +-------------------------+-------------------------+
   | Selected Image Geometry | selected_image_geometry |
   +-------------------------+-------------------------+
   | X Direction             | x_dir_on                |
   +-------------------------+-------------------------+
   | Y Direction             | y_dir_on                |
   +-------------------------+-------------------------+
   | Z Direction             | z_dir_on                |
   +-------------------------+-------------------------+

   .. py:method:: Execute(mask_array_path, num_iterations, operation, selected_image_geometry, x_dir_on, y_dir_on, z_dir_on)

      :param complex.ArraySelectionParameter mask_array_path: Boolean array where true voxels are used. False voxels are ignored.
      :param complex.Int32Parameter num_iterations: Number of erode/dilate iterations to perform
      :param complex.ChoicesParameter operation: Whether to dilate (0) or erode (1)
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.BoolParameter x_dir_on: Whether to erode/dilate in the X direction
      :param complex.BoolParameter y_dir_on: Whether to erode/dilate in the Y direction
      :param complex.BoolParameter z_dir_on: Whether to erode/dilate in the Z direction
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExecuteProcessFilter:
.. py:class:: ExecuteProcessFilter

   **UI Display Name:** *Execute Process*

   This filter allows the user to execute any application, program, shell script or any other executable program on the computer system. Any output can be found in the user specified log file.

   `Link to the full online documentation for ExecuteProcessFilter <http://www.dream3d.io/nx_reference_manual/Filters/ExecuteProcessFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------+-----------------------+
   | UI Display             | Python Named Argument |
   +========================+=======================+
   | Command Line Arguments | arguments             |
   +------------------------+-----------------------+
   | Should Block           | blocking              |
   +------------------------+-----------------------+
   | Output Log File        | output_log_file       |
   +------------------------+-----------------------+
   | Timeout (ms)           | timeout               |
   +------------------------+-----------------------+

   .. py:method:: Execute(arguments, blocking, output_log_file, timeout)

      :param complex.StringParameter arguments: The complete command to execute.
      :param complex.BoolParameter blocking: Whether to block the process while the command executes or not
      :param complex.FileSystemPathParameter output_log_file: The log file where the output from the process will be stored
      :param complex.Int32Parameter timeout: The amount of time to wait for the command to start/finish when blocking is selected
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExportDREAM3DFilter:
.. py:class:: ExportDREAM3DFilter

   **UI Display Name:** *Write DREAM3D NX File*

   This **Filter** dumps the data structure to an hdf5 file with the .dream3d extension.

   `Link to the full online documentation for ExportDREAM3DFilter <http://www.dream3d.io/nx_reference_manual/Filters/ExportDREAM3DFilter>`_ 

   Mapping of UI display to python named argument

   +------------------+-----------------------+
   | UI Display       | Python Named Argument |
   +==================+=======================+
   | Export File Path | export_file_path      |
   +------------------+-----------------------+
   | Write Xdmf File  | write_xdmf_file       |
   +------------------+-----------------------+

   .. py:method:: Execute(export_file_path, write_xdmf_file)

      :param complex.FileSystemPathParameter export_file_path: The file path the DataStructure should be written to as an HDF5 file.
      :param complex.BoolParameter write_xdmf_file: Whether or not to write the data out an xdmf file
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExtractComponentAsArrayFilter:
.. py:class:: ExtractComponentAsArrayFilter

   **UI Display Name:** *Extract/Remove Components*

   This **Filter** will do one of the following to one component of a multicomponent **Attribute Array**:- Remove 1 component from multicomponent **Attribute Array** completely [This is done implicitly so long as **Move Extracted Components To New Array** is false]- Extract 1 component from multicomponent **Attribute Array** and store it in a new **DataArray** without removing from original- Extract 1 component from multicomponent **Attribute Array** and store it in a new **DataArray** and remove that component from the original

   `Link to the full online documentation for ExtractComponentAsArrayFilter <http://www.dream3d.io/nx_reference_manual/Filters/ExtractComponentAsArrayFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------------------+------------------------------+
   | UI Display                                 | Python Named Argument        |
   +============================================+==============================+
   | Component Index to Extract                 | comp_number                  |
   +--------------------------------------------+------------------------------+
   | Move Extracted Components to New Array     | move_components_to_new_array |
   +--------------------------------------------+------------------------------+
   | Scalar Attribute Array                     | new_array_path               |
   +--------------------------------------------+------------------------------+
   | Remove Extracted Components from Old Array | remove_components_from_array |
   +--------------------------------------------+------------------------------+
   | Multicomponent Attribute Array             | selected_array_path          |
   +--------------------------------------------+------------------------------+

   .. py:method:: Execute(comp_number, move_components_to_new_array, new_array_path, remove_components_from_array, selected_array_path)

      :param complex.Int32Parameter comp_number: The index of the component in each tuple to be removed
      :param complex.BoolParameter move_components_to_new_array: If true the extracted components will be placed in a new array
      :param complex.DataObjectNameParameter new_array_path: The DataArray to store the extracted components
      :param complex.BoolParameter remove_components_from_array: If true the extracted components will be deleted
      :param complex.ArraySelectionParameter selected_array_path: The array to extract componenets from
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExtractInternalSurfacesFromTriangleGeometry:
.. py:class:: ExtractInternalSurfacesFromTriangleGeometry

   **UI Display Name:** *Extract Internal Surfaces From Triangle Geometry*

   This **Filter** extracts any **Triangles** from the supplied **Triangle Geometry** that contain any *internal nodes*, then uses these extracted **Triangles** to create a new **Data Container** with the reduced **Triangle Geometry**.  This operation is the same as removing all **Triangles** that only lie of the outer surface of the supplied **Triangle Geometry**.  The user must supply a **Vertex Attribute Array** that defines the type for each node of the **Triangle Geometry**.  Node types may take the following values:

   `Link to the full online documentation for ExtractInternalSurfacesFromTriangleGeometry <http://www.dream3d.io/nx_reference_manual/Filters/ExtractInternalSurfacesFromTriangleGeometry>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+---------------------------+
   | UI Display                     | Python Named Argument     |
   +================================+===========================+
   | Copy Face Arrays               | copy_triangle_array_paths |
   +--------------------------------+---------------------------+
   | Copy Vertex Arrays             | copy_vertex_array_paths   |
   +--------------------------------+---------------------------+
   | Face Data Attribute Matrix     | face_data_name            |
   +--------------------------------+---------------------------+
   | Created Triangle Geometry Path | internal_triangle_geom    |
   +--------------------------------+---------------------------+
   | Node Types Array               | node_types                |
   +--------------------------------+---------------------------+
   | Triangle Geometry              | triangle_geom             |
   +--------------------------------+---------------------------+
   | Vertex Data Attribute Matrix   | vertex_data_name          |
   +--------------------------------+---------------------------+

   .. py:method:: Execute(copy_triangle_array_paths, copy_vertex_array_paths, face_data_name, internal_triangle_geom, node_types, triangle_geom, vertex_data_name)

      :param complex.MultiArraySelectionParameter copy_triangle_array_paths: Paths to face-related DataArrays that should be copied to the new geometry
      :param complex.MultiArraySelectionParameter copy_vertex_array_paths: Paths to vertex-related DataArrays that should be copied to the new geometry
      :param complex.DataObjectNameParameter face_data_name: Created face data AttributeMatrix name
      :param complex.DataGroupCreationParameter internal_triangle_geom: Path to create the new Triangle Geometry
      :param complex.ArraySelectionParameter node_types: Path to the Node Types array
      :param complex.GeometrySelectionParameter triangle_geom: Path to the existing Triangle Geometry
      :param complex.DataObjectNameParameter vertex_data_name: Created vertex data AttributeMatrix name
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExtractVertexGeometryFilter:
.. py:class:: ExtractVertexGeometryFilter

   **UI Display Name:** *Extract Vertex Geometry*

   This filter will extract all the voxel centers of an Image Geometry or a RectilinearGrid geometryinto a new VertexGeometry. The user is given the option to copy or move cell arrays over to thenewly created VertexGeometry.

   `Link to the full online documentation for ExtractVertexGeometryFilter <http://www.dream3d.io/nx_reference_manual/Filters/ExtractVertexGeometryFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------+---------------------------+
   | UI Display                          | Python Named Argument     |
   +=====================================+===========================+
   | Array Handling                      | array_handling            |
   +-------------------------------------+---------------------------+
   | Included Attribute Arrays           | included_data_array_paths |
   +-------------------------------------+---------------------------+
   | Input Geometry                      | input_geometry_path       |
   +-------------------------------------+---------------------------+
   | Mask                                | mask_array_path           |
   +-------------------------------------+---------------------------+
   | Output Shared Vertex List Name      | shared_vertex_list_name   |
   +-------------------------------------+---------------------------+
   | Use Mask                            | use_mask                  |
   +-------------------------------------+---------------------------+
   | Output Vertex Attribute Matrix Name | vertex_attr_matrix_name   |
   +-------------------------------------+---------------------------+
   | Output Vertex Geometry              | vertex_geometry_path      |
   +-------------------------------------+---------------------------+

   .. py:method:: Execute(array_handling, included_data_array_paths, input_geometry_path, mask_array_path, shared_vertex_list_name, use_mask, vertex_attr_matrix_name, vertex_geometry_path)

      :param complex.ChoicesParameter array_handling: Move or Copy input data arrays
      :param complex.MultiArraySelectionParameter included_data_array_paths: The arrays to copy/move to the vertex array
      :param complex.DataGroupSelectionParameter input_geometry_path: The input Image/RectilinearGrid Geometry to convert
      :param complex.ArraySelectionParameter mask_array_path: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.DataObjectNameParameter shared_vertex_list_name: The name of the shared vertex list that will be created
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :param complex.DataObjectNameParameter vertex_attr_matrix_name: The name of the vertex attribute matrix that will be created
      :param complex.DataGroupCreationParameter vertex_geometry_path: The complete path to the vertex geometry that will be created
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FeatureDataCSVWriterFilter:
.. py:class:: FeatureDataCSVWriterFilter

   **UI Display Name:** *Export Feature Data as CSV File*

   This **Filter** writes the data associated with each **Feature** to a file name specified by the user in *CSV* format. Every array in the **Feature** map is written as a column of data in the *CSV* file.  The user can choose to also write the neighbor data. Neighbor data are data arrays that are associated with the neighbors of a **Feature**, such as: list of neighbors, list of misorientations, list of shared surface areas, etc. These blocks of info are written after the scalar data arrays.  Since the number of neighbors is variable for each **Feature**, the data is written as follows (for each **Feature**): Id, number of neighbors, value1, value2,...valueN.

   `Link to the full online documentation for FeatureDataCSVWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/FeatureDataCSVWriterFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+------------------------------------+
   | UI Display                    | Python Named Argument              |
   +===============================+====================================+
   | Feature Attribute Matrix      | cell_feature_attribute_matrix_path |
   +-------------------------------+------------------------------------+
   | Delimiter                     | delimiter_choice_int               |
   +-------------------------------+------------------------------------+
   | Output File                   | feature_data_file                  |
   +-------------------------------+------------------------------------+
   | Write Neighbor Data           | write_neighborlist_data            |
   +-------------------------------+------------------------------------+
   | Write Number of Features Line | write_num_features_line            |
   +-------------------------------+------------------------------------+

   .. py:method:: Execute(cell_feature_attribute_matrix_path, delimiter_choice_int, feature_data_file, write_neighborlist_data, write_num_features_line)

      :param complex.DataGroupSelectionParameter cell_feature_attribute_matrix_path: Input Feature Attribute Matrix
      :param complex.ChoicesParameter delimiter_choice_int: Default Delimiter is Comma
      :param complex.FileSystemPathParameter feature_data_file: Path to the output file to write.
      :param complex.BoolParameter write_neighborlist_data: Should the neighbor list data be written to the file
      :param complex.BoolParameter write_num_features_line: Should the number of features be written to the file.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FeatureFaceCurvatureFilter:
.. py:class:: FeatureFaceCurvatureFilter

   **UI Display Name:** *Fill Bad Data*

   This **Filter** calculates _principal direction vectors_ and the _principal curvatures_, and optionally the _mean_ and _Gaussian_ curvature, for each **Triangle** in a **Triangle Geometry** using the technique in [1]. The groups of **Triangles** over which to compute the curvatures is determines by the **Features** they are associated, denoted by their _Face Labels_. The curvature information will be stored in a **Face Attribute Matrix**.

   `Link to the full online documentation for FeatureFaceCurvatureFilter <http://www.dream3d.io/nx_reference_manual/Filters/FeatureFaceCurvatureFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------+-----------------------------+
   | UI Display                          | Python Named Argument       |
   +=====================================+=============================+
   | Compute Gaussian Curvature          | compute_gaussian_curvature  |
   +-------------------------------------+-----------------------------+
   | Compute Mean Curvature              | compute_mean_curvature      |
   +-------------------------------------+-----------------------------+
   | Compute Principal Direction Vectors | compute_principal_direction |
   +-------------------------------------+-----------------------------+
   | Compute Weingarten Matrix           | compute_weingarten_matrix   |
   +-------------------------------------+-----------------------------+
   | Face Attribute Matrix               | face_attribute_matrix       |
   +-------------------------------------+-----------------------------+
   | Face Centroids                      | face_centroids              |
   +-------------------------------------+-----------------------------+
   | Face Labels                         | face_labels                 |
   +-------------------------------------+-----------------------------+
   | Feature Normals                     | face_normals                |
   +-------------------------------------+-----------------------------+
   | Feature Face IDs                    | feature_face_ids            |
   +-------------------------------------+-----------------------------+
   | Guassian Curvature                  | gaussian_curvature          |
   +-------------------------------------+-----------------------------+
   | Mean Curvature                      | mean_curvature              |
   +-------------------------------------+-----------------------------+
   | Neighborhood Ring Count             | neighborhood_ring           |
   +-------------------------------------+-----------------------------+
   | Principal Curvature 1               | principal_curvature_1       |
   +-------------------------------------+-----------------------------+
   | Principal Curvature 2               | principal_curvature_2       |
   +-------------------------------------+-----------------------------+
   | Principal Direction 1               | principal_direction_1       |
   +-------------------------------------+-----------------------------+
   | Principal Direction 2               | principal_direction_2       |
   +-------------------------------------+-----------------------------+
   | Triangle Geometry                   | triangle_geom               |
   +-------------------------------------+-----------------------------+
   | Use Face Normals for Curve Fitting  | use_normals                 |
   +-------------------------------------+-----------------------------+
   | Weingarten Matrix                   | weingarten_matrix           |
   +-------------------------------------+-----------------------------+

   .. py:method:: Execute(compute_gaussian_curvature, compute_mean_curvature, compute_principal_direction, compute_weingarten_matrix, face_attribute_matrix, face_centroids, face_labels, face_normals, feature_face_ids, gaussian_curvature, mean_curvature, neighborhood_ring, principal_curvature_1, principal_curvature_2, principal_direction_1, principal_direction_2, triangle_geom, use_normals, weingarten_matrix)

      :param complex.BoolParameter compute_gaussian_curvature: 
      :param complex.BoolParameter compute_mean_curvature: 
      :param complex.BoolParameter compute_principal_direction: 
      :param complex.BoolParameter compute_weingarten_matrix: 
      :param complex.AttributeMatrixSelectionParameter face_attribute_matrix: 
      :param complex.ArraySelectionParameter face_centroids: 
      :param complex.ArraySelectionParameter face_labels: 
      :param complex.ArraySelectionParameter face_normals: 
      :param complex.ArraySelectionParameter feature_face_ids: 
      :param complex.ArrayCreationParameter gaussian_curvature: 
      :param complex.ArrayCreationParameter mean_curvature: 
      :param complex.Int32Parameter neighborhood_ring: 
      :param complex.ArrayCreationParameter principal_curvature_1: 
      :param complex.ArrayCreationParameter principal_curvature_2: 
      :param complex.ArrayCreationParameter principal_direction_1: 
      :param complex.ArrayCreationParameter principal_direction_2: 
      :param complex.GeometrySelectionParameter triangle_geom: 
      :param complex.BoolParameter use_normals: 
      :param complex.ArrayCreationParameter weingarten_matrix: 
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FillBadDataFilter:
.. py:class:: FillBadDataFilter

   **UI Display Name:** *Fill Bad Data*

   This **Filter** removes small *noise* in the data, but keeps larger regions that are possibly **Features**, e.g., pores or defects. This **Filter** collects the *bad* **Cells** (*Feature Id = 0*) and *erodes* them until none remain. However, contiguous groups of *bad* **Cells** that have at least as many **Cells** as the minimum allowed defect size entered by the user will not be _eroded_.

   `Link to the full online documentation for FillBadDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/FillBadDataFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------+--------------------------+
   | UI Display                  | Python Named Argument    |
   +=============================+==========================+
   | Cell Phases                 | cell_phases_array_path   |
   +-----------------------------+--------------------------+
   | Cell Feature Ids            | feature_ids_path         |
   +-----------------------------+--------------------------+
   | Attribute Arrays to Ignore  | ignored_data_array_paths |
   +-----------------------------+--------------------------+
   | Minimum Allowed Defect Size | min_allowed_defect_size  |
   +-----------------------------+--------------------------+
   | Selected Image Geometry     | selected_image_geometry  |
   +-----------------------------+--------------------------+
   | Store Defects as New Phase  | store_as_new_phase       |
   +-----------------------------+--------------------------+

   .. py:method:: Execute(cell_phases_array_path, feature_ids_path, ignored_data_array_paths, min_allowed_defect_size, selected_image_geometry, store_as_new_phase)

      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs.
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Element belongs
      :param complex.MultiArraySelectionParameter ignored_data_array_paths: The list of arrays to ignore when performing the algorithm
      :param complex.Int32Parameter min_allowed_defect_size: The size at which a group of bad Cells are left unfilled as a 'defect'
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.BoolParameter store_as_new_phase: Whether to change the phase of 'defect' larger than the minimum allowed size above
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindArrayStatisticsFilter:
.. py:class:: FindArrayStatisticsFilter

   **UI Display Name:** *Find Attribute Array Statistics*

   This **Filter** computes a variety of statistics for a given scalar array.  The currently available statistics are array length, minimum, maximum, (arithmetic) mean, median, mode, standard deviation, and summation; any combination of these statistics may be computed by this **Filter**.  Any scalar array, of any primitive type, may be used as input.  The type of the output arrays depends on the kind of statistic computed:

   `Link to the full online documentation for FindArrayStatisticsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindArrayStatisticsFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------------+-------------------------------+
   | UI Display                              | Python Named Argument         |
   +=========================================+===============================+
   | Compute Statistics Per Feature/Ensemble | compute_by_index              |
   +-----------------------------------------+-------------------------------+
   | Destination Attribute Matrix            | destination_attribute_matrix  |
   +-----------------------------------------+-------------------------------+
   | Feature-Has-Data Array Name             | feature_has_data_array_name   |
   +-----------------------------------------+-------------------------------+
   | Feature Ids                             | feature_ids_path              |
   +-----------------------------------------+-------------------------------+
   | Find Histogram                          | find_histogram                |
   +-----------------------------------------+-------------------------------+
   | Find Length                             | find_length                   |
   +-----------------------------------------+-------------------------------+
   | Find Maximum                            | find_max                      |
   +-----------------------------------------+-------------------------------+
   | Find Mean                               | find_mean                     |
   +-----------------------------------------+-------------------------------+
   | Find Median                             | find_median                   |
   +-----------------------------------------+-------------------------------+
   | Find Minimum                            | find_min                      |
   +-----------------------------------------+-------------------------------+
   | Find Mode                               | find_mode                     |
   +-----------------------------------------+-------------------------------+
   | Find Standard Deviation                 | find_std_deviation            |
   +-----------------------------------------+-------------------------------+
   | Find Summation                          | find_summation                |
   +-----------------------------------------+-------------------------------+
   | Find Number of Unique Values            | find_unique_values            |
   +-----------------------------------------+-------------------------------+
   | Histogram Array Name                    | histogram_array_name          |
   +-----------------------------------------+-------------------------------+
   | Length Array Name                       | length_array_name             |
   +-----------------------------------------+-------------------------------+
   | Mask                                    | mask_array_path               |
   +-----------------------------------------+-------------------------------+
   | Histogram Max Value                     | max_range                     |
   +-----------------------------------------+-------------------------------+
   | Maximum Array Name                      | maximum_array_name            |
   +-----------------------------------------+-------------------------------+
   | Mean Array Name                         | mean_array_name               |
   +-----------------------------------------+-------------------------------+
   | Median Array Name                       | median_array_name             |
   +-----------------------------------------+-------------------------------+
   | Histogram Min Value                     | min_range                     |
   +-----------------------------------------+-------------------------------+
   | Minimum Array Name                      | minimum_array_name            |
   +-----------------------------------------+-------------------------------+
   | Mode Array Name                         | mode_array_name               |
   +-----------------------------------------+-------------------------------+
   | Most Populated Bin Array Name           | most_populated_bin_array_name |
   +-----------------------------------------+-------------------------------+
   | Number of Bins                          | num_bins                      |
   +-----------------------------------------+-------------------------------+
   | Number of Unique Values Array Name      | number_unique_values          |
   +-----------------------------------------+-------------------------------+
   | Attribute Array to Compute Statistics   | selected_array_path           |
   +-----------------------------------------+-------------------------------+
   | Standardize Data                        | standardize_data              |
   +-----------------------------------------+-------------------------------+
   | Standardized Data Array Name            | standardized_array_name       |
   +-----------------------------------------+-------------------------------+
   | Standard Deviation Array Name           | std_deviation_array_name      |
   +-----------------------------------------+-------------------------------+
   | Summation Array Name                    | summation_array_name          |
   +-----------------------------------------+-------------------------------+
   | Use Full Range for Histogram            | use_full_range                |
   +-----------------------------------------+-------------------------------+
   | Use Mask                                | use_mask                      |
   +-----------------------------------------+-------------------------------+

   .. py:method:: Execute(compute_by_index, destination_attribute_matrix, feature_has_data_array_name, feature_ids_path, find_histogram, find_length, find_max, find_mean, find_median, find_min, find_mode, find_std_deviation, find_summation, find_unique_values, histogram_array_name, length_array_name, mask_array_path, max_range, maximum_array_name, mean_array_name, median_array_name, min_range, minimum_array_name, mode_array_name, most_populated_bin_array_name, num_bins, number_unique_values, selected_array_path, standardize_data, standardized_array_name, std_deviation_array_name, summation_array_name, use_full_range, use_mask)

      :param complex.BoolParameter compute_by_index: Whether the statistics should be computed on a Feature/Ensemble basis
      :param complex.DataGroupCreationParameter destination_attribute_matrix: Attribute Matrix in which to store the computed statistics
      :param complex.DataObjectNameParameter feature_has_data_array_name: The name of the boolean array that indicates whether or not each feature contains any data.  This array is especially useful to help determine whether or not the outputted statistics are actually valid or not for a given feature.
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Element belongs
      :param complex.BoolParameter find_histogram: Whether to compute the histogram of the input array
      :param complex.BoolParameter find_length: Whether to compute the length of the input array
      :param complex.BoolParameter find_max: Whether to compute the maximum of the input array
      :param complex.BoolParameter find_mean: Whether to compute the arithmetic mean of the input array
      :param complex.BoolParameter find_median: Whether to compute the median of the input array
      :param complex.BoolParameter find_min: Whether to compute the minimum of the input array
      :param complex.BoolParameter find_mode: Whether to compute the mode of the input array
      :param complex.BoolParameter find_std_deviation: Whether to compute the standard deviation of the input array
      :param complex.BoolParameter find_summation: Whether to compute the summation of the input array
      :param complex.BoolParameter find_unique_values: Whether to compute the number of unique values in the input array
      :param complex.DataObjectNameParameter histogram_array_name: The name of the histogram array
      :param complex.DataObjectNameParameter length_array_name: The name of the length array
      :param complex.ArraySelectionParameter mask_array_path: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.Float64Parameter max_range: Max cutoff value for histogram
      :param complex.DataObjectNameParameter maximum_array_name: The name of the maximum array
      :param complex.DataObjectNameParameter mean_array_name: The name of the mean array
      :param complex.DataObjectNameParameter median_array_name: The name of the median array
      :param complex.Float64Parameter min_range: Min cutoff value for histogram
      :param complex.DataObjectNameParameter minimum_array_name: The name of the minimum array
      :param complex.DataObjectNameParameter mode_array_name: The name of the mode array
      :param complex.DataObjectNameParameter most_populated_bin_array_name: The name of the Most Populated Bin array
      :param complex.Int32Parameter num_bins: Number of bins in histogram
      :param complex.DataObjectNameParameter number_unique_values: The name of the array which stores the calculated number of unique values
      :param complex.ArraySelectionParameter selected_array_path: Input Attribute Array for which to compute statistics
      :param complex.BoolParameter standardize_data: Should a standardized data array be generated
      :param complex.DataObjectNameParameter standardized_array_name: The name of the standardized data array
      :param complex.DataObjectNameParameter std_deviation_array_name: The name of the standard deviation array
      :param complex.DataObjectNameParameter summation_array_name: The name of the summation array
      :param complex.BoolParameter use_full_range: If true, ignore min and max and use min and max from array upon which histogram is computed
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindBiasedFeaturesFilter:
.. py:class:: FindBiasedFeaturesFilter

   **UI Display Name:** *Find Biased Features (Bounding Box)*

   This **Filter** determines which **Features** are *biased* by the outer surfaces of the sample. Larger **Features** are more likely to intersect the outer surfaces and thus it is not sufficient to only note which **Features** touch the outer surfaces of the sample. Denoting which **Features** are biased is important so that they may be excluded from any statistical analyses. The algorithm for determining whether a **Feature** is *biased* is as follows:

   `Link to the full online documentation for FindBiasedFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindBiasedFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------+-----------------------------+
   | UI Display           | Python Named Argument       |
   +======================+=============================+
   | Biased Features      | biased_features_array_name  |
   +----------------------+-----------------------------+
   | Apply Phase by Phase | calc_by_phase               |
   +----------------------+-----------------------------+
   | Centroids            | centroids_array_path        |
   +----------------------+-----------------------------+
   | Image Geometry       | image_geometry_path         |
   +----------------------+-----------------------------+
   | Phases               | phases_array_path           |
   +----------------------+-----------------------------+
   | Surface Features     | surface_features_array_path |
   +----------------------+-----------------------------+

   .. py:method:: Execute(biased_features_array_name, calc_by_phase, centroids_array_path, image_geometry_path, phases_array_path, surface_features_array_path)

      :param complex.DataObjectNameParameter biased_features_array_name: Flag of 1 if Feature is biased or of 0 if it is not
      :param complex.BoolParameter calc_by_phase: Whether to apply the biased Features algorithm per Ensemble
      :param complex.ArraySelectionParameter centroids_array_path: X, Y, Z coordinates of Feature center of mass
      :param complex.GeometrySelectionParameter image_geometry_path: The selected geometry in which to the (biased) features belong
      :param complex.ArraySelectionParameter phases_array_path: Specifies to which Ensemble each Feature belongs. Only required if Apply Phase by Phase is checked
      :param complex.ArraySelectionParameter surface_features_array_path: Flag of 1 if Feature touches an outer surface or of 0 if it does not
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindBoundaryCellsFilter:
.. py:class:: FindBoundaryCellsFilter

   **UI Display Name:** *Find Boundary Cells (Image)*

   This **Filter** determines, for each **Cell**, the number of neighboring **Cells** that are owned by a different **Feature**.  The algorithm for determining this is as follows:

   `Link to the full online documentation for FindBoundaryCellsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindBoundaryCellsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+---------------------------+
   | UI Display              | Python Named Argument     |
   +=========================+===========================+
   | Boundary Cells          | boundary_cells_array_name |
   +-------------------------+---------------------------+
   | Feature Ids             | feature_ids_array_path    |
   +-------------------------+---------------------------+
   | Ignore Feature 0        | ignore_feature_zero       |
   +-------------------------+---------------------------+
   | Image Geometry          | image_geometry_path       |
   +-------------------------+---------------------------+
   | Include Volume Boundary | include_volume_boundary   |
   +-------------------------+---------------------------+

   .. py:method:: Execute(boundary_cells_array_name, feature_ids_array_path, ignore_feature_zero, image_geometry_path, include_volume_boundary)

      :param complex.DataObjectNameParameter boundary_cells_array_name: The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.BoolParameter ignore_feature_zero: Do not use feature 0
      :param complex.GeometrySelectionParameter image_geometry_path: The selected geometry to which the cells belong
      :param complex.BoolParameter include_volume_boundary: Include the Cell boundaries
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindBoundaryElementFractionsFilter:
.. py:class:: FindBoundaryElementFractionsFilter

   **UI Display Name:** *Find Feature Boundary Element Fractions*

   This **Filter** calculates the fraction of **Elements** of each **Feature** that are on the "surface" of that **Feature**.  The **Filter** simply iterates through all **Elements** asking for the **Feature** that owns them and if the **Element** is a "surface" **Element**.  Each **Feature** counts the total number of **Elements** it owns and the number of those **Elements** that are "surface" **Elements**.  The fraction is then stored for each **Feature**.

   `Link to the full online documentation for FindBoundaryElementFractionsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindBoundaryElementFractionsFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+------------------------------------+
   | UI Display                | Python Named Argument              |
   +===========================+====================================+
   | Surface Element Fractions | boundary_cell_fractions_array_name |
   +---------------------------+------------------------------------+
   | Surface Elements          | boundary_cells_array_path          |
   +---------------------------+------------------------------------+
   | Feature Data              | feature_data_attribute_matrix_path |
   +---------------------------+------------------------------------+
   | Feature Ids               | feature_ids_array_path             |
   +---------------------------+------------------------------------+

   .. py:method:: Execute(boundary_cell_fractions_array_name, boundary_cells_array_path, feature_data_attribute_matrix_path, feature_ids_array_path)

      :param complex.DataObjectNameParameter boundary_cell_fractions_array_name: Name of created Data Array containing fraction of Elements belonging to the Feature that are "surface" Elements
      :param complex.ArraySelectionParameter boundary_cells_array_path: DataArray containing the number of neighboring Elements of a given Element that belong to a different Feature than itself
      :param complex.AttributeMatrixSelectionParameter feature_data_attribute_matrix_path: Parent Attribute Matrix for the Surface Element Fractions Array to be created in
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindDifferencesMap:
.. py:class:: FindDifferencesMap

   **UI Display Name:** *Find Differences Map*

   This **Filter** calculates the difference between two **Attribute Arrays**. The arrays must have the same *primitive type* (e.g., float), *component dimensions* and *number of tuples*. The **Filter** uses the following pseudocode to calculate the difference map:

   `Link to the full online documentation for FindDifferencesMap <http://www.dream3d.io/nx_reference_manual/Filters/FindDifferencesMap>`_ 

   Mapping of UI display to python named argument

   +--------------------+---------------------------+
   | UI Display         | Python Named Argument     |
   +====================+===========================+
   | Difference Map     | difference_map_array_path |
   +--------------------+---------------------------+
   | First Input Array  | first_input_array_path    |
   +--------------------+---------------------------+
   | Second Input Array | second_input_array_path   |
   +--------------------+---------------------------+

   .. py:method:: Execute(difference_map_array_path, first_input_array_path, second_input_array_path)

      :param complex.ArrayCreationParameter difference_map_array_path: DataPath for created Difference Map DataArray
      :param complex.DataPathSelectionParameter first_input_array_path: DataPath to the first input DataArray
      :param complex.DataPathSelectionParameter second_input_array_path: DataPath to the second input DataArray
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindEuclideanDistMapFilter:
.. py:class:: FindEuclideanDistMapFilter

   **UI Display Name:** *Find Euclidean Distance Map*

   This **Filter** calculates the distance of each **Cell** from the nearest **Feature** boundary, **triple line** and/or **quadruple point**.  The following algorithm explains the process:

   `Link to the full online documentation for FindEuclideanDistMapFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindEuclideanDistMapFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------------+------------------------------+
   | UI Display                                   | Python Named Argument        |
   +==============================================+==============================+
   | Output arrays are Manhattan distance (int32) | calc_manhattan_dist          |
   +----------------------------------------------+------------------------------+
   | Calculate Distance to Boundaries             | do_boundaries                |
   +----------------------------------------------+------------------------------+
   | Calculate Distance to Quadruple Points       | do_quad_points               |
   +----------------------------------------------+------------------------------+
   | Calculate Distance to Triple Lines           | do_triple_lines              |
   +----------------------------------------------+------------------------------+
   | Cell Feature Ids                             | feature_ids_path             |
   +----------------------------------------------+------------------------------+
   | Boundary Distances                           | g_bdistances_array_name      |
   +----------------------------------------------+------------------------------+
   | Nearest Boundary Cells                       | nearest_neighbors_array_name |
   +----------------------------------------------+------------------------------+
   | Quadruple Point Distances                    | q_pdistances_array_name      |
   +----------------------------------------------+------------------------------+
   | Store the Nearest Boundary Cells             | save_nearest_neighbors       |
   +----------------------------------------------+------------------------------+
   | Selected Image Geometry                      | selected_image_geometry      |
   +----------------------------------------------+------------------------------+
   | Triple Line Distances                        | t_jdistances_array_name      |
   +----------------------------------------------+------------------------------+

   .. py:method:: Execute(calc_manhattan_dist, do_boundaries, do_quad_points, do_triple_lines, feature_ids_path, g_bdistances_array_name, nearest_neighbors_array_name, q_pdistances_array_name, save_nearest_neighbors, selected_image_geometry, t_jdistances_array_name)

      :param complex.BoolParameter calc_manhattan_dist: If Manhattan distance is used then results are stored as int32 otherwise results are stored as float32
      :param complex.BoolParameter do_boundaries: Whether the distance of each Cell to a Feature boundary is calculated
      :param complex.BoolParameter do_quad_points: Whether the distance of each Cell to a quadruple point between Features is calculated
      :param complex.BoolParameter do_triple_lines: Whether the distance of each Cell to a triple line between Features is calculated
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.DataObjectNameParameter g_bdistances_array_name: The name of the array with the distance the cells are from the boundary of the Feature they belong to.
      :param complex.DataObjectNameParameter nearest_neighbors_array_name: The name of the array with the indices of the closest cell that touches a boundary, triple and quadruple point for each cell.
      :param complex.DataObjectNameParameter q_pdistances_array_name: The name of the array with the distance the cells are from a quadruple point of Features.
      :param complex.BoolParameter save_nearest_neighbors: Whether to store the nearest neighbors of Cell
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.DataObjectNameParameter t_jdistances_array_name: The name of the array with the distance the cells are from a triple junction of Features.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeatureCentroidsFilter:
.. py:class:: FindFeatureCentroidsFilter

   **UI Display Name:** *Find Feature Centroids*

   This **Filter** calculates the *centroid* of each **Feature** by determining the average X, Y, and Z position of all the **Cells** belonging to the **Feature**. Note that **Features** that intersect the outer surfaces of the sample will still have *centroids* calculated, but they will be *centroids* of the truncated part of the **Feature** that lies inside the sample.

   `Link to the full online documentation for FindFeatureCentroidsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeatureCentroidsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+--------------------------+
   | UI Display                    | Python Named Argument    |
   +===============================+==========================+
   | Centroids                     | centroids_array_path     |
   +-------------------------------+--------------------------+
   | Cell Feature Attribute Matrix | feature_attribute_matrix |
   +-------------------------------+--------------------------+
   | Cell Feature Ids              | feature_ids_path         |
   +-------------------------------+--------------------------+
   | Selected Image Geometry       | selected_image_geometry  |
   +-------------------------------+--------------------------+

   .. py:method:: Execute(centroids_array_path, feature_attribute_matrix, feature_ids_path, selected_image_geometry)

      :param complex.DataObjectNameParameter centroids_array_path: DataPath to create the 'Centroids' output array
      :param complex.AttributeMatrixSelectionParameter feature_attribute_matrix: The cell feature attribute matrix
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry whose Features' centroids will be calculated
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeatureClusteringFilter:
.. py:class:: FindFeatureClusteringFilter

   **UI Display Name:** *Find Feature Clustering*

   This Filter determines the radial distribution function (RDF), as a histogram, of a given set of **Features**. Currently, the **Features** need to be of the same **Ensemble** (specified by the user), and the resulting RDF is stored as **Ensemble** data. This Filter also returns the clustering list (the list of all the inter-**Feature** distances) and the minimum and maximum separation distances. The algorithm proceeds as follows:

   `Link to the full online documentation for FindFeatureClusteringFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeatureClusteringFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------+-------------------------------------+
   | UI Display                       | Python Named Argument               |
   +==================================+=====================================+
   | Biased Features                  | biased_features_array_path          |
   +----------------------------------+-------------------------------------+
   | Cell Ensemble Attribute Matrix   | cell_ensemble_attribute_matrix_name |
   +----------------------------------+-------------------------------------+
   | Centroids                        | centroids_array_path                |
   +----------------------------------+-------------------------------------+
   | Clustering List                  | clustering_list_array_name          |
   +----------------------------------+-------------------------------------+
   | Equivalent Diameters             | equivalent_diameters_array_path     |
   +----------------------------------+-------------------------------------+
   | Phases                           | feature_phases_array_path           |
   +----------------------------------+-------------------------------------+
   | Max and Min Separation Distances | max_min_array_name                  |
   +----------------------------------+-------------------------------------+
   | Number of Bins for RDF           | number_of_bins                      |
   +----------------------------------+-------------------------------------+
   | Phase Index                      | phase_number                        |
   +----------------------------------+-------------------------------------+
   | Radial Distribution Function     | rdf_array_name                      |
   +----------------------------------+-------------------------------------+
   | Remove Biased Features           | remove_biased_features              |
   +----------------------------------+-------------------------------------+
   | Seed Value                       | seed_value                          |
   +----------------------------------+-------------------------------------+
   | Selected Image Geometry          | selected_image_geometry             |
   +----------------------------------+-------------------------------------+
   | Set Random Seed                  | set_random_seed                     |
   +----------------------------------+-------------------------------------+

   .. py:method:: Execute(biased_features_array_path, cell_ensemble_attribute_matrix_name, centroids_array_path, clustering_list_array_name, equivalent_diameters_array_path, feature_phases_array_path, max_min_array_name, number_of_bins, phase_number, rdf_array_name, remove_biased_features, seed_value, selected_image_geometry, set_random_seed)

      :param complex.ArraySelectionParameter biased_features_array_path: Specifies which features are biased and therefor should be removed if the Remove Biased Features option is on
      :param complex.AttributeMatrixSelectionParameter cell_ensemble_attribute_matrix_name: The path to the cell ensemble attribute matrix where the RDF and RDF min and max distance arrays will be stored
      :param complex.ArraySelectionParameter centroids_array_path: X, Y, Z coordinates of Feature center of mass
      :param complex.DataObjectNameParameter clustering_list_array_name: Distance of each Features's centroid to ever other Features's centroid
      :param complex.ArraySelectionParameter equivalent_diameters_array_path: Diameter of a sphere with the same volume as the Feature
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which Ensemble each Feature belongs
      :param complex.DataObjectNameParameter max_min_array_name: The max and min distance found between Features
      :param complex.Int32Parameter number_of_bins: Number of bins to split the RDF
      :param complex.Int32Parameter phase_number: Ensemble number for which to calculate the RDF and clustering list
      :param complex.DataObjectNameParameter rdf_array_name: A histogram of the normalized frequency at each bin
      :param complex.BoolParameter remove_biased_features: Remove the biased features
      :param complex.UInt64Parameter seed_value: The seed value used to randomly generate the points in the RDF
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.BoolParameter set_random_seed: When checked, allows the user to set the seed value used to randomly generate the points in the RDF
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeaturePhasesBinaryFilter:
.. py:class:: FindFeaturePhasesBinaryFilter

   **UI Display Name:** *Find Feature Phases Binary*

   This **Filter** assigns an **Ensemble** Id number to binary data. The *true* **Cells** will be **Ensemble** 1, and *false* **Cells** will be **Ensemble** 0. This **Filter** is generally useful when the **Cell Ensembles** were not known ahead of time. For example, if an image is segmented into precipitates and non-precipitates, this **Filter** will assign the precipitates to **Ensemble** 1, and the non-precipitates to **Ensemble** 0.

   `Link to the full online documentation for FindFeaturePhasesBinaryFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeaturePhasesBinaryFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------+---------------------------------+
   | UI Display                       | Python Named Argument           |
   +==================================+=================================+
   | Cell Data Attribute Matrix       | cell_data_attribute_matrix_path |
   +----------------------------------+---------------------------------+
   | Feature Ids                      | feature_ids_array_path          |
   +----------------------------------+---------------------------------+
   | Binary Feature Phases Array Name | feature_phases_array_name       |
   +----------------------------------+---------------------------------+
   | Mask                             | good_voxels_array_path          |
   +----------------------------------+---------------------------------+

   .. py:method:: Execute(cell_data_attribute_matrix_path, feature_ids_array_path, feature_phases_array_name, good_voxels_array_path)

      :param complex.AttributeMatrixSelectionParameter cell_data_attribute_matrix_path: The Cell Data Attribute Matrix within the Image Geometry where the Binary Phases Array will be created
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.DataObjectNameParameter feature_phases_array_name: Created Data Array name to specify to which Ensemble each Feature belongs
      :param complex.ArraySelectionParameter good_voxels_array_path: Data Array that specifies if the Cell is to be counted in the algorithm
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeaturePhasesFilter:
.. py:class:: FindFeaturePhasesFilter

   **UI Display Name:** *Find Feature Phases*

   This **Filter** determines the **Ensemble** of each **Feature** by querying the **Ensemble** of the **Elements** that belong to the **Feature**. Note that it is assumed that all **Elements** belonging to a **Feature** are of the same **Feature**, and thus any **Element** can be used to determine the **Ensemble** of the **Feature** that owns that **Element**.

   `Link to the full online documentation for FindFeaturePhasesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeaturePhasesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------------+
   | UI Display                    | Python Named Argument               |
   +===============================+=====================================+
   | Cell Feature Attribute Matrix | cell_features_attribute_matrix_path |
   +-------------------------------+-------------------------------------+
   | Phases                        | cell_phases_array_path              |
   +-------------------------------+-------------------------------------+
   | Feature Ids                   | feature_ids_path                    |
   +-------------------------------+-------------------------------------+
   | Feature Phases                | feature_phases_array_path           |
   +-------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_features_attribute_matrix_path, cell_phases_array_path, feature_ids_path, feature_phases_array_path)

      :param complex.AttributeMatrixSelectionParameter cell_features_attribute_matrix_path: The AttributeMatrix that stores the feature data for the input **Feature Ids**.
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Element belongs
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Element belongs
      :param complex.DataObjectNameParameter feature_phases_array_path: The name of the feature attribute matrix in which to store the found feature phases array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindLargestCrossSectionsFilter:
.. py:class:: FindLargestCrossSectionsFilter

   **UI Display Name:** *Find Feature Largest Cross-Section Areas*

   This **Filter** calculates the largest cross-sectional area on a user-defined plane for all **Features**.  The **Filter** simply iterates through all **Cells** (on each section) asking for **Feature** that owns them.  On each section, the count of **Cells** for each **Feature** is then converted to an area and stored as the *LargestCrossSection* if the area for the current section is larger than the existing *LargestCrossSection* for that **Feature**.

   `Link to the full online documentation for FindLargestCrossSectionsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindLargestCrossSectionsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+------------------------------------+
   | UI Display                    | Python Named Argument              |
   +===============================+====================================+
   | Cell Feature Attribute Matrix | cell_feature_attribute_matrix_path |
   +-------------------------------+------------------------------------+
   | Feature Ids                   | feature_ids_array_path             |
   +-------------------------------+------------------------------------+
   | Image Geometry                | image_geometry_path                |
   +-------------------------------+------------------------------------+
   | Largest Cross Sections        | largest_cross_sections_array_path  |
   +-------------------------------+------------------------------------+
   | Plane of Interest             | plane                              |
   +-------------------------------+------------------------------------+

   .. py:method:: Execute(cell_feature_attribute_matrix_path, feature_ids_array_path, image_geometry_path, largest_cross_sections_array_path, plane)

      :param complex.AttributeMatrixSelectionParameter cell_feature_attribute_matrix_path: The path to the cell feature attribute matrix
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.GeometrySelectionParameter image_geometry_path: The complete path to the input image geometry
      :param complex.DataObjectNameParameter largest_cross_sections_array_path: Area of largest cross-section for Feature perpendicular to the user specified direction
      :param complex.ChoicesParameter plane: Specifies which plane to consider when determining the maximum cross-section for each Feature
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindNeighborListStatistics:
.. py:class:: FindNeighborListStatistics

   **UI Display Name:** *Find Neighbor List Statistics*

   This **Filter** finds the selected statistics for each list contained in a NeighborList container. Each of those statistics are reported back as new Attribute Arrays. The user selectable statistics are:

   `Link to the full online documentation for FindNeighborListStatistics <http://www.dream3d.io/nx_reference_manual/Filters/FindNeighborListStatistics>`_ 

   Mapping of UI display to python named argument

   +------------------------------------+-------------------------+
   | UI Display                         | Python Named Argument   |
   +====================================+=========================+
   | Find Length                        | find_length             |
   +------------------------------------+-------------------------+
   | Find Maximum                       | find_maximum            |
   +------------------------------------+-------------------------+
   | Find Mean                          | find_mean               |
   +------------------------------------+-------------------------+
   | Find Median                        | find_median             |
   +------------------------------------+-------------------------+
   | Find Minimum                       | find_minimum            |
   +------------------------------------+-------------------------+
   | Find Standard Deviation            | find_standard_deviation |
   +------------------------------------+-------------------------+
   | Find Summation                     | find_summation          |
   +------------------------------------+-------------------------+
   | NeighborList to Compute Statistics | input_array             |
   +------------------------------------+-------------------------+
   | Length                             | length                  |
   +------------------------------------+-------------------------+
   | Maximum                            | maximum                 |
   +------------------------------------+-------------------------+
   | Mean                               | mean                    |
   +------------------------------------+-------------------------+
   | Median                             | median                  |
   +------------------------------------+-------------------------+
   | Minimum                            | minimum                 |
   +------------------------------------+-------------------------+
   | Standard Deviation                 | standard_deviation      |
   +------------------------------------+-------------------------+
   | Summation                          | summation               |
   +------------------------------------+-------------------------+

   .. py:method:: Execute(find_length, find_maximum, find_mean, find_median, find_minimum, find_standard_deviation, find_summation, input_array, length, maximum, mean, median, minimum, standard_deviation, summation)

      :param complex.BoolParameter find_length: Specifies whether or not the filter creates the Length array during calculations
      :param complex.BoolParameter find_maximum: Specifies whether or not the filter creates the Maximum array during calculations
      :param complex.BoolParameter find_mean: Specifies whether or not the filter creates the Mean array during calculations
      :param complex.BoolParameter find_median: Specifies whether or not the filter creates the Median array during calculations
      :param complex.BoolParameter find_minimum: Specifies whether or not the filter creates the Minimum array during calculations
      :param complex.BoolParameter find_standard_deviation: Specifies whether or not the filter creates the Standard Deviation array during calculations
      :param complex.BoolParameter find_summation: Specifies whether or not the filter creates the Summation array during calculations
      :param complex.NeighborListSelectionParameter input_array: Input Data Array to compute statistics
      :param complex.DataObjectNameParameter length: Path to create the Length array during calculations
      :param complex.DataObjectNameParameter maximum: Path to create the Maximum array during calculations
      :param complex.DataObjectNameParameter mean: Path to create the Mean array during calculations
      :param complex.DataObjectNameParameter median: Path to create the Median array during calculations
      :param complex.DataObjectNameParameter minimum: Path to create the Minimum array during calculations
      :param complex.DataObjectNameParameter standard_deviation: Path to create the Standard Deviation array during calculations
      :param complex.DataObjectNameParameter summation: Path to create the Summation array during calculations
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindNeighborhoodsFilter:
.. py:class:: FindNeighborhoodsFilter

   **UI Display Name:** *Find Feature Neighborhoods*

   This **Filter** determines the number of **Features**, for each **Feature**, whose *centroids* lie within a distance equal to a user defined multiple of the average *Equivalent Sphere Diameter* (*average of all **Features**).  The algorithm for determining the number of **Features** is given below:

   `Link to the full online documentation for FindNeighborhoodsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindNeighborhoodsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+---------------------------------+
   | UI Display                    | Python Named Argument           |
   +===============================+=================================+
   | Centroids                     | centroids_array_path            |
   +-------------------------------+---------------------------------+
   | Equivalent Diameters          | equivalent_diameters_array_path |
   +-------------------------------+---------------------------------+
   | Phases                        | feature_phases_array_path       |
   +-------------------------------+---------------------------------+
   | Multiples of Average Diameter | multiples_of_average            |
   +-------------------------------+---------------------------------+
   | NeighborhoodList              | neighborhood_list_array_name    |
   +-------------------------------+---------------------------------+
   | Neighborhoods                 | neighborhoods_array_name        |
   +-------------------------------+---------------------------------+
   | Selected Image Geometry       | selected_image_geometry_path    |
   +-------------------------------+---------------------------------+

   .. py:method:: Execute(centroids_array_path, equivalent_diameters_array_path, feature_phases_array_path, multiples_of_average, neighborhood_list_array_name, neighborhoods_array_name, selected_image_geometry_path)

      :param complex.ArraySelectionParameter centroids_array_path: Path to the array specifying the X, Y, Z coordinates of Feature center of mass
      :param complex.ArraySelectionParameter equivalent_diameters_array_path: Path to the array specifying the diameter of a sphere with the same volume as the Feature
      :param complex.ArraySelectionParameter feature_phases_array_path: Path to the array specifying to which Ensemble each Feature belongs
      :param complex.Float32Parameter multiples_of_average: Defines the search radius to use when looking for 'neighboring' Features
      :param complex.DataObjectNameParameter neighborhood_list_array_name: List of the Features whose centroids are within the user specified multiple of equivalent sphere diameter from each Feature
      :param complex.DataObjectNameParameter neighborhoods_array_name: Number of Features that have their centroid within the user specified multiple of equivalent sphere diameters from each Feature
      :param complex.GeometrySelectionParameter selected_image_geometry_path: The target geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindNeighbors:
.. py:class:: FindNeighbors

   **UI Display Name:** *Find Feature Neighbors*

   This **Filter** determines, for each **Feature**, the number of other **Features** that are in contact with it.  The algorithm for determining the number of "contiguous" neighbors of each **Feature** is as follows:

   `Link to the full online documentation for FindNeighbors <http://www.dream3d.io/nx_reference_manual/Filters/FindNeighbors>`_ 

   Mapping of UI display to python named argument

   +------------------------------+--------------------------+
   | UI Display                   | Python Named Argument    |
   +==============================+==========================+
   | Boundary Cells               | boundary_cells           |
   +------------------------------+--------------------------+
   | Cell Feature AttributeMatrix | cell_feature_arrays      |
   +------------------------------+--------------------------+
   | Feature Ids                  | feature_ids              |
   +------------------------------+--------------------------+
   | Image Geometry               | image_geometry           |
   +------------------------------+--------------------------+
   | Neighbor List                | neighbor_list            |
   +------------------------------+--------------------------+
   | Number of Neighbors          | number_of_neighbors      |
   +------------------------------+--------------------------+
   | Shared Surface Area List     | shared_surface_area_list |
   +------------------------------+--------------------------+
   | Store Boundary Cells Array   | store_boundary_cells     |
   +------------------------------+--------------------------+
   | Store Surface Features Array | store_surface_features   |
   +------------------------------+--------------------------+
   | Surface Features             | surface_features         |
   +------------------------------+--------------------------+

   .. py:method:: Execute(boundary_cells, cell_feature_arrays, feature_ids, image_geometry, neighbor_list, number_of_neighbors, shared_surface_area_list, store_boundary_cells, store_surface_features, surface_features)

      :param complex.DataObjectNameParameter boundary_cells: The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6. Only created if Store Boundary Cells Array is checked
      :param complex.AttributeMatrixSelectionParameter cell_feature_arrays: Feature Attribute Matrix of the selected Feature Ids
      :param complex.ArraySelectionParameter feature_ids: Specifies to which Feature each cell belongs
      :param complex.GeometrySelectionParameter image_geometry: The geometry in which to identify feature neighbors
      :param complex.DataObjectNameParameter neighbor_list: List of the contiguous neighboring Features for a given Feature
      :param complex.DataObjectNameParameter number_of_neighbors: Number of contiguous neighboring Features for a given Feature
      :param complex.DataObjectNameParameter shared_surface_area_list: List of the shared surface area for each of the contiguous neighboring Features for a given Feature
      :param complex.BoolParameter store_boundary_cells: Whether to store the boundary Cells array
      :param complex.BoolParameter store_surface_features: Whether to store the surface Features array
      :param complex.DataObjectNameParameter surface_features: The name of the surface features data array. Flag equal to 1 if the Feature touches an outer surface of the sample and equal to 0 if it does not. Only created if Store Surface Features Array is checked
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindNumFeaturesFilter:
.. py:class:: FindNumFeaturesFilter

   **UI Display Name:** *Find Number of Features*

   This **Filter** determines the number of **Features** in each **Ensemble** by summing the total number of rows in the feature attribute matrix belonging to each phase.

   `Link to the full online documentation for FindNumFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindNumFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+--------------------------------+
   | UI Display                | Python Named Argument          |
   +===========================+================================+
   | Ensemble Attribute Matrix | ensemble_attribute_matrix_path |
   +---------------------------+--------------------------------+
   | Feature Phases            | feature_phases_array_path      |
   +---------------------------+--------------------------------+
   | Number of Features        | num_features_array_path        |
   +---------------------------+--------------------------------+

   .. py:method:: Execute(ensemble_attribute_matrix_path, feature_phases_array_path, num_features_array_path)

      :param complex.DataGroupSelectionParameter ensemble_attribute_matrix_path: The path to the ensemble attribute matrix where the number of features array will be stored
      :param complex.ArraySelectionParameter feature_phases_array_path: Array specifying which Ensemble each Feature belongs
      :param complex.DataObjectNameParameter num_features_array_path: The number of Features that belong to each Ensemble
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindSurfaceAreaToVolumeFilter:
.. py:class:: FindSurfaceAreaToVolumeFilter

   **UI Display Name:** *Find Surface Area to Volume & Sphericity*

   This **Filter** calculates the ratio of surface area to volume for each **Feature** in an **Image Geometry**. First, all the boundary **Cells** are found for each **Feature**. Next, the surface area for each face that is in contact with a different **Feature** is totalled. This number is divided by the volume of each **Feature**, calculated by taking the number of **Cells** of each **Feature** and multiplying by the volume of a **Cell**.

   `Link to the full online documentation for FindSurfaceAreaToVolumeFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindSurfaceAreaToVolumeFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------+--------------------------------------+
   | UI Display                   | Python Named Argument                |
   +==============================+======================================+
   | Calculate Sphericity         | calculate_sphericity                 |
   +------------------------------+--------------------------------------+
   | Cell Feature Ids             | feature_ids_path                     |
   +------------------------------+--------------------------------------+
   | Number of Cells              | num_cells_array_path                 |
   +------------------------------+--------------------------------------+
   | Selected Image Geometry      | selected_image_geometry              |
   +------------------------------+--------------------------------------+
   | Sphericity Array Name        | sphericity_array_name                |
   +------------------------------+--------------------------------------+
   | Surface Area to Volume Ratio | surface_area_volume_ratio_array_name |
   +------------------------------+--------------------------------------+

   .. py:method:: Execute(calculate_sphericity, feature_ids_path, num_cells_array_path, selected_image_geometry, sphericity_array_name, surface_area_volume_ratio_array_name)

      :param complex.BoolParameter calculate_sphericity: Whether or not to calculate the sphericity of each Feature
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.ArraySelectionParameter num_cells_array_path: Number of Cells that are owned by the Feature. This value does not place any distinction between Cells that may be of a different size
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.DataObjectNameParameter sphericity_array_name: The sphericity of each feature
      :param complex.DataObjectNameParameter surface_area_volume_ratio_array_name: Ratio of surface area to volume for each Feature. The units are inverse length
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindSurfaceFeatures:
.. py:class:: FindSurfaceFeatures

   **UI Display Name:** *Find Surface Features*

   This **Filter** determines whether a **Feature** touches an outer surface of the sample. This is accomplished by simply querying the **Feature** owners of the **Cells** that sit at either . Any **Feature** that owns one of those **Cells** is said to touch an outer surface and all other **Features** are said to not touch an outer surface of the sample.

   `Link to the full online documentation for FindSurfaceFeatures <http://www.dream3d.io/nx_reference_manual/Filters/FindSurfaceFeatures>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Cell Feature Attribute Matrix | feature_attribute_matrix_path |
   +-------------------------------+-------------------------------+
   | Feature Geometry              | feature_geometry_path         |
   +-------------------------------+-------------------------------+
   | Feature Ids                   | feature_ids_path              |
   +-------------------------------+-------------------------------+
   | Mark Feature 0 Neighbors      | mark_feature_0_neighbors      |
   +-------------------------------+-------------------------------+
   | Surface Features              | surface_features_array_path   |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(feature_attribute_matrix_path, feature_geometry_path, feature_ids_path, mark_feature_0_neighbors, surface_features_array_path)

      :param complex.DataGroupSelectionParameter feature_attribute_matrix_path: The path to the cell feature attribute matrix associated with the input feature ids array
      :param complex.GeometrySelectionParameter feature_geometry_path: The geometry in which to find surface features
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.BoolParameter mark_feature_0_neighbors: Marks features that are neighbors with feature 0.  If this option is off, only features that reside on the edge of the geometry will be marked.
      :param complex.DataObjectNameParameter surface_features_array_path: The created surface features array. Flag of 1 if Feature touches an outer surface or of 0 if it does not
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindTriangleGeomCentroidsFilter:
.. py:class:: FindTriangleGeomCentroidsFilter

   **UI Display Name:** *Find Feature Centroids from Triangle Geometry*

   This **Filter** determines the centroids of each **Feature** in a **Triangle Geometry**. The centroids are determinedusing the following algorithm:

   `Link to the full online documentation for FindTriangleGeomCentroidsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindTriangleGeomCentroidsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Calculated Centroids          | centroids_array_name          |
   +-------------------------------+-------------------------------+
   | Face Labels                   | face_labels_array_path        |
   +-------------------------------+-------------------------------+
   | Face Feature Attribute Matrix | feature_attribute_matrix_name |
   +-------------------------------+-------------------------------+
   | Triangle Geometry             | triangle_geometry_path        |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(centroids_array_name, face_labels_array_path, feature_attribute_matrix_name, triangle_geometry_path)

      :param complex.DataObjectNameParameter centroids_array_name: Centroid values created in the Face Feature Data
      :param complex.ArraySelectionParameter face_labels_array_path: The DataPath to the FaceLabels values.
      :param complex.DataGroupSelectionParameter feature_attribute_matrix_name: The DataPath to the AttributeMatrix that holds feature data for the faces
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the Geometry for which to calculate the normals
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindTriangleGeomSizesFilter:
.. py:class:: FindTriangleGeomSizesFilter

   **UI Display Name:** *Find Feature Volumes from Triangle Geometry*

   This **Filter** computes the enclosed volume of each **Feature** in a **Triangle Geometry**. The result is the volume ofeach surface meshed **Feature**, or alternatively the volume of each unique polyhedron defined by the given _FaceLabels_ array. The volume of any generic polyhedron can be computed using the following algorithm:

   `Link to the full online documentation for FindTriangleGeomSizesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindTriangleGeomSizesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Face Labels                   | face_labels_array_path        |
   +-------------------------------+-------------------------------+
   | Face Feature Attribute Matrix | feature_attribute_matrix_name |
   +-------------------------------+-------------------------------+
   | Triangle Geometry             | triangle_geometry_path        |
   +-------------------------------+-------------------------------+
   | Calculated Volumes            | volumes_array_name            |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(face_labels_array_path, feature_attribute_matrix_name, triangle_geometry_path, volumes_array_name)

      :param complex.ArraySelectionParameter face_labels_array_path: The DataPath to the FaceLabels values.
      :param complex.DataGroupSelectionParameter feature_attribute_matrix_name: The DataPath to the AttributeMatrix that holds feature data for the faces
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the Geometry for which to calculate the normals
      :param complex.DataObjectNameParameter volumes_array_name: Calculated volumes data created in the Face Feature Data Attribute Matrix
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindVertexToTriangleDistancesFilter:
.. py:class:: FindVertexToTriangleDistancesFilter

   **UI Display Name:** *Find Vertex to Triangle Distances*

   This **Filter** computes distances between points in a **Vertex Geoemtry** and triangles in a **Triangle Geoemtry**.  Specifically, for each point in the **Vertex Geometry**, the Euclidean distance to the closest triangle in the **Triangle Geoemtry** is stored.  This distance is *signed*: if the point lies on the side of the triangle to which the triangle normal points, then the distance is positive; otherwise, the distance is negative. Additionally, the ID the closest triangle is stored for each point.

   `Link to the full online documentation for FindVertexToTriangleDistancesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindVertexToTriangleDistancesFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+--------------------------------+
   | UI Display                 | Python Named Argument          |
   +============================+================================+
   | Closest Triangle Ids Array | closest_triangle_id_array_path |
   +----------------------------+--------------------------------+
   | Distances Array            | distances_array_path           |
   +----------------------------+--------------------------------+
   | Target Triangle Geometry   | triangle_data_container        |
   +----------------------------+--------------------------------+
   | Triangle Normals           | triangle_normals_array_path    |
   +----------------------------+--------------------------------+
   | Source Vertex Geometry     | vertex_data_container          |
   +----------------------------+--------------------------------+

   .. py:method:: Execute(closest_triangle_id_array_path, distances_array_path, triangle_data_container, triangle_normals_array_path, vertex_data_container)

      :param complex.DataObjectNameParameter closest_triangle_id_array_path: The array to store the ID of the closest triangle
      :param complex.DataObjectNameParameter distances_array_path: The array to store distance between vertex and triangle
      :param complex.GeometrySelectionParameter triangle_data_container: The triangle geometry to compare against
      :param complex.ArraySelectionParameter triangle_normals_array_path: The triangle geometry's normals array
      :param complex.GeometrySelectionParameter vertex_data_container: The Vertex Geometry point cloud to map to triangles
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindVolFractionsFilter:
.. py:class:: FindVolFractionsFilter

   **UI Display Name:** *Find Volume Fractions of Ensembles*

   This **Filter** determines the volume fraction of each **Ensemble**. The **Filter** counts the number of **Cells** belonging to each **Ensemble** and stores the number fraction.

   `Link to the full online documentation for FindVolFractionsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindVolFractionsFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+-------------------------------------+
   | UI Display                     | Python Named Argument               |
   +================================+=====================================+
   | Cell Ensemble Attribute Matrix | cell_ensemble_attribute_matrix_path |
   +--------------------------------+-------------------------------------+
   | Cell Phases                    | cell_phases_array_path              |
   +--------------------------------+-------------------------------------+
   | Volume Fractions               | vol_fractions_array_path            |
   +--------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_ensemble_attribute_matrix_path, cell_phases_array_path, vol_fractions_array_path)

      :param complex.DataGroupSelectionParameter cell_ensemble_attribute_matrix_path: The path to the cell ensemble attribute matrix where the output volume fractions array will be stored
      :param complex.ArraySelectionParameter cell_phases_array_path: Array specifying which Ensemble each Cell belong
      :param complex.DataObjectNameParameter vol_fractions_array_path: Fraction of volume that belongs to each Ensemble
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateColorTableFilter:
.. py:class:: GenerateColorTableFilter

   **UI Display Name:** *Generate Color Table*

   This **Filter** generates a color table array for a given 1-component input array.  Each element of the input arrayis normalized and converted to a color based on where the value falls in the spectrum of the selected color preset.

   `Link to the full online documentation for GenerateColorTableFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateColorTableFilter>`_ 

   Mapping of UI display to python named argument

   +------------------+--------------------------+
   | UI Display       | Python Named Argument    |
   +==================+==========================+
   | Output RGB Array | rgb_array_path           |
   +------------------+--------------------------+
   | Data Array       | selected_data_array_path |
   +------------------+--------------------------+
   | Select Preset... | selected_preset          |
   +------------------+--------------------------+

   .. py:method:: Execute(rgb_array_path, selected_data_array_path, selected_preset)

      :param complex.DataObjectNameParameter rgb_array_path: The rgb array created by normalizing each element of the input array and converting to a color based on the selected preset color scheme
      :param complex.ArraySelectionParameter selected_data_array_path: The complete path to the data array from which to create the rgb array by applying the selected preset color scheme
      :param complex.GenerateColorTableParameter selected_preset: Select a preset color scheme to apply to the created array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateVectorColorsFilter:
.. py:class:: GenerateVectorColorsFilter

   **UI Display Name:** *Generate Vector Colors*

   This **Filter** generates a color for each **Element** based on the vector assigned to that **Element** in the input vector data.  The color scheme assigns a unique color to all points on the unit hemisphere using a HSV-like scheme. The color space is approximately represented by the following legend.

   `Link to the full online documentation for GenerateVectorColorsFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateVectorColorsFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------------------------+-------------------------------+
   | UI Display                                           | Python Named Argument         |
   +======================================================+===============================+
   | Vector Colors                                        | cell_vector_colors_array_name |
   +------------------------------------------------------+-------------------------------+
   | Mask                                                 | good_voxels_array_path        |
   +------------------------------------------------------+-------------------------------+
   | Apply to Good Voxels Only (Bad Voxels Will Be Black) | use_good_voxels               |
   +------------------------------------------------------+-------------------------------+
   | Vector Attribute Array                               | vectors_array_path            |
   +------------------------------------------------------+-------------------------------+

   .. py:method:: Execute(cell_vector_colors_array_name, good_voxels_array_path, use_good_voxels, vectors_array_path)

      :param complex.DataObjectNameParameter cell_vector_colors_array_name: RGB colors
      :param complex.ArraySelectionParameter good_voxels_array_path: Used to define Elements as good or bad 
      :param complex.BoolParameter use_good_voxels: Whether or not to assign colors to bad voxels or leave them black
      :param complex.ArraySelectionParameter vectors_array_path: Vectors the colors will represent
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _IdentifySample:
.. py:class:: IdentifySample

   **UI Display Name:** *Isolate Largest Feature (Identify Sample)*

   Often when performing a serial sectioning experiment (especially in the FIB-SEM), the sample is *overscanned* resulting in a border of *bad* data around the sample.  This **Filter** attempts to *identify* the sample within the overscanned volume.  The **Filter** makes the assumption that there is only one contiguous set of **Cells** that belong to the sample. The **Filter** requires that the user has already *thresheld* the data to determine which **Cells** are *good* and which are *bad*.  The algorithm for the identification of the sample is then as follows:

   `Link to the full online documentation for IdentifySample <http://www.dream3d.io/nx_reference_manual/Filters/IdentifySample>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-----------------------+
   | UI Display                    | Python Named Argument |
   +===============================+=======================+
   | Fill Holes in Largest Feature | fill_holes            |
   +-------------------------------+-----------------------+
   | Mask                          | good_voxels           |
   +-------------------------------+-----------------------+
   | Image Geometry                | image_geometry        |
   +-------------------------------+-----------------------+

   .. py:method:: Execute(fill_holes, good_voxels, image_geometry)

      :param complex.BoolParameter fill_holes: Whether to fill holes within sample after it is identified
      :param complex.ArraySelectionParameter good_voxels: DataPath to the mask array defining what is sample and what is not
      :param complex.GeometrySelectionParameter image_geometry: DataPath to the target ImageGeom
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImageContouringFilter:
.. py:class:: ImageContouringFilter

   **UI Display Name:** *Contour*

   This filter will draw a 3 dimensional contouring line through an Image Geometry based on an input value.

   `Link to the full online documentation for ImageContouringFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImageContouringFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------+----------------------------+
   | UI Display                       | Python Named Argument      |
   +==================================+============================+
   | Contour Value                    | iso_val_geometry           |
   +----------------------------------+----------------------------+
   | Name of Output Triangle Geometry | new_triangle_geometry_name |
   +----------------------------------+----------------------------+
   | Data Array to Contour            | selected_data_array        |
   +----------------------------------+----------------------------+
   | Selected Image Geometry          | selected_image_geometry    |
   +----------------------------------+----------------------------+

   .. py:method:: Execute(iso_val_geometry, new_triangle_geometry_name, selected_data_array, selected_image_geometry)

      :param complex.Float64Parameter iso_val_geometry: The value to contour on
      :param complex.DataObjectNameParameter new_triangle_geometry_name: This is where the contouring line will be stored
      :param complex.ArraySelectionParameter selected_data_array: This is the data that will be checked for the contouring iso value
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportBinaryCTNorthstarFilter:
.. py:class:: ImportBinaryCTNorthstarFilter

   **UI Display Name:** *Import North Star Imaging CT (.nsihdr/.nsidat)*

   This **Filter** will import a NorthStar Imaging data set consisting of a single .nsihdr and one or more .nsidat files. The data is read into an Image Geometry. The user can import a subvolume instead of reading the entire data set into memory.

   `Link to the full online documentation for ImportBinaryCTNorthstarFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportBinaryCTNorthstarFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------+----------------------------+
   | UI Display                       | Python Named Argument      |
   +==================================+============================+
   | Cell Attribute Matrix Name       | cell_attribute_matrix_name |
   +----------------------------------+----------------------------+
   | Density Array                    | density_array_name         |
   +----------------------------------+----------------------------+
   | Ending XYZ Voxel for Subvolume   | end_voxel_coord            |
   +----------------------------------+----------------------------+
   | Image Geometry Path              | image_geometry_path        |
   +----------------------------------+----------------------------+
   | Import Subvolume                 | import_subvolume           |
   +----------------------------------+----------------------------+
   | Input Header File                | input_header_file          |
   +----------------------------------+----------------------------+
   | Length Unit                      | length_unit                |
   +----------------------------------+----------------------------+
   | Starting XYZ Voxel for Subvolume | start_voxel_coord          |
   +----------------------------------+----------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, density_array_name, end_voxel_coord, image_geometry_path, import_subvolume, input_header_file, length_unit, start_voxel_coord)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name used to create the Cell Attribute Matrix.
      :param complex.DataObjectNameParameter density_array_name: The name used to create the Density data array.
      :param complex.VectorInt32Parameter end_voxel_coord: The ending subvolume voxel (inclusive)
      :param complex.DataGroupCreationParameter image_geometry_path: The path that will be used to create the Image Geometry.
      :param complex.BoolParameter import_subvolume: Import a subvolume instead of the entire volume
      :param complex.FileSystemPathParameter input_header_file: The path to the .nsihdr file
      :param complex.ChoicesParameter length_unit: The length unit that will be set into the created image geometry
      :param complex.VectorInt32Parameter start_voxel_coord: The starting subvolume voxel
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportCSVDataFilter:
.. py:class:: ImportCSVDataFilter

   **UI Display Name:** *Import CSV Data*

   This **Filter** reads CSV data from any text-based file and imports the data into DREAM3D-NX-style arrays.  The user uses the **Filter's** wizard to specify which file to import, how the data is formatted, what to call each array, and what type each array should be.

   `Link to the full online documentation for ImportCSVDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportCSVDataFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------+-----------------------+
   | UI Display           | Python Named Argument |
   +======================+=======================+
   | New Data Group       | created_data_group    |
   +----------------------+-----------------------+
   | Existing Data Group  | selected_data_group   |
   +----------------------+-----------------------+
   | CSV Tuple Dimensions | tuple_dimensions      |
   +----------------------+-----------------------+
   | Use Existing Group   | use_existing_group    |
   +----------------------+-----------------------+
   | CSV Wizard Data      | wizard_data           |
   +----------------------+-----------------------+

   .. py:method:: Execute(created_data_group, selected_data_group, tuple_dimensions, use_existing_group, wizard_data)

      :param complex.DataGroupCreationParameter created_data_group: Store the imported CSV data arrays in a newly created group.
      :param complex.DataGroupSelectionParameter selected_data_group: Store the imported CSV data arrays in this existing group.
      :param complex.DynamicTableParameter tuple_dimensions: The tuple dimensions for the imported CSV data arrays
      :param complex.BoolParameter use_existing_group: Store the imported CSV data arrays in an existing group.
      :param complex.ImportCSVDataParameter wizard_data: Holds all relevant csv file data collected from the wizard
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportDREAM3DFilter:
.. py:class:: ImportDREAM3DFilter

   **UI Display Name:** *Read DREAM.3D File*

   This **Filter** reads the data structure to an hdf5 file with the .dream3d extension.

   `Link to the full online documentation for ImportDREAM3DFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportDREAM3DFilter>`_ 

   Mapping of UI display to python named argument

   +------------------+-----------------------+
   | UI Display       | Python Named Argument |
   +==================+=======================+
   | Import File Path | import_file_data      |
   +------------------+-----------------------+

   .. py:method:: Execute(import_file_data)

      :param complex.Dream3dImportParameter import_file_data: The HDF5 file path the DataStructure should be imported from.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportDeformKeyFileV12Filter:
.. py:class:: ImportDeformKeyFileV12Filter

   **UI Display Name:** *Import Deform Key File (v12)*

   This **Filter** reads DEFORM v12 key files and saves the data in a newly created **Data Container**.

   `Link to the full online documentation for ImportDeformKeyFileV12Filter <http://www.dream3d.io/nx_reference_manual/Filters/ImportDeformKeyFileV12Filter>`_ 

   Mapping of UI display to python named argument

   +------------------+------------------------------+
   | UI Display       | Python Named Argument        |
   +==================+==============================+
   | Cell Data Name   | cell_attribute_matrix_name   |
   +------------------+------------------------------+
   | Input File       | input_file_path              |
   +------------------+------------------------------+
   | Quad Geometry    | quad_geom_path               |
   +------------------+------------------------------+
   | Vertex Data Name | vertex_attribute_matrix_name |
   +------------------+------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, input_file_path, quad_geom_path, vertex_attribute_matrix_name)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name of the created Attribute Matrix for cell data
      :param complex.FileSystemPathParameter input_file_path: File path that points to the imported file
      :param complex.DataGroupCreationParameter quad_geom_path: The created Quad Geometry from  imported from file
      :param complex.DataObjectNameParameter vertex_attribute_matrix_name: The name of the created Attribute Matrix for vertex data
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportHDF5Dataset:
.. py:class:: ImportHDF5Dataset

   **UI Display Name:** *Import HDF5 Dataset*

   This **Filter** allows the user to import datasets from an HDF5 file and store them as attribute arrays in DREAM.3D.  This filter supports importing datasets with any number of dimensions, as long as the created attribute array's total number of components and the tuple count of the destination attribute matrix multiply together to match the HDF5 dataset's total number of elements.

   `Link to the full online documentation for ImportHDF5Dataset <http://www.dream3d.io/nx_reference_manual/Filters/ImportHDF5Dataset>`_ 

   Mapping of UI display to python named argument

   +------------------+-----------------------+
   | UI Display       | Python Named Argument |
   +==================+=======================+
   | Select HDF5 File | import_hd_f5_file     |
   +------------------+-----------------------+

   .. py:method:: Execute(import_hd_f5_file)

      :param complex.ImportHDF5DatasetParameter import_hd_f5_file: The HDF5 file data to import
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportTextFilter:
.. py:class:: ImportTextFilter

   **UI Display Name:** *Import ASCII Data Array*

   This **Filter** allows the user to import a plain text file containing the contents of a single Attribute Array. The delimeters can be one of the following:

   `Link to the full online documentation for ImportTextFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportTextFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------------------------------------------+-----------------------+
   | UI Display                                                                 | Python Named Argument |
   +============================================================================+=======================+
   | Set Tuple Dimensions [not required if creating inside an Attribute Matrix] | advanced_options      |
   +----------------------------------------------------------------------------+-----------------------+
   | Data Format                                                                | data_format           |
   +----------------------------------------------------------------------------+-----------------------+
   | Delimiter                                                                  | delimiter_choice      |
   +----------------------------------------------------------------------------+-----------------------+
   | Input File                                                                 | input_file            |
   +----------------------------------------------------------------------------+-----------------------+
   | Number of Components                                                       | n_comp                |
   +----------------------------------------------------------------------------+-----------------------+
   | Skip Header Lines                                                          | n_skip_lines          |
   +----------------------------------------------------------------------------+-----------------------+
   | Data Array Dimensions (Slowest to Fastest Dimensions)                      | n_tuples              |
   +----------------------------------------------------------------------------+-----------------------+
   | Created Array Path                                                         | output_data_array     |
   +----------------------------------------------------------------------------+-----------------------+
   | Scalar Type                                                                | scalar_type           |
   +----------------------------------------------------------------------------+-----------------------+

   .. py:method:: Execute(advanced_options, data_format, delimiter_choice, input_file, n_comp, n_skip_lines, n_tuples, output_data_array, scalar_type)

      :param complex.BoolParameter advanced_options: This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix
      :param complex.DataStoreFormatParameter data_format: This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.
      :param complex.ChoicesParameter delimiter_choice: Delimiter for values on a line
      :param complex.FileSystemPathParameter input_file: File path that points to the imported file
      :param complex.UInt64Parameter n_comp: Number of columns
      :param complex.UInt64Parameter n_skip_lines: Number of lines at the start of the file to skip
      :param complex.DynamicTableParameter n_tuples: Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.
      :param complex.ArrayCreationParameter output_data_array: DataPath or Name for the underlying Data Array
      :param complex.NumericTypeParameter scalar_type: Data Type to interpret and store data into.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportVolumeGraphicsFileFilter:
.. py:class:: ImportVolumeGraphicsFileFilter

   **UI Display Name:** *Import Volume Graphics File (.vgi/.vol)*

   This **Filter** will import Volume Graphics data files in the form of .vgi/.vol pairs. Both files must exist and be in the same directory for the filter to work. The .vgi file is read to find out the dimensions, spacing and units of the data. The name of the .vol file is also contained in the .vgi file.

   `Link to the full online documentation for ImportVolumeGraphicsFileFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportVolumeGraphicsFileFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------+----------------------------+
   | UI Display               | Python Named Argument      |
   +==========================+============================+
   | Cell Attribute Matrix    | cell_attribute_matrix_name |
   +--------------------------+----------------------------+
   | Density                  | density_array_name         |
   +--------------------------+----------------------------+
   | Image Geometry           | new_image_geometry         |
   +--------------------------+----------------------------+
   | VolumeGraphics .vgi File | vg_header_file             |
   +--------------------------+----------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, density_array_name, new_image_geometry, vg_header_file)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The attribute matrix created as a child of the image geometry
      :param complex.DataObjectNameParameter density_array_name: The data array created as a child of the attribute matrix
      :param complex.DataGroupCreationParameter new_image_geometry: Path to create the Image Geometry
      :param complex.FileSystemPathParameter vg_header_file: The input VolumeGraphics file
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _InitializeData:
.. py:class:: InitializeData

   **UI Display Name:** *Initialize Data*

   This **Filter** allows the user to define a subvolume of the data set in which the **Filter** will reset all data by writing *zeros (0)* into every array for every **Cell** within the subvolume.

   `Link to the full online documentation for InitializeData <http://www.dream3d.io/nx_reference_manual/Filters/InitializeData>`_ 

   Mapping of UI display to python named argument

   +----------------------+-----------------------+
   | UI Display           | Python Named Argument |
   +======================+=======================+
   | Cell Arrays          | cell_arrays           |
   +----------------------+-----------------------+
   | Image Geometry       | image_geom_path       |
   +----------------------+-----------------------+
   | Initialization Range | init_range            |
   +----------------------+-----------------------+
   | Initialization Type  | init_type             |
   +----------------------+-----------------------+
   | Initialization Value | init_value            |
   +----------------------+-----------------------+
   | Max Point            | max_point             |
   +----------------------+-----------------------+
   | Min Point            | min_point             |
   +----------------------+-----------------------+

   .. py:method:: Execute(cell_arrays, image_geom_path, init_range, init_type, init_value, max_point, min_point)

      :param complex.MultiArraySelectionParameter cell_arrays: The cell data arrays in which to initialize a sub-volume to zeros
      :param complex.GeometrySelectionParameter image_geom_path: The geometry containing the cell data for which to initialize
      :param complex.VectorFloat64Parameter init_range: The initialization range if Random With Range Initialization Type is selected
      :param complex.ChoicesParameter init_type: Tells how to initialize the data
      :param complex.Float64Parameter init_value: The initialization value if Manual Initialization Type is selected
      :param complex.VectorUInt64Parameter max_point: The maximum x, y, z bound in cells
      :param complex.VectorUInt64Parameter min_point: The minimum x, y, z bound in cells
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _InterpolatePointCloudToRegularGridFilter:
.. py:class:: InterpolatePointCloudToRegularGridFilter

   **UI Display Name:** *Interpolate Point Cloud to Regular Grid*

   This **Filter** interpolates the values of arrays stored in a **Vertex Geometry** onto a user-selected **Image Geometry**.  The user defines the (x,y,z) radii of a kernel in *real space units*.  This kernel can be intialized to either *uniform* or *Gaussian*.  The interpolation algorithm proceeds as follows:

   `Link to the full online documentation for InterpolatePointCloudToRegularGridFilter <http://www.dream3d.io/nx_reference_manual/Filters/InterpolatePointCloudToRegularGridFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------------+-------------------------+
   | UI Display                      | Python Named Argument   |
   +=================================+=========================+
   | Attribute Arrays to Copy        | copy_arrays             |
   +---------------------------------+-------------------------+
   | Gaussian Sigmas                 | guassian_sigmas         |
   +---------------------------------+-------------------------+
   | Interpolated Image Geometry     | image_geom              |
   +---------------------------------+-------------------------+
   | Attribute Arrays to Interpolate | interpolate_arrays      |
   +---------------------------------+-------------------------+
   | Interpolated Group              | interpolated_group      |
   +---------------------------------+-------------------------+
   | Interpolation Technique         | interpolation_technique |
   +---------------------------------+-------------------------+
   | Kernel Distances Group          | kernel_distances_array  |
   +---------------------------------+-------------------------+
   | Kernel Size                     | kernel_size             |
   +---------------------------------+-------------------------+
   | Mask                            | mask                    |
   +---------------------------------+-------------------------+
   | Store Kernel Distances          | store_kernel_distances  |
   +---------------------------------+-------------------------+
   | Use Mask                        | use_mask                |
   +---------------------------------+-------------------------+
   | Vertex Geometry to Interpolate  | vertex_geom             |
   +---------------------------------+-------------------------+
   | Voxel Indices                   | voxel_indices           |
   +---------------------------------+-------------------------+

   .. py:method:: Execute(copy_arrays, guassian_sigmas, image_geom, interpolate_arrays, interpolated_group, interpolation_technique, kernel_distances_array, kernel_size, mask, store_kernel_distances, use_mask, vertex_geom, voxel_indices)

      :param complex.MultiArraySelectionParameter copy_arrays: DataPaths to copy
      :param complex.VectorFloat32Parameter guassian_sigmas: Specifies the Gaussian sigmas
      :param complex.GeometrySelectionParameter image_geom: DataPath to interpolated geometry
      :param complex.MultiArraySelectionParameter interpolate_arrays: DataPaths to interpolate
      :param complex.DataObjectNameParameter interpolated_group: DataPath to created DataGroup for interpolated data
      :param complex.ChoicesParameter interpolation_technique: Selected Interpolation Technique
      :param complex.DataObjectNameParameter kernel_distances_array: DataPath to created DataGroup for kernel distances data
      :param complex.VectorFloat32Parameter kernel_size: Specifies the kernel size
      :param complex.ArraySelectionParameter mask: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.BoolParameter store_kernel_distances: Specifies whether or not to store kernel distances
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :param complex.GeometrySelectionParameter vertex_geom: DataPath to geometry to interpolate
      :param complex.ArraySelectionParameter voxel_indices: DataPath to voxel indices
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _IterativeClosestPointFilter:
.. py:class:: IterativeClosestPointFilter

   **UI Display Name:** *Iterative Closest Point*

   This **Filter** estimates the rigid body transformation (i.e., rotation and translation) between two sets of points represted by **Vertex Geometries** using the *iterative closest point* (ICP) algorithm.  The two **Vertex Geometries** are not required to have the same number of points.  The **Filter** first initializes temporary storage for each set of points and a global transformation.  Then, the alignment algorithm iterates through the following steps:

   `Link to the full online documentation for IterativeClosestPointFilter <http://www.dream3d.io/nx_reference_manual/Filters/IterativeClosestPointFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------------+-----------------------+
   | UI Display                              | Python Named Argument |
   +=========================================+=======================+
   | Apply Transformation to Moving Geometry | apply_transformation  |
   +-----------------------------------------+-----------------------+
   | Moving Vertex Geometry                  | moving_vertex         |
   +-----------------------------------------+-----------------------+
   | Number of Iterations                    | num_iterations        |
   +-----------------------------------------+-----------------------+
   | Target Vertex Geometry                  | target_vertex         |
   +-----------------------------------------+-----------------------+
   | Output Transform Array                  | transform_array       |
   +-----------------------------------------+-----------------------+

   .. py:method:: Execute(apply_transformation, moving_vertex, num_iterations, target_vertex, transform_array)

      :param complex.BoolParameter apply_transformation: If checked, geometry will be updated implicitly
      :param complex.DataPathSelectionParameter moving_vertex: The geometry to align [mutable]
      :param complex.UInt64Parameter num_iterations: The number of times to run the algorithm [more increases accuracy]
      :param complex.DataPathSelectionParameter target_vertex: The geometry to be matched against [immutable]
      :param complex.ArrayCreationParameter transform_array: This is the array to store the transform matrix in
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _KMeansFilter:
.. py:class:: KMeansFilter

   **UI Display Name:** *K Means*

   ***Warning:* The randomnes in this filter is not currently consistent between operating systems even if the same seed is used. Specifically between Unix and Windows. This does not affect the results, but the IDs will not correspond. For example if the Cluster Identifier at index one on Linux is 1 it could be 2 on Windows, the overarching clusters will be the same, but their IDs will be different.**

   `Link to the full online documentation for KMeansFilter <http://www.dream3d.io/nx_reference_manual/Filters/KMeansFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+-------------------------------+
   | UI Display                     | Python Named Argument         |
   +================================+===============================+
   | Distance Metric                | distance_metric               |
   +--------------------------------+-------------------------------+
   | Cluster Attribute Matrix       | feature_attribute_matrix_path |
   +--------------------------------+-------------------------------+
   | Cluster Ids Array Name         | feature_ids_array_name        |
   +--------------------------------+-------------------------------+
   | Number of Clusters             | init_clusters                 |
   +--------------------------------+-------------------------------+
   | Mask                           | mask_array_path               |
   +--------------------------------+-------------------------------+
   | Cluster Means Array Name       | means_array_name              |
   +--------------------------------+-------------------------------+
   | Seed                           | seed_value                    |
   +--------------------------------+-------------------------------+
   | Attribute Array to Cluster     | selected_array_path           |
   +--------------------------------+-------------------------------+
   | Use Mask                       | use_mask                      |
   +--------------------------------+-------------------------------+
   | Use Seed for Random Generation | use_seed                      |
   +--------------------------------+-------------------------------+

   .. py:method:: Execute(distance_metric, feature_attribute_matrix_path, feature_ids_array_name, init_clusters, mask_array_path, means_array_name, seed_value, selected_array_path, use_mask, use_seed)

      :param complex.ChoicesParameter distance_metric: Distance Metric type to be used for calculations
      :param complex.DataGroupCreationParameter feature_attribute_matrix_path: name and path of Attribute Matrix to hold Cluster Data
      :param complex.DataObjectNameParameter feature_ids_array_name: name of the ids array to be created in Attribute Array to Cluster's parent group
      :param complex.UInt64Parameter init_clusters: This will be the tuple size for Cluster Attribute Matrix and the values within
      :param complex.ArraySelectionParameter mask_array_path: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.DataObjectNameParameter means_array_name: name of the Means array to be created in Cluster Attribute Matrix
      :param complex.UInt64Parameter seed_value: The seed fed into the random generator
      :param complex.ArraySelectionParameter selected_array_path: The array to cluster from
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :param complex.BoolParameter use_seed: When true the user will be able to put in a seed for random generation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _KMedoidsFilter:
.. py:class:: KMedoidsFilter

   **UI Display Name:** *K Medoids*

   ***Warning:* The randomnes in this filter is not currently consistent between operating systems even if the same seed is used. Specifically between Unix and Windows. This does not affect the results, but the IDs will not correspond. For example if the Cluster Identifier at index one on Linux is 1 it could be 2 on Windows, the overarching clusters will be the same, but their IDs will be different.**

   `Link to the full online documentation for KMedoidsFilter <http://www.dream3d.io/nx_reference_manual/Filters/KMedoidsFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+-------------------------------+
   | UI Display                     | Python Named Argument         |
   +================================+===============================+
   | Distance Metric                | distance_metric               |
   +--------------------------------+-------------------------------+
   | Cluster Attribute Matrix       | feature_attribute_matrix_path |
   +--------------------------------+-------------------------------+
   | Cluster Ids Array Name         | feature_ids_array_name        |
   +--------------------------------+-------------------------------+
   | Number of Clusters             | init_clusters                 |
   +--------------------------------+-------------------------------+
   | Mask                           | mask_array_path               |
   +--------------------------------+-------------------------------+
   | Cluster Medoids Array Name     | medoids_array_name            |
   +--------------------------------+-------------------------------+
   | Seed                           | seed_value                    |
   +--------------------------------+-------------------------------+
   | Attribute Array to Cluster     | selected_array_path           |
   +--------------------------------+-------------------------------+
   | Use Mask                       | use_mask                      |
   +--------------------------------+-------------------------------+
   | Use Seed for Random Generation | use_seed                      |
   +--------------------------------+-------------------------------+

   .. py:method:: Execute(distance_metric, feature_attribute_matrix_path, feature_ids_array_name, init_clusters, mask_array_path, medoids_array_name, seed_value, selected_array_path, use_mask, use_seed)

      :param complex.ChoicesParameter distance_metric: Distance Metric type to be used for calculations
      :param complex.DataGroupCreationParameter feature_attribute_matrix_path: name and path of Attribute Matrix to hold Cluster Data
      :param complex.DataObjectNameParameter feature_ids_array_name: name of the ids array to be created in Attribute Array to Cluster's parent group
      :param complex.UInt64Parameter init_clusters: This will be the tuple size for Cluster Attribute Matrix and the values within
      :param complex.ArraySelectionParameter mask_array_path: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.DataObjectNameParameter medoids_array_name: name of the medoids array to be created in Cluster Attribute Matrix
      :param complex.UInt64Parameter seed_value: The seed fed into the random generator
      :param complex.ArraySelectionParameter selected_array_path: The array to find the medoids for
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :param complex.BoolParameter use_seed: When true the user will be able to put in a seed for random generation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _LaplacianSmoothingFilter:
.. py:class:: LaplacianSmoothingFilter

   **UI Display Name:** *Laplacian Smoothing*

   This **Filter** applies Laplacian smoothing to a **Triangle Geometry** that represents a surface mesh. A. Belyaev [2] has a concise explanation of the Laplacian Smoothing as follows:

   `Link to the full online documentation for LaplacianSmoothingFilter <http://www.dream3d.io/nx_reference_manual/Filters/LaplacianSmoothingFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-----------------------------------+
   | UI Display                    | Python Named Argument             |
   +===============================+===================================+
   | Iteration Steps               | iteration_steps                   |
   +-------------------------------+-----------------------------------+
   | Default Lambda                | lambda_value                      |
   +-------------------------------+-----------------------------------+
   | Mu Factor                     | mu_factor                         |
   +-------------------------------+-----------------------------------+
   | Quadruple Points Lambda       | quad_point_lambda                 |
   +-------------------------------+-----------------------------------+
   | Node Type                     | surface_mesh_node_type_array_path |
   +-------------------------------+-----------------------------------+
   | Outer Points Lambda           | surface_point_lambda              |
   +-------------------------------+-----------------------------------+
   | Outer Quadruple Points Lambda | surface_quad_point_lambda         |
   +-------------------------------+-----------------------------------+
   | Outer Triple Line Lambda      | surface_triple_line_lambda        |
   +-------------------------------+-----------------------------------+
   | Triangle Geometry             | triangle_geometry_data_path       |
   +-------------------------------+-----------------------------------+
   | Triple Line Lambda            | triple_line_lambda                |
   +-------------------------------+-----------------------------------+
   | Use Taubin Smoothing          | use_taubin_smoothing              |
   +-------------------------------+-----------------------------------+

   .. py:method:: Execute(iteration_steps, lambda_value, mu_factor, quad_point_lambda, surface_mesh_node_type_array_path, surface_point_lambda, surface_quad_point_lambda, surface_triple_line_lambda, triangle_geometry_data_path, triple_line_lambda, use_taubin_smoothing)

      :param complex.Int32Parameter iteration_steps: Number of iteration steps to perform. More steps causes more smoothing but will also cause the volume to shrink more. Increasing this number too high may cause collapse of points!
      :param complex.Float32Parameter lambda_value: Value of λ to apply to general internal nodes that are not triple lines, quadruple points or on the surface of the volume
      :param complex.Float32Parameter mu_factor: A value that is multiplied by Lambda the result of which is the mu in Taubin's paper. The value should be a negative value.
      :param complex.Float32Parameter quad_point_lambda: Value of λ to apply to nodes designated as quadruple points.
      :param complex.ArraySelectionParameter surface_mesh_node_type_array_path: The complete path to the array specifying the type of node in the Geometry
      :param complex.Float32Parameter surface_point_lambda: The value of λ to apply to nodes that lie on the outer surface of the volume
      :param complex.Float32Parameter surface_quad_point_lambda: Value of λ for the quadruple Points that lie on the outer surface of the volume.
      :param complex.Float32Parameter surface_triple_line_lambda: Value of λ for triple lines that lie on the outer surface of the volume
      :param complex.GeometrySelectionParameter triangle_geometry_data_path: The complete path to the surface mesh Geometry for which to apply Laplacian smoothing
      :param complex.Float32Parameter triple_line_lambda: Value of λ to apply to nodes designated as triple line nodes.
      :param complex.BoolParameter use_taubin_smoothing: Use Taubin's Lambda-Mu algorithm.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _LosAlamosFFTWriterFilter:
.. py:class:: LosAlamosFFTWriterFilter

   **UI Display Name:** *Export Los Alamos FFT File*

   This **Filter** will create the directories along the path to the file if possible.

   `Link to the full online documentation for LosAlamosFFTWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/LosAlamosFFTWriterFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------+------------------------------+
   | UI Display            | Python Named Argument        |
   +=======================+==============================+
   | Euler Angles          | cell_euler_angles_array_path |
   +-----------------------+------------------------------+
   | Feature Phases        | cell_phases_array_path       |
   +-----------------------+------------------------------+
   | Feature Ids           | feature_ids_array_path       |
   +-----------------------+------------------------------+
   | Parent Image Geometry | image_geom_path              |
   +-----------------------+------------------------------+
   | Output File Path      | output_file                  |
   +-----------------------+------------------------------+

   .. py:method:: Execute(cell_euler_angles_array_path, cell_phases_array_path, feature_ids_array_path, image_geom_path, output_file)

      :param complex.ArraySelectionParameter cell_euler_angles_array_path: Data Array containing the three angles defining the orientation for each of the Cell in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter cell_phases_array_path: Data Array that specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.GeometrySelectionParameter image_geom_path: The parent image geometry holding the subsequent arrays
      :param complex.FileSystemPathParameter output_file: The path to the output file
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _MapPointCloudToRegularGridFilter:
.. py:class:: MapPointCloudToRegularGridFilter

   **UI Display Name:** *Map Point Cloud to Regular Grid*

   This **Filter** determines, for a user-defined grid, in which voxel each point in a **Vertex Geometry** lies.  The user can either construct a sampling grid by specifying the dimensions, or select a pre-existing **Image Geometry** to use as the sampling grid.  The voxel indices that each point lies in are stored on the vertices.

   `Link to the full online documentation for MapPointCloudToRegularGridFilter <http://www.dream3d.io/nx_reference_manual/Filters/MapPointCloudToRegularGridFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-------------------------+
   | UI Display              | Python Named Argument   |
   +=========================+=========================+
   | Created Cell Data Name  | cell_data_name          |
   +-------------------------+-------------------------+
   | Existing Image Geometry | existing_image_geometry |
   +-------------------------+-------------------------+
   | Grid Dimensions         | grid_dimensions         |
   +-------------------------+-------------------------+
   | Mask                    | mask                    |
   +-------------------------+-------------------------+
   | Created Image Geometry  | new_image_geometry      |
   +-------------------------+-------------------------+
   | Sampling Grid Type      | sampling_grid_type      |
   +-------------------------+-------------------------+
   | Use Mask                | use_mask                |
   +-------------------------+-------------------------+
   | Vertex Geometry         | vertex_geometry         |
   +-------------------------+-------------------------+
   | Created Voxel Indices   | voxel_indices           |
   +-------------------------+-------------------------+

   .. py:method:: Execute(cell_data_name, existing_image_geometry, grid_dimensions, mask, new_image_geometry, sampling_grid_type, use_mask, vertex_geometry, voxel_indices)

      :param complex.DataObjectNameParameter cell_data_name: The name of the cell data attribute matrix to be created within the created Image Geometry
      :param complex.GeometrySelectionParameter existing_image_geometry: Path to the existing Image Geometry
      :param complex.VectorInt32Parameter grid_dimensions: Target grid size
      :param complex.ArraySelectionParameter mask: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.DataGroupCreationParameter new_image_geometry: Path to create the Image Geometry
      :param complex.ChoicesParameter sampling_grid_type: Specifies how data is saved or accessed
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :param complex.GeometrySelectionParameter vertex_geometry: Path to the target Vertex Geometry
      :param complex.DataObjectNameParameter voxel_indices: Path to the created Voxel Indices array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _MinNeighbors:
.. py:class:: MinNeighbors

   **UI Display Name:** *Minimum Number of Neighbors*

   This **Filter** sets the minimum number of contiguous neighboring **Features** a **Feature** must have to remain in the structure. Entering zero results in nothing changing.  Entering a number larger than the maximum number of neighbors of any **Feature** generates an *error* (since all **Features** would be removed). The user needs to proceed conservatively here when choosing the value for the minimum to avoid accidentally exceeding the maximum. After **Features** are removed for not having enough neighbors, the remaining **Features** are *coarsened* iteratively, one **Cell** per iteration, until the gaps left by the removed **Features** are filled.  Effectively, this is an isotropic **Feature** growth in the regions around removed **Features**.

   `Link to the full online documentation for MinNeighbors <http://www.dream3d.io/nx_reference_manual/Filters/MinNeighbors>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-----------------------+
   | UI Display                 | Python Named Argument |
   +============================+=======================+
   | Apply to Single Phase Only | apply_to_single_phase |
   +----------------------------+-----------------------+
   | Cell Attribute Matrix      | cell_attribute_matrix |
   +----------------------------+-----------------------+
   | Feature Ids                | feature_ids           |
   +----------------------------+-----------------------+
   | Feature Phases             | feature_phases        |
   +----------------------------+-----------------------+
   | Cell Arrays to Ignore      | ignored_voxel_arrays  |
   +----------------------------+-----------------------+
   | Image Geometry             | image_geom            |
   +----------------------------+-----------------------+
   | Minimum Number Neighbors   | min_num_neighbors     |
   +----------------------------+-----------------------+
   | Number of Neighbors        | num_neighbors         |
   +----------------------------+-----------------------+
   | Phase Index                | phase_number          |
   +----------------------------+-----------------------+

   .. py:method:: Execute(apply_to_single_phase, cell_attribute_matrix, feature_ids, feature_phases, ignored_voxel_arrays, image_geom, min_num_neighbors, num_neighbors, phase_number)

      :param complex.BoolParameter apply_to_single_phase: Whether to apply minimum to single ensemble or all ensembles
      :param complex.AttributeMatrixSelectionParameter cell_attribute_matrix: The cell data attribute matrix in which to apply the minimum neighbors algorithm
      :param complex.ArraySelectionParameter feature_ids: Specifies to which Feature each Element belongs
      :param complex.ArraySelectionParameter feature_phases: Specifies to which Ensemble each Feature belongs. Only required if Apply to Single Phase Only is checked
      :param complex.MultiArraySelectionParameter ignored_voxel_arrays: The arrays to ignore when applying the minimum neighbors algorithm
      :param complex.GeometrySelectionParameter image_geom: The target geometry
      :param complex.UInt64Parameter min_num_neighbors: Number of neighbors a Feature must have to remain as a Feature
      :param complex.ArraySelectionParameter num_neighbors: Number of contiguous neighboring Features for each Feature
      :param complex.UInt64Parameter phase_number: Which Ensemble to apply minimum to. Only needed if Apply to Single Phase Only is checked
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _MoveData:
.. py:class:: MoveData

   **UI Display Name:** *Move Data*

   This **Filter** allows the user to move an **Attribute Array** from one **Attribute Matrix** to another compatible **Attribute Matrix** or to move an **Attribute Matrix** from one **Data Container** to another **Data Container**. **Attribute Matrices** are compatible if the *number of tuples* are equal, **not** the actual *tuple dimensions*.

   `Link to the full online documentation for MoveData <http://www.dream3d.io/nx_reference_manual/Filters/MoveData>`_ 

   Mapping of UI display to python named argument

   +--------------+-----------------------+
   | UI Display   | Python Named Argument |
   +==============+=======================+
   | Data to Move | data                  |
   +--------------+-----------------------+
   | New Parent   | new_parent            |
   +--------------+-----------------------+

   .. py:method:: Execute(data, new_parent)

      :param complex.MultiPathSelectionParameter data: The complete paths to the data object(s) to be moved
      :param complex.DataGroupSelectionParameter new_parent: The complete path to the parent data object to which the data will be moved
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _MultiThresholdObjects:
.. py:class:: MultiThresholdObjects

   **UI Display Name:** *Multi-Threshold Objects*

   This **Filter** allows the user to input single or multiple criteria for thresholding **Attribute Arrays** in an **Attribute Matrix**. Internally, the algorithm creates the output boolean arrays for each comparison that the user creates.  Comparisons can be either a value and boolean operator (*Less Than*, *Greater Than*, *Equal To*, *Not Equal To*) or a collective set of comparisons. Then all the output arrays are compared with their given comparison operator ( *And* / *Or* ) with the value of a set being the result of its own comparisons calculated from top to bottom.

   `Link to the full online documentation for MultiThresholdObjects <http://www.dream3d.io/nx_reference_manual/Filters/MultiThresholdObjects>`_ 

   Mapping of UI display to python named argument

   +------------------------+------------------------+
   | UI Display             | Python Named Argument  |
   +========================+========================+
   | Data Thresholds        | array_thresholds       |
   +------------------------+------------------------+
   | Mask Array             | created_data_path      |
   +------------------------+------------------------+
   | Mask Type              | created_mask_type      |
   +------------------------+------------------------+
   | Custom FALSE Value     | custom_false_value     |
   +------------------------+------------------------+
   | Custom TRUE Value      | custom_true_value      |
   +------------------------+------------------------+
   | Use Custom FALSE Value | use_custom_false_value |
   +------------------------+------------------------+
   | Use Custom TRUE Value  | use_custom_true_value  |
   +------------------------+------------------------+

   .. py:method:: Execute(array_thresholds, created_data_path, created_mask_type, custom_false_value, custom_true_value, use_custom_false_value, use_custom_true_value)

      :param complex.ArrayThresholdsParameter array_thresholds: DataArray thresholds to mask
      :param complex.DataObjectNameParameter created_data_path: DataPath to the created Mask Array
      :param complex.DataTypeParameter created_mask_type: DataType used for the created Mask Array
      :param complex.Float64Parameter custom_false_value: This is the custom FALSE value that will be output to the mask array
      :param complex.Float64Parameter custom_true_value: This is the custom TRUE value that will be output to the mask array
      :param complex.BoolParameter use_custom_false_value: Specifies whether to output a custom FALSE value (the default value is 0)
      :param complex.BoolParameter use_custom_true_value: Specifies whether to output a custom TRUE value (the default value is 1)
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _NearestPointFuseRegularGridsFilter:
.. py:class:: NearestPointFuseRegularGridsFilter

   **UI Display Name:** *Fuse Regular Grids (Nearest Point)*

   This **Filter** fuses two **Image Geometry** data sets together. The grid of **Cells** in the *Reference* **Data Container** is overlaid on the grid of **Cells** in the *Sampling* **Data Container**.  Each **Cell** in the *Reference* **Data Container** is associated with the nearest point in the *Sampling* **Data Container** (i.e., no *interpolation* is performed).  All the attributes of the **Cell** in the *Sampling* **Data Container** are then assigned to the **Cell** in the *Reference* **Data Container**.  Additional to the **Cell** attributes being copied, all **Feature and Ensemble Attribute Matrices** from the *Sampling* **Data Container** are copied to the *Reference* **Data Container**.

   `Link to the full online documentation for NearestPointFuseRegularGridsFilter <http://www.dream3d.io/nx_reference_manual/Filters/NearestPointFuseRegularGridsFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------------+--------------------------------------+
   | UI Display                      | Python Named Argument                |
   +=================================+======================================+
   | Fill Value                      | fill_value                           |
   +---------------------------------+--------------------------------------+
   | Reference Cell Attribute Matrix | reference_cell_attribute_matrix_path |
   +---------------------------------+--------------------------------------+
   | Reference Image Geometry        | reference_geometry_path              |
   +---------------------------------+--------------------------------------+
   | Sampling Cell Attribute Matrix  | sampling_cell_attribute_matrix_path  |
   +---------------------------------+--------------------------------------+
   | Sampling Image Geometry         | sampling_geometry_path               |
   +---------------------------------+--------------------------------------+
   | Use Custom Fill Value           | use_fill                             |
   +---------------------------------+--------------------------------------+

   .. py:method:: Execute(fill_value, reference_cell_attribute_matrix_path, reference_geometry_path, sampling_cell_attribute_matrix_path, sampling_geometry_path, use_fill)

      :param complex.Float64Parameter fill_value: This is the value that will appear in the arrays outside the overlap
      :param complex.DataGroupSelectionParameter reference_cell_attribute_matrix_path: The attribute matrix for the reference geometry
      :param complex.GeometrySelectionParameter reference_geometry_path: This is the geometry that will store the values from the overlap
      :param complex.DataGroupSelectionParameter sampling_cell_attribute_matrix_path: The attribute matrix for the sampling geometry
      :param complex.GeometrySelectionParameter sampling_geometry_path: This is the geometry that will be copied into the reference geometry at the overlap
      :param complex.BoolParameter use_fill: If false all copied arrays will be filled with 0 by default
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _PartitionGeometryFilter:
.. py:class:: PartitionGeometryFilter

   **UI Display Name:** *Partition Geometry*

   This **Filter** generates a partition grid and assigns partition IDs for every voxel/node of a given geometry.

   `Link to the full online documentation for PartitionGeometryFilter <http://www.dream3d.io/nx_reference_manual/Filters/PartitionGeometryFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------+-----------------------------------+
   | UI Display                             | Python Named Argument             |
   +========================================+===================================+
   | Existing Partition Grid                | existing_partitioning_scheme_path |
   +----------------------------------------+-----------------------------------+
   | Feature Attribute Matrix               | feature_attr_matrix_name          |
   +----------------------------------------+-----------------------------------+
   | Input Geometry to Partition            | geometry_to_partition             |
   +----------------------------------------+-----------------------------------+
   | Cell Length (Physical Units)           | length_per_partition              |
   +----------------------------------------+-----------------------------------+
   | Minimum Grid Coordinate                | lower_left_coord                  |
   +----------------------------------------+-----------------------------------+
   | Number Of Cells Per Axis               | number_of_partitions_per_axis     |
   +----------------------------------------+-----------------------------------+
   | Out-Of-Bounds Feature ID               | out_of_bounds_value               |
   +----------------------------------------+-----------------------------------+
   | Partition Ids                          | partition_ids_array_name          |
   +----------------------------------------+-----------------------------------+
   | Select the partitioning mode           | partitioning_mode                 |
   +----------------------------------------+-----------------------------------+
   | Partition Grid Origin                  | partitioning_scheme_origin        |
   +----------------------------------------+-----------------------------------+
   | Input Geometry Cell Attribute Matrix   | ps_attribute_matrix_path          |
   +----------------------------------------+-----------------------------------+
   | Partition Grid Geometry                | ps_geometry                       |
   +----------------------------------------+-----------------------------------+
   | Cell Attribute Matrix                  | ps_geometry_am_name               |
   +----------------------------------------+-----------------------------------+
   | Feature Ids                            | ps_geometry_data_name             |
   +----------------------------------------+-----------------------------------+
   | Starting Feature ID                    | starting_partition_id             |
   +----------------------------------------+-----------------------------------+
   | Maximum Grid Coordinate                | upper_right_coord                 |
   +----------------------------------------+-----------------------------------+
   | Use Vertex Mask (Node Geometries Only) | use_vertex_mask                   |
   +----------------------------------------+-----------------------------------+
   | Vertex Mask                            | vertex_mask_path                  |
   +----------------------------------------+-----------------------------------+

   .. py:method:: Execute(existing_partitioning_scheme_path, feature_attr_matrix_name, geometry_to_partition, length_per_partition, lower_left_coord, number_of_partitions_per_axis, out_of_bounds_value, partition_ids_array_name, partitioning_mode, partitioning_scheme_origin, ps_attribute_matrix_path, ps_geometry, ps_geometry_am_name, ps_geometry_data_name, starting_partition_id, upper_right_coord, use_vertex_mask, vertex_mask_path)

      :param complex.GeometrySelectionParameter existing_partitioning_scheme_path: This is an existing Image Geometry that defines the partition grid that will be used.
      :param complex.DataObjectNameParameter feature_attr_matrix_name: The name of the feature attribute matrix that will be created as a child of the input geometry.
      :param complex.GeometrySelectionParameter geometry_to_partition: The input geometry that will be partitioned
      :param complex.VectorFloat32Parameter length_per_partition: The length in physical units for each cell in the partition grid. The physical units are automatically set by the input geometry.
      :param complex.VectorFloat32Parameter lower_left_coord: Minimum grid coordinate used to create the partition grid
      :param complex.VectorInt32Parameter number_of_partitions_per_axis: The number of cells along each axis of the partition grid
      :param complex.Int32Parameter out_of_bounds_value: The value used as the feature id for voxels/nodes that are outside the bounds of the partition grid.
      :param complex.DataObjectNameParameter partition_ids_array_name: The name of the partition ids output array stored in the input cell attribute matrix
      :param complex.ChoicesParameter partitioning_mode: Mode can be 'Basic (0)', 'Advanced (1)', 'Bounding Box (2)', 'Existing Partition Grid (3)'
      :param complex.VectorFloat32Parameter partitioning_scheme_origin: The origin of the generated partition geometry
      :param complex.AttributeMatrixSelectionParameter ps_attribute_matrix_path: The attribute matrix that represents the cell data for the geometry.(Vertex=>Node Geometry, Cell=>Image/Rectilinear)
      :param complex.DataGroupCreationParameter ps_geometry: The complete path to the created partition grid geometry
      :param complex.DataObjectNameParameter ps_geometry_am_name: The name of the cell attribute matrix that will contain the partition grid's cell data arrays.
      :param complex.DataObjectNameParameter ps_geometry_data_name: The name of the feature ids array that will contain the feature ids of the generated partition grid.
      :param complex.Int32Parameter starting_partition_id: The value to start the partition grid's feature ids at.
      :param complex.VectorFloat32Parameter upper_right_coord: Maximum grid coordinate used to create the partition grid
      :param complex.BoolParameter use_vertex_mask: Feature ID values will only be placed on vertices that have a 'true' mask value. All others will have the Out-Of-Bounds Feature ID value used instead
      :param complex.ArraySelectionParameter vertex_mask_path: The complete path to the vertex mask array.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _PointSampleTriangleGeometryFilter:
.. py:class:: PointSampleTriangleGeometryFilter

   **UI Display Name:** *Point Sample Triangle Geometry*

   This **Filter** randomly samples point locations on **Triangles** in a **Triangle Geometry**.  The sampled point locations are then used to construct a **Vertex Geometry**.  The number of point samples may either be specified manually or by inferring from another **Geometry**:

   `Link to the full online documentation for PointSampleTriangleGeometryFilter <http://www.dream3d.io/nx_reference_manual/Filters/PointSampleTriangleGeometryFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------+---------------------------+
   | UI Display                        | Python Named Argument     |
   +===================================+===========================+
   | Mask                              | mask_array_path           |
   +-----------------------------------+---------------------------+
   | Number of Sample Points           | number_of_samples         |
   +-----------------------------------+---------------------------+
   | Face Attribute Arrays to Transfer | selected_data_array_paths |
   +-----------------------------------+---------------------------+
   | Face Areas                        | triangle_areas_array_path |
   +-----------------------------------+---------------------------+
   | Triangle Geometry to Sample       | triangle_geometry_path    |
   +-----------------------------------+---------------------------+
   | Use Mask                          | use_mask                  |
   +-----------------------------------+---------------------------+
   | Vertex Data                       | vertex_data_group_path    |
   +-----------------------------------+---------------------------+
   | Vertex Geometry Name              | vertex_geometry_path      |
   +-----------------------------------+---------------------------+

   .. py:method:: Execute(mask_array_path, number_of_samples, selected_data_array_paths, triangle_areas_array_path, triangle_geometry_path, use_mask, vertex_data_group_path, vertex_geometry_path)

      :param complex.ArraySelectionParameter mask_array_path: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.Int32Parameter number_of_samples: The number of sample points to use
      :param complex.MultiArraySelectionParameter selected_data_array_paths: The paths to the Face Attribute Arrays to transfer to the created Vertex Geometry where the mask is false, if Use Mask is checked
      :param complex.ArraySelectionParameter triangle_areas_array_path: The complete path to the array specifying the area of each Face
      :param complex.DataPathSelectionParameter triangle_geometry_path: The complete path to the triangle Geometry from which to sample
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :param complex.DataObjectNameParameter vertex_data_group_path: The complete path to the vertex data arrays for the Vertex Geometry
      :param complex.DataGroupCreationParameter vertex_geometry_path: The complete path to the DataGroup holding the Vertex Geometry that represents the sampling points
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _QuickSurfaceMeshFilter:
.. py:class:: QuickSurfaceMeshFilter

   **UI Display Name:** *Quick Surface Mesh*

   This **Filter** generates a **Triangle Geometry** from a grid **Geometry** (either an **Image Geometry** or a **RectGrid Geometry**) that represents a surface mesh of the present **Features**. The algorithm proceeds by creating a pair of **Triangles** for each face of the **Cell** where the neighboring **Cells** have a different **Feature** Id value. The meshing operation is extremely quick but can result in a surface mesh that is very "stair stepped". The user is encouraged to use a smoothing operation to reduce this "blockiness".

   `Link to the full online documentation for QuickSurfaceMeshFilter <http://www.dream3d.io/nx_reference_manual/Filters/QuickSurfaceMeshFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------+------------------------------------+
   | UI Display                          | Python Named Argument              |
   +=====================================+====================================+
   | Face Data [AttributeMatrix]         | face_data_group_name               |
   +-------------------------------------+------------------------------------+
   | Face Feature Data [AttributeMatrix] | face_feature_attribute_matrix_name |
   +-------------------------------------+------------------------------------+
   | Face Labels                         | face_labels_array_name             |
   +-------------------------------------+------------------------------------+
   | Feature Ids                         | feature_ids_path                   |
   +-------------------------------------+------------------------------------+
   | Attempt to Fix Problem Voxels       | fix_problem_voxels                 |
   +-------------------------------------+------------------------------------+
   | Generate Triple Lines               | generate_triple_lines              |
   +-------------------------------------+------------------------------------+
   | Grid Geometry                       | grid_geometry_data_path            |
   +-------------------------------------+------------------------------------+
   | NodeType                            | node_types_array_name              |
   +-------------------------------------+------------------------------------+
   | Attribute Arrays to Transfer        | selected_data_array_paths          |
   +-------------------------------------+------------------------------------+
   | Created Triangle Geometry           | triangle_geometry_name             |
   +-------------------------------------+------------------------------------+
   | Vertex Data [AttributeMatrix]       | vertex_data_group_name             |
   +-------------------------------------+------------------------------------+

   .. py:method:: Execute(face_data_group_name, face_feature_attribute_matrix_name, face_labels_array_name, feature_ids_path, fix_problem_voxels, generate_triple_lines, grid_geometry_data_path, node_types_array_name, selected_data_array_paths, triangle_geometry_name, vertex_data_group_name)

      :param complex.DataObjectNameParameter face_data_group_name: The complete path to the DataGroup where the Face Data of the Triangle Geometry will be created
      :param complex.DataObjectNameParameter face_feature_attribute_matrix_name: The complete path to the DataGroup where the Feature Data will be stored.
      :param complex.DataObjectNameParameter face_labels_array_name: The complete path to the Array specifying which Features are on either side of each Face in the Triangle Geometry
      :param complex.ArraySelectionParameter feature_ids_path: The complete path to the Array specifying which Feature each Cell belongs to
      :param complex.BoolParameter fix_problem_voxels: See help page.
      :param complex.BoolParameter generate_triple_lines: Experimental feature. May not work.
      :param complex.GeometrySelectionParameter grid_geometry_data_path: The complete path to the Grid Geometry from which to create a Triangle Geometry
      :param complex.DataObjectNameParameter node_types_array_name: The complete path to the Array specifying the type of node in the Triangle Geometry
      :param complex.MultiArraySelectionParameter selected_data_array_paths: The paths to the Arrays specifying which Cell Attribute Arrays to transfer to the created Triangle Geometry
      :param complex.DataGroupCreationParameter triangle_geometry_name: The name of the created Triangle Geometry
      :param complex.DataObjectNameParameter vertex_data_group_name: The complete path to the DataGroup where the Vertex Data of the Triangle Geometry will be created
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RawBinaryReaderFilter:
.. py:class:: RawBinaryReaderFilter

   **UI Display Name:** *Raw Binary Importer*

   This **Filter** is designed to read data stored in files on the users system in *binary* form. The data file should **not** have any type of header before the data in the file. The user should know exactly how the data is stored in the file and properly define this in the user interface. Not correctly identifying the type of data can cause serious issues since this **Filter**  is simply reading the data into a pre-allocated array interpreted as the user defines.

   `Link to the full online documentation for RawBinaryReaderFilter <http://www.dream3d.io/nx_reference_manual/Filters/RawBinaryReaderFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------+------------------------------+
   | UI Display             | Python Named Argument        |
   +========================+==============================+
   | Output Attribute Array | created_attribute_array_path |
   +------------------------+------------------------------+
   | Endian                 | endian                       |
   +------------------------+------------------------------+
   | Input File             | input_file                   |
   +------------------------+------------------------------+
   | Number of Components   | number_of_components         |
   +------------------------+------------------------------+
   | Scalar Type            | scalar_type                  |
   +------------------------+------------------------------+
   | Skip Header Bytes      | skip_header_bytes            |
   +------------------------+------------------------------+
   | Data Array Dimensions  | tuple_dimensions             |
   +------------------------+------------------------------+

   .. py:method:: Execute(created_attribute_array_path, endian, input_file, number_of_components, scalar_type, skip_header_bytes, tuple_dimensions)

      :param complex.ArrayCreationParameter created_attribute_array_path: The complete path to the created Attribute Array
      :param complex.ChoicesParameter endian: The endianness of the data
      :param complex.FileSystemPathParameter input_file: The input binary file path
      :param complex.UInt64Parameter number_of_components: The number of values at each tuple
      :param complex.NumericTypeParameter scalar_type: Data type of the binary data
      :param complex.UInt64Parameter skip_header_bytes: Number of bytes to skip before reading data
      :param complex.DynamicTableParameter tuple_dimensions: Slowest to Fastest Dimensions (ZYX for example)
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RegularGridSampleSurfaceMeshFilter:
.. py:class:: RegularGridSampleSurfaceMeshFilter

   **UI Display Name:** *Sample Triangle Geometry on Regular Grid*

   This **Filter** "samples" a triangulated surface mesh on a rectilinear grid. The user can specify the number of **Cells** along the X, Y, and Z directions in addition to the resolution in each direction and origin to define a rectilinear grid.  The sampling is then performed by the following steps:

   `Link to the full online documentation for RegularGridSampleSurfaceMeshFilter <http://www.dream3d.io/nx_reference_manual/Filters/RegularGridSampleSurfaceMeshFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------+-------------------------------------+
   | UI Display                          | Python Named Argument               |
   +=====================================+=====================================+
   | Cell Data Name                      | cell_attribute_matrix_name          |
   +-------------------------------------+-------------------------------------+
   | Dimensions (Voxels)                 | dimensions                          |
   +-------------------------------------+-------------------------------------+
   | Feature Ids Name                    | feature_ids_array_name              |
   +-------------------------------------+-------------------------------------+
   | Image Geometry                      | image_geom_path                     |
   +-------------------------------------+-------------------------------------+
   | Length Units (For Description Only) | length_unit                         |
   +-------------------------------------+-------------------------------------+
   | Origin                              | origin                              |
   +-------------------------------------+-------------------------------------+
   | Spacing                             | spacing                             |
   +-------------------------------------+-------------------------------------+
   | Face Labels                         | surface_mesh_face_labels_array_path |
   +-------------------------------------+-------------------------------------+
   | Triangle Geometry                   | triangle_geometry_path              |
   +-------------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, dimensions, feature_ids_array_name, image_geom_path, length_unit, origin, spacing, surface_mesh_face_labels_array_path, triangle_geometry_path)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name for the cell data Attribute Matrix within the Image geometry
      :param complex.VectorUInt64Parameter dimensions: The dimensions of the created Image geometry
      :param complex.DataObjectNameParameter feature_ids_array_name: The name for the feature ids array in cell data Attribute Matrix
      :param complex.DataGroupCreationParameter image_geom_path: The name and path for the image geometry to be created
      :param complex.ChoicesParameter length_unit: The units to be displayed below
      :param complex.VectorFloat32Parameter origin: The origin of the created Image geometry
      :param complex.VectorFloat32Parameter spacing: The spacing of the created Image geometry
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Array specifying which Features are on either side of each Face
      :param complex.GeometrySelectionParameter triangle_geometry_path: The geometry to be sampled onto grid
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RemoveFlaggedFeaturesFilter:
.. py:class:: RemoveFlaggedFeaturesFilter

   **UI Display Name:** *Extract/Remove Flagged Features*

   This **Filter** will remove **Features** that have been flagged by another **Filter** from the structure.  The **Filter** requires that the user point to a boolean array at the **Feature** level that tells the **Filter** whether the **Feature** should remain in the structure.  If the boolean array is *false* for a **Feature**, then all **Cells** that belong to that **Feature** are temporarily *unassigned* and after all *undesired* **Features** are removed, the remaining **Features** are isotropically coarsened to fill in the gaps left by the removed **Features**.

   `Link to the full online documentation for RemoveFlaggedFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/RemoveFlaggedFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Created Image Geometry Prefix | created_image_geometry_prefix |
   +-------------------------------+-------------------------------+
   | Cell Feature Ids              | feature_ids_path              |
   +-------------------------------+-------------------------------+
   | Fill-in Removed Features      | fill_removed_features         |
   +-------------------------------+-------------------------------+
   | Flagged Features              | flagged_features_array_path   |
   +-------------------------------+-------------------------------+
   | Selected Operation            | functionality                 |
   +-------------------------------+-------------------------------+
   | Attribute Arrays to Ignore    | ignored_data_array_paths      |
   +-------------------------------+-------------------------------+
   | Selected Image Geometry       | image_geometry                |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(created_image_geometry_prefix, feature_ids_path, fill_removed_features, flagged_features_array_path, functionality, ignored_data_array_paths, image_geometry)

      :param complex.StringParameter created_image_geometry_prefix: The prefix name for each of new cropped (extracted) geometry 

NOTE: a '-' will automatically be added between the prefix and number
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.BoolParameter fill_removed_features: Whether or not to fill in the gaps left by the removed Features
      :param complex.ArraySelectionParameter flagged_features_array_path: Specifies whether the Feature will remain in the structure or not
      :param complex.ChoicesParameter functionality: Whether to extract features into new geometry or remove or extract then remove
      :param complex.MultiArraySelectionParameter ignored_data_array_paths: The list of arrays to ignore when removing flagged features
      :param complex.GeometrySelectionParameter image_geometry: The target geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RemoveFlaggedVertices:
.. py:class:: RemoveFlaggedVertices

   **UI Display Name:** *Remove Flagged Vertices*

   This **Filter** removes **Vertices** from the supplied **Vertex Geometry** that are flagged by a boolean mask array.Specifically, **Vertices** flagged as *true* are removed from the **Geometry**. A new reduced **Vertex Geometry** iscreated that contains all the remaining **Vertices**. It is unknown until run time how many **Vertices** will be removedfrom the **Geometry**. Therefore, this **Filter** requires that a new **Data Container** be created to contain thereduced **Vertex Geometry**. This new **Data Container** will contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Data Container**. Additionally, all **Vertex** data will be copied, withtuples *removed* for any **Vertices** removed by the **Filter**. The user must supply a name for the reduced **DataContainer**, but all other copied objects (**Attribute Matrices** and **Attribute Arrays**) will retain the same namesas the original source.

   `Link to the full online documentation for RemoveFlaggedVertices <http://www.dream3d.io/nx_reference_manual/Filters/RemoveFlaggedVertices>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-----------------------+
   | UI Display              | Python Named Argument |
   +=========================+=======================+
   | Flagged Vertex Array    | mask                  |
   +-------------------------+-----------------------+
   | Reduced Vertex Geometry | reduced_vertex        |
   +-------------------------+-----------------------+
   | Vertex Geometry         | vertex_geom           |
   +-------------------------+-----------------------+

   .. py:method:: Execute(mask, reduced_vertex, vertex_geom)

      :param complex.ArraySelectionParameter mask: DataPath to the conditional array that will be used to decide which vertices are removed.
      :param complex.DataGroupCreationParameter reduced_vertex: Created Vertex Geometry DataPath. This will be created during the filter.
      :param complex.GeometrySelectionParameter vertex_geom: Path to the target Vertex Geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RemoveMinimumSizeFeaturesFilter:
.. py:class:: RemoveMinimumSizeFeaturesFilter

   **UI Display Name:** *Remove Minimum Size Features*

   This **Filter** removes **Features** that have a total number of **Cells** below the minimum threshold defined by the user. Entering a number larger than the largest **Feature** generates an *error* (since all **Features** would be removed). Hence, a choice of threshold should be carefully be chosen if it is not known how many **Cells** are in the largest **Features**. After removing all the small **Features**, the remaining **Features** are isotropically coarsened to fill the gaps left by the small **Features**.

   `Link to the full online documentation for RemoveMinimumSizeFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/RemoveMinimumSizeFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+---------------------------+
   | UI Display                    | Python Named Argument     |
   +===============================+===========================+
   | Apply to Single Phase         | apply_single_phase        |
   +-------------------------------+---------------------------+
   | FeatureIds Array              | feature_ids_path          |
   +-------------------------------+---------------------------+
   | Phases Array                  | feature_phases_path       |
   +-------------------------------+---------------------------+
   | Image Geometry                | image_geom_path           |
   +-------------------------------+---------------------------+
   | Minimum Allowed Features Size | min_allowed_features_size |
   +-------------------------------+---------------------------+
   | Num Cells Array               | num_cells_path            |
   +-------------------------------+---------------------------+
   | Phase Index                   | phase_number              |
   +-------------------------------+---------------------------+

   .. py:method:: Execute(apply_single_phase, feature_ids_path, feature_phases_path, image_geom_path, min_allowed_features_size, num_cells_path, phase_number)

      :param complex.BoolParameter apply_single_phase: Apply to Single Phase
      :param complex.ArraySelectionParameter feature_ids_path: DataPath to FeatureIds DataArray
      :param complex.ArraySelectionParameter feature_phases_path: DataPath to Feature Phases DataArray
      :param complex.DataPathSelectionParameter image_geom_path: DataPath to Image Geometry
      :param complex.Int64Parameter min_allowed_features_size: Minimum allowed features size
      :param complex.ArraySelectionParameter num_cells_path: DataPath to NumCells DataArray
      :param complex.Int64Parameter phase_number: Target phase to remove
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RenameDataObject:
.. py:class:: RenameDataObject

   **UI Display Name:** *Rename DataObject*

   This **Filter** renames a user chosen **Data Object** of any type.

   `Link to the full online documentation for RenameDataObject <http://www.dream3d.io/nx_reference_manual/Filters/RenameDataObject>`_ 

   Mapping of UI display to python named argument

   +----------------------+-----------------------+
   | UI Display           | Python Named Argument |
   +======================+=======================+
   | DataObject to Rename | data_object           |
   +----------------------+-----------------------+
   | New Name             | new_name              |
   +----------------------+-----------------------+

   .. py:method:: Execute(data_object, new_name)

      :param complex.DataPathSelectionParameter data_object: DataPath pointing to the target DataObject
      :param complex.StringParameter new_name: Target DataObject name
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ReplaceElementAttributesWithNeighborValuesFilter:
.. py:class:: ReplaceElementAttributesWithNeighborValuesFilter

   **UI Display Name:** *Replace Element Attributes with Neighbor (Threshold)*

   This **Filter** first identifies all **Cells** that have a value that meets the selected threshold value set by theuser. Then, for each of those **Cells**, their neighboring **Cells** are checked to determine the neighbor **Cell** withmaximum or minimum value. The attributes of the neighbor with the maximum/minimum value are then reassigned to thereference **Cell**.

   `Link to the full online documentation for ReplaceElementAttributesWithNeighborValuesFilter <http://www.dream3d.io/nx_reference_manual/Filters/ReplaceElementAttributesWithNeighborValuesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-----------------------------+
   | UI Display              | Python Named Argument       |
   +=========================+=============================+
   | Input Comparison Array  | confidence_index_array_path |
   +-------------------------+-----------------------------+
   | Loop Until Gone         | loop                        |
   +-------------------------+-----------------------------+
   | Threshold Value         | min_confidence              |
   +-------------------------+-----------------------------+
   | Comparison Operator     | selected_comparison         |
   +-------------------------+-----------------------------+
   | Selected Image Geometry | selected_image_geometry     |
   +-------------------------+-----------------------------+

   .. py:method:: Execute(confidence_index_array_path, loop, min_confidence, selected_comparison, selected_image_geometry)

      :param complex.ArraySelectionParameter confidence_index_array_path: The DataPath to the input array to use for comparison
      :param complex.BoolParameter loop: The algorithm will keep looping until all pixels have been evaluated
      :param complex.Float32Parameter min_confidence: The value to of the threshold
      :param complex.ChoicesParameter selected_comparison: The operator to use for comparisons. 0=Less, 1=Greater Than
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ResampleImageGeomFilter:
.. py:class:: ResampleImageGeomFilter

   **UI Display Name:** *Resample Data (Image Geometry)*

   This **Filter** changes the **Cell** spacing/resolution based on inputs from the user. The values entered are the desired new spacings (not multiples of the current resolution).  The number of **Cells** in the volume will change when the spacing values are changed and thus the user should be cautious of generating "too many" **Cells** by entering very small values (i.e., very high resolution). Thus, this **Filter** will perform a down-sampling or up-sampling procedure.

   `Link to the full online documentation for ResampleImageGeomFilter <http://www.dream3d.io/nx_reference_manual/Filters/ResampleImageGeomFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+------------------------------------+
   | UI Display                    | Python Named Argument              |
   +===============================+====================================+
   | Cell Feature Attribute Matrix | cell_feature_attribute_matrix_path |
   +-------------------------------+------------------------------------+
   | Feature IDs                   | feature_ids_path                   |
   +-------------------------------+------------------------------------+
   | Created Image Geometry        | new_data_container_path            |
   +-------------------------------+------------------------------------+
   | Perform In Place              | remove_original_geometry           |
   +-------------------------------+------------------------------------+
   | Renumber Features             | renumber_features                  |
   +-------------------------------+------------------------------------+
   | Selected Image Geometry       | selected_image_geometry            |
   +-------------------------------+------------------------------------+
   | New Spacing                   | spacing                            |
   +-------------------------------+------------------------------------+

   .. py:method:: Execute(cell_feature_attribute_matrix_path, feature_ids_path, new_data_container_path, remove_original_geometry, renumber_features, selected_image_geometry, spacing)

      :param complex.AttributeMatrixSelectionParameter cell_feature_attribute_matrix_path: DataPath to the feature Attribute Matrix
      :param complex.ArraySelectionParameter feature_ids_path: DataPath to Cell Feature IDs array
      :param complex.DataGroupCreationParameter new_data_container_path: The location of the resampled geometry
      :param complex.BoolParameter remove_original_geometry: Removes the original Image Geometry after filter is completed
      :param complex.BoolParameter renumber_features: Specifies if the feature IDs should be renumbered
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry to resample
      :param complex.VectorFloat32Parameter spacing: The new spacing values (dx, dy, dz). Larger spacing will cause less voxels, smaller spacing will cause more voxels.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ResampleRectGridToImageGeomFilter:
.. py:class:: ResampleRectGridToImageGeomFilter

   **UI Display Name:** *Resample Rectilinear Grid to Image Geom*

   This **Filter** will resample an existing RectilinearGrid onto a regular grid (Image Geometry) and copy cell data into the newly created Image Geometry Data Container during the process.

   `Link to the full online documentation for ResampleRectGridToImageGeomFilter <http://www.dream3d.io/nx_reference_manual/Filters/ResampleRectGridToImageGeomFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------+----------------------------------+
   | UI Display               | Python Named Argument            |
   +==========================+==================================+
   | Dimensions (Voxels)      | dimensions                       |
   +--------------------------+----------------------------------+
   | Cell Attribute Matrix    | image_geom_cell_attribute_matrix |
   +--------------------------+----------------------------------+
   | Created Image Geometry   | image_geometry_path              |
   +--------------------------+----------------------------------+
   | Input Rectilinear Grid   | rectilinear_grid_path            |
   +--------------------------+----------------------------------+
   | Attribute Arrays to Copy | selected_data_array_paths        |
   +--------------------------+----------------------------------+

   .. py:method:: Execute(dimensions, image_geom_cell_attribute_matrix, image_geometry_path, rectilinear_grid_path, selected_data_array_paths)

      :param complex.VectorInt32Parameter dimensions: The image geometry voxel dimensions in which to re-sample the rectilinear grid geometry
      :param complex.DataObjectNameParameter image_geom_cell_attribute_matrix: The name of the cell data Attribute Matrix created with the Image Geometry
      :param complex.DataGroupCreationParameter image_geometry_path: Path to the created Image Geometry
      :param complex.GeometrySelectionParameter rectilinear_grid_path: Path to the RectGrid Geometry to be re-sampled
      :param complex.MultiArraySelectionParameter selected_data_array_paths: Rectilinear Grid Cell Data to possibly copy
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ReverseTriangleWindingFilter:
.. py:class:: ReverseTriangleWindingFilter

   **UI Display Name:** *Reverse Triangle Winding*

   This **Filter** reverses the *winding* for each **Triangle** in a **Triangle Geometry**. This will *reverse* the direction of calculated **Triangle** normals. Some analysis routines require the normals to be pointing "away" from the center of a **Feature**. This **Filter** allows for manipulation of this construct.

   `Link to the full online documentation for ReverseTriangleWindingFilter <http://www.dream3d.io/nx_reference_manual/Filters/ReverseTriangleWindingFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------+------------------------+
   | UI Display        | Python Named Argument  |
   +===================+========================+
   | Triangle Geometry | triangle_geometry_path |
   +-------------------+------------------------+

   .. py:method:: Execute(triangle_geometry_path)

      :param complex.GeometrySelectionParameter triangle_geometry_path: The DataPath to then input Triangle Geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RobustAutomaticThreshold:
.. py:class:: RobustAutomaticThreshold

   **UI Display Name:** *Robust Automatic Threshold*

   This **Filter** automatically computes a threshold value for a scalar **Attribute Array** based on the array's gradient magnitude, producing a boolean array that is *false* where the input array is less than the threshold value and *true* otherwise.  The threshold value is computed using the following equation:

   `Link to the full online documentation for RobustAutomaticThreshold <http://www.dream3d.io/nx_reference_manual/Filters/RobustAutomaticThreshold>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-----------------------+
   | UI Display              | Python Named Argument |
   +=========================+=======================+
   | Input Array             | array_to_threshold    |
   +-------------------------+-----------------------+
   | Mask                    | created_mask_path     |
   +-------------------------+-----------------------+
   | Gradient Magnitude Data | gradient_array        |
   +-------------------------+-----------------------+

   .. py:method:: Execute(array_to_threshold, created_mask_path, gradient_array)

      :param complex.ArraySelectionParameter array_to_threshold: DataArray to Threshold
      :param complex.DataObjectNameParameter created_mask_path: Created mask based on Input Array and Gradient Magnitude
      :param complex.ArraySelectionParameter gradient_array: The complete path to the Array specifying the gradient magnitude of the Input Array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RotateSampleRefFrameFilter:
.. py:class:: RotateSampleRefFrameFilter

   **UI Display Name:** *Rotate Sample Reference Frame*

   **NOTE: As of July 2023, this filter is only verified to work with a rotation angle of 180 degrees, a rotation axis of (010), and a (0, 0, 0) origin.**

   `Link to the full online documentation for RotateSampleRefFrameFilter <http://www.dream3d.io/nx_reference_manual/Filters/RotateSampleRefFrameFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------+--------------------------+
   | UI Display                       | Python Named Argument    |
   +==================================+==========================+
   | Created Image Geometry           | created_image_geometry   |
   +----------------------------------+--------------------------+
   | Perform In-Place Rotation        | remove_original_geometry |
   +----------------------------------+--------------------------+
   | Perform Slice By Slice Transform | rotate_slice_by_slice    |
   +----------------------------------+--------------------------+
   | Rotation Axis-Angle [<ijk>w]     | rotation_axis            |
   +----------------------------------+--------------------------+
   | Transformation Matrix            | rotation_matrix          |
   +----------------------------------+--------------------------+
   | Rotation Representation          | rotation_representation  |
   +----------------------------------+--------------------------+
   | Selected Image Geometry          | selected_image_geometry  |
   +----------------------------------+--------------------------+

   .. py:method:: Execute(created_image_geometry, remove_original_geometry, rotate_slice_by_slice, rotation_axis, rotation_matrix, rotation_representation, selected_image_geometry)

      :param complex.DataGroupCreationParameter created_image_geometry: The location of the rotated geometry
      :param complex.BoolParameter remove_original_geometry: Performs the rotation in-place for the given Image Geometry
      :param complex.BoolParameter rotate_slice_by_slice: This option is specific to EBSD Data and is not generally used.
      :param complex.VectorFloat32Parameter rotation_axis: Axis-Angle in sample reference frame to rotate about.
      :param complex.DynamicTableParameter rotation_matrix: The 4x4 Transformation Matrix
      :param complex.ChoicesParameter rotation_representation: Which form used to represent rotation (axis angle or rotation matrix)
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry on which to perform the rotation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ScalarSegmentFeaturesFilter:
.. py:class:: ScalarSegmentFeaturesFilter

   **UI Display Name:** *Segment Features (Scalar)*

   This **Filter** segments the **Features** by grouping neighboring **Cells** that satisfy the *scalar tolerance*, i.e., have a scalar difference less than the value set by the user. The process by which the **Features** are identified is given below and is a standard *burn algorithm*.

   `Link to the full online documentation for ScalarSegmentFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/ScalarSegmentFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------+
   | UI Display                    | Python Named Argument   |
   +===============================+=========================+
   | Active                        | active_array_path       |
   +-------------------------------+-------------------------+
   | Cell Feature Attribute Matrix | cell_feature_group_path |
   +-------------------------------+-------------------------+
   | Cell Feature IDs              | feature_ids_path        |
   +-------------------------------+-------------------------+
   | Grid Geometry                 | grid_geometry_path      |
   +-------------------------------+-------------------------+
   | Scalar Array to Segment       | input_array_path        |
   +-------------------------------+-------------------------+
   | Mask                          | mask_path               |
   +-------------------------------+-------------------------+
   | Randomize Feature IDs         | randomize_features      |
   +-------------------------------+-------------------------+
   | Scalar Tolerance              | scalar_tolerance        |
   +-------------------------------+-------------------------+
   | Use Mask Array                | use_mask                |
   +-------------------------------+-------------------------+

   .. py:method:: Execute(active_array_path, cell_feature_group_path, feature_ids_path, grid_geometry_path, input_array_path, mask_path, randomize_features, scalar_tolerance, use_mask)

      :param complex.DataObjectNameParameter active_array_path: Created array
      :param complex.DataObjectNameParameter cell_feature_group_path: Created Cell Feature Attribute Matrix
      :param complex.DataObjectNameParameter feature_ids_path: Path to the created Feature IDs path
      :param complex.DataPathSelectionParameter grid_geometry_path: DataPath to target Grid Geometry
      :param complex.ArraySelectionParameter input_array_path: Path to the DataArray to segment
      :param complex.ArraySelectionParameter mask_path: Path to the DataArray Mask
      :param complex.BoolParameter randomize_features: Specifies if feature IDs should be randomized during calculations
      :param complex.Int32Parameter scalar_tolerance: Tolerance for segmenting input Cell Data
      :param complex.BoolParameter use_mask: Determines if a mask array is used for segmenting
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _SetImageGeomOriginScalingFilter:
.. py:class:: SetImageGeomOriginScalingFilter

   **UI Display Name:** *Set Origin & Spacing (Image Geom)*

   This **Filter** changes the origin and/or the spacing of an **Image Geometry**. For example, if the current origin is at (0, 0, 0) and the user would like the origin to be (10, 4, 8), then the user should enter the following input values:

   `Link to the full online documentation for SetImageGeomOriginScalingFilter <http://www.dream3d.io/nx_reference_manual/Filters/SetImageGeomOriginScalingFilter>`_ 

   Mapping of UI display to python named argument

   +----------------+-----------------------+
   | UI Display     | Python Named Argument |
   +================+=======================+
   | Change Origin  | change_origin         |
   +----------------+-----------------------+
   | Change Spacing | change_resolution     |
   +----------------+-----------------------+
   | Image Geometry | image_geom            |
   +----------------+-----------------------+
   | Origin         | origin                |
   +----------------+-----------------------+
   | Spacing        | spacing               |
   +----------------+-----------------------+

   .. py:method:: Execute(change_origin, change_resolution, image_geom, origin, spacing)

      :param complex.BoolParameter change_origin: Specifies if the origin should be changed
      :param complex.BoolParameter change_resolution: Specifies if the spacing should be changed
      :param complex.GeometrySelectionParameter image_geom: Path to the target ImageGeom
      :param complex.VectorFloat64Parameter origin: Specifies the new origin values
      :param complex.VectorFloat64Parameter spacing: Specifies the new spacing values
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _SharedFeatureFaceFilter:
.. py:class:: SharedFeatureFaceFilter

   **UI Display Name:** *Generate Triangle Face Ids*

   This **Filter** assigns a unique Id to each **Triangle** in a **Triangle Geometry** that represents the _uniqueboundary_ on which that **Triangle** resides. For example, if there were only two **Features** that shared one boundary,then the **Triangles** on that boundary would be labeled with a single unique Id. This procedure creates _unique groups_of **Triangles**, which themselves are a set of **Features**. Thus, this **Filter** also creates a **Feature AttributeMatrix** for this new set of **Features**, and creates **Attribute Arrays** for their Ids and number of **Triangles**. Thisprocess can be considered a **segmentation** where each unique id is the shared boundary between two features.

   `Link to the full online documentation for SharedFeatureFaceFilter <http://www.dream3d.io/nx_reference_manual/Filters/SharedFeatureFaceFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+--------------------------------------+
   | UI Display                    | Python Named Argument                |
   +===============================+======================================+
   | Face Labels                   | face_labels_array_path               |
   +-------------------------------+--------------------------------------+
   | Feature Face Ids              | feature_face_ids_array_name          |
   +-------------------------------+--------------------------------------+
   | Feature Face Labels           | feature_face_labels_array_name       |
   +-------------------------------+--------------------------------------+
   | Feature Number of Triangles   | feature_num_triangles_array_name     |
   +-------------------------------+--------------------------------------+
   | Face Feature Attribute Matrix | grain_boundary_attribute_matrix_name |
   +-------------------------------+--------------------------------------+
   | Randomize Face IDs            | randomize_features                   |
   +-------------------------------+--------------------------------------+
   | Triangle Geometry             | triangle_geometry_path               |
   +-------------------------------+--------------------------------------+

   .. py:method:: Execute(face_labels_array_path, feature_face_ids_array_name, feature_face_labels_array_name, feature_num_triangles_array_name, grain_boundary_attribute_matrix_name, randomize_features, triangle_geometry_path)

      :param complex.ArraySelectionParameter face_labels_array_path: The DataPath to the FaceLabels values.
      :param complex.DataObjectNameParameter feature_face_ids_array_name: The name of the calculated Feature Face Ids DataArray
      :param complex.DataObjectNameParameter feature_face_labels_array_name: The name of the array that holds the calculated Feature Face Labels
      :param complex.DataObjectNameParameter feature_num_triangles_array_name: The name of the array that holds the calculated number of triangles for each feature face
      :param complex.DataObjectNameParameter grain_boundary_attribute_matrix_name: The name of the AttributeMatrix that holds the **Feature Face** data
      :param complex.BoolParameter randomize_features: Specifies if feature IDs should be randomized. Can be helpful when visualizing the faces.
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the Geometry for which to calculate the normals
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _SilhouetteFilter:
.. py:class:: SilhouetteFilter

   **UI Display Name:** *Silhouette*

   This **Filter** computes the silhouette for a clustered **Attribute Array**.  The user must select both the original array that has been clustered and the array of cluster Ids.  The silhouette represents a measure for the quality of a clustering.  Specifically, the silhouette provides a measure for how strongly a given point belongs to its own cluster compared to all other clusters.  The silhouette is computed as follows:

   `Link to the full online documentation for SilhouetteFilter <http://www.dream3d.io/nx_reference_manual/Filters/SilhouetteFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+------------------------+
   | UI Display                    | Python Named Argument  |
   +===============================+========================+
   | Distance Metric               | distance_metric        |
   +-------------------------------+------------------------+
   | Cluster Ids                   | feature_ids_array_path |
   +-------------------------------+------------------------+
   | Mask                          | mask_array_path        |
   +-------------------------------+------------------------+
   | Attribute Array to Silhouette | selected_array_path    |
   +-------------------------------+------------------------+
   | Silhouette                    | silhouette_array_path  |
   +-------------------------------+------------------------+
   | Use Mask                      | use_mask               |
   +-------------------------------+------------------------+

   .. py:method:: Execute(distance_metric, feature_ids_array_path, mask_array_path, selected_array_path, silhouette_array_path, use_mask)

      :param complex.ChoicesParameter distance_metric: Distance Metric type to be used for calculations
      :param complex.ArraySelectionParameter feature_ids_array_path: The DataPath to the DataArray that specifies which cluster each point belongs
      :param complex.ArraySelectionParameter mask_array_path: DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.
      :param complex.ArraySelectionParameter selected_array_path: The DataPath to the input DataArray
      :param complex.ArrayCreationParameter silhouette_array_path: The DataPath to the calculated output Silhouette array values
      :param complex.BoolParameter use_mask: Specifies whether or not to use a mask array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _SplitAttributeArrayFilter:
.. py:class:: SplitAttributeArrayFilter

   **UI Display Name:** *Split Multicomponent Attribute Array*

   This **Filter** splits an n-component **Attribute Array** into **n** scalar arrays, where each array is one of the original components.  Any arbitrary component array may be split in this manner, and the output arrays will have the same primitive type as the input array.  The original array is not modified (unless the option to remove the original array is selected); instead, **n** new arrays are created.  For example, consider an unsigned 8-bit array with three components:

   `Link to the full online documentation for SplitAttributeArrayFilter <http://www.dream3d.io/nx_reference_manual/Filters/SplitAttributeArrayFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------------------+------------------------------+
   | UI Display                            | Python Named Argument        |
   +=======================================+==============================+
   | Components to Extract                 | components_to_extract        |
   +---------------------------------------+------------------------------+
   | Remove Original Array                 | delete_original_array        |
   +---------------------------------------+------------------------------+
   | Multicomponent Attribute Array        | multicomponent_array         |
   +---------------------------------------+------------------------------+
   | Postfix                               | postfix                      |
   +---------------------------------------+------------------------------+
   | Select Specific Components to Extract | select_components_to_extract |
   +---------------------------------------+------------------------------+

   .. py:method:: Execute(components_to_extract, delete_original_array, multicomponent_array, postfix, select_components_to_extract)

      :param complex.DynamicTableParameter components_to_extract: The components from the input array to be extracted into separate arrays
      :param complex.BoolParameter delete_original_array: Whether or not to remove the original multicomponent array after splitting
      :param complex.ArraySelectionParameter multicomponent_array: The multicomponent Attribute Array to split
      :param complex.StringParameter postfix: Postfix to add to the end of the split Attribute Arrays
      :param complex.BoolParameter select_components_to_extract: Whether or not to specify only certain components to be extracted
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _StlFileReaderFilter:
.. py:class:: StlFileReaderFilter

   **UI Display Name:** *Import STL File*

   This **Filter**  will read a binary STL File and create a **Triangle Geometry** object in memory. The STL reader is very strict to the STL specification. An explanation of the STL file format can be found on `Wikipedia <https://en.wikipedia.org/wiki/STL>`_. The structure of the file is as follows:

   `Link to the full online documentation for StlFileReaderFilter <http://www.dream3d.io/nx_reference_manual/Filters/StlFileReaderFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-----------------------+
   | UI Display                 | Python Named Argument |
   +============================+=======================+
   | Face Matrix Name           | face_matrix           |
   +----------------------------+-----------------------+
   | Geometry Name [Data Group] | geometry_data_path    |
   +----------------------------+-----------------------+
   | Scale Factor               | scale_factor          |
   +----------------------------+-----------------------+
   | Scale Output Geometry      | scale_output          |
   +----------------------------+-----------------------+
   | Shared Face Matrix Name    | shared_face_matrix    |
   +----------------------------+-----------------------+
   | Shared Vertex Matrix Name  | shared_vertex_matrix  |
   +----------------------------+-----------------------+
   | STL File                   | stl_file_path         |
   +----------------------------+-----------------------+
   | Vertex Matrix Name         | vertex_matrix         |
   +----------------------------+-----------------------+

   .. py:method:: Execute(face_matrix, geometry_data_path, scale_factor, scale_output, shared_face_matrix, shared_vertex_matrix, stl_file_path, vertex_matrix)

      :param complex.StringParameter face_matrix: Name of the created Face Attribute Matrix
      :param complex.DataGroupCreationParameter geometry_data_path: The complete path to the DataGroup containing the created Geometry data
      :param complex.Float32Parameter scale_factor: The factor by which to scale the geometry
      :param complex.BoolParameter scale_output: Scale the output Triangle Geometry by the Scaling Factor
      :param complex.StringParameter shared_face_matrix: Name of the created Shared Face Attribute Matrix
      :param complex.StringParameter shared_vertex_matrix: Name of the created Shared Vertex Attribute Matrix
      :param complex.FileSystemPathParameter stl_file_path: Input STL File
      :param complex.StringParameter vertex_matrix: Name of the created Vertex Attribute Matrix
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _TriangleCentroidFilter:
.. py:class:: TriangleCentroidFilter

   **UI Display Name:** *Calculate Triangle Centroids*

   This **Filter** computes the centroid of each **Triangle** in a **Triangle Geometry** by calculating the average position of all 3 **Vertices** that make up the **Triangle**.

   `Link to the full online documentation for TriangleCentroidFilter <http://www.dream3d.io/nx_reference_manual/Filters/TriangleCentroidFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------+------------------------+
   | UI Display             | Python Named Argument  |
   +========================+========================+
   | Created Face Centroids | centroids_array_name   |
   +------------------------+------------------------+
   | Triangle Geometry      | triangle_geometry_path |
   +------------------------+------------------------+

   .. py:method:: Execute(centroids_array_name, triangle_geometry_path)

      :param complex.DataObjectNameParameter centroids_array_name: The complete path to the array storing the calculated centroids
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the Geometry for which to calculate the normals
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _TriangleDihedralAngleFilter:
.. py:class:: TriangleDihedralAngleFilter

   **UI Display Name:** *Calculate Triangle Minimum Dihedral Angle*

   This **Filter** computes the minimum dihedral angle of each **Triangle** in a **Triangle Geometry** by utilizing matrix mathematics

   `Link to the full online documentation for TriangleDihedralAngleFilter <http://www.dream3d.io/nx_reference_manual/Filters/TriangleDihedralAngleFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+--------------------------------------------------+
   | UI Display              | Python Named Argument                            |
   +=========================+==================================================+
   | Created Dihedral Angles | surface_mesh_triangle_dihedral_angles_array_name |
   +-------------------------+--------------------------------------------------+
   | Triangle Geometry       | tri_geometry_data_path                           |
   +-------------------------+--------------------------------------------------+

   .. py:method:: Execute(surface_mesh_triangle_dihedral_angles_array_name, tri_geometry_data_path)

      :param complex.DataObjectNameParameter surface_mesh_triangle_dihedral_angles_array_name: The name of the array storing the calculated dihedral angles
      :param complex.GeometrySelectionParameter tri_geometry_data_path: The complete path to the Geometry for which to calculate the dihedral angles
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _TriangleNormalFilter:
.. py:class:: TriangleNormalFilter

   **UI Display Name:** *Calculate Triangle Normals*

   This **Filter** computes the normal of each **Triangle** in a **Triangle Geometry** by utilizing matrix subtraction, cross product, and normalization to implement the following theory:For a triangle with point1, point2, point3, if the vector U = point2 - point1 and the vector V = point3 - point1

   `Link to the full online documentation for TriangleNormalFilter <http://www.dream3d.io/nx_reference_manual/Filters/TriangleNormalFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------+------------------------------------------+
   | UI Display           | Python Named Argument                    |
   +======================+==========================================+
   | Created Face Normals | surface_mesh_triangle_normals_array_path |
   +----------------------+------------------------------------------+
   | Triangle Geometry    | tri_geometry_data_path                   |
   +----------------------+------------------------------------------+

   .. py:method:: Execute(surface_mesh_triangle_normals_array_path, tri_geometry_data_path)

      :param complex.DataObjectNameParameter surface_mesh_triangle_normals_array_path: The complete path to the array storing the calculated normals
      :param complex.GeometrySelectionParameter tri_geometry_data_path: The complete path to the Geometry for which to calculate the normals
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _UncertainRegularGridSampleSurfaceMeshFilter:
.. py:class:: UncertainRegularGridSampleSurfaceMeshFilter

   **UI Display Name:** *Sample Triangle Geometry on Uncertain Regular Grid*

   This **Filter** "samples" a triangulated surface mesh on a rectilinear grid, but with "uncertainty" in the absolute position of the **Cells**.  The "uncertainty" is meant to simulate the possible positioning error in a sampling probe.  The user can specify the number of **Cells** along the X, Y, and Z directions in addition to the resolution in each direction and origin to define a rectilinear grid.  The sampling, with "uncertainty", is then performed by the following steps:

   `Link to the full online documentation for UncertainRegularGridSampleSurfaceMeshFilter <http://www.dream3d.io/nx_reference_manual/Filters/UncertainRegularGridSampleSurfaceMeshFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+-------------------------------------+
   | UI Display                     | Python Named Argument               |
   +================================+=====================================+
   | Cell Data Name                 | cell_attribute_matrix_name          |
   +--------------------------------+-------------------------------------+
   | Number of Cells per Axis       | dimensions                          |
   +--------------------------------+-------------------------------------+
   | Feature Ids Name               | feature_ids_array_name              |
   +--------------------------------+-------------------------------------+
   | Image Geometry                 | image_geom_path                     |
   +--------------------------------+-------------------------------------+
   | Origin                         | origin                              |
   +--------------------------------+-------------------------------------+
   | Seed                           | seed_value                          |
   +--------------------------------+-------------------------------------+
   | Spacing                        | spacing                             |
   +--------------------------------+-------------------------------------+
   | Face Labels                    | surface_mesh_face_labels_array_path |
   +--------------------------------+-------------------------------------+
   | Triangle Geometry              | triangle_geometry_path              |
   +--------------------------------+-------------------------------------+
   | Uncertainty                    | uncertainty                         |
   +--------------------------------+-------------------------------------+
   | Use Seed for Random Generation | use_seed                            |
   +--------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, dimensions, feature_ids_array_name, image_geom_path, origin, seed_value, spacing, surface_mesh_face_labels_array_path, triangle_geometry_path, uncertainty, use_seed)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name for the cell data Attribute Matrix within the Image geometry
      :param complex.VectorUInt64Parameter dimensions: The dimensions of the created Image geometry
      :param complex.DataObjectNameParameter feature_ids_array_name: The name for the feature ids array in cell data Attribute Matrix
      :param complex.DataGroupCreationParameter image_geom_path: The name and path for the image geometry to be created
      :param complex.VectorFloat32Parameter origin: The origin of the created Image geometry
      :param complex.UInt64Parameter seed_value: The seed fed into the random generator
      :param complex.VectorFloat32Parameter spacing: The spacing of the created Image geometry
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Array specifying which Features are on either side of each Face
      :param complex.GeometrySelectionParameter triangle_geometry_path: The geometry to be sampled onto grid
      :param complex.VectorFloat32Parameter uncertainty: uncertainty values associated with X, Y and Z positions of Cells
      :param complex.BoolParameter use_seed: When true the user will be able to put in a seed for random generation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _VtkRectilinearGridWriterFilter:
.. py:class:: VtkRectilinearGridWriterFilter

   **UI Display Name:** *Vtk Rectilinear Grid Exporter*

   This Filter reads the **Feature** and phase ids together with image parameters required by Vtk to an output file named by the user. The file is used to generate the image of the **Features** and phases of the **Features**.

   `Link to the full online documentation for VtkRectilinearGridWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/VtkRectilinearGridWriterFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+---------------------------+
   | UI Display                | Python Named Argument     |
   +===========================+===========================+
   | Image Geometry            | image_geometry_path       |
   +---------------------------+---------------------------+
   | Output File               | output_file               |
   +---------------------------+---------------------------+
   | Cell Data Arrays to Write | selected_data_array_paths |
   +---------------------------+---------------------------+
   | Write Binary File         | write_binary_file         |
   +---------------------------+---------------------------+

   .. py:method:: Execute(image_geometry_path, output_file, selected_data_array_paths, write_binary_file)

      :param complex.GeometrySelectionParameter image_geometry_path: The path to the image geometry in which to write out to the vtk file
      :param complex.FileSystemPathParameter output_file: The output vtk file in which the geometry data is written
      :param complex.MultiArraySelectionParameter selected_data_array_paths: The paths to the cell data arrays to write out with the geometry
      :param complex.BoolParameter write_binary_file: Whether or not to write the vtk file in binary
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _WriteASCIIDataFilter:
.. py:class:: WriteASCIIDataFilter

   **UI Display Name:** *Export ASCII Data*

   This **Filter** accepts DataArray(s) as input, extracts the data, creates the file(s), and writes it out according to parameter choices

   `Link to the full online documentation for WriteASCIIDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/WriteASCIIDataFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+---------------------------+
   | UI Display                 | Python Named Argument     |
   +============================+===========================+
   | Delimiter                  | delimiter                 |
   +----------------------------+---------------------------+
   | File Extension             | file_extension            |
   +----------------------------+---------------------------+
   | Header and Index Options   | includes                  |
   +----------------------------+---------------------------+
   | Maximum Elements Per Line  | max_val_per_line          |
   +----------------------------+---------------------------+
   | Output Directory           | output_dir                |
   +----------------------------+---------------------------+
   | Output Path                | output_path               |
   +----------------------------+---------------------------+
   | Output Type                | output_style              |
   +----------------------------+---------------------------+
   | Attribute Arrays to Export | selected_data_array_paths |
   +----------------------------+---------------------------+

   .. py:method:: Execute(delimiter, file_extension, includes, max_val_per_line, output_dir, output_path, output_style, selected_data_array_paths)

      :param complex.ChoicesParameter delimiter: The delimiter separating the data
      :param complex.StringParameter file_extension: The file extension for the output file(s)
      :param complex.ChoicesParameter includes: Default Include is Headers only
      :param complex.Int32Parameter max_val_per_line: Number of tuples to print on each line
      :param complex.FileSystemPathParameter output_dir: The output file path
      :param complex.FileSystemPathParameter output_path: The output file path
      :param complex.ChoicesParameter output_style: Whether to output a folder of files or a single file with all the data in column form
      :param complex.MultiArraySelectionParameter selected_data_array_paths: Output arrays to be written as ASCII representations
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _WriteBinaryDataFilter:
.. py:class:: WriteBinaryDataFilter

   **UI Display Name:** *Export Binary Data*

   This **Filter** accepts DataArray(s) as input, extracts the data, creates the file(s), and writes it out to a single file in binary

   `Link to the full online documentation for WriteBinaryDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/WriteBinaryDataFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+---------------------------+
   | UI Display                 | Python Named Argument     |
   +============================+===========================+
   | Endianess                  | endianess                 |
   +----------------------------+---------------------------+
   | File Extension             | file_extension            |
   +----------------------------+---------------------------+
   | Output Path                | output_path               |
   +----------------------------+---------------------------+
   | Attribute Arrays to Export | selected_data_array_paths |
   +----------------------------+---------------------------+

   .. py:method:: Execute(endianess, file_extension, output_path, selected_data_array_paths)

      :param complex.ChoicesParameter endianess: Default is little endian
      :param complex.StringParameter file_extension: The file extension for the output file
      :param complex.FileSystemPathParameter output_path: The output file path
      :param complex.MultiArraySelectionParameter selected_data_array_paths: The arrays to be exported to a binary file
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _WriteStlFileFilter:
.. py:class:: WriteStlFileFilter

   **UI Display Name:** *Export STL Files from Triangle Geometry*

   This **Filter** will write a binary STL File for each unique **Feature** Id in the associated **Triangle** geometry. The STL files will be named with the [Feature_Id].stl. The user can designate an optional prefix for the files.

   `Link to the full online documentation for WriteStlFileFilter <http://www.dream3d.io/nx_reference_manual/Filters/WriteStlFileFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-----------------------+
   | UI Display                 | Python Named Argument |
   +============================+=======================+
   | Face labels                | feature_ids_path      |
   +----------------------------+-----------------------+
   | Feature Phases             | feature_phases_path   |
   +----------------------------+-----------------------+
   | Group by Feature Phases    | group_by_feature      |
   +----------------------------+-----------------------+
   | Output STL Directory       | output_stl_directory  |
   +----------------------------+-----------------------+
   | STL File Prefix            | output_stl_prefix     |
   +----------------------------+-----------------------+
   | Selected Triangle Geometry | triangle_geom_path    |
   +----------------------------+-----------------------+

   .. py:method:: Execute(feature_ids_path, feature_phases_path, group_by_feature, output_stl_directory, output_stl_prefix, triangle_geom_path)

      :param complex.ArraySelectionParameter feature_ids_path: The triangle feature ids array to order/index files by
      :param complex.ArraySelectionParameter feature_phases_path: The feature phases array to further order/index files by
      :param complex.BoolParameter group_by_feature: Further partition the stl files by feature phases
      :param complex.FileSystemPathParameter output_stl_directory: Directory to dump the STL file(s) to
      :param complex.StringParameter output_stl_prefix: The prefix name of created files (other values will be appended later - including the .stl extension)
      :param complex.GeometrySelectionParameter triangle_geom_path: The geometry to print
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


