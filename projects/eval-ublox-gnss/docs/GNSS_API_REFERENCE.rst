no-OS GNSS API Reference
=========================

Overview
--------

The no-OS GNSS API provides a unified interface for GNSS timing and positioning functionality across different device types and protocols. This API abstracts the underlying complexity of UBX and NMEA protocols while maintaining high precision timing capabilities.

Table of Contents
-----------------

1. `Data Structures`_
2. `API Functions`_
3. `Constants and Enumerations`_
4. `Error Codes`_
5. `Usage Patterns`_
6. `Device Configuration Examples`_

Data Structures
---------------

Core Initialization Structures
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``struct no_os_gnss_pps_config``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configuration structure for Pulse-Per-Second (PPS) output.

.. code-block:: c

   struct no_os_gnss_pps_config {
       bool pps_enable;           // Enable/disable PPS output
       enum gnss_pps_frequency frequency;    // PPS frequency setting
       enum gnss_pps_pulse_length pulse_length;  // PPS pulse length setting
   };

**Fields:**

- ``pps_enable``: Boolean flag to enable or disable PPS output
- ``frequency``: PPS frequency (typically ``GNSS_PPS_1HZ`` for 1 Hz output)
- ``pulse_length``: PPS pulse duration (typically ``GNSS_PPS_LENGTH_100MS``)

``struct no_os_gnss_init_param``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Main initialization parameter structure.

.. code-block:: c

   struct no_os_gnss_init_param {
       uint32_t device_id;                           // Device identifier
       struct no_os_gnss_pps_config pps_config;     // PPS configuration
       struct gnss_init_param gnss_init_param;      // Platform-specific parameters
       const struct no_os_gnss_platform_ops *platform_ops;  // Platform operations
       void *extra;                                  // Additional platform data
   };

**Fields:**

- ``device_id``: Unique identifier for the device instance
- ``pps_config``: PPS output configuration settings
- ``gnss_init_param``: Platform-specific initialization parameters
- ``platform_ops``: Pointer to platform operations structure
- ``extra``: Additional platform-specific data (optional)

``struct no_os_gnss_desc``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Device descriptor structure.

.. code-block:: c

   struct no_os_gnss_desc {
       uint32_t device_id;                           // Device identifier
       struct gnss_dev *gnss_device;                // Underlying device handle
       const struct no_os_gnss_platform_ops *platform_ops;  // Platform operations
       struct no_os_mutex_desc *mutex;              // Thread safety mutex
       void *extra;                                  // Additional platform data
   };

Timing Data Structures
~~~~~~~~~~~~~~~~~~~~~~~

``struct gnss_precise_time``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

High-precision timing information structure.

.. code-block:: c

   struct gnss_precise_time {
       uint32_t unix_epoch;           // Unix timestamp (seconds)
       int32_t nanoseconds;           // Nanosecond fraction (can be negative)
       uint32_t time_accuracy;        // Time accuracy estimate (ns)
       bool time_valid;               // Time validity flag
       bool time_fully_resolved;      // Full resolution flag
       bool date_valid;               // Date validity flag
       uint16_t year, month, day;     // Date components
       uint8_t hour, minute, second;  // Time components
   };

**Fields:**

- ``unix_epoch``: Standard Unix timestamp in seconds since epoch
- ``nanoseconds``: Sub-second precision (UBX: actual ns, NMEA: ms × 1000000)
- ``time_accuracy``: Timing accuracy estimate in nanoseconds
- ``time_valid``: Indicates if timing data is valid and usable
- ``time_fully_resolved``: Indicates if time is fully resolved (UBX only)
- ``date_valid``: Indicates if date information is valid
- ``year``, ``month``, ``day``: Date components (UTC)
- ``hour``, ``minute``, ``second``: Time components (UTC)

``struct gnss_nmea_position``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Position and fix quality information (NMEA devices).

.. code-block:: c

   struct gnss_nmea_position {
       uint8_t fix_quality;          // Fix quality: 0=invalid, 1=GPS, 2=DGPS
       uint8_t satellites_used;      // Number of satellites in use
       float hdop;                   // Horizontal dilution of precision
       float altitude;               // Altitude above mean sea level (meters)
       float latitude;               // Latitude in decimal degrees
       float longitude;              // Longitude in decimal degrees
       bool position_valid;          // Position data validity
       bool fix_valid;               // GPS fix validity
   };

Platform Operations Structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``struct no_os_gnss_platform_ops``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Platform-specific operation function pointers.

.. code-block:: c

   struct no_os_gnss_platform_ops {
       int32_t (*init)(struct no_os_gnss_desc **, const struct no_os_gnss_init_param *);
       int32_t (*refresh_timing_data)(struct no_os_gnss_desc *);
       int32_t (*is_timing_valid)(struct no_os_gnss_desc *, bool *);
       int32_t (*get_unified_timing)(struct no_os_gnss_desc *, struct gnss_precise_time *);
       uint32_t (*get_unix_epoch_unified)(struct no_os_gnss_desc *, uint32_t *);
       int32_t (*get_position_data)(struct no_os_gnss_desc *, struct gnss_nmea_position *);
       int32_t (*remove)(struct no_os_gnss_desc *);
   };

