#include "sensor_init.h"

int ret;

void init_adt7420(void){
struct adt7420_dev *adt7420;

// adt7420_init_param.adt7420_dev_init = &adt7420_user_init;

ret = adt7420_init(&adt7420, adt7420_user_init);
if (ret)
	{
	printf("Failed to Initialise ADT7420 - %d\n\r", ret);
	goto error_adt7420;
	}

no_os_mdelay(200);

ret = adt7420_reset(adt7420);
if (ret)
	{
	printf("Failed to Reset ADT7420 - %d\n\r", ret);
	goto error_adt7420;
	}

error_adt7420:
	adt7420_remove(adt7420);
}

// Setup connection to the ADXL Part 
void init_adxl355(void){
struct adxl355_dev* adxl355_desc;
// ADXL355  Variables 
struct adxl355_frac_repr x[32] = { 0 };
struct adxl355_frac_repr y[32] = { 0 };
struct adxl355_frac_repr z[32] = { 0 };
struct adxl355_frac_repr temp;
union adxl355_sts_reg_flags status_flags = { 0 };
uint8_t fifo_entries = 0;

adxl355_ip.comm_init.spi_init = adxl355_spi_ip;

ret = adxl355_init(&adxl355_desc, adxl355_ip);
if (ret)
	{
	printf("Failed to Initialise ADXL355 - %d\n\r", ret);
	goto error_adxl355;
	}
	
ret = adxl355_soft_reset(adxl355_desc);
if (ret)
	{
	printf("Failed to Soft Reset ADXL355 - %d\n\r", ret);
	goto error_adxl355;
	}

// no_os_mdelay(200);

ret = adxl355_set_odr_lpf(adxl355_desc, ADXL355_ODR_3_906HZ);
if (ret)
	{
	printf("Failed to Initialise ADXL355 Filter - %d\n\r", ret);
	goto error_adxl355;
	}

ret = adxl355_set_op_mode(adxl355_desc, ADXL355_MEAS_TEMP_ON_DRDY_OFF);
if (ret)
	{
	printf("Failed to Set Mode ADXL355 - %d\n\r", ret);
	goto error_adxl355;
	}

error_adxl355:
	adxl355_remove(adxl355_desc);
}