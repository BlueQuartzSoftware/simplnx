Installation
============

Latest Version: 24.06.05
-------------------------

The *simplnx* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment and installing SIMPLNX into that environment.


SIMPLNX Installation
^^^^^^^^^^^^^^^^^^^^

.. attention::

    This kind of installation will allow you to utilize the SIMPLNX library through python. 

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.10
    conda activate nxpython
    conda install -c bluequartzsoftware simplnx



Full DREAM3D-NX Installation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. attention::

    This kind of installation will allow you to utilize the full DREAM3D-NX application

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.10
    conda activate nxpython
    conda install -c bluequartzsoftware dream3dnx

