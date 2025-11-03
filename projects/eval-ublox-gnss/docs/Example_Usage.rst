GNSS Driver Example Usage and Testing Guide
===========================================

This document provides comprehensive testing procedures, hardware setup instructions for the APARD32690 board, and troubleshooting guidance for the no-OS GNSS driver project.

Table of Contents
-----------------

- `Hardware Setup`_
- `Software Setup`_
- `Basic Usage Examples`_
- `Advanced Configuration`_
- `Testing Procedures`_
- `Performance Validation`_
- `Troubleshooting Guide`_
- `Production Deployment`_

---

Hardware Setup
--------------

Required Components
~~~~~~~~~~~~~~~~~~~

Primary Board
^^^^^^^^^^^^^

- **APARD32690**: MAX32690 Evaluation Kit from Analog Devices
- **Power Supply**: 5V USB or external power adapter
- **USB Cable**: USB-A to USB-Micro for programming and debug output

GNSS Module Options
^^^^^^^^^^^^^^^^^^^

1. **u-blox ZED-F9P** (Recommended for high precision)

   - Multi-band RTK GNSS receiver
   - Full UBX protocol support
   - Centimeter-level precision
   - 3.3V operation

2. **u-blox ZED-F9T** (Timing-focused)

   - High-precision timing GNSS receiver
   - Optimized for timing applications
   - Nanosecond-level timing accuracy
   - 3.3V operation

3. **u-blox NEO-M8N** (General purpose)

   - Standard precision GNSS receiver
   - Cost-effective option
   - Meter-level accuracy
   - 3.3V operation

Physical Connections
~~~~~~~~~~~~~~~~~~~~

APARD32690 Pin Assignments
^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-----------+----------------+-----------+------------------------------------+
| Function  | APARD32690 Pin | Pin Label | Description                        |
+===========+================+===========+====================================+
| Power     | J4-3           | 3V3       | 3.3V power output (500mA max)      |
+-----------+----------------+-----------+------------------------------------+
| Ground    | J4-4           | GND       | Common ground reference            |
+-----------+----------------+-----------+------------------------------------+
| UART1 TX  | J1-25          | P2.1      | Data transmission to GNSS          |
+-----------+----------------+-----------+------------------------------------+
| UART1 RX  | J1-24          | P2.0      | Data reception from GNSS           |
+-----------+----------------+-----------+------------------------------------+
| Reset     | J1-22          | P2.7      | GNSS hardware reset (optional)     |
+-----------+----------------+-----------+------------------------------------+
| PPS       | J1-20          | P2.9      | Pulse per second input (optional)  |
+-----------+----------------+-----------+------------------------------------+

GNSS Module Connections
^^^^^^^^^^^^^^^^^^^^^^^

::

   APARD32690 Connector          u-blox GNSS Module
   ===================          ==================
   J4-3 (3V3)      -----------> VCC/VDD (3.3V)
   J4-4 (GND)      -----------> GND
   J1-25 (P2.1)    -----------> RXD (GNSS receive)
   J1-24 (P2.0)    <----------- TXD (GNSS transmit)
   J1-22 (P2.7)    -----------> RST/RESET (optional)
   J1-20 (P2.9)    <----------- PPS/TIMEPULSE (optional)

Detailed Wiring Diagram
^^^^^^^^^^^^^^^^^^^^^^^^

::

   APARD32690 Board               u-blox GNSS Module


    J4 (Power)
     3: 3V3                   >  VCC (Pin 1)
     4: GND                   >  GND (Pin 2)

    J1 (GPIO/UART)
     20: P2.9 <                  PPS (Pin 3)
     22: P2.7                  >  RST (Pin 4)
     24: P2.0 <                  TXD (Pin 5)
     25: P2.1                  >  RXD (Pin 6)



Antenna Setup
~~~~~~~~~~~~~

Antenna Requirements
^^^^^^^^^^^^^^^^^^^^

