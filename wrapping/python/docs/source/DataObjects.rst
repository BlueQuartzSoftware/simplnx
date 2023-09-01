DataStructure Objects
======================

.. _DataStructure:

The **complex** DataStructure can be filled with various types of objects. Those are
all listed below. In the **DREAM3D-NX** user interface, the DataStructure of any
pipeline can be inspected via the "DataStructure" view, shown below outlined in 
a yellow box at the right side of the user interface.

.. image:: Images/UI_DataStructure.png
   :width: 1358
   :height: 809
   :scale: 45

.. _DataObject:

DataObject
----------

This is the abstract base class for all other objects that can be inserted into the
DataStructure_ . It should never be used as the appropriate class from the list
below should be used instead.

.. _DataGroup:

DataGroup
---------

The DataStructure_ is a flexible heirarchy that stores all **complex** :ref:`DataObjects <DataObject>`
that are created. A basic :ref:`DataObject` that can be created is a :ref:`DataGroup` which is a 
simple grouping mechanism that can be thought of as similar in concept to a folder or directory that 
is created on the file system.

.. _DataPath:

DataPath
---------

A DataPath is a complex class that describes the path to a :ref:`DataObject` within 
the DataStructure_ . The path is constructed as a python list of string objects.
For example if we have a top level group called **MyGroup** and a **DataArray** 
called *Euler Angles* within that group the **DataPath** object that would be constructed is the following

.. code:: python

  array_path = cx.DataPath(['MyGroup', 'Euler Angles'])

.. _DataArray:

DataArray
-----------

The DataArray is the main class that holds the raw data. It is typically a contiguous
chunk of memory that is allocated to hold the data that will be processed. The DataArray
has a few properties that should be well understood by the user before starting to develop
codes that are based on **complex**. The 

.. image:: Images/DataArray_Explanation.png
   :height: 664
   :width: 1177
   :scale: 50
   :alt: DataArray memory schematic

+ Name: Each **DataArray** has a name that is assigned to it. Most any character can be used except for the '/' character.
+ Tuple Shape: The DataArray will have a tuple shape that is describe by an array values that are listed in "C" order of slowest to fastest moving dimension.
+ Component Shape: At **each** tuple, there can be multiple values which are described by the *component shape* which is an array of values that are listed in teh "C" order of slowest to fastest moving dimension.

Referring to the figure above, The **DataArray** that has been created is a 2D DataArray with 
dimensions of 4 high and 5 tuples wide. Each tuple has 3 components, the RGB values of a color image. Refer to the 
memory schemtic in the above image to understand how this would be layed out in memory and subsequently
accessed with the *numpy* API. The following is the python code that would craete the *DataArray* used
in the example.

.. code:: python

  data_structure = cx.DataStructure()
  result = cx.CreateDataArray.execute(data_structure=data_structure, 
                                        component_count=3, 
                                        data_format="", 
                                        initialization_value="0", 
                                        numeric_type=cx.NumericType.float32, 
                                        output_data_array=cx.DataPath(["2D Array"]), 
                                        tuple_dimensions=[[4, 5]])

.. _DataStore:

DataStore
----------

The DataStore is the C++ object that actually allocates the memory necessary to store
data in complex/DREAM3D. The Python API is intentially limited to getting a Numpy.View()
so that python developers can have a consistent well known interace to the DataArray_.


AttributeMatrix
----------------

An AttributeMatrix is specialized :ref:`DataGroup` that has two main criteria that must be met when 
inserting into the AttributeMatrix:

1) No :ref:`DataGroup` may be inserted into the AttributeMatrix
2) All :ref:`DataArray` objects that are inserted into the AttributeMatrix **must** have the same number of tuples.

The predominant use of an AttributeMatrix is to group together :ref:`DataArray` objects that represent DataArrays that
all appear on a specific **Geometry**. For example if you have an **Image Geometry** that is 10 voxels wide (X) by 20
voxels tall (Y) by 30 voxels deep (Z), the AttributeMatrix that holds the various DataArrays will have the same dimensions, 
(but expressed in reverse order). This ensures that the arrays that represent that data are all fully allocated and accessible. This
concept can be summarized in the figure below.

.. image:: Images/AttributeMatrix_CellData_Figure.png
   :width: 1993
   :height: 1121
   :scale: 35

In the figure a 2D EBSD data set has been collected. The data set was collected on a regular grid (Image Geometry)
and has 9 different DataArrays. So for each **Scan Point** the index of that scan point can be computed, this index value
represents the tuple index into any given DataArray. That can be used to access a specific value of the DataArray
that represents the value of the Array, Euler Angles for instance, at that tuple index.

