GNSS Driver Implementation Guide
=================================

This document provides comprehensive information about the no-OS GNSS driver implementation, architecture details, and platform integration guidelines.

Table of Contents
-----------------

- `Architecture Overview`_
- `Component Details`_
- `Protocol Implementation`_
- `Timing Subsystem`_
- `PPS Configuration`_
- `Platform Integration`_
- `Performance Optimization`_
- `Debugging and Troubleshooting`_
- `Extending the Driver`_

---

Architecture Overview
---------------------

The GNSS driver follows the no-OS framework's layered architecture pattern, providing clear separation of concerns between generic API functionality and hardware-specific implementation.

Design Principles
~~~~~~~~~~~~~~~~~

1. **Platform Abstraction**: Generic API works with any GNSS hardware through platform operations
2. **Protocol Agnostic**: Supports both binary UBX and ASCII NMEA protocols transparently
3. **Thread Safety**: All operations protected by mutex for multi-threaded environments
4. **Resource Management**: Proper allocation/deallocation with error handling
5. **Extensibility**: Easy to add support for new GNSS chipsets

Layer Structure
~~~~~~~~~~~~~~~

::

   Application Layer
       ↓
   no-OS GNSS API Layer (drivers/api/no_os_gnss.c)
       ↓
   Platform Operations (function pointers)
       ↓
   Platform Implementation (drivers/gnss/ublox_gnss/)
       ↓
   Hardware Abstraction Layer (UART, GPIO, IRQ)
       ↓
   Hardware (device, u-blox GNSS modules)

---

Component Details
-----------------