- **Frequency Coverage**: GPS L1/L2, GLONASS G1/G2, Galileo E1/E5, BeiDou B1/B2
- **Gain**: 25-35 dB typical
- **LNA**: Low noise amplifier recommended for indoor testing
- **Connector**: SMA, MMCX, or u.FL depending on module

Antenna Placement Guidelines
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Outdoor Testing** (Recommended):

   - Clear sky view
   - Away from large metal structures
   - Minimal multipath interference

2. **Indoor Testing** (Limited):

   - Near window with sky view
   - Active antenna with LNA
   - Extended acquisition time expected

3. **Ground Plane**:

   - Minimum 70mm x 70mm metal ground plane
   - Improve GNSS performance and reduce multipath

Power Supply Considerations
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Current Requirements
^^^^^^^^^^^^^^^^^^^^

- **APARD32690**: ~150mA typical operation
- **u-blox GNSS**: 25-35mA typical (varies by model)
- **Total System**: ~200mA typical, 300mA peak

Power Supply Options
^^^^^^^^^^^^^^^^^^^^

1. **USB Power** (Recommended for development):

   - Connect via USB-Micro cable
   - Stable 5V supply from PC/hub
   - Integrated current limiting

2. **External Power**:

   - 5V @ 500mA minimum supply
   - Connect to appropriate power jack
   - Verify polarity before connection

---

Software Setup
--------------

Development Environment
~~~~~~~~~~~~~~~~~~~~~~~

Prerequisites
^^^^^^^^^^^^^

1. **no-OS Framework**: Latest version from Analog Devices
2. **Maxim Micros SDK**: For MAX32690 platform support
3. **ARM GCC Toolchain**: Version 9.3.0 or later
4. **Make**: GNU Make 4.0 or later
5. **Terminal Emulator**: PuTTY, TeraTerm, or similar

Installation Steps
^^^^^^^^^^^^^^^^^^

1. **Download and Extract no-OS**:

   .. code-block:: bash

      git clone https://github.com/analogdevicesinc/no-OS.git
      cd no-OS

2. **Install ARM Toolchain** (Windows):

   .. code-block:: bash

      # Download and install GNU ARM Embedded Toolchain
      # Add to PATH environment variable
      arm-none-eabi-gcc --version  # Verify installation

3. **Setup Maxim SDK**:

   .. code-block:: bash

      # Download and install Maxim Micros SDK
      # Set MAXIM_PATH environment variable
      export MAXIM_PATH=/path/to/maxim/sdk

4. **Verify Environment**:

   .. code-block:: bash

      cd projects/eval-ublox-gnss
      make PLATFORM=maxim TARGET=max32690 clean

Initial Build and Flash
~~~~~~~~~~~~~~~~~~~~~~~

1. **Navigate to Project Directory**:

   .. code-block:: bash

      cd projects/eval-ublox-gnss

2. **Clean Previous Build**:

   .. code-block:: bash

      make PLATFORM=maxim TARGET=max32690 reset

3. **Build Project**:

   .. code-block:: bash

      make PLATFORM=maxim TARGET=max32690

4. **Flash to Board**:

   .. code-block:: bash

      make PLATFORM=maxim TARGET=max32690 run

Serial Console Setup
~~~~~~~~~~~~~~~~~~~~~

Console Configuration
^^^^^^^^^^^^^^^^^^^^^^

- **Baud Rate**: 115200 bps
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

Terminal Commands
^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # Windows (using PuTTY)
   putty -serial COM3 -sercfg 115200,8,n,1,N

   # Linux/macOS (using screen)
   screen /dev/ttyUSB0 115200

   # Alternative (using minicom)
   minicom -D /dev/ttyUSB0 -b 115200

---

Basic Usage Examples
--------------------

