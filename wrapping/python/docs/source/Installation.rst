Installation
============

Latest Version: 24.08.10
-------------------------

The *simplnx* library can be installed through an Anaconda packages from the *BlueQuartzSoftware* channel. This can be achieved
by creating a new virtual environment and installing SIMPLNX into that environment.


.. attention::

    Intel based MacOS machines can *ONLY* install up to Python 3.11. Due to dependecies from
    conda-forge, Python 3.12 is not possible as of AUG 2024.


Full DREAM3D-NX Installation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. attention::

    This kind of installation will allow you to utilize the full DREAM3D-NX application

.. code:: shell

    conda config --add channels conda-forge
    conda config --set channel_priority strict
    conda create -n nxpython python=3.12
    conda activate nxpython
    conda install -c bluequartzsoftware dream3dnx
