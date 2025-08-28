#include "sensor_map.h"
#include <string.h>

sensor_map_t sensors[SENSORS_MAX];

void sensors_init(void)
{
	// reserve memory
    memset(sensors, 0, sizeof sensors);
}

static int sensors_find_index(uint8_t sensor_id)
{
	// find index of sensor, return -1 if sensor does not exist
    for (int i = 0; i < SENSORS_MAX; i++) {
        if (sensors[i].used && sensors[i].data.sensor_id == sensor_id)
            return i;
    }
    return -1;
}

static int sensors_find_slot(void)
{
	// find free slot in reserved memory, if no more slot is left return -1
    for (int i = 0; i < SENSORS_MAX; i++) {
        if (!sensors[i].used) return i;
    }
    return -1;
}

uint32_t sensors_get_count(void)
{
	// get count of active sensors
    uint32_t n = 0;
    for (int i = 0; i < SENSORS_MAX; ++i) if (sensors[i].used) n++;
    return n;
}

sensor_data* sensors_get_sensor(uint8_t sensor_id)
{
	// get sensor data by sensor id
    int ix = find_index(sensor_id);
    if (ix < 0) return NULL;
    return &sensors[ix].data;
}

void sensors_set_sensor(sensor_data sensor_new)
{
	// update last sensor values, or create sensor in map if sensor does not already exist
    int ix = find_index(sensor_new.sensor_id);
    if (ix < 0)
    {
        ix = find_free_slot();
        if (ix < 0)
        {
        	// all slots in use can't insert
        	return NULL;
        }
        sensors[ix].used = true;
        sensors[ix].data.sensor_id = sensor_id;
    }

    sensor_map_t *s = &sensors[ix];
    s->data.corner_id       = sensor_new.corner_id;
    s->data.power           = sensor_new.power;
    s->data.counter         = sensor_new.counter;
    s->data.abp2_status_raw = sensor_new.abp2_status_raw;
    s->data.abp2_temperature= sensor_new.abp2_temperature;
    s->data.abp2_pressure   = sensor_new.abp2_pressure;
    s->data.rssi            = sensor_new.rssi;
    s->last_seen_ms         = HAL_GetTick();
}

void sensors_clear_timeout(uint32_t now_ms, uint32_t timeout_ms)
{
    for (int i = 0; i < SENSORS_MAX; i++) {
        if (sensors[i].used) {
            uint32_t age = now_ms - sensors[i].last_seen_ms;
            if (age > timeout_ms) {
                sensors[i].used = false;
            }
        }
    }
}