Example 1: Basic GNSS Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include "no_os_gnss.h"
   #include "ublox_gnss.h"
   #include "common_data.h"

   int basic_gnss_example(void)
   {
       struct no_os_gnss_desc *gnss_desc;
       int32_t ret;

       pr_info("=== Basic GNSS Initialization Example ===\n");

       // Initialize GNSS device
       ret = no_os_gnss_init(&gnss_desc, &no_os_gnss_init_param);
       if (ret) {
           pr_err("GNSS initialization failed: %d\n", ret);
           return ret;
       }

       pr_info("GNSS device initialized successfully\n");
       pr_info("Device type: %s\n",
               (no_os_gnss_init_param.device_type == DEVICE_TYPE_UBX_CAPABLE) ?
               "UBX-capable" : "NMEA-only");

       // Hardware reset test (if GPIO available)
       if (no_os_gnss_init_param.gpio_reset) {
           pr_info("Testing hardware reset...\n");
           ret = no_os_gnss_hw_reset(gnss_desc);
           if (ret) {
               pr_warning("Hardware reset failed: %d\n", ret);
           } else {
               pr_info("Hardware reset successful\n");
           }
       }

       // Cleanup
       ret = no_os_gnss_remove(gnss_desc);
       if (ret) {
           pr_err("GNSS cleanup failed: %d\n", ret);
       }

       pr_info("=== Basic Example Complete ===\n");
       return 0;
   }

Example 2: Timing Data Acquisition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   int timing_data_example(void)
   {
       struct no_os_gnss_desc *gnss_desc;
       struct ublox_precise_time precise_time;
       uint32_t fractional_seconds;
       uint32_t unix_epoch;
       int32_t ret;
       int attempts = 0;
       const int max_attempts = 60;  // 5 minutes at 5-second intervals

       pr_info("=== Timing Data Acquisition Example ===\n");

       // Initialize GNSS device
       ret = no_os_gnss_init(&gnss_desc, &no_os_gnss_init_param);
       if (ret) {
           pr_err("GNSS initialization failed: %d\n", ret);
           return ret;
       }

       // Wait for valid timing data
       pr_info("Waiting for GNSS fix and valid timing data...\n");
       pr_info("This may take several minutes outdoors, longer indoors.\n");

       while (attempts < max_attempts) {
           // Refresh timing data from device
           ret = no_os_gnss_refresh_timing_data(gnss_desc);
           if (ret) {
               pr_warning("Failed to refresh timing data: %d (attempt %d/%d)\n",
                          ret, attempts + 1, max_attempts);
           } else {
               // Check if timing data is valid
               if (no_os_gnss_is_timing_valid(gnss_desc)) {
                   pr_info("Valid timing data acquired!\n");
                   break;
               } else {
                   pr_info("Waiting for fix... (attempt %d/%d)\n",
                           attempts + 1, max_attempts);
               }
           }

           attempts++;
           no_os_mdelay(5000);  // Wait 5 seconds between attempts
       }

       if (attempts >= max_attempts) {
           pr_err("Failed to acquire valid timing data after %d attempts\n", max_attempts);
           pr_err("Check antenna connection and sky visibility\n");
           no_os_gnss_remove(gnss_desc);
           return -ETIMEDOUT;
       }

       // Get detailed timing information
       ret = no_os_gnss_get_unified_timing(gnss_desc, &precise_time);
       if (ret) {
           pr_err("Failed to get unified timing: %d\n", ret);
       } else {
           pr_info("=== Detailed Timing Information ===\n");
           pr_info("Date/Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                   precise_time.year, precise_time.month, precise_time.day,
                   precise_time.hour, precise_time.minute, precise_time.second);

           if (no_os_gnss_init_param.device_type == DEVICE_TYPE_UBX_CAPABLE) {
               pr_info("Nanoseconds: %ld\n", precise_time.nanoseconds);
               pr_info("Time Accuracy: %lu ns\n", precise_time.time_accuracy);
           }

           pr_info("Validity Flags:\n");
           pr_info("  Time Valid: %s\n", precise_time.time_valid ? "YES" : "NO");
           pr_info("  Time Fully Resolved: %s\n", precise_time.time_fully_resolved ? "YES" : "NO");
           pr_info("  Date Valid: %s\n", precise_time.date_valid ? "YES" : "NO");
       }

       // Get Unix epoch with fractional seconds
       unix_epoch = no_os_gnss_get_unix_epoch_unified(gnss_desc, &fractional_seconds);
       if (unix_epoch != 0) {
           pr_info("Unix Epoch: %lu.%06lu\n", unix_epoch, fractional_seconds);
       }

       // Cleanup
       no_os_gnss_remove(gnss_desc);
       pr_info("=== Timing Example Complete ===\n");
       return 0;
   }