1. API Layer (``drivers/api/no_os_gnss.c``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The API layer provides the public interface and manages common functionality:

**Responsibilities:**

- Device descriptor allocation and management
- Mutex-based thread safety
- Parameter validation and error handling
- Platform operations routing
- Resource cleanup

**Key Functions:**

.. code-block:: c

   // Core management
   int32_t no_os_gnss_init(struct no_os_gnss_desc **desc,
                           struct no_os_gnss_init_param *init_param);
   int32_t no_os_gnss_remove(struct no_os_gnss_desc *desc);

   // Communication
   int32_t no_os_gnss_read(struct no_os_gnss_desc *desc, uint8_t *data, uint16_t size);
   int32_t no_os_gnss_write(struct no_os_gnss_desc *desc, const uint8_t *data, uint16_t size);

**Thread Safety Implementation:**

.. code-block:: c

   int32_t no_os_gnss_refresh_timing_data(struct no_os_gnss_desc *desc)
   {
       int32_t ret;

       if (!desc || !desc->platform_ops || !desc->platform_ops->refresh_timing)
           return -EINVAL;

       // Acquire mutex for thread safety
       ret = no_os_mutex_lock(desc->mutex);
       if (ret)
           return ret;

       // Call platform implementation
       ret = desc->platform_ops->refresh_timing(desc);

       // Always release mutex
       no_os_mutex_unlock(desc->mutex);
       return ret;
   }

2. Platform Layer (``drivers/gnss/ublox_gnss/``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The platform layer implements hardware-specific functionality:

**Key Files:**

- ``ublox_gnss.h``: Platform-specific definitions and structures
- ``ublox_gnss.c``: Implementation with platform operations wrappers

**Platform Operations Structure:**

.. code-block:: c

   const struct no_os_gnss_platform_ops ublox_gnss_platform_ops = {
       .init = ublox_gnss_platform_init,
       .remove = ublox_gnss_platform_remove,
       .read = ublox_gnss_platform_read,
       .write = ublox_gnss_platform_write,
       .hw_reset = ublox_gnss_platform_hw_reset,
       .refresh_timing = ublox_gnss_platform_refresh_timing,
       .is_timing_valid = ublox_gnss_platform_is_timing_valid,
       .configure_pps = ublox_gnss_platform_configure_pps,
       .get_pps_config = ublox_gnss_platform_get_pps_config,
       .set_pps_enable = ublox_gnss_platform_set_pps_enable,
       .is_pps_enabled = ublox_gnss_platform_is_pps_enabled,
       .get_unified_timing = ublox_gnss_platform_get_unified_timing,
       .get_unix_epoch = ublox_gnss_platform_get_unix_epoch
   };

3. Hardware Abstraction Layer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The driver leverages existing no-OS HAL components:

- **UART**: Communication with GNSS modules
- **GPIO**: Reset control and PPS input
- **IRQ**: Interrupt handling for PPS events
- **Timer**: Timeout and delay management

---

Protocol Implementation
-----------------------

UBX Binary Protocol
~~~~~~~~~~~~~~~~~~~

**Message Structure:**

::

   | Sync1 | Sync2 | Class | ID | Length | Payload | CK_A | CK_B |
   |  0xB5 |  0x62 |   1   | 1  |    2   |    N    |   1  |   1  |

**Key UBX Messages:**

- ``UBX-NAV-PVT`` (0x01 0x07): Position, Velocity, Time solution
- ``UBX-NAV-TIMEUTC`` (0x01 0x21): UTC time solution
- ``UBX-CFG-VALSET`` (0x06 0x8A): Configuration value setting
- ``UBX-CFG-VALGET`` (0x06 0x8B): Configuration value retrieval

**Implementation Details:**

.. code-block:: c

   int gnss_ubx_send_packet(struct gnss_dev *dev, uint8_t cls, uint8_t id,
                            uint8_t *payload, uint16_t length)
   {
       uint8_t *packet;
       uint8_t checksum_a, checksum_b;

       // Build packet with sync bytes, class, ID, length
       packet[0] = UBX_SYNCH_1;    // 0xB5
       packet[1] = UBX_SYNCH_2;    // 0x62
       packet[2] = cls;
       packet[3] = id;
       packet[4] = length & 0xFF;
       packet[5] = (length >> 8) & 0xFF;

       // Copy payload
       if (payload && length > 0)
           memcpy(&packet[6], payload, length);

       // Calculate checksum
       gnss_calculate_checksum(&packet[2], 4 + length, &checksum_a, &checksum_b);
       packet[6 + length] = checksum_a;
       packet[7 + length] = checksum_b;

       return gnss_write(dev, packet, 8 + length);
   }

NMEA ASCII Protocol
~~~~~~~~~~~~~~~~~~~

**Sentence Structure:**

::

   $TALKER,field1,field2,...,fieldN*hh<CR><LF>

**Supported Sentences:**

- ``$GPRMA`` / ``$GNRMA``: Time and date information
- Configuration commands for message rates and settings

**Parsing Implementation:**

.. code-block:: c

   int gnss_parse_gpzda_sentence(const char *sentence, struct gnss_nmea_time *gpzda_time)
   {
       // Parse: $GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*hh
       char *fields[8];
       int field_count = 0;

       // Split by comma and asterisk
       char *token = strtok(sentence_copy, ",*");
       while (token && field_count < 8) {
           fields[field_count++] = token;
           token = strtok(NULL, ",*");
       }

       // Extract time components
       if (field_count >= 5) {
           float time_float = atof(fields[1]);
           int time_int = (int)time_float;

           gpzda_time->hour = time_int / 10000;
           gpzda_time->minute = (time_int % 10000) / 100;
           gpzda_time->second = time_int % 100;
           gpzda_time->milliseconds = (uint16_t)((time_float - time_int) * 1000);

           // Extract date components
           gpzda_time->day = atoi(fields[2]);
           gpzda_time->month = atoi(fields[3]);
           gpzda_time->year = atoi(fields[4]);
       }

       return 0;
   }

---

Timing Subsystem
----------------

Unified Timing Architecture
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The driver provides a unified timing interface that abstracts protocol differences:

.. code-block:: c

   int gnss_get_unified_timing(struct gnss_dev *dev, struct ublox_precise_time *precise_time)
   {
       switch (dev->device_type) {
       case DEVICE_TYPE_UBX_CAPABLE:
           // High precision nanosecond timing from UBX
           ret = gnss_ubx_get_precise_time(dev, precise_time);
           break;

       case DEVICE_TYPE_NMEA_ONLY:
           // Convert millisecond NMEA timing to unified format
           precise_time->unix_epoch = dev->nmea_timing_cache.unix_epoch;
           precise_time->nanoseconds = dev->nmea_timing_cache.milliseconds * 1000000;
           precise_time->time_accuracy = 1000000000; // 1 second estimate
           break;
       }
       return ret;
   }

Precision Comparison
~~~~~~~~~~~~~~~~~~~~

+----------+----------------+------------------+------------------------+
| Protocol | Time Resolution| Typical Accuracy | Data Source            |
+==========+================+==================+========================+
| UBX      | 1 nanosecond   | ±25ns            | NAV-PVT, NAV-TIMEUTC   |
+----------+----------------+------------------+------------------------+
| NMEA     | 1 millisecond  | ±1ms             | GPZDA/GNZDA sentences  |
+----------+----------------+------------------+------------------------+

Time Validity Checking
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   bool gnss_is_timing_valid(struct gnss_dev *dev)
   {
       switch (dev->device_type) {
       case DEVICE_TYPE_UBX_CAPABLE:
           // Check UBX time valid flag (bit 1 in NAV-PVT valid field)
           return (dev->nav_data.valid & 0x02) != 0;

       case DEVICE_TYPE_NMEA_ONLY:
           // Check both time and date validity from cached NMEA data
           return dev->nmea_timing_cache.time_valid && dev->nmea_timing_cache.date_valid;
       }
       return false;
   }

---

PPS Configuration
-----------------

UBX PPS Configuration Structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   struct ublox_pps_config {
       enum ublox_pps_pulse_mode pulse_mode;       // Period or frequency mode
       enum ublox_pps_length_mode length_mode;    // Length definition mode
       uint32_t period_us;                         // Time pulse period (μs)
       uint32_t frequency_hz;                      // Frequency (when in freq mode)
       uint32_t period_lock_us;                    // Period when locked to GNSS
       uint32_t length_us;                         // Pulse length (μs)
       uint32_t length_lock_us;                    // Pulse length when locked
       enum ublox_pps_enable enable;              // Enable/disable PPS
       enum ublox_pps_enable sync_gnss;           // Sync to GNSS time
       enum ublox_pps_enable use_locked;          // Use locked parameters
       enum ublox_pps_enable align_to_tow;        // Align to time-of-week
       enum ublox_pps_polarity polarity;          // Signal polarity
       enum ublox_pps_time_grid time_grid;        // Time reference grid
   };

Configuration Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   int32_t ublox_gnss_platform_configure_pps(struct no_os_gnss_desc *desc,
                                             const void *pps_config)
   {
       const struct ublox_pps_config *config = (const struct ublox_pps_config *)pps_config;
       struct gnss_dev *platform_dev = (struct gnss_dev *)desc->extra;

       // Configure each PPS parameter using UBX CFG-VALSET commands

       // Time pulse definition (period or frequency)
       ret = gnss_ubx_set_val(platform_dev, UBLOX_CFG_TP_PULSE_DEF,
                             config->pulse_mode, 1, GNSS_CONFIG_LAYER_RAM);

       // Set period or frequency based on mode
       if (config->pulse_mode == UBLOX_PPS_PULSE_MODE_PERIOD) {
           ret = gnss_ubx_set_val(platform_dev, UBLOX_CFG_TP_PERIOD_TP1,
                                 config->period_us, 4, GNSS_CONFIG_LAYER_RAM);
       } else {
           uint32_t period_us = (config->frequency_hz > 0) ?
                               (1000000 / config->frequency_hz) : 1000000;
           ret = gnss_ubx_set_val(platform_dev, UBLOX_CFG_TP_PERIOD_TP1,
                                 period_us, 4, GNSS_CONFIG_LAYER_RAM);
       }

       // Configure all other parameters...
       return ret;
   }

PPS Configuration Keys (u-blox)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-------------------------------+--------------------------------+
| Parameter       | Configuration Key             | Description                    |
+=================+===============================+================================+
| Pulse Definition| ``UBLOX_CFG_TP_PULSE_DEF``    | Period vs frequency mode       |
+-----------------+-------------------------------+--------------------------------+
| Period          | ``UBLOX_CFG_TP_PERIOD_TP1``   | Time pulse period (μs)         |
+-----------------+-------------------------------+--------------------------------+
| Locked Period   | ``UBLOX_CFG_TP_PERIOD_LOCK_TP1`` | Period when GNSS locked    |
+-----------------+-------------------------------+--------------------------------+
| Length          | ``UBLOX_CFG_TP_LEN_TP1``      | Pulse width (μs)               |
+-----------------+-------------------------------+--------------------------------+
| Locked Length   | ``UBLOX_CFG_TP_LEN_LOCK_TP1`` | Pulse width when locked        |
+-----------------+-------------------------------+--------------------------------+
| Enable          | ``UBLOX_CFG_TP_TP1_ENA``      | Enable/disable output          |
+-----------------+-------------------------------+--------------------------------+
| Sync GNSS       | ``UBLOX_CFG_TP_SYNC_GNSS_TP1``| Synchronize to GNSS           |
+-----------------+-------------------------------+--------------------------------+
| Polarity        | ``UBLOX_CFG_TP_POL_TP1``      | Output polarity                |
+-----------------+-------------------------------+--------------------------------+
| Time Grid       | ``UBLOX_CFG_TP_TIMEGRID_TP1`` | Reference time system          |
+-----------------+-------------------------------+--------------------------------+

---

Platform Integration
--------------------

Adding New GNSS Hardware Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To add support for a new GNSS chipset:

1. **Create Platform Directory:**

   ::

      drivers/gnss/new_chipset/
      ├── new_chipset.h
      ├── new_chipset.c
      └── Makefile

2. **Implement Platform Operations:**

   .. code-block:: c

      const struct no_os_gnss_platform_ops new_chipset_platform_ops = {
          .init = new_chipset_platform_init,
          .remove = new_chipset_platform_remove,
          .read = new_chipset_platform_read,
          .write = new_chipset_platform_write,
          // ... implement all required operations
      };

3. **Define Platform-Specific Structures:**

   .. code-block:: c

      struct new_chipset_pps_config {
          // Platform-specific PPS parameters
      };

      struct new_chipset_precise_time {
          // Platform-specific timing structure
      };

Configuration Layer Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The driver uses the UBX CFG-VALSET/CFG-VALGET mechanism for configuration:

.. code-block:: c

   int gnss_ubx_set_val(struct gnss_dev *dev, uint32_t key_id,
                       uint64_t value, uint8_t value_size, uint8_t layer)
   {
       uint8_t payload[value_size + 8];

       payload[0] = 0;           // Version
       payload[1] = layer;       // Configuration layer
       payload[2] = 0;           // Reserved
       payload[3] = 0;           // Reserved

       // Add configuration key
       no_os_put_unaligned_le32(key_id, &payload[4]);

       // Add value based on size
       switch (value_size) {
       case 1: payload[8] = (uint8_t)value; break;
       case 2: no_os_put_unaligned_le16((uint16_t)value, &payload[8]); break;
       case 4: no_os_put_unaligned_le32((uint32_t)value, &payload[8]); break;
       case 8: no_os_put_unaligned_le64(value, &payload[8]); break;
       }

       return gnss_ubx_send_packet(dev, UBX_CLASS_CFG, UBX_CFG_VALSET,
                                  payload, value_size + 8);
   }

---

Performance Optimization
------------------------

Memory Management
~~~~~~~~~~~~~~~~~

**Static Allocation**: The driver minimizes dynamic allocation:

.. code-block:: c

   // Use stack allocation for temporary buffers
   uint8_t packet_buffer[UBX_MAX_PACKET_SIZE];

   // Only allocate large structures that persist
   struct gnss_dev *dev = no_os_calloc(1, sizeof(*dev));

**Buffer Management**: Efficient packet handling:

.. code-block:: c

   int gnss_ubx_receive_packet(struct gnss_dev *dev, struct ubx_packet *packet)
   {
       // Read header first to determine payload size
       uint8_t header[6];
       ret = gnss_read(dev, header, 6);

       packet->length = no_os_get_unaligned_le16(&header[4]);

       // Only allocate exact payload size needed
       packet->payload = no_os_calloc(packet->length, sizeof(uint8_t));

       // Read payload + checksum
       ret = gnss_read(dev, buffer, packet->length + 2);

       return ret;
   }

CPU Usage Optimization
~~~~~~~~~~~~~~~~~~~~~~

**Polling Strategy**: Efficient timing data updates:

.. code-block:: c

   if (get_irq_flag_state()) {
       reset_irq_flag_state();

       /* Step 1: Refresh timing data */
       ret = no_os_gnss_refresh_timing_data(gnss_desc);
       if (ret) {
           pr_err("Failed to refresh timing data: %d\n", ret);
           ret = no_os_irq_enable(gnss_desc->gnss_device->irq_ctrl,
                                  GPIO_IRQ_PIN);
           continue;
       }

       /* Step 2: Check if timing data is valid */
       ret = no_os_gnss_is_timing_valid(gnss_desc, &timing_valid);
       if (ret || !timing_valid) {
           pr_warning("Timing data is not valid, waiting for fix...\n");
           ret = no_os_irq_enable(gnss_desc->gnss_device->irq_ctrl,
                                  GPIO_IRQ_PIN);
           continue;
       }

       /* Step 3: Get comprehensive timing information */
       ret = no_os_gnss_get_unified_timing(gnss_desc, &precise_time);
       if (ret) {
           pr_warning("Failed to get unified timing: %d\n", ret);
           ret = no_os_irq_enable(gnss_desc->gnss_device->irq_ctrl,
                                  GPIO_IRQ_PIN);
           continue;
       }

       /* Step 4: Get Unix epoch with best available precision */
       unix_epoch = no_os_gnss_get_unix_epoch_unified(gnss_desc,
               &fractional_seconds);
       /* Display unified timing information */
       pr_info("=== Unified Timing Results ===\n");
       pr_info("Date/Time: %04d-%02d-%02d %02d:%02d:%02d\n",
           precise_time.year, precise_time.month, precise_time.day,
           precise_time.hour, precise_time.minute, precise_time.second);

       pr_info("Unix Epoch: %lu.%06lu\n", unix_epoch, fractional_seconds);
       pr_info("Nanoseconds: %ld\n", precise_time.nanoseconds);
       pr_info("Time Accuracy: %lu ns\n", precise_time.time_accuracy);
       pr_info("Flags: Valid=%s, Resolved=%s, Date Valid=%s\n",
           precise_time.time_valid ? "YES" : "NO",
           precise_time.time_fully_resolved ? "YES" : "NO",
           precise_time.date_valid ? "YES" : "NO");
         /* Display GPS position and fix information for NMEA devices */
       if (gnss_desc->gnss_device->device_type == GNSS_DEVICE_NMEA_ONLY) {
           ret = no_os_gnss_get_position_data(gnss_desc, &position_data);
           if (!ret) {
               pr_info("GPS Fix: Quality=%d, Satellites=%d, HDOP=%.1f\n",
                   position_data.fix_quality, position_data.satellites_used,
                   position_data.hdop);
               pr_info("Position: Lat=%.6f°, Lon=%.6f°, Alt=%.1fm\n",
                   position_data.latitude, position_data.longitude,
                   position_data.altitude);
               pr_info("Fix Status: %s, Position Valid: %s\n",
                   position_data.fix_valid ? "YES" : "NO",
                   position_data.position_valid ? "YES" : "NO");
           } else {
               pr_info("GPS Position: Data not available \n");
           }
       }
           ret = no_os_irq_enable(gnss_desc->gnss_device->irq_ctrl,
                                  GPIO_IRQ_PIN);
   }

**Protocol Selection**: Choose optimal protocol for application:

- UBX: When nanosecond precision is required
- NMEA: When millisecond precision is sufficient and CPU resources are limited

Debugging and Troubleshooting
------------------------------

Debug Output Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

Enable comprehensive logging:

.. code-block:: c

   // In main.c or driver files
   #ifndef DEBUG_LEVEL
   #define DEBUG_LEVEL PR_DEBUG
   #endif

   // Enable different log levels
   pr_debug("UBX packet received: class=0x%02X, id=0x%02X\n", cls, id);
   pr_info("GNSS device initialized successfully\n");
   pr_warning("PPS configuration failed: %d\n", ret);
   pr_err("GNSS initialization failed: %d\n", ret);

Common Issues and Solutions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**1. Checksum Verification Failures**

.. code-block:: c

   // Debug UBX packet integrity
   int gnss_verify_checksum(uint8_t *data, uint16_t length,
                           uint8_t checksumA, uint8_t checksumB)
   {
       uint8_t calc_a, calc_b;

       gnss_calculate_checksum(data, length, &calc_a, &calc_b);

       if (calc_a != checksumA || calc_b != checksumB) {
           pr_err("Checksum mismatch: expected A=0x%02X B=0x%02X, got A=0x%02X B=0x%02X\n",
                  checksumA, checksumB, calc_a, calc_b);

           // Dump packet for analysis
           pr_debug("Packet data:");
           for (int i = 0; i < length && i < 32; i++) {
               pr_debug(" %02X", data[i]);
           }
           pr_debug("\n");

           return -EBADMSG;
       }

       return 0;
   }

**2. Timing Validity Issues**

.. code-block:: c

   // Debug timing data validity
   void debug_timing_status(struct gnss_dev *dev)
   {
       if (dev->device_type == DEVICE_TYPE_UBX_CAPABLE) {
           pr_debug("UBX NAV-PVT valid flags: 0x%02X\n", dev->nav_data.valid);
           pr_debug("  Date valid: %s\n", (dev->nav_data.valid & 0x01) ? "YES" : "NO");
           pr_debug("  Time valid: %s\n", (dev->nav_data.valid & 0x02) ? "YES" : "NO");
           pr_debug("  Time fully resolved: %s\n", (dev->nav_data.valid & 0x04) ? "YES" : "NO");
           pr_debug("  Fix type: %d\n", dev->nav_data.fixType);
           pr_debug("  Satellites: %d\n", dev->nav_data.numSV);
       } else {
           pr_debug("NMEA timing cache status:\n");
           pr_debug("  Time valid: %s\n", dev->nmea_timing_cache.time_valid ? "YES" : "NO");
           pr_debug("  Date valid: %s\n", dev->nmea_timing_cache.date_valid ? "YES" : "NO");
           pr_debug("  Last update: %04d-%02d-%02d %02d:%02d:%02d\n",
                   dev->nmea_timing_cache.year, dev->nmea_timing_cache.month,
                   dev->nmea_timing_cache.day, dev->nmea_timing_cache.hour,
                   dev->nmea_timing_cache.minute, dev->nmea_timing_cache.second);
       }
   }

Logic Analyzer Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~

For deep protocol debugging:

**UBX Protocol Analysis:**

- Trigger on sync bytes (0xB5 0x62)
- Decode class/ID fields
- Verify checksum calculation
- Monitor ACK/NAK responses

**NMEA Protocol Analysis:**

- Trigger on sentence start ($)
- Parse comma-delimited fields
- Verify checksum (XOR between $ and *)
- Monitor sentence timing and rates

Hardware Validation
~~~~~~~~~~~~~~~~~~~

**Signal Integrity Checks:**

.. code-block:: c

   int validate_pps_signal(struct no_os_gpio_desc *pps_gpio)
   {
       uint32_t high_count = 0, low_count = 0;

       pr_info("Monitoring PPS signal for 10 seconds...\n");

       for (int i = 0; i < 10000; i++) {  // 10 seconds at 1ms sampling
           uint8_t state;
           no_os_gpio_get_value(pps_gpio, &state);

           if (state) {
               high_count++;
           } else {
               low_count++;
           }

           no_os_mdelay(1);
       }

       pr_info("PPS signal statistics:\n");
       pr_info("  High samples: %lu (%.1f%%)\n", high_count,
              (float)high_count / 100.0);
       pr_info("  Low samples: %lu (%.1f%%)\n", low_count,
              (float)low_count / 100.0);

       // Expect roughly 10% high for 100ms pulse in 1Hz signal
       if (high_count > 500 && high_count < 1500) {
           pr_info("PPS signal appears healthy\n");
           return 0;
       } else {
           pr_warning("PPS signal may not be functioning correctly\n");
           return -1;
       }
   }

---

Extending the Driver
--------------------

Adding Custom GNSS Protocols
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To support additional protocols beyond UBX and NMEA:

1. **Define Protocol Interface:**

.. code-block:: c

   struct gnss_protocol_ops {
       int (*send_command)(struct gnss_dev *dev, const void *cmd);
       int (*receive_response)(struct gnss_dev *dev, void *response);
       int (*parse_timing)(const void *data, struct gnss_timing *timing);
       int (*configure_output)(struct gnss_dev *dev, uint32_t message_mask);
   };

2. **Implement Protocol Handler:**

.. code-block:: c

   int my_protocol_parse_timing(const void *data, struct gnss_timing *timing)
   {
       // Parse proprietary timing message format
       const struct my_timing_msg *msg = (const struct my_timing_msg *)data;

       timing->year = msg->year;
       timing->month = msg->month;
       // ... extract all timing fields

       return 0;
   }

Custom PPS Configuration
~~~~~~~~~~~~~~~~~~~~~~~~

For GNSS modules with unique PPS capabilities:

.. code-block:: c

   struct my_gnss_pps_config {
       uint32_t custom_parameter1;
       uint32_t custom_parameter2;
       enum my_pps_mode mode;
       // ... platform-specific parameters
   };

   int32_t my_gnss_configure_pps(struct no_os_gnss_desc *desc, const void *pps_config)
   {
       const struct my_gnss_pps_config *config = pps_config;

       // Send proprietary configuration commands
       return my_gnss_send_pps_config(desc->extra, config);
   }

Performance Monitoring
~~~~~~~~~~~~~~~~~~~~~~

Add runtime performance metrics:

.. code-block:: c

   struct gnss_performance_stats {
       uint32_t packets_sent;
       uint32_t packets_received;
       uint32_t checksum_errors;
       uint32_t timeout_errors;
       uint32_t avg_response_time_us;
   };

   void update_performance_stats(struct gnss_dev *dev, uint32_t response_time)
   {
       dev->stats.packets_received++;

       // Update rolling average response time
       dev->stats.avg_response_time_us =
           (dev->stats.avg_response_time_us * 0.9) + (response_time * 0.1);
   }

Integration with Real-Time Systems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For hard real-time requirements:

.. code-block:: c

   // Priority-based timing updates
   int gnss_high_priority_timing_update(struct gnss_dev *dev)
   {
       // Disable interrupts during critical timing read
       uint32_t irq_state = no_os_irq_disable();

       // Fast timing data acquisition
       int ret = gnss_ubx_poll_nav_pvt(dev, &dev->nav_data);

       // Re-enable interrupts
       no_os_irq_restore(irq_state);

       return ret;
   }

---

This comprehensive guide covers the essential aspects of the no-OS GNSS driver implementation. For additional details on specific functions or platform integration, refer to the source code and API documentation.