MAX32672EVkit no-OS Example Project
======================================

.. no-os-doxygen::

Prerequisites
-------------

Prior to building the project, a couple steps are necessary in order to get the Maxim Micros SDK and setup the environment. These are presented in the ***Build Prerequisites*** section of no-OS build guide available here: https://wiki.analog.com/resources/no-os/build .

The MaximSDK provides distributions of `arm-none-eabi-` GCC compiler + utilities and `OpenOCD`, so you don't have to install these separately.

Building the project
--------------------

The project includes 2 different examples:

1. basic_example - may be selected by setting the MAX32672_BASIC_EXAMPLE = y (and all the other examples to n) in the main Makefile.
This example is meant to print "Hello world" over UART0. 

2. ping_example (selected by default) - may be selected by setting the MAX32672_PING_EXAMPLE = y (and all the other examples to n) in the main Makefile.
When the code is running, the board will be pingable using the configured ip address of the board. 

#. Type `make RELEASE=y -j`, in order to build the project. The `RELEASE` flag adds `-O2` optimization. It should be omitted during debugging.

A successful build should end with the following terminal output:

.. code-block:: bash

	[11:11:27] [HEX] max32672.hex
	[11:11:27] max32672.hex is ready
	rm /home/xvr/MaximSDK_new/Libraries/CMSIS/Device/Maxim/MAX32672/Source/GCC/startup_max32672.s
	[11:11:21] Done (build/max32672.elf)

The binary and executable files are now available in the `build` directory (`max32672.hex` and `max32672.elf` files).

Programming the MCU
-------------------

Before the MCU can be programmed a few steps are necessary:

#. Replace the DAPLINK firmware for the MAX32625PICO. This is only required to be done one time.

    * Download the firmware image from the following link: https://github.com/MaximIntegrated/max32625pico-firmware-images/raw/master/bin/max32625_max32650fthr_if_crc_swd_v1.0.6.bin .

    * Make sure the MAX32625PICO is not connected to the PC.

    * Press the button on the MAX32625PICO, and keep it pressed while you plug the USB cable in the MAX32625PICO.

    * Release the button once you can see a `MAINTENANCE` drive being mounted.

    * Copy the firmware binary file to the `MAINTANANCE` drive. It should unmount and a `DAPLINK` drive should appear instead.

#. Connect the MAX32625PICO board to the PC and the APARD32690 board. If everything went well, you should see a mass storage device named `DAPLINK` in your filesystem.

#. Power on the APARD32690 board.

The microcontroller may be programmed in 2 ways:
1. Drag-and-drop the binary (.hex) file in the `DAPLINK` directory. The drive should be unmounted and mounted again, once the programming is done.
2. While in the project's root directory, type `make RELEASE=y run`. This method uses OpenOCD in order to load the binary file. If the programming is successful, you should see the following terminal output:

.. code-block:: bash

	** Programming Started **
	** Programming Finished **
	** Verify Started **
	** Verified OK **
	** Resetting Target **
	shutdown command invoked
	[11:27:42] apard32690.elf uploaded to board