Example 3: PPS Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   int pps_configuration_example(void)
   {
       struct no_os_gnss_desc *gnss_desc;
       struct ublox_pps_config pps_config;
       int32_t ret;

       pr_info("=== PPS Configuration Example ===\n");

       // Initialize GNSS device
       ret = no_os_gnss_init(&gnss_desc, &no_os_gnss_init_param);
       if (ret) {
           pr_err("GNSS initialization failed: %d\n", ret);
           return ret;
       }

       // Configure PPS for 1Hz output with 100ms pulse width
       pps_config = (struct ublox_pps_config) {
           .pulse_mode = UBLOX_PPS_PULSE_MODE_PERIOD,
           .length_mode = UBLOX_PPS_LENGTH_MODE_MICROSECONDS,
           .period_us = 1000000,        // 1 second (1Hz)
           .period_lock_us = 1000000,   // Same when locked
           .length_us = 100000,         // 100ms pulse width
           .length_lock_us = 100000,    // Same when locked
           .enable = UBLOX_PPS_ENABLE,
           .sync_gnss = UBLOX_PPS_ENABLE,
           .use_locked = UBLOX_PPS_ENABLE,
           .align_to_tow = UBLOX_PPS_ENABLE,
           .polarity = UBLOX_PPS_POLARITY_RISING,
           .time_grid = UBLOX_PPS_TIME_GRID_UTC
       };

       pr_info("Configuring PPS output...\n");
       ret = no_os_gnss_configure_pps(gnss_desc, &pps_config);
       if (ret) {
           pr_err("PPS configuration failed: %d\n", ret);
       } else {
           pr_info("PPS configured successfully\n");

           // Verify PPS is enabled
           bool pps_enabled = no_os_gnss_is_pps_enabled(gnss_desc);
           pr_info("PPS Status: %s\n", pps_enabled ? "ENABLED" : "DISABLED");

           // Read back current configuration
           struct ublox_pps_config current_config;
           ret = no_os_gnss_get_pps_config(gnss_desc, &current_config);
           if (ret == 0) {
               pr_info("PPS Configuration Readback:\n");
               pr_info("  Mode: %s\n",
                       (current_config.pulse_mode == UBLOX_PPS_PULSE_MODE_PERIOD) ?
                       "Period" : "Frequency");
               pr_info("  Period: %lu μs\n", current_config.period_us);
               pr_info("  Pulse Width: %lu μs\n", current_config.length_us);
               pr_info("  Polarity: %s\n",
                       (current_config.polarity == UBLOX_PPS_POLARITY_RISING) ?
                       "Rising" : "Falling");
           }
       }

       // Test PPS enable/disable
       pr_info("Testing PPS enable/disable...\n");

       ret = no_os_gnss_set_pps_enable(gnss_desc, NO_OS_GNSS_DISABLE);
       if (ret == 0) {
           pr_info("PPS disabled successfully\n");
           no_os_mdelay(2000);

           ret = no_os_gnss_set_pps_enable(gnss_desc, NO_OS_GNSS_ENABLE);
           if (ret == 0) {
               pr_info("PPS re-enabled successfully\n");
           }
       }

       // Cleanup
       no_os_gnss_remove(gnss_desc);
       pr_info("=== PPS Example Complete ===\n");
       return 0;
   }

---

Advanced Configuration
----------------------

Custom PPS Configurations
~~~~~~~~~~~~~~~~~~~~~~~~~~

