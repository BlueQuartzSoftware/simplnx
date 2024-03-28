.. This is the root documentation file. Note that if you are
   compiling additional plugins, and you want the python docs
   generated, you will need to add those to the list below

DREAM3D-NX Python Docs
=======================

Latest Version: 1.2.7
---------------------

The *simplnx* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.10
    conda activate nxpython
    conda install -c bluequartzsoftware simplnx

.. toctree::
   :maxdepth: 3
   :caption: Conceptual

   Installation
   Overview
   DataObjects
   Geometry

.. toctree::
   :maxdepth: 3
   :caption: Using SIMPLNX

   Python_Introduction
   User_API
   Reference_Frame_Notes

.. toctree::
   :maxdepth: 3
   :caption: Filter Writers

   Writing_A_New_Python_Filter
   Developer_API

.. toctree::
   :maxdepth: 1
   :caption: Filter API/Docs

@PLUGIN_LIST@

.. toctree::
   :maxdepth: 1
   :caption: Release Notes

   ReleaseNotes_110
   ReleaseNotes_120
   ReleaseNotes_121
   ReleaseNotes_123
   ReleaseNotes_124
   ReleaseNotes_126
   ReleaseNotes_127

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
