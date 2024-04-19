.. This is the root documentation file. Note that if you are
   compiling additional plugins, and you want the python docs
   generated, you will need to add those to the list below

DREAM3D-NX Python Docs (v1.2.7)
=================================

Installation
----------------------
   
The *simplnx* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment and installing the `dream3dnx` python package.

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.12
    conda activate nxpython
    conda install -c bluequartzsoftware dream3dnx


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

.. toctree::
   :maxdepth: 2
   :caption: Using SIMPLNX in Python

   Python_Introduction
   User_API
   Reference_Frame_Notes

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

   ReleaseNotes_130
   ReleaseNotes_127
   ReleaseNotes_126
   ReleaseNotes_124
   ReleaseNotes_123
   ReleaseNotes_121
   ReleaseNotes_120
   ReleaseNotes_110

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