High-Frequency PPS (10Hz)
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   struct ublox_pps_config pps_10hz = {
       .pulse_mode = UBLOX_PPS_PULSE_MODE_FREQUENCY,
       .length_mode = UBLOX_PPS_LENGTH_MODE_MICROSECONDS,
       .frequency_hz = 10,              // 10Hz output
       .period_us = 100000,            // 100ms period
       .length_us = 10000,             // 10ms pulse width (10% duty cycle)
       .enable = UBLOX_PPS_ENABLE,
       .sync_gnss = UBLOX_PPS_ENABLE,
       .polarity = UBLOX_PPS_POLARITY_RISING,
       .time_grid = UBLOX_PPS_TIME_GRID_UTC
   };

Precise Timing PPS (1PPS with 1μs width)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   struct ublox_pps_config pps_precise = {
       .pulse_mode = UBLOX_PPS_PULSE_MODE_PERIOD,
       .length_mode = UBLOX_PPS_LENGTH_MODE_MICROSECONDS,
       .period_us = 1000000,           // 1 second period
       .length_us = 1,                 // 1μs pulse width for precise timing
       .enable = UBLOX_PPS_ENABLE,
       .sync_gnss = UBLOX_PPS_ENABLE,
       .use_locked = UBLOX_PPS_ENABLE,
       .align_to_tow = UBLOX_PPS_ENABLE,
       .polarity = UBLOX_PPS_POLARITY_RISING,
       .time_grid = UBLOX_PPS_TIME_GRID_UTC
   };

NMEA-Only Device Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Configure for NMEA-only device (legacy GPS)
   struct no_os_gnss_init_param nmea_gps_init = {
       .device_id = 0,
       .uart_init = &uart_gnss_ip,
       .gpio_reset = gpio_gnss_reset_desc,
       .device_type = DEVICE_TYPE_NMEA_ONLY,
       .nmea_constellation_type = NMEA_GPS_ONLY,  // GPS-only ($GPxxx sentences)
       .baud_rate = 4800,  // Common baudrate for legacy devices
       .platform_ops = &ublox_gnss_platform_ops
   };

   // Configure for modern GNSS multi-constellation
   struct no_os_gnss_init_param nmea_gnss_init = {
       .device_id = 0,
       .uart_init = &uart_gnss_ip,
       .gpio_reset = gpio_gnss_reset_desc,
       .device_type = DEVICE_TYPE_NMEA_ONLY,
       .nmea_constellation_type = NMEA_GNSS_MULTI,  // Multi-GNSS ($GNxxx sentences)
       .baud_rate = 9600,
       .platform_ops = &ublox_gnss_platform_ops
   };

Multi-Device Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   int multi_device_example(void)
   {
       struct no_os_gnss_desc *primary_gnss;
       struct no_os_gnss_desc *backup_gnss;
       int32_t ret;

       // Configure primary device (UBX-capable)
       struct no_os_gnss_init_param primary_init = no_os_gnss_init_param;
       primary_init.device_id = 0;
       primary_init.device_type = DEVICE_TYPE_UBX_CAPABLE;

       // Configure backup device (NMEA-only)
       struct no_os_gnss_init_param backup_init = no_os_gnss_init_param;
       backup_init.device_id = 1;
       backup_init.device_type = DEVICE_TYPE_NMEA_ONLY;
       backup_init.uart_init = &uart_gnss_backup_ip;  // Different UART

       // Initialize both devices
       ret = no_os_gnss_init(&primary_gnss, &primary_init);
       if (ret) {
           pr_err("Primary GNSS initialization failed: %d\n", ret);
           return ret;
       }

       ret = no_os_gnss_init(&backup_gnss, &backup_init);
       if (ret) {
           pr_warning("Backup GNSS initialization failed: %d\n", ret);
           // Continue with primary only
       }

       // Use primary device with backup fallback
       // ... application logic ...

       // Cleanup
       no_os_gnss_remove(primary_gnss);
       if (backup_gnss) {
           no_os_gnss_remove(backup_gnss);
       }

       return 0;
   }

---

Testing Procedures
------------------

