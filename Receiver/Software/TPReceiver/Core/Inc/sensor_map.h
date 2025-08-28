#ifndef INC_SENSOR_MAP_H_
#define INC_SENSOR_MAP_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "cc1101.h"

#define SENSORS_MAX 16       // define blocked memory size

typedef enum {
	NONE =			0x00,
	CORNERID_FL	=	0x01,
	CORNERID_FR	=	0x02,
	CORNERID_RL	=	0x03,
	CORNERID_RR	=	0x04,
};

typedef struct {
    uint8_t  sensor_id;
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


void        sensors_init(void);
uint32_t    sensors_count(void);

// NULL wenn nicht vorhanden
sensor_data* sensors_get(uint8_t sensor_id);

// gibt Ptr auf Slot zurück (neu angelegt oder aktualisiert), NULL wenn voll
sensor_data* sensors_upsert(uint8_t sensor_id,
                            uint8_t corner_id,
                            cc1101_tx_power power,
                            uint8_t counter,
                            uint8_t abp2_status_raw,
                            float temperature,
                            float pressure,
                            int16_t rssi);

// optional: alte Einträge entfernen (z. B. >10s inaktiv)
void        sensors_prune_stale(uint32_t now_ms, uint32_t timeout_ms);

// direkter Zugriff (z. B. fürs Debug-Fenster)
extern sensor_map_t g_sensors[SENSORS_MAX];

#endif /* INC_SENSOR_MAP_H_ */
