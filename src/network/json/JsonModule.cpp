#include "JsonModule.hpp"
#include "SchedulerRules.hpp"
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>

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

JsonModule::JsonModule(/* args */)
{
}

JsonModule::~JsonModule()
{
}

void JsonModule::parse(char *buffer_in, void *struct_out)
{
    int ret = json_obj_parse(buffer_in, sizeof(buffer_in), rules_json_obj, ARRAY_SIZE(rules_json_obj), struct_out);
    if (ret < 0)
    {
        printk("Error to parse rules: %d\n\r", ret);
    }
    else
    {
        printk("Parser return value: %d\n\r", ret);
    }
}
void JsonModule::encode(void *struct_in, char *buffer_out)
{
}