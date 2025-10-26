#include "JsonModule.hpp"
#include "SchedulerRules.hpp"
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include "MQTT_utils.hpp"

// For debug
//  static void print_rules(const Rules_t *r)
//  {
//      printk("=== Scheduler Rule ===\n");
//      printk("Date     : %04u-%02u-%02u\n",
//             r->date.year,
//             r->date.month,
//             r->date.day);

//     printk("Time     : %02u:%02u\n",
//            r->time.hour,
//            r->time.minutes);

//     printk("Period   : %s\n",
//            (r->period == WEEKLY) ? "WEEKLY" : "SPECIF");

//     printk("Weekdays : 0x%02X\n", r->week_days);
//     printk("Amount   : %u\n", r->amount);
//     printk("======================\n");
// }

static const struct json_obj_descr rules_specific_date[] = {
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, year, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, month, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, day, JSON_TOK_UINT),
};

static const struct json_obj_descr rules_time[] = {
    JSON_OBJ_DESCR_PRIM(TimeRule_t, hour, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(TimeRule_t, minutes, JSON_TOK_UINT),
};

static const struct json_obj_descr rules_json_obj[] = {

    JSON_OBJ_DESCR_OBJECT(Rules_t, date, rules_specific_date),
    JSON_OBJ_DESCR_OBJECT(Rules_t, time, rules_time),
    JSON_OBJ_DESCR_PRIM(Rules_t, period, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, week_days, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, amount, JSON_TOK_UINT),

};

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

int32_t JsonModule::parse(char *buffer_in, void *struct_out)
{
    int ret = json_obj_parse(buffer_in, strlen(buffer_in), rules_json_obj, ARRAY_SIZE(rules_json_obj), struct_out);
    if (ret < 0)
    {
        //printk("Error to parse rules: %d\n\r", ret);
    }
    else
    {
        //printk("Parser return value: %d\n\r", ret);
    }
    return ret;
}

int32_t JsonModule::encode(void *struct_in, char *buffer_out, size_t buf_len)
{
    int ret = json_obj_encode_buf(publish_payload,ARRAY_SIZE(publish_payload),struct_in, buffer_out,buf_len - 1);
    if(ret != 0){
        //printk("Error to encode\n\r");
    }
    return ret;
}
