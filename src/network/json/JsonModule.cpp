#include "JsonModule.hpp"
#include "SchedulerRules.hpp"
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include "MQTT_utils.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(JSON_LOGS);

// For debug
static void print_rules(const Rules_t *r)
{
    LOG_INF("=== Scheduler Rule ===");
    LOG_INF("Date     : %04u-%02u-%02u",
           r->date.year,
           r->date.month,
           r->date.day);

    LOG_INF("Time     : %02u:%02u",
           r->time.hour,
           r->time.minutes);

    LOG_INF("Period   : %s",
           (r->period == WEEKLY) ? "WEEKLY" : "SPECIF");

    LOG_INF("Weekdays : 0x%02X", r->week_days);
    LOG_INF("Amount   : %u", r->amount);
    LOG_INF("======================");
}

static const struct json_obj_descr publish_payload[] = {
    JSON_OBJ_DESCR_PRIM(struct level_sensor, unit, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(struct level_sensor, value, JSON_TOK_UINT),
};

JsonModule::JsonModule(/* args */)
{
}

JsonModule::~JsonModule()
{
}

int32_t JsonModule::parse(char *buffer_in, void *struct_out, const struct json_obj_descr *json_obj,size_t json_obj_size)
{
    int ret = json_obj_parse(buffer_in, strlen(buffer_in), json_obj, json_obj_size, struct_out);
    if (ret < 0)
    {
        LOG_ERR("Error to parse: %d", ret);
    }
    else
    {
        LOG_INF("Parser return value: %d", ret);
    }
    return ret;
}

int32_t JsonModule::encode(void *struct_in, char *buffer_out, size_t buf_len)
{
    int ret = json_obj_encode_buf(publish_payload, ARRAY_SIZE(publish_payload), struct_in, buffer_out, buf_len - 1);
    if (ret != 0)
    {
        LOG_ERR("Error to encode: %d",ret);
    }
    return ret;
}