Functional Testing Checklist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Hardware Connectivity Test
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int test_hardware_connectivity(void)
   {
       struct no_os_gnss_desc *gnss_desc;
       uint8_t test_data[] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34}; // UBX-MON-VER
       uint8_t response[256];
       int32_t ret;

       pr_info("=== Hardware Connectivity Test ===\n");

       ret = no_os_gnss_init(&gnss_desc, &no_os_gnss_init_param);
       if (ret) {
           pr_err("FAIL: GNSS initialization failed: %d\n", ret);
           return ret;
       }

       // Test write operation
       pr_info("Testing UART write...\n");
       ret = no_os_gnss_write(gnss_desc, test_data, sizeof(test_data));
       if (ret < 0) {
           pr_err("FAIL: UART write failed: %d\n", ret);
       } else {
           pr_info("PASS: UART write successful (%d bytes)\n", ret);
       }

       no_os_mdelay(1000);

       // Test read operation
       pr_info("Testing UART read...\n");
       ret = no_os_gnss_read(gnss_desc, response, sizeof(response));
       if (ret > 0) {
           pr_info("PASS: UART read successful (%d bytes)\n", ret);
           pr_debug("Response: ");
           for (int i = 0; i < min(ret, 32); i++) {
               pr_debug("%02X ", response[i]);
           }
           pr_debug("\n");
       } else {
           pr_warning("WARNING: No response received (normal for some devices)\n");
       }

       no_os_gnss_remove(gnss_desc);
       pr_info("=== Hardware Test Complete ===\n");
       return 0;
   }

2. Timing Accuracy Test
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int test_timing_accuracy(void)
   {
       struct no_os_gnss_desc *gnss_desc;
       struct ublox_precise_time precise_time;
       uint32_t prev_epoch = 0, curr_epoch;
       uint32_t prev_frac = 0, curr_frac;
       int32_t ret;
       int measurements = 10;

       pr_info("=== Timing Accuracy Test ===\n");

       ret = no_os_gnss_init(&gnss_desc, &no_os_gnss_init_param);
       if (ret) return ret;

       // Wait for initial fix
       pr_info("Waiting for initial GNSS fix...\n");
       while (!no_os_gnss_is_timing_valid(gnss_desc)) {
           no_os_gnss_refresh_timing_data(gnss_desc);
           no_os_mdelay(1000);
       }

       pr_info("Measuring timing stability over %d samples...\n", measurements);

       for (int i = 0; i < measurements; i++) {
           ret = no_os_gnss_refresh_timing_data(gnss_desc);
           if (ret) continue;

           curr_epoch = no_os_gnss_get_unix_epoch_unified(gnss_desc, &curr_frac);
           ret = no_os_gnss_get_unified_timing(gnss_desc, &precise_time);

           if (curr_epoch > 0 && ret == 0) {
               pr_info("Sample %d: %lu.%06lu, accuracy: %lu ns\n",
                       i+1, curr_epoch, curr_frac, precise_time.time_accuracy);

               // Check for time jumps
               if (prev_epoch > 0) {
                   uint32_t time_diff = curr_epoch - prev_epoch;
                   if (time_diff > 2) {  // Allow for 1-2 second polling interval
                       pr_warning("Large time jump detected: %lu seconds\n", time_diff);
                   }
               }

               prev_epoch = curr_epoch;
               prev_frac = curr_frac;
           }

           no_os_mdelay(1000);
       }

       no_os_gnss_remove(gnss_desc);
       pr_info("=== Timing Test Complete ===\n");
       return 0;
   }

Troubleshooting Guide
---------------------

Common Issues and Solutions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Issue 1: GNSS Device Not Detected
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Symptoms:**

- Initialization fails with timeout error
- No response to UART commands
- Error: ``GNSS initialization failed: -110``

**Solutions:**

1. **Power Issues:**

   - Verify 3.3V supply with multimeter
   - Check current limit on power supply
   - Replace GNSS module if excessive current draw

2. **Wiring Issues:**

   - Swap TX/RX connections (common mistake)
   - Verify continuity with multimeter
   - Check for cold solder joints

