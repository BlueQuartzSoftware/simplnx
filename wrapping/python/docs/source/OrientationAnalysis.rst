OrientationAnalysis
===================

.. py:module:: OrientationAnalysis

.. _AlignSectionsMisorientationFilter:
.. py:class:: AlignSectionsMisorientationFilter

   **UI Display Name:** *Align Sections (Misorientation)*

   This **Filter** attempts to align consecutive 'sections' perpendicular to the Z-direction of the sample by determining the position that results in the minimum amount of misorientation between **Cells** directly above-below each other. The algorithm of this **Filter** is as follows:

   `Link to the full online documentation for AlignSectionsMisorientationFilter <http://www.dream3d.io/nx_reference_manual/Filters/AlignSectionsMisorientationFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------+-------------------------------+
   | UI Display                         | Python Named Argument         |
   +====================================+===============================+
   | Alignment File Path                | alignment_shift_file_name     |
   +------------------------------------+-------------------------------+
   | Phases                             | cell_phases_array_path        |
   +------------------------------------+-------------------------------+
   | Crystal Structures                 | crystal_structures_array_path |
   +------------------------------------+-------------------------------+
   | Mask                               | good_voxels_array_path        |
   +------------------------------------+-------------------------------+
   | Misorientation Tolerance (Degrees) | misorientation_tolerance      |
   +------------------------------------+-------------------------------+
   | Quaternions                        | quats_array_path              |
   +------------------------------------+-------------------------------+
   | Selected Image Geometry            | selected_image_geometry_path  |
   +------------------------------------+-------------------------------+
   | Use Mask Array                     | use_good_voxels               |
   +------------------------------------+-------------------------------+
   | Write Alignment Shift File         | write_alignment_shifts        |
   +------------------------------------+-------------------------------+

   .. py:method:: Execute(alignment_shift_file_name, cell_phases_array_path, crystal_structures_array_path, good_voxels_array_path, misorientation_tolerance, quats_array_path, selected_image_geometry_path, use_good_voxels, write_alignment_shifts)

      :param complex.FileSystemPathParameter alignment_shift_file_name: The output file path where the user would like the shifts applied to the section to be written.
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter good_voxels_array_path: Path to the DataArray Mask
      :param complex.Float32Parameter misorientation_tolerance: Tolerance used to decide if Cells above/below one another should be considered to be the same. The value selected should be similar to the tolerance one would use to define Features (i.e., 2-10 degrees)
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :param complex.GeometrySelectionParameter selected_image_geometry_path: The target geometry
      :param complex.BoolParameter use_good_voxels: Whether to remove some Cells from consideration in the alignment process
      :param complex.BoolParameter write_alignment_shifts: Whether to write the shifts applied to each section to a file
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _AlignSectionsMutualInformationFilter:
.. py:class:: AlignSectionsMutualInformationFilter

   **UI Display Name:** *Align Sections (Mutual Information)*

   This **Filter** segments each 2D slice, creating *Feature Ids* that are used when determining the *mutual information* between neighboring slices. The slices are shifted relative to one another until the position of maximum *mutual information*  is determined for each section.  The *Feature Ids* are temporary, they apply to this **Filter** only and are not related to the *Feature Ids* generated in other **Filters**.  The algorithm of this **Filter** is listed below:

   `Link to the full online documentation for AlignSectionsMutualInformationFilter <http://www.dream3d.io/nx_reference_manual/Filters/AlignSectionsMutualInformationFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-------------------------------+
   | UI Display                 | Python Named Argument         |
   +============================+===============================+
   | Alignment File Path        | alignment_shift_file_name     |
   +----------------------------+-------------------------------+
   | Phases                     | cell_phases_array_path        |
   +----------------------------+-------------------------------+
   | Crystal Structures         | crystal_structures_array_path |
   +----------------------------+-------------------------------+
   | Mask                       | good_voxels_array_path        |
   +----------------------------+-------------------------------+
   | Misorientation Tolerance   | misorientation_tolerance      |
   +----------------------------+-------------------------------+
   | Quaternions                | quats_array_path              |
   +----------------------------+-------------------------------+
   | Selected Image Geometry    | selected_image_geometry_path  |
   +----------------------------+-------------------------------+
   | Use Mask Array             | use_good_voxels               |
   +----------------------------+-------------------------------+
   | Write Alignment Shift File | write_alignment_shifts        |
   +----------------------------+-------------------------------+

   .. py:method:: Execute(alignment_shift_file_name, cell_phases_array_path, crystal_structures_array_path, good_voxels_array_path, misorientation_tolerance, quats_array_path, selected_image_geometry_path, use_good_voxels, write_alignment_shifts)

      :param complex.FileSystemPathParameter alignment_shift_file_name: The output file path where the user would like the shifts applied to the section to be written. Only needed if Write Alignment Shifts File is checked.
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs.
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble.
      :param complex.ArraySelectionParameter good_voxels_array_path: Specifies if the Cell is to be counted in the algorithm. Only required if Use Mask Array is checked.
      :param complex.Float32Parameter misorientation_tolerance: Tolerance used to decide if Cells above/below one another should be considered to be the same. The value selected should be similar to the tolerance one would use to define Features (i.e., 2-10 degrees).
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation.
      :param complex.GeometrySelectionParameter selected_image_geometry_path: The target geometry
      :param complex.BoolParameter use_good_voxels: Whether to remove some Cells from consideration in the alignment process.
      :param complex.BoolParameter write_alignment_shifts: Whether to write the shifts applied to each section to a file.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _BadDataNeighborOrientationCheckFilter:
.. py:class:: BadDataNeighborOrientationCheckFilter

   **UI Display Name:** *Neighbor Orientation Comparison (Bad Data)*

   This **Filter** compares the orientations of *bad* **Cells** with their neighbor **Cells**.  If the misorientation is below a user defined tolerance for a user defined number of neighbor **Cells** , then the *bad* **Cell** will be changed to a *good* **Cell**.

   `Link to the full online documentation for BadDataNeighborOrientationCheckFilter <http://www.dream3d.io/nx_reference_manual/Filters/BadDataNeighborOrientationCheckFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------+-------------------------------+
   | UI Display                         | Python Named Argument         |
   +====================================+===============================+
   | Cell Phases                        | cell_phases_array_path        |
   +------------------------------------+-------------------------------+
   | Crystal Structures                 | crystal_structures_array_path |
   +------------------------------------+-------------------------------+
   | Mask                               | good_voxels_array_path        |
   +------------------------------------+-------------------------------+
   | Image Geometry                     | image_geometry_path           |
   +------------------------------------+-------------------------------+
   | Misorientation Tolerance (Degrees) | misorientation_tolerance      |
   +------------------------------------+-------------------------------+
   | Required Number of Neighbors       | number_of_neighbors           |
   +------------------------------------+-------------------------------+
   | Quaternions                        | quats_array_path              |
   +------------------------------------+-------------------------------+

   .. py:method:: Execute(cell_phases_array_path, crystal_structures_array_path, good_voxels_array_path, image_geometry_path, misorientation_tolerance, number_of_neighbors, quats_array_path)

      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each phase
      :param complex.ArraySelectionParameter good_voxels_array_path: Used to define Cells as good or bad
      :param complex.GeometrySelectionParameter image_geometry_path: The target geometry
      :param complex.Float32Parameter misorientation_tolerance: Angular tolerance used to compare with neighboring Cells
      :param complex.Int32Parameter number_of_neighbors: Minimum number of neighbor Cells that must have orientations within above tolerance to allow Cell to be changed
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CAxisSegmentFeaturesFilter:
.. py:class:: CAxisSegmentFeaturesFilter

   **UI Display Name:** *Segment Features (C-Axis Misalignment)*

   This **Filter** segments the **Features** by grouping neighboring **Cells** that satisfy the *C-axis misalignment tolerance*, i.e., have misalignment angle less than the value set by the user. The *C-axis misalignment* refers to the angle between the <001> directions (C-axis in the hexagonal system) that is present between neighboring **Cells**.  The process by which the **Features** are identified is given below and is a standard *burn algorithm*.

   `Link to the full online documentation for CAxisSegmentFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/CAxisSegmentFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------------+------------------------------------+
   | UI Display                                | Python Named Argument              |
   +===========================================+====================================+
   | Active                                    | active_array_name                  |
   +-------------------------------------------+------------------------------------+
   | Cell Feature Attribute Matrix             | cell_feature_attribute_matrix_name |
   +-------------------------------------------+------------------------------------+
   | Phases                                    | cell_phases_array_path             |
   +-------------------------------------------+------------------------------------+
   | Crystal Structures                        | crystal_structures_array_path      |
   +-------------------------------------------+------------------------------------+
   | Feature Ids                               | feature_ids_array_name             |
   +-------------------------------------------+------------------------------------+
   | Mask                                      | good_voxels_array_path             |
   +-------------------------------------------+------------------------------------+
   | Image Geometry                            | image_geometry_path                |
   +-------------------------------------------+------------------------------------+
   | C-Axis Misorientation Tolerance (Degrees) | misorientation_tolerance           |
   +-------------------------------------------+------------------------------------+
   | Quaternions                               | quats_array_path                   |
   +-------------------------------------------+------------------------------------+
   | Randomize Feature Ids                     | randomize_feature_ids              |
   +-------------------------------------------+------------------------------------+
   | Use Mask Array                            | use_good_voxels                    |
   +-------------------------------------------+------------------------------------+

   .. py:method:: Execute(active_array_name, cell_feature_attribute_matrix_name, cell_phases_array_path, crystal_structures_array_path, feature_ids_array_name, good_voxels_array_path, image_geometry_path, misorientation_tolerance, quats_array_path, randomize_feature_ids, use_good_voxels)

      :param complex.DataObjectNameParameter active_array_name: Specifies if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). At the end of the Filter, all Features will be Active
      :param complex.DataObjectNameParameter cell_feature_attribute_matrix_name: The name of the created feature attribute matrix
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.DataObjectNameParameter feature_ids_array_name: Specifies to which Feature each Cell belongs
      :param complex.ArraySelectionParameter good_voxels_array_path: Specifies if the Cell is to be counted in the algorithm. Only required if Use Mask Array is checked
      :param complex.GeometrySelectionParameter image_geometry_path: The path to the input image geometry
      :param complex.Float32Parameter misorientation_tolerance: Tolerance (in degrees) used to determine if neighboring Cells belong to the same Feature
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :param complex.BoolParameter randomize_feature_ids: Specifies whether to randomize the feature ids
      :param complex.BoolParameter use_good_voxels: Specifies whether to use a boolean array to exclude some Cells from the Feature identification process
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ConvertOrientations:
.. py:class:: ConvertOrientations

   **UI Display Name:** *Convert Orientation Representation*

   This **Filter** generates a new orientation representation (see Data Layout Table below) for each **Element**, given the *Input Orientation Representation* for the **Element**. The following table lists the various orientation representations that are supported. DREAM3D is capable of converting between any representation with some caveats.

   `Link to the full online documentation for ConvertOrientations <http://www.dream3d.io/nx_reference_manual/Filters/ConvertOrientations>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-------------------------------+
   | UI Display              | Python Named Argument         |
   +=========================+===============================+
   | Input Orientations      | input_orientation_array_path  |
   +-------------------------+-------------------------------+
   | Input Orientation Type  | input_type                    |
   +-------------------------+-------------------------------+
   | Output Orientations     | output_orientation_array_name |
   +-------------------------+-------------------------------+
   | Output Orientation Type | output_type                   |
   +-------------------------+-------------------------------+

   .. py:method:: Execute(input_orientation_array_path, input_type, output_orientation_array_name, output_type)

      :param complex.ArraySelectionParameter input_orientation_array_path: The complete path to the incoming orientation representation data array
      :param complex.ChoicesParameter input_type: Specifies the incoming orientation representation
      :param complex.DataObjectNameParameter output_orientation_array_name: The name of the data array with the converted orientation representation
      :param complex.ChoicesParameter output_type: Specifies to which orientation representation to convert the incoming data
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ConvertQuaternionFilter:
.. py:class:: ConvertQuaternionFilter

   **UI Display Name:** *Convert Quaternion Order*

   Internally DREAM.3D assumes that a quaternion is laid out in the order such that < x, y, z >, w or Vector-Scalar ordering. Codes and algorithms external to DREAM.3D may store quaternions in the opposite or Scalar-Vector order (w < x,y,z >). This filter will allow the user to easily convert imported Quaternions into the representation that DREAM.3D expects.

   `Link to the full online documentation for ConvertQuaternionFilter <http://www.dream3d.io/nx_reference_manual/Filters/ConvertQuaternionFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------+----------------------------+
   | UI Display           | Python Named Argument      |
   +======================+============================+
   | Conversion Type      | conversion_type            |
   +----------------------+----------------------------+
   | Delete Original Data | delete_original_data       |
   +----------------------+----------------------------+
   | Output Quaternions   | output_data_array_path     |
   +----------------------+----------------------------+
   | Input Quaternions    | quaternion_data_array_path |
   +----------------------+----------------------------+

   .. py:method:: Execute(conversion_type, delete_original_data, output_data_array_path, quaternion_data_array_path)

      :param complex.ChoicesParameter conversion_type: The conversion type: To Scalar Vector=0, To Vector Scalar=1
      :param complex.BoolParameter delete_original_data: Should the original quaternions array be deleted from the DataStructure
      :param complex.DataObjectNameParameter output_data_array_path: The DataPath to the converted quaternions
      :param complex.ArraySelectionParameter quaternion_data_array_path: Specifies the quaternions to convert
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _CreateEnsembleInfoFilter:
.. py:class:: CreateEnsembleInfoFilter

   **UI Display Name:** *Create Ensemble Info*

   This **Filter** allows the user to enter basic crystallographic information about each phase. The Laue class, Phase Type, and Phase Name can all be entered by the user. The information is stored in an EnsembleAttributeMatrix. These values are needed to allow the calculation of certain kinds of crystallographic statistics on the volume, if they have not already been provided by some other means. Each row in the table lists the __Crystal Structure__, __Phase Type__, and __Phase Name__. The proper values for the crystal structure and phase type come from internal constants within DREAM.3D and are listed here:

   `Link to the full online documentation for CreateEnsembleInfoFilter <http://www.dream3d.io/nx_reference_manual/Filters/CreateEnsembleInfoFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+-------------------------------------+
   | UI Display                | Python Named Argument               |
   +===========================+=====================================+
   | Ensemble Attribute Matrix | cell_ensemble_attribute_matrix_name |
   +---------------------------+-------------------------------------+
   | Crystal Structures        | crystal_structures_array_name       |
   +---------------------------+-------------------------------------+
   | Created Ensemble Info     | ensemble                            |
   +---------------------------+-------------------------------------+
   | Phase Names               | phase_names_array_name              |
   +---------------------------+-------------------------------------+
   | Phase Types               | phase_types_array_name              |
   +---------------------------+-------------------------------------+

   .. py:method:: Execute(cell_ensemble_attribute_matrix_name, crystal_structures_array_name, ensemble, phase_names_array_name, phase_types_array_name)

      :param complex.DataGroupCreationParameter cell_ensemble_attribute_matrix_name: The complete path to the attribute matrix in which to store the ensemble phase data arrays
      :param complex.DataObjectNameParameter crystal_structures_array_name: The name of the data array representing the crystal structure for each Ensemble
      :param complex.EnsembleInfoParameter ensemble: The values with which to populate the crystal structures, phase types, and phase names data arrays. Each row corresponds to an ensemble phase.
      :param complex.DataObjectNameParameter phase_names_array_name: The name of the string array representing the phase names for each Ensemble
      :param complex.DataObjectNameParameter phase_types_array_name: The name of the data array representing the phase types for each Ensemble
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _EBSDSegmentFeaturesFilter:
.. py:class:: EBSDSegmentFeaturesFilter

   **UI Display Name:** *Segment Features (Misorientation)*

   This **Filter** segments the **Features** by grouping neighboring **Cells** that satisfy the *misorientation tolerance*, i.e., have misorientation angle less than the value set by the user. The process by which the **Features** are identified is given below and is a standard *burn algorithm*.

   `Link to the full online documentation for EBSDSegmentFeaturesFilter <http://www.dream3d.io/nx_reference_manual/Filters/EBSDSegmentFeaturesFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------+------------------------------------+
   | UI Display                         | Python Named Argument              |
   +====================================+====================================+
   | Active                             | active_array_name                  |
   +------------------------------------+------------------------------------+
   | Cell Feature Attribute Matrix      | cell_feature_attribute_matrix_name |
   +------------------------------------+------------------------------------+
   | Phases                             | cell_phases_array_path             |
   +------------------------------------+------------------------------------+
   | Crystal Structures                 | crystal_structures_array_path      |
   +------------------------------------+------------------------------------+
   | Cell Feature Ids                   | feature_ids_array_name             |
   +------------------------------------+------------------------------------+
   | Mask                               | good_voxels_array_path             |
   +------------------------------------+------------------------------------+
   | Grid Geometry                      | grid_geometry_path                 |
   +------------------------------------+------------------------------------+
   | Misorientation Tolerance (Degrees) | misorientation_tolerance           |
   +------------------------------------+------------------------------------+
   | Quaternions                        | quats_array_path                   |
   +------------------------------------+------------------------------------+
   | Randomize Feature IDs              | randomize_features                 |
   +------------------------------------+------------------------------------+
   | Use Mask Array                     | use_good_voxels                    |
   +------------------------------------+------------------------------------+

   .. py:method:: Execute(active_array_name, cell_feature_attribute_matrix_name, cell_phases_array_path, crystal_structures_array_path, feature_ids_array_name, good_voxels_array_path, grid_geometry_path, misorientation_tolerance, quats_array_path, randomize_features, use_good_voxels)

      :param complex.DataObjectNameParameter active_array_name: The name of the array which specifies if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). At the end of the Filter, all Features will be Active
      :param complex.DataObjectNameParameter cell_feature_attribute_matrix_name: The name of the created cell feature attribute matrix
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.DataObjectNameParameter feature_ids_array_name: Specifies to which Feature each Cell belongs.
      :param complex.ArraySelectionParameter good_voxels_array_path: Path to the data array that specifies if the Cell is to be counted in the algorithm
      :param complex.GeometrySelectionParameter grid_geometry_path: DataPath to target Grid Geometry
      :param complex.Float32Parameter misorientation_tolerance: Tolerance (in degrees) used to determine if neighboring Cells belong to the same Feature
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :param complex.BoolParameter randomize_features: Specifies if feature IDs should be randomized during calculations
      :param complex.BoolParameter use_good_voxels: Specifies whether to use a boolean array to exclude some Cells from the Feature identification process
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _EbsdToH5EbsdFilter:
.. py:class:: EbsdToH5EbsdFilter

   **UI Display Name:** *Import Orientation File(s) to H5EBSD*

   This **Filter** will convert orientation data obtained from Electron Backscatter Diffraction (EBSD) experiments into a single file archive based on the `HDF5 <http://www.hdfgroup.org>`_ file specification. See the **Supported File Formats** section below for information on file compatibility. This **Filter** is typically run as a single **Filter** **Pipeline** to perform the conversion. All subsequent **Pipelines** should then use the Read H5EBSD File **Filter** to import the H5EBSD file into DREAM.3D for analysis, as opposed to re-importing the raw EBSD files.  The primary purpose of this **Filter** is to import a stack of data that forms a 3D volume.  If the user wishes to import a single data file, then the **Filters** Read EDAX EBSD Data (.ang), Read EDAX EBSD Data (.h5), or Read Oxford Instr. EBSD Data (.ctf) should be used for EDAX .ang, EDAX .h5, or Oxford .ctf files, respectively.

   `Link to the full online documentation for EbsdToH5EbsdFilter <http://www.dream3d.io/nx_reference_manual/Filters/EbsdToH5EbsdFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-----------------------+
   | UI Display              | Python Named Argument |
   +=========================+=======================+
   | Input File List         | input_file_list_info  |
   +-------------------------+-----------------------+
   | Output H5Ebsd File      | output_file_path      |
   +-------------------------+-----------------------+
   | Reference Frame Options | reference_frame       |
   +-------------------------+-----------------------+
   | Stacking Order          | stacking_order        |
   +-------------------------+-----------------------+
   | Z Spacing (Microns)     | z_spacing             |
   +-------------------------+-----------------------+

   .. py:method:: Execute(input_file_list_info, output_file_path, reference_frame, stacking_order, z_spacing)

      :param complex.GeneratedFileListParameter input_file_list_info: The values that are used to generate the input file list. See GeneratedFileListParameter for more information.
      :param complex.FileSystemPathParameter output_file_path: The path to the generated .h5ebsd file
      :param complex.ChoicesParameter reference_frame: The reference frame transformation. 0=EDAX(.ang), 1=Oxford(.ctf), 2=No/Unknown Transformation, 3=HEDM-IceNine
      :param complex.ChoicesParameter stacking_order: The order the files should be placed into the 
      :param complex.Float32Parameter z_spacing: The spacing between each slice of data
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _EnsembleInfoReaderFilter:
.. py:class:: EnsembleInfoReaderFilter

   **UI Display Name:** *Import Ensemble Info File*

   This **Filter** reads in information about the crystal structure and phase types of all the **Features** that are contained in a **Cell** based volume. These values are needed to allow the calculation of statistics on the volume, if they have not already been provided by some other means.  The format of the input file is a simple ASCII text file with the extension .ini or .txt. The first group in the file is the name [EnsembleInfo] in square brackets with the key Number_Phases=*number of phases* that are contained in the volume. Subsequent groups in the file list the __Phase Number__, __Crystal Structure__ and __Phase Type__. The proper values for the crystal structure and phase type come from internal constants within DREAM.3D and are listed here:

   `Link to the full online documentation for EnsembleInfoReaderFilter <http://www.dream3d.io/nx_reference_manual/Filters/EnsembleInfoReaderFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+-------------------------------------+
   | UI Display                | Python Named Argument               |
   +===========================+=====================================+
   | Ensemble Attribute Matrix | cell_ensemble_attribute_matrix_name |
   +---------------------------+-------------------------------------+
   | Crystal Structures        | crystal_structures_array_name       |
   +---------------------------+-------------------------------------+
   | Data Container            | data_container_name                 |
   +---------------------------+-------------------------------------+
   | Input Ensemble Info File  | input_file                          |
   +---------------------------+-------------------------------------+
   | Phase Types               | phase_types_array_name              |
   +---------------------------+-------------------------------------+

   .. py:method:: Execute(cell_ensemble_attribute_matrix_name, crystal_structures_array_name, data_container_name, input_file, phase_types_array_name)

      :param complex.DataObjectNameParameter cell_ensemble_attribute_matrix_name: The name of the created Ensemble Attribute Matrix
      :param complex.DataObjectNameParameter crystal_structures_array_name: The name of the created array representing the crystal structure for each Ensemble
      :param complex.DataGroupSelectionParameter data_container_name: The path to the data object in which the ensemble information will be stored
      :param complex.FileSystemPathParameter input_file: The path to the ini formatted input file
      :param complex.DataObjectNameParameter phase_types_array_name: The name of the created array representing the phase type for each Ensemble
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExportGBCDGMTFileFilter:
.. py:class:: ExportGBCDGMTFileFilter

   **UI Display Name:** *Export GBCD Pole Figure (GMT 5)*

   This **Filter** creates a .dat file that can be used in conjunction with `GMT <http://gmt.soest.hawaii.edu/>`_ to generate a grain boundary character distribution (GBCD) pole figure. The user must select the relevant phase for which to write the pole figure by entering the *phase index*.

   `Link to the full online documentation for ExportGBCDGMTFileFilter <http://www.dream3d.io/nx_reference_manual/Filters/ExportGBCDGMTFileFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------+-------------------------------+
   | UI Display                | Python Named Argument         |
   +===========================+===============================+
   | Crystal Structures        | crystal_structures_array_path |
   +---------------------------+-------------------------------+
   | GBCD                      | gbcd_array_path               |
   +---------------------------+-------------------------------+
   | Misorientation Axis-Angle | misorientation_rotation       |
   +---------------------------+-------------------------------+
   | Output GMT File           | output_file                   |
   +---------------------------+-------------------------------+
   | Phase of Interest         | phase_of_interest             |
   +---------------------------+-------------------------------+

   .. py:method:: Execute(crystal_structures_array_path, gbcd_array_path, misorientation_rotation, output_file, phase_of_interest)

      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter gbcd_array_path: 5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere
      :param complex.VectorFloat32Parameter misorientation_rotation: Axis-Angle pair values for drawing GBCD
      :param complex.FileSystemPathParameter output_file: The output .dat file path
      :param complex.Int32Parameter phase_of_interest: Index of the Ensemble for which to plot the pole figure
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ExportGBCDTriangleDataFilter:
.. py:class:: ExportGBCDTriangleDataFilter

   **UI Display Name:** *Export GBCD Triangles File*

   This **Filter** writes relevant information about the Grain Boundary Character Distribution (GBCD) on an existing set of triangles.  The information written includes the inward and outward Euler angles, normals, and areas for each triangle.  The file format was originally defined by Prof. Greg Rohrer (CMU).

   `Link to the full online documentation for ExportGBCDTriangleDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ExportGBCDTriangleDataFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------+--------------------------------------+
   | UI Display           | Python Named Argument                |
   +======================+======================================+
   | Average Euler Angles | feature_euler_angles_array_path      |
   +----------------------+--------------------------------------+
   | Output File          | output_file                          |
   +----------------------+--------------------------------------+
   | Face Areas           | surface_mesh_face_areas_array_path   |
   +----------------------+--------------------------------------+
   | Face Labels          | surface_mesh_face_labels_array_path  |
   +----------------------+--------------------------------------+
   | Face Normals         | surface_mesh_face_normals_array_path |
   +----------------------+--------------------------------------+

   .. py:method:: Execute(feature_euler_angles_array_path, output_file, surface_mesh_face_areas_array_path, surface_mesh_face_labels_array_path, surface_mesh_face_normals_array_path)

      :param complex.ArraySelectionParameter feature_euler_angles_array_path: Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z).
      :param complex.FileSystemPathParameter output_file: The output GBCD triangle file path
      :param complex.ArraySelectionParameter surface_mesh_face_areas_array_path: Specifies the area of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Specifies which Features are on either side of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_normals_array_path: Specifies the normal of each Face
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindAvgCAxesFilter:
.. py:class:: FindAvgCAxesFilter

   **UI Display Name:** *Find Average C-Axis Orientations*

   This **Filter** determines the average C-axis location of each **Feature** by the following algorithm:

   `Link to the full online documentation for FindAvgCAxesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindAvgCAxesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Average C-Axes                | avg_c_axes_array_path         |
   +-------------------------------+-------------------------------+
   | Cell Feature Attribute Matrix | cell_feature_attribute_matrix |
   +-------------------------------+-------------------------------+
   | Phases                        | cell_phases_array_path        |
   +-------------------------------+-------------------------------+
   | Crystal Structures            | crystal_structures_array_path |
   +-------------------------------+-------------------------------+
   | Feature Ids                   | feature_ids_array_path        |
   +-------------------------------+-------------------------------+
   | Quaternions                   | quats_array_path              |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(avg_c_axes_array_path, cell_feature_attribute_matrix, cell_phases_array_path, crystal_structures_array_path, feature_ids_array_path, quats_array_path)

      :param complex.DataObjectNameParameter avg_c_axes_array_path: The output average C-Axis values for each feature
      :param complex.AttributeMatrixSelectionParameter cell_feature_attribute_matrix: The path to the cell feature attribute matrix
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.ArraySelectionParameter quats_array_path: Input quaternion array
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindAvgOrientationsFilter:
.. py:class:: FindAvgOrientationsFilter

   **UI Display Name:** *Find Feature Average Orientations*

   This **Filter** determines the average orientation of each **Feature** by the following algorithm:

   `Link to the full online documentation for FindAvgOrientationsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindAvgOrientationsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Average Euler Angles          | avg_euler_angles_array_path   |
   +-------------------------------+-------------------------------+
   | Average Quaternions           | avg_quats_array_path          |
   +-------------------------------+-------------------------------+
   | Cell Feature Attribute Matrix | cell_feature_attribute_matrix |
   +-------------------------------+-------------------------------+
   | Cell Feature Ids              | cell_feature_ids_array_path   |
   +-------------------------------+-------------------------------+
   | Cell Phases                   | cell_phases_array_path        |
   +-------------------------------+-------------------------------+
   | Cell Quaternions              | cell_quats_array_path         |
   +-------------------------------+-------------------------------+
   | Crystal Structures            | crystal_structures_array_path |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(avg_euler_angles_array_path, avg_quats_array_path, cell_feature_attribute_matrix, cell_feature_ids_array_path, cell_phases_array_path, cell_quats_array_path, crystal_structures_array_path)

      :param complex.DataObjectNameParameter avg_euler_angles_array_path: The name of the array specifying the orientation of each Feature in Bunge convention (Z-X-Z)
      :param complex.DataObjectNameParameter avg_quats_array_path: The name of the array specifying the average orientation of the Feature in quaternion representation
      :param complex.AttributeMatrixSelectionParameter cell_feature_attribute_matrix: The path to the cell feature attribute matrix
      :param complex.ArraySelectionParameter cell_feature_ids_array_path: Specifies to which Feature each Cell belongs.
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter cell_quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindBoundaryStrengthsFilter:
.. py:class:: FindBoundaryStrengthsFilter

   **UI Display Name:** *Find Feature Boundary Strength Metrics*

   This **Filter** calculates the same metrics as in the Find Neighbor Slip Transmission Metrics **Filter**.  However, this **Filter** stores the values in the **Face Attribute Matrix** of a **Triangle Geometry**.  The algorithm the **Filter** uses is as follows:

   `Link to the full online documentation for FindBoundaryStrengthsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindBoundaryStrengthsFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+-------------------------------------+
   | UI Display              | Python Named Argument               |
   +=========================+=====================================+
   | Average Quaternions     | avg_quats_array_path                |
   +-------------------------+-------------------------------------+
   | Crystal Structures      | crystal_structures_array_path       |
   +-------------------------+-------------------------------------+
   | Phases                  | feature_phases_array_path           |
   +-------------------------+-------------------------------------+
   | Loading Direction (XYZ) | loading                             |
   +-------------------------+-------------------------------------+
   | F1s                     | surface_mesh_f1s_array_name         |
   +-------------------------+-------------------------------------+
   | F1spts                  | surface_mesh_f1spts_array_name      |
   +-------------------------+-------------------------------------+
   | F7s                     | surface_mesh_f7s_array_name         |
   +-------------------------+-------------------------------------+
   | Face Labels             | surface_mesh_face_labels_array_path |
   +-------------------------+-------------------------------------+
   | mPrimes                 | surface_meshm_primes_array_name     |
   +-------------------------+-------------------------------------+

   .. py:method:: Execute(avg_quats_array_path, crystal_structures_array_path, feature_phases_array_path, loading, surface_mesh_f1s_array_name, surface_mesh_f1spts_array_name, surface_mesh_f7s_array_name, surface_mesh_face_labels_array_path, surface_meshm_primes_array_name)

      :param complex.ArraySelectionParameter avg_quats_array_path: Data Array that specifies the average orientation of each Feature in quaternion representation
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each phase
      :param complex.ArraySelectionParameter feature_phases_array_path: Data Array that specifies to which Ensemble each Feature belongs
      :param complex.VectorFloat64Parameter loading: The loading axis for the sample
      :param complex.DataObjectNameParameter surface_mesh_f1s_array_name: DataArray Name to store the calculated F1s Values
      :param complex.DataObjectNameParameter surface_mesh_f1spts_array_name: DataArray Name to store the calculated F1spts Values
      :param complex.DataObjectNameParameter surface_mesh_f7s_array_name: DataArray Name to store the calculated F7s Values
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Data Array that specifies which Features are on either side of each Face
      :param complex.DataObjectNameParameter surface_meshm_primes_array_name: DataArray Name to store the calculated mPrimes Values
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindCAxisLocationsFilter:
.. py:class:: FindCAxisLocationsFilter

   **UI Display Name:** *Find C-Axis Locations*

   This **Filter** determines the direction <u,v,w> of the C-axis for each **Element** by applying the quaternion of the **Element** to the <001> direction, which is the C-axis for *Hexagonal* materials.  This will tell where the C-axis of the **Element** sits in the *sample reference frame*.

   `Link to the full online documentation for FindCAxisLocationsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindCAxisLocationsFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------+-------------------------------+
   | UI Display         | Python Named Argument         |
   +====================+===============================+
   | C-Axis Locations   | c_axis_locations_array_name   |
   +--------------------+-------------------------------+
   | Phases             | cell_phases_array_path        |
   +--------------------+-------------------------------+
   | Crystal Structures | crystal_structures_array_path |
   +--------------------+-------------------------------+
   | Quaternions        | quats_array_path              |
   +--------------------+-------------------------------+

   .. py:method:: Execute(c_axis_locations_array_name, cell_phases_array_path, crystal_structures_array_path, quats_array_path)

      :param complex.DataObjectNameParameter c_axis_locations_array_name: DataPath to calculated C-Axis locations
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter quats_array_path: DataPath to input quaternion values
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeatureNeighborCAxisMisalignmentsFilter:
.. py:class:: FindFeatureNeighborCAxisMisalignmentsFilter

   **UI Display Name:** *Find Feature Neighbor C-Axis Misalignments*

   This **Filter** determines, for each **Feature**, the C-axis misalignments with the **Features** that are in contact with it.  The C-axis misalignments are stored as a list (for each **Feature**) of angles (in degrees).

   `Link to the full online documentation for FindFeatureNeighborCAxisMisalignmentsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeatureNeighborCAxisMisalignmentsFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------------------+-------------------------------------+
   | UI Display                            | Python Named Argument               |
   +=======================================+=====================================+
   | Average C-Axis Misalignments          | avg_c_axis_misalignments_array_name |
   +---------------------------------------+-------------------------------------+
   | Average Quaternions                   | avg_quats_array_path                |
   +---------------------------------------+-------------------------------------+
   | C-Axis Misalignment List              | c_axis_misalignment_list_array_name |
   +---------------------------------------+-------------------------------------+
   | Crystal Structures                    | crystal_structures_array_path       |
   +---------------------------------------+-------------------------------------+
   | Phases                                | feature_phases_array_path           |
   +---------------------------------------+-------------------------------------+
   | Find Average Misalignment Per Feature | find_avg_misals                     |
   +---------------------------------------+-------------------------------------+
   | Neighbor List                         | neighbor_list_array_path            |
   +---------------------------------------+-------------------------------------+

   .. py:method:: Execute(avg_c_axis_misalignments_array_name, avg_quats_array_path, c_axis_misalignment_list_array_name, crystal_structures_array_path, feature_phases_array_path, find_avg_misals, neighbor_list_array_path)

      :param complex.DataObjectNameParameter avg_c_axis_misalignments_array_name: Number weighted average of neighbor C-axis misalignments. Only created if Find Average Misalignment Per Feature is checked
      :param complex.ArraySelectionParameter avg_quats_array_path: Defines the average orientation of the Feature in quaternion representation
      :param complex.DataObjectNameParameter c_axis_misalignment_list_array_name: List of the C-axis misalignment angles (in degrees) with the contiguous neighboring Features for a given Feature
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which Ensemble each **Feature** belongs
      :param complex.BoolParameter find_avg_misals: Whether the average of the C-axis misalignments with the neighboring Features should be stored for each Feature
      :param complex.NeighborListSelectionParameter neighbor_list_array_path: List of the contiguous neighboring Features for a given Feature
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeatureReferenceCAxisMisorientationsFilter:
.. py:class:: FindFeatureReferenceCAxisMisorientationsFilter

   **UI Display Name:** *Find Feature Reference C-Axis Misalignments*

   This **Filter** calculates the misorientation angle between the C-axis of each **Cell** within a **Feature** and the average C-axis for that **Feature** and stores that value for each **Cell**.  The average and standard deviation of those values for all **Cells** belonging to the same **Feature** is also stored for each **Feature**.

   `Link to the full online documentation for FindFeatureReferenceCAxisMisorientationsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeatureReferenceCAxisMisorientationsFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------------+-----------------------------------------------------+
   | UI Display                               | Python Named Argument                               |
   +==========================================+=====================================================+
   | Average C-Axes                           | avg_c_axes_array_path                               |
   +------------------------------------------+-----------------------------------------------------+
   | Phases                                   | cell_phases_array_path                              |
   +------------------------------------------+-----------------------------------------------------+
   | Crystal Structures                       | crystal_structures_array_path                       |
   +------------------------------------------+-----------------------------------------------------+
   | Average C-Axis Misorientations           | feature_avg_c_axis_misorientations_array_name       |
   +------------------------------------------+-----------------------------------------------------+
   | Feature Ids                              | feature_ids_array_path                              |
   +------------------------------------------+-----------------------------------------------------+
   | Feature Reference C-Axis Misorientations | feature_reference_c_axis_misorientations_array_name |
   +------------------------------------------+-----------------------------------------------------+
   | Feature Stdev C-Axis Misorientations     | feature_stdev_c_axis_misorientations_array_name     |
   +------------------------------------------+-----------------------------------------------------+
   | Image Geometry                           | image_geometry_path                                 |
   +------------------------------------------+-----------------------------------------------------+
   | Quaternions                              | quats_array_path                                    |
   +------------------------------------------+-----------------------------------------------------+

   .. py:method:: Execute(avg_c_axes_array_path, cell_phases_array_path, crystal_structures_array_path, feature_avg_c_axis_misorientations_array_name, feature_ids_array_path, feature_reference_c_axis_misorientations_array_name, feature_stdev_c_axis_misorientations_array_name, image_geometry_path, quats_array_path)

      :param complex.ArraySelectionParameter avg_c_axes_array_path: The direction of the Feature's C-axis in the sample reference frame
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.DataObjectNameParameter feature_avg_c_axis_misorientations_array_name: Average of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature
      :param complex.ArraySelectionParameter feature_ids_array_path: Data Array that specifies to which Feature each Element belongs
      :param complex.DataObjectNameParameter feature_reference_c_axis_misorientations_array_name: Misorientation angle (in degrees) between Cell's C-axis and the C-axis of the Feature that owns that Cell
      :param complex.DataObjectNameParameter feature_stdev_c_axis_misorientations_array_name: Standard deviation of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature
      :param complex.GeometrySelectionParameter image_geometry_path: The path to the input image geometry
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindFeatureReferenceMisorientationsFilter:
.. py:class:: FindFeatureReferenceMisorientationsFilter

   **UI Display Name:** *Find Feature Reference Misorientations*

   This **Filter** calculates the misorientation angle between each **Cell** within a **Feature** and a *reference orientation* for that **Feature**.  The user can choose the *reference orientation* to be used for the **Features** from a drop-down menu.  The options for the *reference orientation* are the average orientation of the **Feature** or the orientation of the **Cell** that is furthest from the *boundary* of the **Feature**.

   `Link to the full online documentation for FindFeatureReferenceMisorientationsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindFeatureReferenceMisorientationsFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------+----------------------------------------------+
   | UI Display                        | Python Named Argument                        |
   +===================================+==============================================+
   | Average Quaternions               | avg_quats_array_path                         |
   +-----------------------------------+----------------------------------------------+
   | Cell Feature Attribute Matrix     | cell_feature_attribute_matrix_path           |
   +-----------------------------------+----------------------------------------------+
   | Cell Phases                       | cell_phases_array_path                       |
   +-----------------------------------+----------------------------------------------+
   | Crystal Structures                | crystal_structures_array_path                |
   +-----------------------------------+----------------------------------------------+
   | Average Misorientations           | feature_avg_misorientations_array_name       |
   +-----------------------------------+----------------------------------------------+
   | Cell Feature Ids                  | feature_ids_path                             |
   +-----------------------------------+----------------------------------------------+
   | Feature Reference Misorientations | feature_reference_misorientations_array_name |
   +-----------------------------------+----------------------------------------------+
   | Boundary Euclidean Distances      | g_beuclidean_distances_array_path            |
   +-----------------------------------+----------------------------------------------+
   | Quaternions                       | quats_array_path                             |
   +-----------------------------------+----------------------------------------------+
   | Reference Orientation             | reference_orientation                        |
   +-----------------------------------+----------------------------------------------+

   .. py:method:: Execute(avg_quats_array_path, cell_feature_attribute_matrix_path, cell_phases_array_path, crystal_structures_array_path, feature_avg_misorientations_array_name, feature_ids_path, feature_reference_misorientations_array_name, g_beuclidean_distances_array_path, quats_array_path, reference_orientation)

      :param complex.ArraySelectionParameter avg_quats_array_path: Specifies the average orientation of the Feature in quaternion representation (, w). Only required if the reference orientation is selected to be the average of the Feature
      :param complex.AttributeMatrixSelectionParameter cell_feature_attribute_matrix_path: The path to the cell feature attribute matrix
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.DataObjectNameParameter feature_avg_misorientations_array_name: The name of the array containing the average of the Feature reference misorientation values for all of the Cells that belong to the Feature
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.DataObjectNameParameter feature_reference_misorientations_array_name: The name of the array containing the misorientation angle (in degrees) between Cell's orientation and the reference orientation of the Feature that owns that Cell
      :param complex.ArraySelectionParameter g_beuclidean_distances_array_path: Distance the Cells are from the boundary of the Feature they belong to. Only required if the reference orientation is selected to be the orientation at the Feature centroid
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :param complex.ChoicesParameter reference_orientation: Specifies the reference orientation to use when comparing to each Cell
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindGBCDFilter:
.. py:class:: FindGBCDFilter

   **UI Display Name:** *Find GBCD*

   This **Filter** computes the 5D grain boundary character distribution (GBCD) for a **Triangle Geometry**, which is the relative area of grain boundary for a given misorientation and normal. The GBCD can be visualized by using either the **Write GBCD Pole Figure (GMT)** or the **Write GBCD Pole Figure (VTK)** **Filters**.

   `Link to the full online documentation for FindGBCDFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindGBCDFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+--------------------------------------+
   | UI Display                     | Python Named Argument                |
   +================================+======================================+
   | Crystal Structures             | crystal_structures_array_path        |
   +--------------------------------+--------------------------------------+
   | Face Ensemble Attribute Matrix | face_ensemble_attribute_matrix_name  |
   +--------------------------------+--------------------------------------+
   | Average Euler Angles           | feature_euler_angles_array_path      |
   +--------------------------------+--------------------------------------+
   | Phases                         | feature_phases_array_path            |
   +--------------------------------+--------------------------------------+
   | GBCD                           | gbcd_array_name                      |
   +--------------------------------+--------------------------------------+
   | GBCD Spacing (Degrees)         | gbcd_resolution                      |
   +--------------------------------+--------------------------------------+
   | Face Areas                     | surface_mesh_face_areas_array_path   |
   +--------------------------------+--------------------------------------+
   | Face Labels                    | surface_mesh_face_labels_array_path  |
   +--------------------------------+--------------------------------------+
   | Face Normals                   | surface_mesh_face_normals_array_path |
   +--------------------------------+--------------------------------------+
   | Triangle Geometry              | triangle_geometry                    |
   +--------------------------------+--------------------------------------+

   .. py:method:: Execute(crystal_structures_array_path, face_ensemble_attribute_matrix_name, feature_euler_angles_array_path, feature_phases_array_path, gbcd_array_name, gbcd_resolution, surface_mesh_face_areas_array_path, surface_mesh_face_labels_array_path, surface_mesh_face_normals_array_path, triangle_geometry)

      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.DataObjectNameParameter face_ensemble_attribute_matrix_name: The name of the created face ensemble attribute matrix
      :param complex.ArraySelectionParameter feature_euler_angles_array_path: Array specifying three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which phase each Feature belongs
      :param complex.DataObjectNameParameter gbcd_array_name: 5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere
      :param complex.Float32Parameter gbcd_resolution: The resolution in degrees for the GBCD calculation
      :param complex.ArraySelectionParameter surface_mesh_face_areas_array_path: Array specifying the area of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Array specifying which Features are on either side of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_normals_array_path: Array specifying the normal of each Face
      :param complex.GeometrySelectionParameter triangle_geometry: Path to the triangle geometry for which to calculate the GBCD
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindGBCDMetricBasedFilter:
.. py:class:: FindGBCDMetricBasedFilter

   **UI Display Name:** *Find GBCD (Metric-Based Approach)*

   This **Filter** computes a section through the five-dimensional grain boundary distirbution for a fixed misorientation. An example of such a section is shown in Fig. 1. Differently than **Find GBCD Filter**, which uses a method based on partition of the boundary space into bins, this **Filter** implements an alternative metric-based approach described by K. Glowinski and A. Morawiec in `Analysis of experimental grain boundary distributions based on boundary-space metrics, Metall. Mater. Trans. A 45, 3189-3194 (2014) <https://link.springer.com/article/10.1007/s11661-014-2325-y>`_

   `Link to the full online documentation for FindGBCDMetricBasedFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindGBCDMetricBasedFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------------------------+---------------------------------------------+
   | UI Display                                            | Python Named Argument                       |
   +=======================================================+=============================================+
   | Limiting Distances                                    | chosen_limit_dists                          |
   +-------------------------------------------------------+---------------------------------------------+
   | Crystal Structures                                    | crystal_structures_array_path               |
   +-------------------------------------------------------+---------------------------------------------+
   | Output Distribution File                              | dist_output_file                            |
   +-------------------------------------------------------+---------------------------------------------+
   | Output Distribution Errors File                       | err_output_file                             |
   +-------------------------------------------------------+---------------------------------------------+
   | Exclude Triangles Directly Neighboring Triple Lines   | exclude_triple_lines                        |
   +-------------------------------------------------------+---------------------------------------------+
   | Average Euler Angles                                  | feature_euler_angles_array_path             |
   +-------------------------------------------------------+---------------------------------------------+
   | Phases                                                | feature_phases_array_path                   |
   +-------------------------------------------------------+---------------------------------------------+
   | Fixed Misorientation                                  | misorientation_rotation                     |
   +-------------------------------------------------------+---------------------------------------------+
   | Node Types                                            | node_types_array_path                       |
   +-------------------------------------------------------+---------------------------------------------+
   | Number of Sampling Points (on a Hemisphere)           | num_sampl_pts                               |
   +-------------------------------------------------------+---------------------------------------------+
   | Phase of Interest                                     | phase_of_interest                           |
   +-------------------------------------------------------+---------------------------------------------+
   | Save Relative Errors Instead of Their Absolute Values | save_relative_err                           |
   +-------------------------------------------------------+---------------------------------------------+
   | Face Areas                                            | surface_mesh_face_areas_array_path          |
   +-------------------------------------------------------+---------------------------------------------+
   | Face Labels                                           | surface_mesh_face_labels_array_path         |
   +-------------------------------------------------------+---------------------------------------------+
   | Face Normals                                          | surface_mesh_face_normals_array_path        |
   +-------------------------------------------------------+---------------------------------------------+
   | Feature Face Labels                                   | surface_mesh_feature_face_labels_array_path |
   +-------------------------------------------------------+---------------------------------------------+
   | Triangle Geometry                                     | triangle_geometry_path                      |
   +-------------------------------------------------------+---------------------------------------------+

   .. py:method:: Execute(chosen_limit_dists, crystal_structures_array_path, dist_output_file, err_output_file, exclude_triple_lines, feature_euler_angles_array_path, feature_phases_array_path, misorientation_rotation, node_types_array_path, num_sampl_pts, phase_of_interest, save_relative_err, surface_mesh_face_areas_array_path, surface_mesh_face_labels_array_path, surface_mesh_face_normals_array_path, surface_mesh_feature_face_labels_array_path, triangle_geometry_path)

      :param complex.ChoicesParameter chosen_limit_dists: The max angles from within which boundary segments are selected for the misorientations and plane inclinations
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.FileSystemPathParameter dist_output_file: The output distribution file path (extension .dat, GMT format)
      :param complex.FileSystemPathParameter err_output_file: The output distribution errors file path (extension .dat, GMT format)
      :param complex.BoolParameter exclude_triple_lines: If checked, only interiors of Faces are included in GBCD
      :param complex.ArraySelectionParameter feature_euler_angles_array_path: Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which phase each Feature belongs
      :param complex.VectorFloat32Parameter misorientation_rotation: Axis-angle representation of the misorientation of interest. Angle value should be in degrees.
      :param complex.ArraySelectionParameter node_types_array_path: Specifies the type of node in the Geometry
      :param complex.Int32Parameter num_sampl_pts: The approximate number of sampling directions
      :param complex.Int32Parameter phase_of_interest: Index of the Ensemble for which to compute GBCD; boundaries having grains of this phase on both its sides will only be taken into account
      :param complex.BoolParameter save_relative_err: Whether or not to save the distribution errors as relative (if exceeds 100%, then rounded down) or absolute
      :param complex.ArraySelectionParameter surface_mesh_face_areas_array_path: Specifies the area of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Specifies which Features are on either side of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_normals_array_path: Specifies the normal of each Face
      :param complex.ArraySelectionParameter surface_mesh_feature_face_labels_array_path: Specifies which original Features are on either side of each boundary Feature
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the triangle geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindGBPDMetricBasedFilter:
.. py:class:: FindGBPDMetricBasedFilter

   **UI Display Name:** *Find GBPD (Metric-Based Approach)*

   This **Filter** computes the grain boundary plane distribution (GBPD) like that shown in Fig. 1. It should be noted that most GBPDs presented so far in literature were obtained using a method based on partition of the grain boundary space into bins, similar to that implemented in the *Find GBCD* **Filter**. This **Filter** calculates the GBPD using an alternative approach adapted from the one included in the *Find GBCD (Metric-based Approach)* **Filter** and described by K. Glowinski and A. Morawiec in `Analysis of experimental grain boundary distributions based on boundary-space metrics, Metall. Mater. Trans. A 45, 3189-3194 (2014) <http://link.springer.com/article/10.1007%2Fs11661-014-2325-y>`_. Briefly, the GBPD is probed at evenly distributed sampling directions (similarly to *Find GBCD (Metric-based Approach)* **Filter**) and areas of mesh segments with their normal vectors deviated by less than a limiting angle &rho;<sub>p</sub>  from a given direction are summed. If *n*<sub>S</sub> is the number of crystal symmetry transformations, each boundary plane segment is represented by up to 4 &times; *n*<sub>S</sub> equivalent vectors, and all of them are processed. It is enough to sample the distribution at directions corresponding to the standard stereographic triangle (or, in general, to a fundamental region corresponding to a considered crystallographic point group); values at remaining points are obtained based on crystal symmetries. After summing the boundary areas, the distribution is normalized. First, the values at sampling vectors are divided by the total area of all segments. Then, in order to express the distribution in the conventional units, i.e., multiples of random distribution (MRDs), the obtained fractional values are divided by the volume *v* = (*A* n<sub>S</sub>) / (4&pi;), where *A* is the area of a spherical cap determined by &rho;<sub>p</sub>.

   `Link to the full online documentation for FindGBPDMetricBasedFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindGBPDMetricBasedFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------------------------+---------------------------------------------+
   | UI Display                                            | Python Named Argument                       |
   +=======================================================+=============================================+
   | Crystal Structures                                    | crystal_structures_array_path               |
   +-------------------------------------------------------+---------------------------------------------+
   | Output Distribution File                              | dist_output_file                            |
   +-------------------------------------------------------+---------------------------------------------+
   | Output Distribution Errors File                       | err_output_file                             |
   +-------------------------------------------------------+---------------------------------------------+
   | Exclude Triangles Directly Neighboring Triple Lines   | exclude_triple_lines                        |
   +-------------------------------------------------------+---------------------------------------------+
   | Average Euler Angles                                  | feature_euler_angles_array_path             |
   +-------------------------------------------------------+---------------------------------------------+
   | Phases                                                | feature_phases_array_path                   |
   +-------------------------------------------------------+---------------------------------------------+
   | Limiting Distance [deg.]                              | limit_dist                                  |
   +-------------------------------------------------------+---------------------------------------------+
   | Node Types                                            | node_types_array_path                       |
   +-------------------------------------------------------+---------------------------------------------+
   | Number of Sampling Points (on a Hemisphere)           | num_sampl_pts                               |
   +-------------------------------------------------------+---------------------------------------------+
   | Phase of Interest                                     | phase_of_interest                           |
   +-------------------------------------------------------+---------------------------------------------+
   | Save Relative Errors Instead of Their Absolute Values | save_relative_err                           |
   +-------------------------------------------------------+---------------------------------------------+
   | Face Areas                                            | surface_mesh_face_areas_array_path          |
   +-------------------------------------------------------+---------------------------------------------+
   | Face Labels                                           | surface_mesh_face_labels_array_path         |
   +-------------------------------------------------------+---------------------------------------------+
   | Face Normals                                          | surface_mesh_face_normals_array_path        |
   +-------------------------------------------------------+---------------------------------------------+
   | Feature Face Labels                                   | surface_mesh_feature_face_labels_array_path |
   +-------------------------------------------------------+---------------------------------------------+
   | Triangle Geometry                                     | triangle_geometry_path                      |
   +-------------------------------------------------------+---------------------------------------------+

   .. py:method:: Execute(crystal_structures_array_path, dist_output_file, err_output_file, exclude_triple_lines, feature_euler_angles_array_path, feature_phases_array_path, limit_dist, node_types_array_path, num_sampl_pts, phase_of_interest, save_relative_err, surface_mesh_face_areas_array_path, surface_mesh_face_labels_array_path, surface_mesh_face_normals_array_path, surface_mesh_feature_face_labels_array_path, triangle_geometry_path)

      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.FileSystemPathParameter dist_output_file: The output distribution file path (extension .dat, GMT format)
      :param complex.FileSystemPathParameter err_output_file: The output distribution errors file path (extension .dat, GMT format)
      :param complex.BoolParameter exclude_triple_lines: If checked, only interiors of Faces are included in GBCD
      :param complex.ArraySelectionParameter feature_euler_angles_array_path: Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which phase each Feature belongs
      :param complex.Float32Parameter limit_dist: The max angle from within which mesh segments are selected
      :param complex.ArraySelectionParameter node_types_array_path: Specifies the type of node in the Geometry
      :param complex.Int32Parameter num_sampl_pts: The approximate number of sampling directions
      :param complex.Int32Parameter phase_of_interest: Index of the Ensemble for which to compute GBPD; boundaries having grains of this phase on both its sides will only be taken into account
      :param complex.BoolParameter save_relative_err: Whether or not to save the distribution errors as relative (if exceeds 100%, then rounded down) or absolute
      :param complex.ArraySelectionParameter surface_mesh_face_areas_array_path: Specifies the area of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Specifies which Features are on either side of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_normals_array_path: Specifies the normal of each Face
      :param complex.ArraySelectionParameter surface_mesh_feature_face_labels_array_path: Specifies which original Features are on either side of each boundary Feature
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the triangle geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindKernelAvgMisorientationsFilter:
.. py:class:: FindKernelAvgMisorientationsFilter

   **UI Display Name:** *Find Kernel Average Misorientations*

   This **Filter** determines the Kernel Average Misorientation (KAM) for each **Cell**.  The user can select the size of the kernel to be used in the calculation.  The kernel size entered by the user is the *radius* of the kernel (i.e., entering values of *1*, *2*, *3* will result in a kernel that is *3*, *5*, and *7* **Cells** in size in the X, Y and Z directions, respectively).  The algorithm for determination of KAM is as follows:

   `Link to the full online documentation for FindKernelAvgMisorientationsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindKernelAvgMisorientationsFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+-------------------------------------------+
   | UI Display                     | Python Named Argument                     |
   +================================+===========================================+
   | Cell Phases                    | cell_phases_array_path                    |
   +--------------------------------+-------------------------------------------+
   | Crystal Structures             | crystal_structures_array_path             |
   +--------------------------------+-------------------------------------------+
   | Cell Feature Ids               | feature_ids_path                          |
   +--------------------------------+-------------------------------------------+
   | Kernel Average Misorientations | kernel_average_misorientations_array_name |
   +--------------------------------+-------------------------------------------+
   | Kernel Radius                  | kernel_size                               |
   +--------------------------------+-------------------------------------------+
   | Quaternions                    | quats_array_path                          |
   +--------------------------------+-------------------------------------------+
   | Selected Image Geometry        | selected_image_geometry_path              |
   +--------------------------------+-------------------------------------------+

   .. py:method:: Execute(cell_phases_array_path, crystal_structures_array_path, feature_ids_path, kernel_average_misorientations_array_name, kernel_size, quats_array_path, selected_image_geometry_path)

      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.DataObjectNameParameter kernel_average_misorientations_array_name: The name of the array containing the average  misorientation (in Degrees) for all Cells within the kernel and the central Cell
      :param complex.VectorInt32Parameter kernel_size: Size of the kernel in the X, Y and Z directions (in number of Cells)
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :param complex.GeometrySelectionParameter selected_image_geometry_path: Path to the target geometry
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindMisorientationsFilter:
.. py:class:: FindMisorientationsFilter

   **UI Display Name:** *Find Feature Neighbor Misorientations*

   This **Filter** determines, for each **Feature**, the misorientations with each of the **Features** that are in contact with it.  The misorientations are stored as a list (for each **Feature**) of angles (in degrees).  The axis of the misorientation is not stored by this **Filter**.

   `Link to the full online documentation for FindMisorientationsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindMisorientationsFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------------+--------------------------------+
   | UI Display                              | Python Named Argument          |
   +=========================================+================================+
   | Average Misorientations                 | avg_misorientations_array_name |
   +-----------------------------------------+--------------------------------+
   | Feature Average Quaternions             | avg_quats_array_path           |
   +-----------------------------------------+--------------------------------+
   | Crystal Structures                      | crystal_structures_array_path  |
   +-----------------------------------------+--------------------------------+
   | Feature Phases                          | feature_phases_array_path      |
   +-----------------------------------------+--------------------------------+
   | Find Average Misorientation Per Feature | find_avg_misors                |
   +-----------------------------------------+--------------------------------+
   | Misorientation List                     | misorientation_list_array_name |
   +-----------------------------------------+--------------------------------+
   | Feature Neighbor List                   | neighbor_list_array_path       |
   +-----------------------------------------+--------------------------------+

   .. py:method:: Execute(avg_misorientations_array_name, avg_quats_array_path, crystal_structures_array_path, feature_phases_array_path, find_avg_misors, misorientation_list_array_name, neighbor_list_array_path)

      :param complex.DataObjectNameParameter avg_misorientations_array_name: The name of the array containing the number weighted average of neighbor misorientations.
      :param complex.ArraySelectionParameter avg_quats_array_path: Defines the average orientation of the Feature in quaternion representation
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which Ensemble each Feature belongs
      :param complex.BoolParameter find_avg_misors: Specifies if the average of the misorienations with the neighboring Features should be stored for each Feature
      :param complex.DataObjectNameParameter misorientation_list_array_name: The name of the data object containing the list of the misorientation angles with the contiguous neighboring Features for a given Feature
      :param complex.NeighborListSelectionParameter neighbor_list_array_path: List of the contiguous neighboring Features for a given Feature
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindSchmidsFilter:
.. py:class:: FindSchmidsFilter

   **UI Display Name:** *Find Schmid Factors*

   This **Filter** calculates the Schmid factor of each **Feature** given its average orientation and a user defined loading axis. The Schmid Factor is the combination of the component of the axial force *F* that lies parallel to the slip direction and the component that lies perpendicular to the slip plane.  The equation for the Schmid Factor is given as:

   `Link to the full online documentation for FindSchmidsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindSchmidsFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------------------------+-------------------------------+
   | UI Display                              | Python Named Argument         |
   +=========================================+===============================+
   | Average Quaternions                     | avg_quats_array_path          |
   +-----------------------------------------+-------------------------------+
   | Crystal Structures                      | crystal_structures_array_path |
   +-----------------------------------------+-------------------------------+
   | Phases                                  | feature_phases_array_path     |
   +-----------------------------------------+-------------------------------+
   | Lambdas                                 | lambdas_array_name            |
   +-----------------------------------------+-------------------------------+
   | Loading Direction                       | loading_direction             |
   +-----------------------------------------+-------------------------------+
   | Override Default Slip System            | override_system               |
   +-----------------------------------------+-------------------------------+
   | Phis                                    | phis_array_name               |
   +-----------------------------------------+-------------------------------+
   | Poles                                   | poles_array_name              |
   +-----------------------------------------+-------------------------------+
   | Schmids                                 | schmids_array_name            |
   +-----------------------------------------+-------------------------------+
   | Slip Direction                          | slip_direction                |
   +-----------------------------------------+-------------------------------+
   | Slip Plane                              | slip_plane                    |
   +-----------------------------------------+-------------------------------+
   | Slip Systems                            | slip_systems_array_name       |
   +-----------------------------------------+-------------------------------+
   | Store Angle Components of Schmid Factor | store_angle_components        |
   +-----------------------------------------+-------------------------------+

   .. py:method:: Execute(avg_quats_array_path, crystal_structures_array_path, feature_phases_array_path, lambdas_array_name, loading_direction, override_system, phis_array_name, poles_array_name, schmids_array_name, slip_direction, slip_plane, slip_systems_array_name, store_angle_components)

      :param complex.ArraySelectionParameter avg_quats_array_path: Specifies the average orienation of each Feature in quaternion representation
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which Ensemble each cell belongs
      :param complex.DataObjectNameParameter lambdas_array_name: The name of the array containing the angle between tensile axis and slip direction.
      :param complex.VectorFloat32Parameter loading_direction: The loading axis for the sample
      :param complex.BoolParameter override_system: Allows the user to manually input the slip plane and slip direction
      :param complex.DataObjectNameParameter phis_array_name: The name of the array containing the angle between tensile axis and slip plane normal. 
      :param complex.DataObjectNameParameter poles_array_name: The name of the array specifying the crystallographic pole that points along the user defined loading direction
      :param complex.DataObjectNameParameter schmids_array_name: The name of the array containing the value of the Schmid factor for the most favorably oriented slip system (i.e., the one with the highest Schmid factor)
      :param complex.VectorFloat32Parameter slip_direction: Vector defining the slip direction.
      :param complex.VectorFloat32Parameter slip_plane: Vector defining the slip plane normal.
      :param complex.DataObjectNameParameter slip_systems_array_name: The name of the array containing the enumeration of the slip system that has the highest Schmid factor
      :param complex.BoolParameter store_angle_components: Whether to store the angle components for each Feature
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindShapesFilter:
.. py:class:: FindShapesFilter

   **UI Display Name:** *Find Feature Shapes*

   This **Filter** calculates the second-order moments of each **Feature** in order to determine the *principal axis lengths, principal axis directions, aspect ratios and moment invariant Omega3s*.  The *principal axis lengths* are those of a "best-fit" ellipsoid.  The algorithm for determining the moments and these values is as follows:

   `Link to the full online documentation for FindShapesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindShapesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------+------------------------------+
   | UI Display              | Python Named Argument        |
   +=========================+==============================+
   | Aspect Ratios           | aspect_ratios_array_name     |
   +-------------------------+------------------------------+
   | Axis Euler Angles       | axis_euler_angles_array_name |
   +-------------------------+------------------------------+
   | Axis Lengths            | axis_lengths_array_name      |
   +-------------------------+------------------------------+
   | Feature Centroids       | centroids_array_path         |
   +-------------------------+------------------------------+
   | Cell Feature Ids        | feature_ids_path             |
   +-------------------------+------------------------------+
   | Omega3s                 | omega3s_array_name           |
   +-------------------------+------------------------------+
   | Selected Image Geometry | selected_image_geometry      |
   +-------------------------+------------------------------+
   | Volumes                 | volumes_array_name           |
   +-------------------------+------------------------------+

   .. py:method:: Execute(aspect_ratios_array_name, axis_euler_angles_array_name, axis_lengths_array_name, centroids_array_path, feature_ids_path, omega3s_array_name, selected_image_geometry, volumes_array_name)

      :param complex.DataObjectNameParameter aspect_ratios_array_name: Ratio of semi-axis lengths (b/a and c/a) for best-fit ellipsoid to Feature
      :param complex.DataObjectNameParameter axis_euler_angles_array_name: Euler angles (in radians) necessary to rotate the sample reference frame to the reference frame of the Feature, where the principal axes of the best-fit ellipsoid are (X, Y, Z)
      :param complex.DataObjectNameParameter axis_lengths_array_name: Semi-axis lengths (a, b, c) for best-fit ellipsoid to Feature
      :param complex.ArraySelectionParameter centroids_array_path: X, Y, Z coordinates of Feature center of mass
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each Cell belongs
      :param complex.DataObjectNameParameter omega3s_array_name: 3rd invariant of the second-order moment matrix for the Feature, does not assume a shape type (i.e., ellipsoid)
      :param complex.GeometrySelectionParameter selected_image_geometry: The target geometry
      :param complex.DataObjectNameParameter volumes_array_name: The volume of each Feature
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindSlipTransmissionMetricsFilter:
.. py:class:: FindSlipTransmissionMetricsFilter

   **UI Display Name:** *Find Neighbor Slip Transmission Metrics*

   This **Filter** calculates a suite of *slip transmission metrics* that are related to the alignment of slip directions and planes across **Feature** boundaries.  The algorithm for calculation of these metrics is as follows:

   `Link to the full online documentation for FindSlipTransmissionMetricsFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindSlipTransmissionMetricsFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------+-------------------------------+
   | UI Display          | Python Named Argument         |
   +=====================+===============================+
   | Average Quaternions | avg_quats_array_path          |
   +---------------------+-------------------------------+
   | Crystal Structures  | crystal_structures_array_path |
   +---------------------+-------------------------------+
   | F1 List             | f1_list_array_name            |
   +---------------------+-------------------------------+
   | F1spt List          | f1spt_list_array_name         |
   +---------------------+-------------------------------+
   | F7 List             | f7_list_array_name            |
   +---------------------+-------------------------------+
   | Phases              | feature_phases_array_path     |
   +---------------------+-------------------------------+
   | mPrime List         | m_prime_list_array_name       |
   +---------------------+-------------------------------+
   | Neighbor List       | neighbor_list_array_path      |
   +---------------------+-------------------------------+

   .. py:method:: Execute(avg_quats_array_path, crystal_structures_array_path, f1_list_array_name, f1spt_list_array_name, f7_list_array_name, feature_phases_array_path, m_prime_list_array_name, neighbor_list_array_path)

      :param complex.ArraySelectionParameter avg_quats_array_path: Data Array that specifies the average orientation of each Feature in quaternion representation
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each phase
      :param complex.DataObjectNameParameter f1_list_array_name: DataArray Name to store the calculated F1s Values
      :param complex.DataObjectNameParameter f1spt_list_array_name: DataArray Name to store the calculated F1spts Values
      :param complex.DataObjectNameParameter f7_list_array_name: DataArray Name to store the calculated F7s Values
      :param complex.ArraySelectionParameter feature_phases_array_path: Data Array that specifies to which Ensemble each Feature belongs
      :param complex.DataObjectNameParameter m_prime_list_array_name: DataArray Name to store the calculated mPrimes Values
      :param complex.NeighborListSelectionParameter neighbor_list_array_path: List of the contiguous neighboring Features for a given Feature
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _FindTriangleGeomShapesFilter:
.. py:class:: FindTriangleGeomShapesFilter

   **UI Display Name:** *Find Feature Shapes from Triangle Geometry*

   This **Filter** calculates the second-order moments of each enclosed **Feature** in a **Triangle Geometry**. Thesecond-order moments allow for the determination of the *principal axis lengths, principal axis directions, aspectratios and moment invariant Omega3s*. The *principal axis lengths* are those of a "best-fit" ellipsoid. The algorithmfor determining the moments and these values is as follows:

   `Link to the full online documentation for FindTriangleGeomShapesFilter <http://www.dream3d.io/nx_reference_manual/Filters/FindTriangleGeomShapesFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Aspect Ratios                 | aspect_ratios_array_name      |
   +-------------------------------+-------------------------------+
   | Axis Euler Angles             | axis_euler_angles_array_name  |
   +-------------------------------+-------------------------------+
   | Axis Lengths                  | axis_lengths_array_name       |
   +-------------------------------+-------------------------------+
   | Face Feature Centroids        | centroids_array_path          |
   +-------------------------------+-------------------------------+
   | Face Labels                   | face_labels_array_path        |
   +-------------------------------+-------------------------------+
   | Face Feature Attribute Matrix | feature_attribute_matrix_name |
   +-------------------------------+-------------------------------+
   | Omega3s                       | omega3s_array_name            |
   +-------------------------------+-------------------------------+
   | Triangle Geometry             | triangle_geometry_path        |
   +-------------------------------+-------------------------------+
   | Face Feature Volumes          | volumes_array_path            |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(aspect_ratios_array_name, axis_euler_angles_array_name, axis_lengths_array_name, centroids_array_path, face_labels_array_path, feature_attribute_matrix_name, omega3s_array_name, triangle_geometry_path, volumes_array_path)

      :param complex.DataObjectNameParameter aspect_ratios_array_name: The name of the DataArray that holds the calculated Aspect Ratios values
      :param complex.DataObjectNameParameter axis_euler_angles_array_name: The name of the DataArray that holds the calculated Axis Euler Angles values
      :param complex.DataObjectNameParameter axis_lengths_array_name: The name of the DataArray that holds the calculated Axis Lengths values
      :param complex.ArraySelectionParameter centroids_array_path: Input DataPath to the **Feature Centroids** for the face data
      :param complex.ArraySelectionParameter face_labels_array_path: The DataPath to the FaceLabels values.
      :param complex.DataGroupSelectionParameter feature_attribute_matrix_name: The DataPath to the AttributeMatrix that holds feature data for the faces
      :param complex.DataObjectNameParameter omega3s_array_name: The name of the DataArray that holds the calculated Omega3 values
      :param complex.GeometrySelectionParameter triangle_geometry_path: The complete path to the Geometry for which to calculate the normals
      :param complex.ArraySelectionParameter volumes_array_path: Input DataPath to the **Feature Volumes** for the face data
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateFZQuaternions:
.. py:class:: GenerateFZQuaternions

   **UI Display Name:** *Reduce Orientations to Fundamental Zone*

   This **Filter** reduces input orientations (Quaternions) into the fundamental zone for the given Laue group.

   `Link to the full online documentation for GenerateFZQuaternions <http://www.dream3d.io/nx_reference_manual/Filters/GenerateFZQuaternions>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------------------------+-------------------------------+
   | UI Display                                               | Python Named Argument         |
   +==========================================================+===============================+
   | Input Phases                                             | cell_phases_array_path        |
   +----------------------------------------------------------+-------------------------------+
   | Crystal Structures                                       | crystal_structures_array_path |
   +----------------------------------------------------------+-------------------------------+
   | Created FZ Quaternions                                   | f_zquats_array_path           |
   +----------------------------------------------------------+-------------------------------+
   | Input Mask [Optional]                                    | good_voxels_array_path        |
   +----------------------------------------------------------+-------------------------------+
   | Input Quaternions                                        | quats_array_path              |
   +----------------------------------------------------------+-------------------------------+
   | Apply to Good Elements Only (Bad Elements Will Be Black) | use_good_voxels               |
   +----------------------------------------------------------+-------------------------------+

   .. py:method:: Execute(cell_phases_array_path, crystal_structures_array_path, f_zquats_array_path, good_voxels_array_path, quats_array_path, use_good_voxels)

      :param complex.ArraySelectionParameter cell_phases_array_path: The phases of the data. The data should be the indices into the Crystal Structures Data Array.
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.DataObjectNameParameter f_zquats_array_path: The name of the array containing the Quaternion that represents an orientation within the fundamental zone for each Element
      :param complex.ArraySelectionParameter good_voxels_array_path: Optional Mask array where valid data is TRUE or 1.
      :param complex.ArraySelectionParameter quats_array_path: The input quaternions to convert.
      :param complex.BoolParameter use_good_voxels: Whether to assign a black color to 'bad' Elements
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateFaceIPFColoringFilter:
.. py:class:: GenerateFaceIPFColoringFilter

   **UI Display Name:** *Generate IPF Colors (Face)*

   This **Filter** generates a pair of colors for each **Triangle** in a **Triangle Geometry** based on the inverse pole figure (IPF) color scheme for the present crystal structure. Each **Triangle** has 2 colors since any **Face** sits at a boundary between 2 **Features** for a well-connected set of **Features** that represent _grains_. The reference direction used for the IPF color generation is the *normal* of the **Triangle**.

   `Link to the full online documentation for GenerateFaceIPFColoringFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateFaceIPFColoringFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------+-----------------------------------------+
   | UI Display           | Python Named Argument                   |
   +======================+=========================================+
   | Crystal Structures   | crystal_structures_array_path           |
   +----------------------+-----------------------------------------+
   | Average Euler Angles | feature_euler_angles_array_path         |
   +----------------------+-----------------------------------------+
   | Phases               | feature_phases_array_path               |
   +----------------------+-----------------------------------------+
   | IPF Colors           | surface_mesh_face_ipf_colors_array_name |
   +----------------------+-----------------------------------------+
   | Face Labels          | surface_mesh_face_labels_array_path     |
   +----------------------+-----------------------------------------+
   | Face Normals         | surface_mesh_face_normals_array_path    |
   +----------------------+-----------------------------------------+

   .. py:method:: Execute(crystal_structures_array_path, feature_euler_angles_array_path, feature_phases_array_path, surface_mesh_face_ipf_colors_array_name, surface_mesh_face_labels_array_path, surface_mesh_face_normals_array_path)

      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_euler_angles_array_path: Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which phase each Feature belongs
      :param complex.DataObjectNameParameter surface_mesh_face_ipf_colors_array_name: A set of two RGB color schemes encoded as unsigned chars for each Face
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Specifies which Features are on either side of each Face
      :param complex.ArraySelectionParameter surface_mesh_face_normals_array_path: Specifies the normal of each Face
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateFaceMisorientationColoringFilter:
.. py:class:: GenerateFaceMisorientationColoringFilter

   **UI Display Name:** *Generate Misorientation Colors (Face)*

   This **Filter** generates a 3 component vector for each **Triangle** in a **Triangle Geometry** that is the axis-angle of the misorientation associated with the **Features** that lie on either side of the **Triangle**.  The axis is normalized, so if the magnitude of the vector is viewed, it will be the *misorientation angle* in degrees.

   `Link to the full online documentation for GenerateFaceMisorientationColoringFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateFaceMisorientationColoringFilter>`_ 

   Mapping of UI display to python named argument

   +-----------------------+----------------------------------------------------+
   | UI Display            | Python Named Argument                              |
   +=======================+====================================================+
   | Average Quaternions   | avg_quats_array_path                               |
   +-----------------------+----------------------------------------------------+
   | Crystal Structures    | crystal_structures_array_path                      |
   +-----------------------+----------------------------------------------------+
   | Phases                | feature_phases_array_path                          |
   +-----------------------+----------------------------------------------------+
   | Face Labels           | surface_mesh_face_labels_array_path                |
   +-----------------------+----------------------------------------------------+
   | Misorientation Colors | surface_mesh_face_misorientation_colors_array_name |
   +-----------------------+----------------------------------------------------+

   .. py:method:: Execute(avg_quats_array_path, crystal_structures_array_path, feature_phases_array_path, surface_mesh_face_labels_array_path, surface_mesh_face_misorientation_colors_array_name)

      :param complex.ArraySelectionParameter avg_quats_array_path: Specifies the average orientation of each Feature in quaternion representation
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which phase each Feature belongs
      :param complex.ArraySelectionParameter surface_mesh_face_labels_array_path: Specifies which Features are on either side of each Face
      :param complex.DataObjectNameParameter surface_mesh_face_misorientation_colors_array_name: A set of RGB color schemes encoded as floats for each Face
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateGBCDPoleFigureFilter:
.. py:class:: GenerateGBCDPoleFigureFilter

   **UI Display Name:** *Generate GBCD Pole Figure*

   This **Filter** creates a pole figure from the Grain Boundary Character Distribution (GBCD) data. The user must select the relevant phase for which to generate the pole figure by entering the *phase index*.

   `Link to the full online documentation for GenerateGBCDPoleFigureFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateGBCDPoleFigureFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------+-------------------------------+
   | UI Display                 | Python Named Argument         |
   +============================+===============================+
   | Cell Attribute Matrix Name | cell_attribute_matrix_name    |
   +----------------------------+-------------------------------+
   | Cell MRD Array Name        | cell_intensity_array_name     |
   +----------------------------+-------------------------------+
   | Crystal Structures         | crystal_structures_array_path |
   +----------------------------+-------------------------------+
   | Input GBCD                 | gbcd_array_path               |
   +----------------------------+-------------------------------+
   | Image Geometry             | image_geometry_name           |
   +----------------------------+-------------------------------+
   | Misorientation Angle-Axis  | misorientation_rotation       |
   +----------------------------+-------------------------------+
   | Output Image Dimension     | output_image_dimension        |
   +----------------------------+-------------------------------+
   | Phase of Interest          | phase_of_interest             |
   +----------------------------+-------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, cell_intensity_array_name, crystal_structures_array_path, gbcd_array_path, image_geometry_name, misorientation_rotation, output_image_dimension, phase_of_interest)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name of the cell attribute matrix for the created image geometry
      :param complex.DataObjectNameParameter cell_intensity_array_name: The name of the created cell intensity data array
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter gbcd_array_path: 5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere
      :param complex.DataGroupCreationParameter image_geometry_name: The path to the created image geometry
      :param complex.VectorFloat32Parameter misorientation_rotation: Angle-Axis values for drawing GBCD
      :param complex.Int32Parameter output_image_dimension: The value to use for the dimensions for the image geometry
      :param complex.Int32Parameter phase_of_interest: Index of the Ensemble for which to plot the pole figure
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateIPFColorsFilter:
.. py:class:: GenerateIPFColorsFilter

   **UI Display Name:** *Generate IPF Colors*

   This **Filter** will generate *inverse pole figure* (IPF) colors for cubic, hexagonal or trigonal crystal structures. The user can enter the *Reference Direction*, which defaults to [001]. The **Filter** also has the option to apply a black color to all "bad" **Elements**, as defined by a boolean *mask* array, which can be generated using the Threshold Objects **Filter**.

   `Link to the full online documentation for GenerateIPFColorsFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateIPFColorsFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------+-------------------------------+
   | UI Display          | Python Named Argument         |
   +=====================+===============================+
   | Euler Angles        | cell_euler_angles_array_path  |
   +---------------------+-------------------------------+
   | IPF Colors          | cell_ipf_colors_array_name    |
   +---------------------+-------------------------------+
   | Phases              | cell_phases_array_path        |
   +---------------------+-------------------------------+
   | Crystal Structures  | crystal_structures_array_path |
   +---------------------+-------------------------------+
   | Mask                | good_voxels_array_path        |
   +---------------------+-------------------------------+
   | Reference Direction | reference_dir                 |
   +---------------------+-------------------------------+
   | Use Mask Array      | use_good_voxels               |
   +---------------------+-------------------------------+

   .. py:method:: Execute(cell_euler_angles_array_path, cell_ipf_colors_array_name, cell_phases_array_path, crystal_structures_array_path, good_voxels_array_path, reference_dir, use_good_voxels)

      :param complex.ArraySelectionParameter cell_euler_angles_array_path: Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)
      :param complex.DataObjectNameParameter cell_ipf_colors_array_name: The name of the array containing the RGB colors encoded as unsigned chars for each Element
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter good_voxels_array_path: Path to the data array used to define Elements as good or bad.
      :param complex.VectorFloat32Parameter reference_dir: The reference axis with respect to compute the IPF colors
      :param complex.BoolParameter use_good_voxels: Whether to assign a black color to 'bad' Elements
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _GenerateQuaternionConjugateFilter:
.. py:class:: GenerateQuaternionConjugateFilter

   **UI Display Name:** *Generate Quaternion Conjugate*

   This filter will generate the transpose of a [1x4] *Quaternion* laid out in memory such that < x, y, z >, w. This can behandy when the user wants to convert the orientation transformation to an opposite effect. The algorihtm will calculatethe conjugate of each quaternion in the array of input quaternions

   `Link to the full online documentation for GenerateQuaternionConjugateFilter <http://www.dream3d.io/nx_reference_manual/Filters/GenerateQuaternionConjugateFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------+----------------------------+
   | UI Display             | Python Named Argument      |
   +========================+============================+
   | Delete Original Data   | delete_original_data       |
   +------------------------+----------------------------+
   | Output Data Array Path | output_data_array_path     |
   +------------------------+----------------------------+
   | Quaternions            | quaternion_data_array_path |
   +------------------------+----------------------------+

   .. py:method:: Execute(delete_original_data, output_data_array_path, quaternion_data_array_path)

      :param complex.BoolParameter delete_original_data: Should the original Data be deleted from the DataStructure
      :param complex.DataObjectNameParameter output_data_array_path: The name of the generated output DataArray
      :param complex.ArraySelectionParameter quaternion_data_array_path: Specifies the quaternions to convert
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _INLWriterFilter:
.. py:class:: INLWriterFilter

   **UI Display Name:** *Export INL File*

   This **Filter** writes out **Cell** data from an **Image Geometry** to a file format used by the Idaho National Laboratory (INL).  The format is columnar and space delimited, with header lines denoted by the "#" character. The columns are the following:

   `Link to the full online documentation for INLWriterFilter <http://www.dream3d.io/nx_reference_manual/Filters/INLWriterFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------+-------------------------------+
   | UI Display         | Python Named Argument         |
   +====================+===============================+
   | Euler Angles       | cell_euler_angles_array_path  |
   +--------------------+-------------------------------+
   | Phases             | cell_phases_array_path        |
   +--------------------+-------------------------------+
   | Crystal Structures | crystal_structures_array_path |
   +--------------------+-------------------------------+
   | Feature Ids        | feature_ids_array_path        |
   +--------------------+-------------------------------+
   | Image Geometry     | image_geom_path               |
   +--------------------+-------------------------------+
   | Material Names     | material_name_array_path      |
   +--------------------+-------------------------------+
   | Number of Features | num_features_array_path       |
   +--------------------+-------------------------------+
   | Output File        | output_file                   |
   +--------------------+-------------------------------+

   .. py:method:: Execute(cell_euler_angles_array_path, cell_phases_array_path, crystal_structures_array_path, feature_ids_array_path, image_geom_path, material_name_array_path, num_features_array_path, output_file)

      :param complex.ArraySelectionParameter cell_euler_angles_array_path: Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_ids_array_path: Specifies to which Feature each Cell belongs
      :param complex.GeometrySelectionParameter image_geom_path: The selected image geometry
      :param complex.DataPathSelectionParameter material_name_array_path: Name of each Ensemble
      :param complex.ArraySelectionParameter num_features_array_path: The number of Features per Ensemble
      :param complex.FileSystemPathParameter output_file: The output .inl file path
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportH5EspritDataFilter:
.. py:class:: ImportH5EspritDataFilter

   **UI Display Name:** *Import Bruker Nano Esprit Data (.h5)*

   This **Filter** will read a single .h5 file into a new **Image Geometry**, allowing the immediate use of **Filters** on the data instead of having to generate the intermediate .h5ebsd file. A **Cell Attribute Matrix** and **Ensemble Attribute Matrix** will also be created to hold the imported EBSD information. Currently, the user has no control over the names of the created **Attribute Arrays**.

   `Link to the full online documentation for ImportH5EspritDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportH5EspritDataFilter>`_ 

   Mapping of UI display to python named argument

   +---------------------------------+-------------------------------------+
   | UI Display                      | Python Named Argument               |
   +=================================+=====================================+
   | Cell Attribute Matrix           | cell_attribute_matrix_name          |
   +---------------------------------+-------------------------------------+
   | Cell Ensemble Attribute Matrix  | cell_ensemble_attribute_matrix_name |
   +---------------------------------+-------------------------------------+
   | Convert Euler Angles to Radians | degrees_to_radians                  |
   +---------------------------------+-------------------------------------+
   | Image Geometry                  | image_geometry_name                 |
   +---------------------------------+-------------------------------------+
   | Origin                          | origin                              |
   +---------------------------------+-------------------------------------+
   | Import Pattern Data             | read_pattern_data                   |
   +---------------------------------+-------------------------------------+
   | Scan Names                      | selected_scan_names                 |
   +---------------------------------+-------------------------------------+
   | Z Spacing (Microns)             | z_spacing                           |
   +---------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, cell_ensemble_attribute_matrix_name, degrees_to_radians, image_geometry_name, origin, read_pattern_data, selected_scan_names, z_spacing)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name of the cell data attribute matrix for the created Image Geometry
      :param complex.DataObjectNameParameter cell_ensemble_attribute_matrix_name: The name of the cell ensemble data attribute matrix for the created Image Geometry
      :param complex.BoolParameter degrees_to_radians: Whether or not to convert the euler angles to radians
      :param complex.DataGroupCreationParameter image_geometry_name: The path to the created Image Geometry
      :param complex.VectorFloat32Parameter origin: The origin of the volume
      :param complex.BoolParameter read_pattern_data: Whether or not to import the pattern data
      :param orientationanalysis.OEMEbsdScanSelectionParameter selected_scan_names: The name of the scan in the .h5 file. EDAX can store multiple scans in a single file
      :param complex.Float32Parameter z_spacing: The spacing in microns between each layer.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ImportH5OimDataFilter:
.. py:class:: ImportH5OimDataFilter

   **UI Display Name:** *Import EDAX OIMAnalysis Data (.h5)*

   This **Filter** will read a single .h5 file into a new **Image Geometry**, allowing the immediate use of **Filters** on the data instead of having to generate the intermediate .h5ebsd file. A **Cell Attribute Matrix** and **Ensemble Attribute Matrix** will also be created to hold the imported EBSD information. Currently, the user has no control over the names of the created **Attribute Arrays**.

   `Link to the full online documentation for ImportH5OimDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ImportH5OimDataFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+-------------------------------------+
   | UI Display                     | Python Named Argument               |
   +================================+=====================================+
   | Cell Attribute Matrix          | cell_attribute_matrix_name          |
   +--------------------------------+-------------------------------------+
   | Cell Ensemble Attribute Matrix | cell_ensemble_attribute_matrix_name |
   +--------------------------------+-------------------------------------+
   | Image Geometry                 | image_geometry_name                 |
   +--------------------------------+-------------------------------------+
   | Origin                         | origin                              |
   +--------------------------------+-------------------------------------+
   | Import Pattern Data            | read_pattern_data                   |
   +--------------------------------+-------------------------------------+
   | Scan Names                     | selected_scan_names                 |
   +--------------------------------+-------------------------------------+
   | Z Spacing (Microns)            | z_spacing                           |
   +--------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, cell_ensemble_attribute_matrix_name, image_geometry_name, origin, read_pattern_data, selected_scan_names, z_spacing)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name of the cell data attribute matrix for the created Image Geometry
      :param complex.DataObjectNameParameter cell_ensemble_attribute_matrix_name: The name of the cell ensemble data attribute matrix for the created Image Geometry
      :param complex.DataGroupCreationParameter image_geometry_name: The path to the created Image Geometry
      :param complex.VectorFloat32Parameter origin: The origin of the volume
      :param complex.BoolParameter read_pattern_data: Whether or not to import the pattern data
      :param orientationanalysis.OEMEbsdScanSelectionParameter selected_scan_names: The name of the scan in the .h5 file. EDAX can store multiple scans in a single file
      :param complex.Float32Parameter z_spacing: The spacing in microns between each layer.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _MergeTwinsFilter:
.. py:class:: MergeTwinsFilter

   **UI Display Name:** *Merge Twins*

   *THIS FILTER ONLY WORKS ON CUBIC-HIGH (m3m) Laue Classes.*

   `Link to the full online documentation for MergeTwinsFilter <http://www.dream3d.io/nx_reference_manual/Filters/MergeTwinsFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------+-----------------------------------------+
   | UI Display                   | Python Named Argument                   |
   +==============================+=========================================+
   | Active                       | active_array_name                       |
   +------------------------------+-----------------------------------------+
   | Angle Tolerance (Degrees)    | angle_tolerance                         |
   +------------------------------+-----------------------------------------+
   | Average Quaternions          | avg_quats_array_path                    |
   +------------------------------+-----------------------------------------+
   | Axis Tolerance (Degrees)     | axis_tolerance                          |
   +------------------------------+-----------------------------------------+
   | Parent Ids                   | cell_parent_ids_array_name              |
   +------------------------------+-----------------------------------------+
   | Contiguous Neighbor List     | contiguous_neighbor_list_array_path     |
   +------------------------------+-----------------------------------------+
   | Crystal Structures           | crystal_structures_array_path           |
   +------------------------------+-----------------------------------------+
   | Cell Feature Ids             | feature_ids_path                        |
   +------------------------------+-----------------------------------------+
   | Parent Ids                   | feature_parent_ids_array_name           |
   +------------------------------+-----------------------------------------+
   | Phases                       | feature_phases_array_path               |
   +------------------------------+-----------------------------------------+
   | Feature Attribute Matrix     | new_cell_feature_attribute_matrix_name  |
   +------------------------------+-----------------------------------------+
   | Non-Contiguous Neighbor List | non_contiguous_neighbor_list_array_path |
   +------------------------------+-----------------------------------------+
   | Use Non-Contiguous Neighbors | use_non_contiguous_neighbors            |
   +------------------------------+-----------------------------------------+

   .. py:method:: Execute(active_array_name, angle_tolerance, avg_quats_array_path, axis_tolerance, cell_parent_ids_array_name, contiguous_neighbor_list_array_path, crystal_structures_array_path, feature_ids_path, feature_parent_ids_array_name, feature_phases_array_path, new_cell_feature_attribute_matrix_name, non_contiguous_neighbor_list_array_path, use_non_contiguous_neighbors)

      :param complex.DataObjectNameParameter active_array_name: The name of the array specifying if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). At the end of the Filter, all Features will be Active
      :param complex.Float32Parameter angle_tolerance: Tolerance allowed when comparing the angle part of the axis-angle representation of the misorientation
      :param complex.ArraySelectionParameter avg_quats_array_path: Specifies the average orientation of each Feature in quaternion representation
      :param complex.Float32Parameter axis_tolerance: Tolerance allowed when comparing the axis part of the axis-angle representation of the misorientation
      :param complex.DataObjectNameParameter cell_parent_ids_array_name: The name of the array specifying to which parent each cell belongs
      :param complex.NeighborListSelectionParameter contiguous_neighbor_list_array_path: List of contiguous neighbors for each Feature.
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ArraySelectionParameter feature_ids_path: Specifies to which Feature each cell belongs
      :param complex.DataObjectNameParameter feature_parent_ids_array_name: The name of the array specifying to which parent each Feature belongs
      :param complex.ArraySelectionParameter feature_phases_array_path: Specifies to which Ensemble each cell belongs
      :param complex.DataObjectNameParameter new_cell_feature_attribute_matrix_name: The name of the created cell feature attribute matrix
      :param complex.NeighborListSelectionParameter non_contiguous_neighbor_list_array_path: List of non-contiguous neighbors for each Feature.
      :param complex.BoolParameter use_non_contiguous_neighbors: Whether to use a list of non-contiguous or contiguous neighbors for each feature when merging
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _NeighborOrientationCorrelationFilter:
.. py:class:: NeighborOrientationCorrelationFilter

   **UI Display Name:** *Neighbor Orientation Correlation*

   This **Filter** first identifies all **Cells** that have a *confidence index* below the minimum set by the user.  Then, for each of those **Cells**, their neighboring **Cells** are checked to determine the number of neighbor **Cells** with orientations different than the reference **Cell** by more than the user defined *misorientation tolerance*.  In addition the neighboring **Cells** are compared with each other to determine the number of neighboring **Cells** that have the same orientation (again within the user defined tolerance). The *Cleanup Level* set by the user is then used to determine which **Cells** are replaced.  If a level of 5 is set, then at least 5 of the neighboring **Cells** must be different than the reference **Cell** and at least 5 of the neighboring **Cells** must be the same as each other (and so on for other *Cleanup Level*). If a **Cell** meets the replacement criteria, then all of its attributes are replaced with the attributes of one of the neighboring **Cells** that are the same as each other.

   `Link to the full online documentation for NeighborOrientationCorrelationFilter <http://www.dream3d.io/nx_reference_manual/Filters/NeighborOrientationCorrelationFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------------+-------------------------------+
   | UI Display                         | Python Named Argument         |
   +====================================+===============================+
   | Cell Phases                        | cell_phases_array_path        |
   +------------------------------------+-------------------------------+
   | Confidence Index                   | confidence_index_array_path   |
   +------------------------------------+-------------------------------+
   | Crystal Structures                 | crystal_structures_array_path |
   +------------------------------------+-------------------------------+
   | Attribute Arrays to Ignore         | ignored_data_array_paths      |
   +------------------------------------+-------------------------------+
   | Image Geometry                     | image_geometry_path           |
   +------------------------------------+-------------------------------+
   | Cleanup Level                      | level                         |
   +------------------------------------+-------------------------------+
   | Minimum Confidence Index           | min_confidence                |
   +------------------------------------+-------------------------------+
   | Misorientation Tolerance (Degrees) | misorientation_tolerance      |
   +------------------------------------+-------------------------------+
   | Quaternions                        | quats_array_path              |
   +------------------------------------+-------------------------------+

   .. py:method:: Execute(cell_phases_array_path, confidence_index_array_path, crystal_structures_array_path, ignored_data_array_paths, image_geometry_path, level, min_confidence, misorientation_tolerance, quats_array_path)

      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Cell belongs
      :param complex.ArraySelectionParameter confidence_index_array_path: Specifies the confidence in the orientation of the Cell (TSL data)
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.MultiArraySelectionParameter ignored_data_array_paths: The list of arrays to ignore
      :param complex.GeometrySelectionParameter image_geometry_path: Path to the target geometry
      :param complex.Int32Parameter level: Minimum number of neighbor Cells that must have orientations within above tolerance to allow Cell to be changed
      :param complex.Float32Parameter min_confidence: Sets the minimum value of 'confidence' a Cell must have
      :param complex.Float32Parameter misorientation_tolerance: Angular tolerance used to compare with neighboring Cells
      :param complex.ArraySelectionParameter quats_array_path: Specifies the orientation of the Cell in quaternion representation
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ReadAngDataFilter:
.. py:class:: ReadAngDataFilter

   **UI Display Name:** *Import EDAX EBSD Data (.ang)*

   This **Filter** will read a single .ang file into a new **Image Geometry**, allowing the immediate use of **Filters** on the data instead of having to generate the intermediate .h5ebsd file. A **Cell Attribute Matrix** and **Ensemble Attribute Matrix** will also be created to hold the imported EBSD information. Currently, the user has no control over the names of the created **Attribute Arrays**. The user should be aware that simply reading the file then performing operations that are dependent on the proper crystallographic and sample reference frame will be undefined or simply **wrong**. In order to bring the crystal reference frame and sample reference frame into coincidence, rotations will need to be applied to the data.

   `Link to the full online documentation for ReadAngDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ReadAngDataFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------+-------------------------------------+
   | UI Display                             | Python Named Argument               |
   +========================================+=====================================+
   | Created Cell Attribute Matrix          | cell_attribute_matrix_name          |
   +----------------------------------------+-------------------------------------+
   | Created Cell Ensemble Attribute Matrix | cell_ensemble_attribute_matrix_name |
   +----------------------------------------+-------------------------------------+
   | Created Image Geometry                 | data_container_name                 |
   +----------------------------------------+-------------------------------------+
   | Input File                             | input_file                          |
   +----------------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, cell_ensemble_attribute_matrix_name, data_container_name, input_file)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The Attribute Matrix where the scan data is stored.
      :param complex.DataObjectNameParameter cell_ensemble_attribute_matrix_name: The Attribute Matrix where the phase information is stored.
      :param complex.DataGroupCreationParameter data_container_name: The complete path to the Geometry being created.
      :param complex.FileSystemPathParameter input_file: The input .ang file path
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ReadCtfDataFilter:
.. py:class:: ReadCtfDataFilter

   **UI Display Name:** *Import Oxford Instr. EBSD Data (.ctf)*

   This **Filter** will read a single .ctf file into a new **Image Geometry**, allowing the immediate use of **Filters** on the data instead of having to generate the intermediate .h5ebsd file. A **Cell Attribute Matrix** and **Ensemble Attribute Matrix** will also be created to hold the imported EBSD information. Currently, the user has no control over the names of the created **Attribute Arrays**. The user should be aware that simply reading the file then performing operations that are dependent on the proper crystallographic and sample reference frame will be **undefined, inaccurate and/or wrong**. In order to bring the crystal reference frame and sample reference frame into coincidence, rotations will need to be applied to the data. An excellant reference for this is the following PDF file:`http://pajarito.materials.cmu.edu/rollett/27750/L17-EBSD-analysis-31Mar16.pdf <http://pajarito.materials.cmu.edu/rollett/27750/L17-EBSD-analysis-31Mar16.pdf>`_

   `Link to the full online documentation for ReadCtfDataFilter <http://www.dream3d.io/nx_reference_manual/Filters/ReadCtfDataFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------------------+-------------------------------------+
   | UI Display                                | Python Named Argument               |
   +===========================================+=====================================+
   | Created Cell Attribute Matrix             | cell_attribute_matrix_name          |
   +-------------------------------------------+-------------------------------------+
   | Created Cell Ensemble Attribute Matrix    | cell_ensemble_attribute_matrix_name |
   +-------------------------------------------+-------------------------------------+
   | Created Image Geometry                    | data_container_name                 |
   +-------------------------------------------+-------------------------------------+
   | Convert Euler Angles to Radians           | degrees_to_radians                  |
   +-------------------------------------------+-------------------------------------+
   | Convert Hexagonal X-Axis to EDAX Standard | edax_hexagonal_alignment            |
   +-------------------------------------------+-------------------------------------+
   | Input File                                | input_file                          |
   +-------------------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, cell_ensemble_attribute_matrix_name, data_container_name, degrees_to_radians, edax_hexagonal_alignment, input_file)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The Attribute Matrix where the scan data is stored.
      :param complex.DataObjectNameParameter cell_ensemble_attribute_matrix_name: The Attribute Matrix where the phase information is stored.
      :param complex.DataGroupCreationParameter data_container_name: The complete path to the Geometry being created.
      :param complex.BoolParameter degrees_to_radians: Whether or not to convert the Euler angles to Radians
      :param complex.BoolParameter edax_hexagonal_alignment: Whether or not to convert a Hexagonal phase to the EDAX standard for x-axis alignment
      :param complex.FileSystemPathParameter input_file: The input .ctf file path
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _ReadH5EbsdFilter:
.. py:class:: ReadH5EbsdFilter

   **UI Display Name:** *Import H5EBSD File*

   This **Filter** reads from the .h5ebsd file that was generated with the *Import Orientation File(s) to H5EBSD* **Filter**.

   `Link to the full online documentation for ReadH5EbsdFilter <http://www.dream3d.io/nx_reference_manual/Filters/ReadH5EbsdFilter>`_ 

   Mapping of UI display to python named argument

   +----------------------------------------+-------------------------------------+
   | UI Display                             | Python Named Argument               |
   +========================================+=====================================+
   | Created Cell Attribute Matrix          | cell_attribute_matrix_name          |
   +----------------------------------------+-------------------------------------+
   | Created Cell Ensemble Attribute Matrix | cell_ensemble_attribute_matrix_name |
   +----------------------------------------+-------------------------------------+
   | Created Image Geometry                 | data_container_name                 |
   +----------------------------------------+-------------------------------------+
   | Import H5Ebsd File                     | read_h5_ebsd_filter                 |
   +----------------------------------------+-------------------------------------+

   .. py:method:: Execute(cell_attribute_matrix_name, cell_ensemble_attribute_matrix_name, data_container_name, read_h5_ebsd_filter)

      :param complex.DataObjectNameParameter cell_attribute_matrix_name: The name of the created cell attribute matrix associated with the imported geometry
      :param complex.DataObjectNameParameter cell_ensemble_attribute_matrix_name: The name of the created cell ensemble attribute matrix associated with the imported geometry
      :param complex.DataGroupCreationParameter data_container_name: The complete path to the imported Image Geometry
      :param orientationanalysis.H5EbsdReaderParameter read_h5_ebsd_filter: The input .h5ebsd file path
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RodriguesConvertorFilter:
.. py:class:: RodriguesConvertorFilter

   **UI Display Name:** *Rodrigues Convertor*

   This *filter* will convert a 3 component Rodrigues vector into a 4 component vector that DREAM.3D expects in itsequations and algorithms. The algorithm is the following:

   `Link to the full online documentation for RodriguesConvertorFilter <http://www.dream3d.io/nx_reference_manual/Filters/RodriguesConvertorFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------------+---------------------------+
   | UI Display                     | Python Named Argument     |
   +================================+===========================+
   | Delete Original Data           | delete_original_data      |
   +--------------------------------+---------------------------+
   | Converted Rodrigues Data Array | output_data_array_path    |
   +--------------------------------+---------------------------+
   | Input Rodrigues Vectors        | rodrigues_data_array_path |
   +--------------------------------+---------------------------+

   .. py:method:: Execute(delete_original_data, output_data_array_path, rodrigues_data_array_path)

      :param complex.BoolParameter delete_original_data: Should the original Rodrigues data array be deleted from the DataStructure
      :param complex.DataObjectNameParameter output_data_array_path: The DataArray name of the converted Rodrigues vectors
      :param complex.ArraySelectionParameter rodrigues_data_array_path: Specifies the Rodrigues data to convert
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _RotateEulerRefFrameFilter:
.. py:class:: RotateEulerRefFrameFilter

   **UI Display Name:** *Rotate Euler Reference Frame*

   This **Filter** performs a passive rotation (Right hand rule) of the Euler Angles about a user defined axis. The *reference frame* is being rotated and thus the *Euler Angles* necessary to represent the same orientation must change to account for the new *reference frame*.  The user can set an *angle* and an *axis* to define the rotation of the *reference frame*.

   `Link to the full online documentation for RotateEulerRefFrameFilter <http://www.dream3d.io/nx_reference_manual/Filters/RotateEulerRefFrameFilter>`_ 

   Mapping of UI display to python named argument

   +------------------------------+------------------------------+
   | UI Display                   | Python Named Argument        |
   +==============================+==============================+
   | Euler Angles                 | cell_euler_angles_array_path |
   +------------------------------+------------------------------+
   | Rotation Axis-Angle [<ijk>w] | rotation_axis                |
   +------------------------------+------------------------------+

   .. py:method:: Execute(cell_euler_angles_array_path, rotation_axis)

      :param complex.ArraySelectionParameter cell_euler_angles_array_path: Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)
      :param complex.VectorFloat32Parameter rotation_axis: Axis-Angle in sample reference frame to rotate about.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _WritePoleFigureFilter:
.. py:class:: WritePoleFigureFilter

   **UI Display Name:** *Generate Pole Figure Images*

   This **Filter** creates a standard pole figure image for each **Ensemble** in a selected **Data Container** with an **Image Geometry**. The **Filter** uses Euler angles in radians and requires the crystal structures for each **Ensemble** array and the corresponding **Ensemble** Ids on the **Cells**. The **Filter** also optionally can use a *mask* array to determine which **Cells** are valid for the pole figure computation.

   `Link to the full online documentation for WritePoleFigureFilter <http://www.dream3d.io/nx_reference_manual/Filters/WritePoleFigureFilter>`_ 

   Mapping of UI display to python named argument

   +-------------------------------+-------------------------------+
   | UI Display                    | Python Named Argument         |
   +===============================+===============================+
   | Euler Angles                  | cell_euler_angles_array_path  |
   +-------------------------------+-------------------------------+
   | Phases                        | cell_phases_array_path        |
   +-------------------------------+-------------------------------+
   | Crystal Structures            | crystal_structures_array_path |
   +-------------------------------+-------------------------------+
   | Pole Figure Type              | generation_algorithm          |
   +-------------------------------+-------------------------------+
   | Mask                          | good_voxels_array_path        |
   +-------------------------------+-------------------------------+
   | Created Image Geometry        | image_geometry_path           |
   +-------------------------------+-------------------------------+
   | Image Layout                  | image_layout                  |
   +-------------------------------+-------------------------------+
   | Pole Figure File Prefix       | image_prefix                  |
   +-------------------------------+-------------------------------+
   | Image Size (Square Pixels)    | image_size                    |
   +-------------------------------+-------------------------------+
   | Lambert Image Size (Pixels)   | lambert_size                  |
   +-------------------------------+-------------------------------+
   | Material Name                 | material_name_array_path      |
   +-------------------------------+-------------------------------+
   | Number of Colors              | num_colors                    |
   +-------------------------------+-------------------------------+
   | Output Directory Path         | output_path                   |
   +-------------------------------+-------------------------------+
   | Save Output as Image Geometry | save_as_image_geometry        |
   +-------------------------------+-------------------------------+
   | Figure Title                  | title                         |
   +-------------------------------+-------------------------------+
   | Use Mask Array                | use_good_voxels               |
   +-------------------------------+-------------------------------+
   | Write Pole Figure as Image    | write_image_to_disk           |
   +-------------------------------+-------------------------------+

   .. py:method:: Execute(cell_euler_angles_array_path, cell_phases_array_path, crystal_structures_array_path, generation_algorithm, good_voxels_array_path, image_geometry_path, image_layout, image_prefix, image_size, lambert_size, material_name_array_path, num_colors, output_path, save_as_image_geometry, title, use_good_voxels, write_image_to_disk)

      :param complex.ArraySelectionParameter cell_euler_angles_array_path: Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each cell belongs
      :param complex.ArraySelectionParameter crystal_structures_array_path: Enumeration representing the crystal structure for each Ensemble
      :param complex.ChoicesParameter generation_algorithm: The type of pole figure generated. 0=Color, 1=Discrete
      :param complex.ArraySelectionParameter good_voxels_array_path: DataPath to the input Mask DataArray
      :param complex.DataGroupCreationParameter image_geometry_path: The path to the created Image Geometry
      :param complex.ChoicesParameter image_layout: How to layout the 3 pole figures. 0=Horizontal, 1=Vertical, 2=Square
      :param complex.StringParameter image_prefix: The prefix to apply to each generated pole figure. Each Phase will have its own pole figure.
      :param complex.Int32Parameter image_size: The number of pixels that define the height and width of **each** output pole figure
      :param complex.Int32Parameter lambert_size: The height/width of the internal Lambert Square that is used for interpolation
      :param complex.DataPathSelectionParameter material_name_array_path: DataPath to the input DataArray that holds the material names
      :param complex.Int32Parameter num_colors: The number of colors to use for the Color Intensity pole figures
      :param complex.FileSystemPathParameter output_path: This is the path to the directory where the pole figures will be created. One file for each phase.
      :param complex.BoolParameter save_as_image_geometry: Save the generated pole figure as an ImageGeometry
      :param complex.StringParameter title: The title to place at the top of the Pole Figure
      :param complex.BoolParameter use_good_voxels: Should the algorithm use a mask array to remove non-indexed points
      :param complex.BoolParameter write_image_to_disk: Should the filter write the pole figure plots to a file.
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


