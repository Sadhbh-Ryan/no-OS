GNSS Driver API Reference
=========================

This document provides a complete reference for the no-OS GNSS driver API.

Table of Contents
-----------------

1. `Core API Functions`_
2. `Data Structures`_
3. `Enumerations`_
4. `Platform Operations`_
5. `Usage Examples`_
6. `Error Codes`_

Core API Functions
------------------

Device Management
~~~~~~~~~~~~~~~~~

``no_os_gnss_init()``
^^^^^^^^^^^^^^^^^^^^^

Initialize a GNSS device.

.. code-block:: c

   int32_t no_os_gnss_init(struct no_os_gnss_desc **desc,
                           struct no_os_gnss_init_param *init_param);

**Parameters:**

- ``desc``: Pointer to GNSS device descriptor pointer
- ``init_param``: Initialization parameters

**Returns:** 0 on success, negative error code on failure

``no_os_gnss_remove()``
^^^^^^^^^^^^^^^^^^^^^^^^

Free resources allocated by ``no_os_gnss_init()``.

.. code-block:: c

   int32_t no_os_gnss_remove(struct no_os_gnss_desc *desc);

Unified Timing API
~~~~~~~~~~~~~~~~~~

``no_os_gnss_refresh_timing_data()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Refresh timing data from the GNSS device.

.. code-block:: c

   int32_t no_os_gnss_refresh_timing_data(struct no_os_gnss_desc *desc);

**Note:** This function must be called before accessing timing data.

``no_os_gnss_is_timing_valid()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Check if timing data is valid.

.. code-block:: c

   bool no_os_gnss_is_timing_valid(struct no_os_gnss_desc *desc);

``no_os_gnss_get_unified_timing()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Get comprehensive timing information.

.. code-block:: c

   int32_t no_os_gnss_get_unified_timing(struct no_os_gnss_desc *desc,
                                         struct ublox_precise_time *precise_time);

PPS Configuration
~~~~~~~~~~~~~~~~~

``no_os_gnss_configure_pps()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configure PPS output with platform-specific parameters.

.. code-block:: c

   int32_t no_os_gnss_configure_pps(struct no_os_gnss_desc *desc,
                                    const void *pps_config);

**Example (UBX devices):**

.. code-block:: c

   struct ublox_pps_config pps_config = {
       .pulse_mode = UBLOX_PPS_PULSE_MODE_PERIOD,
       .period_us = 1000000,  // 1Hz
       .length_us = 500000,   // 500ms pulse width
       .enable = UBLOX_PPS_ENABLE,
       .polarity = UBLOX_PPS_POLARITY_RISING
   };
   ret = no_os_gnss_configure_pps(gnss_desc, &pps_config);

Data Structures
---------------

``struct no_os_gnss_init_param``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Initialization parameters for GNSS device.

.. code-block:: c

   struct no_os_gnss_init_param {
       uint8_t device_id;
       struct no_os_uart_init_param *uart_init;
       struct no_os_gpio_desc *gpio_reset;
       struct no_os_irq_ctrl_desc *irq_ctrl;
       enum device_type device_type;
       enum nmea_constellation_support nmea_constellation_type;
       struct {
           enum no_os_gnss_enable ubx_input_enable;
           enum no_os_gnss_enable nmea_input_enable;
           enum no_os_gnss_enable ubx_output_enable;
           enum no_os_gnss_enable nmea_output_enable;
       } protocols;
       uint32_t baud_rate;
       void *pps_config;
       const struct no_os_gnss_platform_ops *platform_ops;
       void *extra;
   };

``struct ublox_precise_time``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Precise timing information structure.

.. code-block:: c

   struct ublox_precise_time {
       uint32_t unix_epoch;
       int32_t nanoseconds;
       uint32_t time_accuracy;
       bool time_valid;
       bool time_fully_resolved;
       bool date_valid;
       uint16_t year;
       uint8_t month;
       uint8_t day;
       uint8_t hour;
       uint8_t minute;
       uint8_t second;
   };

Enumerations
------------

Device Types
~~~~~~~~~~~~

.. code-block:: c

   enum device_type {
       DEVICE_TYPE_UBX_CAPABLE,  // UBX-capable device
       DEVICE_TYPE_NMEA_ONLY     // NMEA-only device
   };

PPS Configuration (UBX-specific)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   enum ublox_pps_pulse_mode {
       UBLOX_PPS_PULSE_MODE_PERIOD = 0,
       UBLOX_PPS_PULSE_MODE_FREQUENCY = 1
   };

   enum ublox_pps_polarity {
       UBLOX_PPS_POLARITY_FALLING = 0,
       UBLOX_PPS_POLARITY_RISING = 1
   };

Usage Example
-------------

.. code-block:: c

   #include "no_os_gnss.h"
   #include "ublox_gnss.h"

   int32_t gnss_example(void)
   {
       struct no_os_gnss_desc *gnss_desc;
       struct ublox_precise_time precise_time;
       int32_t ret;

       // Initialize
       ret = no_os_gnss_init(&gnss_desc, &no_os_gnss_init_param);
       if (ret) return ret;

       // Configure PPS
       struct ublox_pps_config pps_config = {
           .pulse_mode = UBLOX_PPS_PULSE_MODE_PERIOD,
           .period_us = 1000000,
           .enable = UBLOX_PPS_ENABLE
       };
       no_os_gnss_configure_pps(gnss_desc, &pps_config);

       // Get timing
       while (1) {
           ret = no_os_gnss_refresh_timing_data(gnss_desc);
           if (ret == 0 && no_os_gnss_is_timing_valid(gnss_desc)) {
               no_os_gnss_get_unified_timing(gnss_desc, &precise_time);
               // Use timing data...
           }
           no_os_mdelay(5000);
       }

       no_os_gnss_remove(gnss_desc);
       return 0;
   }

Error Codes
-----------

- ``0``: Success
- ``-EINVAL``: Invalid argument
- ``-ENOMEM``: Out of memory
- ``-EIO``: I/O error
- ``-ENOTSUP``: Operation not supported

Thread Safety
-------------

All API functions are thread-safe with internal mutex protection.