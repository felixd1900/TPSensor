#ifndef INC_SENSOR_MAP_H_
#define INC_SENSOR_MAP_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "cc1101.h"

#define SENSORS_MAX 10       // define blocked memory size

typedef enum {
	NONE =			0x00,
	CORNERID_FL	=	0x01,
	CORNERID_FR	=	0x02,
	CORNERID_RL	=	0x03,
	CORNERID_RR	=	0x04,
}corner_id;

typedef struct {
    uint8_t  sensor_id;
    uint8_t  sensor_type;
    uint8_t  corner_id;
    cc1101_tx_power power;
    uint8_t  counter;
    uint8_t  abp2_status_raw;
    float    abp2_temperature;
    float    abp2_pressure;
    int16_t  rssi;
} sensor_data;

typedef struct {
    bool         used;
    uint32_t     last_seen_ms;  // HAL_GetTick()
    sensor_data  data;
} sensor_map_t;


uint32_t    sensors_count(sensor_map_t *sensors);

sensor_data* sensors_get_sensor(sensor_map_t *sensors, uint8_t sensor_id);
void sensors_set_sensor(sensor_map_t *sensors, sensor_data sensor_new);

void sensors_clear_timeout(sensor_map_t *sensors, uint32_t now_ms, uint32_t timeout_ms);


#endif /* INC_SENSOR_MAP_H_ */