API Functions
-------------

Device Management
~~~~~~~~~~~~~~~~~

``no_os_gnss_init()``
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int32_t no_os_gnss_init(struct no_os_gnss_desc **desc,
                           const struct no_os_gnss_init_param *param);

**Description:** Initialize GNSS device and communication interfaces.

**Parameters:**

- ``desc``: Pointer to device descriptor pointer (output)
- ``param``: Initialization parameters

**Returns:**

- ``0``: Success
- ``< 0``: Error code

**Example:**

.. code-block:: c

   struct no_os_gnss_desc *gnss_desc;
   struct no_os_gnss_init_param init_param = {
       .device_id = 0,
       .pps_config = {
           .pps_enable = true,
           .frequency = GNSS_PPS_1HZ,
           .pulse_length = GNSS_PPS_LENGTH_100MS
       },
       .gnss_init_param = {
           .uart_init = &uart_gnss_ip,
           .device_type = GNSS_DEVICE_UBX_CAPABLE,
           .baud_rate = 38400
       },
       .platform_ops = &ublox_gnss_ops
   };

   int ret = no_os_gnss_init(&gnss_desc, &init_param);

``no_os_gnss_remove()``
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int32_t no_os_gnss_remove(struct no_os_gnss_desc *desc);

**Description:** Free resources allocated by ``no_os_gnss_init()``.

**Parameters:**

- ``desc``: Device descriptor

**Returns:**

- ``0``: Success
- ``< 0``: Error code

Timing Functions
~~~~~~~~~~~~~~~~

``no_os_gnss_refresh_timing_data()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int32_t no_os_gnss_refresh_timing_data(struct no_os_gnss_desc *desc);

**Description:** Refresh timing data from GNSS device. Must be called before accessing timing information.

**Parameters:**

- ``desc``: Device descriptor

**Returns:**

- ``0``: Success
- ``< 0``: Error code

**Usage Notes:**

- Call this function periodically to update timing data
- Required before using any timing getter functions
- For UBX devices: polls NAV-PVT and NAV-TIMEUTC messages
- For NMEA devices: parses GPRMC sentences for timing

``no_os_gnss_is_timing_valid()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int32_t no_os_gnss_is_timing_valid(struct no_os_gnss_desc *desc, bool *valid);

**Description:** Check if current timing data is valid and can be used for synchronization.

**Parameters:**

- ``desc``: Device descriptor
- ``valid``: Pointer to validity flag (output)

**Returns:**

- ``0``: Success
- ``< 0``: Error code

**Example:**

.. code-block:: c

   bool timing_valid;
   int ret = no_os_gnss_is_timing_valid(gnss_desc, &timing_valid);
   if (ret == 0 && timing_valid) {
       // Timing data is valid, safe to use for synchronization
   }

``no_os_gnss_get_unified_timing()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int32_t no_os_gnss_get_unified_timing(struct no_os_gnss_desc *desc,
                                         struct gnss_precise_time *timing);

**Description:** Get comprehensive timing information with best available precision.

**Parameters:**

- ``desc``: Device descriptor
- ``timing``: Pointer to timing structure (output)

**Returns:**

- ``0``: Success
- ``< 0``: Error code

**Precision Notes:**

- **UBX devices**: Nanosecond precision timing
- **NMEA devices**: Millisecond precision converted to nanosecond format

``no_os_gnss_get_unix_epoch_unified()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   uint32_t no_os_gnss_get_unix_epoch_unified(struct no_os_gnss_desc *desc,
                                              uint32_t *fractional_seconds);

**Description:** Get Unix epoch timestamp with best available sub-second precision.

**Parameters:**

- ``desc``: Device descriptor
- ``fractional_seconds``: Pointer to fractional seconds (output)

**Returns:**

- Unix epoch time in seconds
- ``0``: Error or invalid data

**Fractional Format:**

- **UBX devices**: Microseconds (0-999999)
- **NMEA devices**: Milliseconds × 1000 (0-999000, steps of 1000)

Position Functions (NMEA Devices)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``no_os_gnss_get_position_data()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int32_t no_os_gnss_get_position_data(struct no_os_gnss_desc *desc,
                                        struct gnss_nmea_position *position_data);

**Description:** Get GPS fix quality and position information for NMEA-only devices.

**Parameters:**

- ``desc``: Device descriptor
- ``position_data``: Pointer to position structure (output)

**Returns:**

- ``0``: Success
- ``< 0``: Error code (including data not available)

**Usage Notes:**

- Only available for NMEA-only devices
- Position data refreshed at 1/10th rate of timing data
- Provides fix quality, satellite count, and coordinates

Constants and Enumerations
---------------------------

PPS Configuration
~~~~~~~~~~~~~~~~~

.. code-block:: c

   enum gnss_pps_frequency {
       GNSS_PPS_1HZ = 1,      // 1 Hz (standard)
       GNSS_PPS_10HZ = 10     // 10 Hz (high rate)
   };

   enum gnss_pps_pulse_length {
       GNSS_PPS_LENGTH_100MS = 100,    // 100 millisecond pulse
       GNSS_PPS_LENGTH_10MS = 10       // 10 millisecond pulse
   };

