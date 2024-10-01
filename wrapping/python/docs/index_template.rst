.. This is the root documentation file. Note that if you are
   compiling additional plugins, and you want the python docs
   generated, you will need to add those to the list below

DREAM3D-NX Python Docs (v24.09.30)
===================================

.. image:: https://anaconda.org/bluequartzsoftware/dream3dnx/badges/version.svg   
   :target: https://anaconda.org/bluequartzsoftware/dream3dnx
.. image:: https://anaconda.org/bluequartzsoftware/dream3dnx/badges/latest_release_date.svg   
   :target: https://anaconda.org/bluequartzsoftware/dream3dnx
.. image:: https://anaconda.org/bluequartzsoftware/dream3dnx/badges/latest_release_relative_date.svg   
   :target: https://anaconda.org/bluequartzsoftware/dream3dnx
.. image:: https://anaconda.org/bluequartzsoftware/dream3dnx/badges/platforms.svg   
   :target: https://anaconda.org/bluequartzsoftware/dream3dnx
.. image:: https://anaconda.org/bluequartzsoftware/dream3dnx/badges/downloads.svg   
   :target: https://anaconda.org/bluequartzsoftware/dream3dnx


Installation
----------------------
   
The *simplnx* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment and installing the `dream3dnx` python package. Please check the latest release
notes for the latest changes to the API.

DREAM3D-NX is built on top of a library called `simplnx` which is why all of the python code
is prefixed as such.

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.12
    conda activate nxpython
    conda install -c bluequartzsoftware dream3dnx


.. attention::

   MacOS: Intel MacOS does not support Python 3.12 due to a library conflict between Qt 6 and the other libraries. 
   

How to use SIMPLNX from Python
------------------------------

.. attention::

   All users should start with the :ref:`SIMPLNX Python Introduction<SIMPLNXPythonIntroduction>`


.. attention::

   Users wanting to integrate the functionality included with SIMPLNX/DREAM3D-NX from within Python, you should 
   use the link to :ref:`SIMPLNX User API Docs <UserAPIDocs>`


.. attention::
   
   Users wanting to extend DREAM3D-NX by writing filters in the python language should go to the :ref:`Writing a Python Filter <WritingPythonFilters>`


.. toctree::
   :maxdepth: 2
   :caption: Conceptual

   Installation
   Overview
   DataObjects
   Geometry
   Reference_Frame_Notes

.. toctree::
   :maxdepth: 2
   :caption: Using SIMPLNX in Python

   Python_Introduction
   User_API
   Tutorial_1
   Tutorial_2
   Tutorial_3

.. toctree::
   :maxdepth: 1
   :caption: SIMPLNX Filter Docs

@PLUGIN_LIST@

.. toctree::
   :maxdepth: 2
   :caption: SIMPLNX Filter Writing API

   Writing_A_New_Python_Filter
   Developer_API

.. toctree::
   :maxdepth: 1
   :caption: Release Notes

   release_notes/ReleaseNotes_240930
   release_notes/ReleaseNotes_240803
   release_notes/ReleaseNotes_240605
   release_notes/ReleaseNotes_130
   release_notes/ReleaseNotes_127
   release_notes/ReleaseNotes_126
   release_notes/ReleaseNotes_124
   release_notes/ReleaseNotes_123
   release_notes/ReleaseNotes_121
   release_notes/ReleaseNotes_120
   release_notes/ReleaseNotes_110

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