.. _WriteStatsGenOdfAngleFileFilter:
.. py:class:: WriteStatsGenOdfAngleFileFilter

   **UI Display Name:** *Export StatsGenerator ODF Angle File*

   This **Filter** is used in a workflow where the user would like to generate a synthetic microstructure with an ODF that matches (as closely as possible) an existing experimental data set or other data set that is being mimicked. The basic workflow is the following:

   `Link to the full online documentation for WriteStatsGenOdfAngleFileFilter <http://www.dream3d.io/nx_reference_manual/Filters/WriteStatsGenOdfAngleFileFilter>`_ 

   Mapping of UI display to python named argument

   +--------------------------+------------------------------+
   | UI Display               | Python Named Argument        |
   +==========================+==============================+
   | Euler Angles             | cell_euler_angles_array_path |
   +--------------------------+------------------------------+
   | Phases                   | cell_phases_array_path       |
   +--------------------------+------------------------------+
   | Convert to Degrees       | convert_to_degrees           |
   +--------------------------+------------------------------+
   | Delimiter                | delimiter                    |
   +--------------------------+------------------------------+
   | Mask                     | good_voxels_array_path       |
   +--------------------------+------------------------------+
   | Output File              | output_file                  |
   +--------------------------+------------------------------+
   | Default Sigma            | sigma                        |
   +--------------------------+------------------------------+
   | Only Write Good Elements | use_good_voxels              |
   +--------------------------+------------------------------+
   | Default Weight           | weight                       |
   +--------------------------+------------------------------+

   .. py:method:: Execute(cell_euler_angles_array_path, cell_phases_array_path, convert_to_degrees, delimiter, good_voxels_array_path, output_file, sigma, use_good_voxels, weight)

      :param complex.ArraySelectionParameter cell_euler_angles_array_path: Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)
      :param complex.ArraySelectionParameter cell_phases_array_path: Specifies to which Ensemble each Element belongs
      :param complex.BoolParameter convert_to_degrees: Whether to convert the Euler angles from radians to degrees. If the Euler angles are already in degrees, this option will 'convert' the data again, resulting in garbage orientations!
      :param complex.ChoicesParameter delimiter: The delimiter separating the data
      :param complex.ArraySelectionParameter good_voxels_array_path: Used to define Elements as good or bad. Only required if Only Write Good Elements is checked
      :param complex.FileSystemPathParameter output_file: The output angles file path
      :param complex.Int32Parameter sigma: This value will be used for the Sigma column
      :param complex.BoolParameter use_good_voxels: Whether to only write the Euler angles for those elements denoted as true in the supplied mask array
      :param complex.Float32Parameter weight: This value will be used for the Weight column
      :return: Returns a complex.Result object that holds any warnings and/or errors that were encountered during execution.
      :rtype: complex.Result