Device Types
~~~~~~~~~~~~

.. code-block:: c

   enum device_type {
       GNSS_DEVICE_UBX_CAPABLE,    // Modern u-blox devices with UBX protocol
       GNSS_DEVICE_NMEA_ONLY       // NMEA-only devices (any manufacturer)
   };

   enum nmea_constellation_support {
       NMEA_GPS_ONLY,      // Legacy GPS-only devices ($GPxxx sentences)
       NMEA_GNSS_MULTI     // Multi-constellation devices ($GNxxx sentences)
   };

Error Codes
-----------

+-----------------+------------------+-----------------------------------------------+
| Code            | Name             | Description                                   |
+=================+==================+===============================================+
| ``0``           | Success          | Operation completed successfully              |
+-----------------+------------------+-----------------------------------------------+
| ``-EINVAL``     | Invalid argument | NULL pointer or invalid parameter             |
+-----------------+------------------+-----------------------------------------------+
| ``-ENOMEM``     | Out of memory    | Memory allocation failed                      |
+-----------------+------------------+-----------------------------------------------+
| ``-EIO``        | I/O error        | Communication error with device               |
+-----------------+------------------+-----------------------------------------------+
| ``-ETIMEDOUT``  | Timeout          | Operation timed out                           |
+-----------------+------------------+-----------------------------------------------+
| ``-ENODATA``    | No data          | Valid response received but no data available |
+-----------------+------------------+-----------------------------------------------+

Usage Patterns
--------------

Basic Timing Application
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   struct no_os_gnss_desc *gnss_desc;
   struct gnss_precise_time timing;
   uint32_t fractional_seconds;
   bool timing_valid;

   // Initialize device
   ret = no_os_gnss_init(&gnss_desc, &init_param);

   while (1) {
       // Refresh timing data
       ret = no_os_gnss_refresh_timing_data(gnss_desc);
       if (ret) continue;

       // Check validity
       ret = no_os_gnss_is_timing_valid(gnss_desc, &timing_valid);
       if (ret || !timing_valid) continue;

       // Get timing information
       ret = no_os_gnss_get_unified_timing(gnss_desc, &timing);
       uint32_t epoch = no_os_gnss_get_unix_epoch_unified(gnss_desc, &fractional_seconds);

       // Use timing for synchronization
       sync_rtc_from_gnss(&timing);

       delay_ms(1000);
   }

Position Monitoring (NMEA Devices)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   struct gnss_nmea_position position;

   // After timing data refresh
   ret = no_os_gnss_get_position_data(gnss_desc, &position);
   if (ret == 0) {
       printf("GPS Fix Quality: %d\n", position.fix_quality);
       printf("Satellites: %d\n", position.satellites_used);
       printf("Position: %.6f°, %.6f°\n", position.latitude, position.longitude);
       printf("Altitude: %.1f m\n", position.altitude);
   }

Device Configuration Examples
-----------------------------

UBX-Capable Devices (Modern u-blox)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   .gnss_init_param = {
       .uart_init = &uart_gnss_ip,
       .device_type = GNSS_DEVICE_UBX_CAPABLE,
       .ubx_input_enable = ENABLE,
       .ubx_output_enable = ENABLE,
       .nmea_output_enable = DISABLE,
       .baud_rate = 38400
   }

NMEA GPS-Only Devices
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   .gnss_init_param = {
       .uart_init = &uart_gnss_ip,
       .device_type = GNSS_DEVICE_NMEA_ONLY,
       .nmea_constellation_type = NMEA_GPS_ONLY,
       .baud_rate = 9600
   }

NMEA Multi-Constellation Devices
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   .gnss_init_param = {
       .uart_init = &uart_gnss_ip,
       .device_type = GNSS_DEVICE_NMEA_ONLY,
       .nmea_constellation_type = NMEA_GNSS_MULTI,
       .baud_rate = 9600
   }

Thread Safety
-------------

The API provides thread safety through internal mutex protection. Multiple threads can safely:

- Call getter functions simultaneously
- Access different device instances concurrently

**Note:** Avoid calling ``no_os_gnss_refresh_timing_data()`` from multiple threads on the same device instance.

Performance Considerations
--------------------------

Timing Precision
~~~~~~~~~~~~~~~~

- **UBX devices**: Nanosecond precision with ~25ns accuracy
- **NMEA devices**: Millisecond precision, suitable for most applications

Update Rates
~~~~~~~~~~~~

- **Timing data**: Updated every refresh cycle
- **Position data** (NMEA only): Updated every 10th refresh cycle for efficiency

Communication
~~~~~~~~~~~~~

- UBX protocol: Binary, efficient, less bandwidth
- NMEA protocol: ASCII, universal compatibility, higher bandwidth

Debugging
---------

Enable debug output by defining:

.. code-block:: c

   #define GNSS_DEBUG

Common debug information includes:

- Protocol message parsing
- Communication errors
- Timing validation status
- GPS week rollover corrections