3. **Configuration Issues:**

   - Try different baud rates: 9600, 38400, 115200
   - Reset GNSS module to factory defaults
   - Use different UART peripheral if available

Issue 2: No Valid Timing Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Symptoms:**

- Device initializes but timing never becomes valid
- Warning: ``Timing data is not valid, waiting for fix...``
- Long acquisition times (>10 minutes)

**Solutions:**

1. **Antenna Issues:**

   - Use active antenna with LNA for indoor testing
   - Ensure proper antenna ground plane (70mm minimum)
   - Check antenna cable integrity
   - Move to location with clear sky view

2. **Environmental Issues:**

   - Move outdoors for initial testing
   - Avoid testing near metal structures
   - Allow 5-15 minutes for cold start acquisition
   - Check for GPS jamming sources

3. **Device Configuration:**

   - Enable multiple constellation support (GPS+GLONASS+Galileo)
   - Adjust acquisition settings for faster fix
   - Use assisted GNSS if available

Error Code Reference
~~~~~~~~~~~~~~~~~~~~

+-----------+------------+--------------------+--------------------------------------+
| Error Code| Symbol     | Meaning            | Common Causes                        |
+===========+============+====================+======================================+
| -22       | ``EINVAL`` | Invalid argument   | NULL pointer, bad parameter          |
+-----------+------------+--------------------+--------------------------------------+
| -12       | ``ENOMEM`` | Out of memory      | Insufficient RAM, memory leak        |
+-----------+------------+--------------------+--------------------------------------+
| -5        | ``EIO``    | I/O error          | UART failure, device disconnected    |
+-----------+------------+--------------------+--------------------------------------+
| -110      | ``ETIMEDOUT`` | Timeout         | Device not responding, wrong baud rate |
+-----------+------------+--------------------+--------------------------------------+
| -61       | ``ENODATA``| No data available  | No GNSS fix, invalid sentence        |
+-----------+------------+--------------------+--------------------------------------+
| -95       | ``ENOTSUP``| Not supported      | Feature not available on device type |
+-----------+------------+--------------------+--------------------------------------+
| -77       | ``EBADMSG``| Bad message        | Checksum error, corrupted data       |
+-----------+------------+--------------------+--------------------------------------+

---

Production Deployment
---------------------

Pre-Deployment Validation
~~~~~~~~~~~~~~~~~~~~~~~~~~

Manufacturing Test Suite
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   int manufacturing_test_suite(void)
   {
       int test_results = 0;

       pr_info("=== Manufacturing Test Suite ===\n");

       // Test 1: Hardware connectivity
       test_results |= test_hardware_connectivity();

       // Test 2: Basic initialization
       test_results |= (basic_gnss_example() ? 0x01 : 0);

       // Test 3: Timing acquisition (short version)
       test_results |= (test_timing_accuracy() ? 0x02 : 0);

       // Test 4: PPS configuration
       test_results |= (test_pps_signal() ? 0x04 : 0);

       // Test 5: Memory usage
       test_results |= (test_memory_usage() ? 0x08 : 0);

       pr_info("=== Manufacturing Test Results ===\n");
       pr_info("Test result code: 0x%02X\n", test_results);

       if (test_results == 0) {
           pr_info("PASS: All manufacturing tests passed\n");
           return 0;
       } else {
           pr_err("FAIL: One or more tests failed\n");
           return -1;
       }
   }

Maintenance and Updates
~~~~~~~~~~~~~~~~~~~~~~~

Firmware Update Procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Pre-Update Validation:**

   - Back up current configuration
   - Test new firmware on development board
   - Verify compatibility with hardware revision

2. **Update Process:**

   - Flash new firmware via USB/debug interface
   - Verify flash integrity
   - Test basic functionality

3. **Post-Update Validation:**

   - Run manufacturing test suite
   - Verify timing accuracy
   - Check PPS signal integrity
   - Validate against performance benchmarks

---

This comprehensive testing and usage guide provides all the necessary information for successful deployment and operation of the no-OS GNSS driver on the APARD32690 platform. For additional support, refer to the API documentation and driver implementation guide.