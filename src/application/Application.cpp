#include "Application.hpp"
#include "Storage.hpp"
#include "RTC.hpp"
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#include "Storage.hpp"

// LOG_MODULE_REGISTER(APPLICATION_LOG, CONFIG_LOG_DEFAULT_LEVEL);

K_THREAD_STACK_DEFINE(APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE);


static const struct json_obj_descr rules_specific_date[] = {
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, year, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, month, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, day, JSON_TOK_UINT),
};

static const struct json_obj_descr rules_time[] = {
    JSON_OBJ_DESCR_PRIM(TimeRule_t, hour, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(TimeRule_t, minutes, JSON_TOK_UINT),
};

static const struct json_obj_descr rules[] = {

    JSON_OBJ_DESCR_OBJECT(Rules_t, date, rules_specific_date),
    JSON_OBJ_DESCR_OBJECT(Rules_t, time, rules_time),
    JSON_OBJ_DESCR_PRIM(Rules_t, period, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, week_days, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, amount, JSON_TOK_UINT),

};

Application::Application()
{
}

Application::~Application()
{
}


int Application::get_rules()
{
    // TODO: Check how to store a struct in filesystem (Maybe a better approach would be store the json string and parse in this function)
    //  Storage &fs = Storage::getInstance();
    //  fs.read_data(RULES_ID, this->rules);
    //  printk("--Time: Hour: %d ----- Minutes: %d-- \r\n", this->rules.time.hour, this->rules.time.minutes);
    //  printk("--Date: Year: %d ----- Month: %d -----Day: %d -- \r\n", this->rules.date.year, this->rules.date.month, this->rules.date.day);
    //  printk("--Week days: %d -- \r\n", this->rules.week_days);
    //  printk("--Period: %d -- \r\n", this->rules.period);
    //  printk("--Amount: %d -- \r\n", this->rules.amount);
    //  If cant return erro code (TODO)
    return 0;
}

void Application::dispense_food()
{
    // this->motor.move_for(this->rules.amount);
    printk("DISPENSER %d AMOUNT OF FOOD\n\r", this->rules.amount);
}

bool Application::is_time_match()
{
    if (this->rtc.get_hour() != this->rules.time.hour)
    {
        printk("Hours Time Does Not Matchs\r\n");
        return false;
    }
    else if (this->rtc.get_minute() != this->rules.time.minutes)
    {
        printk("Minutes Time Does Not Matchs\r\n");
        return false;
    }
    else
    {
        printk("Time Matchs\r\n");
        return true;
    }
}

bool Application::is_date_match()
{
    if (this->rtc.get_day() != this->rules.date.day)
    {
        printk("Day Does Not Matchs\r\n");
        return false;
    }
    else if (this->rtc.get_month() != this->rules.date.month)
    {
        printk("Month Does Not Matchs\r\n");
        return false;
    }
    else if (this->rtc.get_year() != this->rules.date.year)
    {
        printk("Year Does Not Matchs\r\n");
        return false;
    }
    else
    {
        printk("Specifc Date Matchs\r\n");
        return true;
    }
}

bool Application::is_week_days_match(uint8_t week_day)
{

    week_day = this->rules.week_days & (1 << week_day);
    if (week_day)
    {
        printk("Week Day Matchs\r\n");
        return true;
    }
    else
    {
        printk("Week Day Does Not Matchs\r\n");
        return false;
    }
}

bool Application::check_rules()
{
    if (this->rules.period == WEEKLY)
    {
        uint8_t week_day = this->rtc.get_week_day();
        if (!this->is_week_days_match(week_day))
        {
            printk("RULES WEEK DAYS DOES NOT MATCHS\r\n");
            return false;
        }
        else if (!this->is_time_match())
        {
            printk("RULES TIME DOES NOT MATCHS\r\n");
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (this->rules.period == SPECIF)
    {
        if (!this->is_date_match())
        {
            printk("RULES DATE DOES NOT MATCHS\r\n");
            return false;
        }
        else
        {
            printk("RULES DOES NOT MATCHS\r\n");
            return true;
        }
    }
    else
    {
        printk("RULES PERIOD DOES NOT MATCHS\r\n");
        return false;
    }
}

bool Application::init_wifi()
{
    /*Storage for wifi credentials*/
    int ret;
    char ssid[16];
    char psk[16];
    Storage &fs = Storage::getInstance();
    ret = fs.init_storage();
    if (ret != 0)
    {
        printk("Error to init Fs\r\n");
        return false;
    }

    ret = fs.read_data(SSID_ID, ssid, sizeof(ssid));

    if (ret < 0)
    {
        printk("No SSID data stored\r\n");
        return false;
    }
    else
    {
        printk("SSID Data founded in FS\r\n");
        printk("SSID data: [%s]\r\n", ssid);
    }
    ret = fs.read_data(PASSWORD_ID, psk, sizeof(psk));
    if (ret < 0)
    {
        printk("No PSK data stored\r\n");
        return false;
    }
    else
    {
        printk("PSK Data founded in FS\r\n");
    }
    this->network.wifi_init();
    ret = this->network.connect_to_wifi(ssid, psk);
    if (ret < 0)
    {
        printk("Connecto to wifi return < 0\r\n");
        return false;
    }
    return true;
}

void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);

    // MAIN APP LOOP
    printk("App Task created!!\r\n");
    if (!self->init_wifi())
    {
        printk("ERROR TO INIT WIFI\n\r");
    }
    self->rtc.sync_time();
    // self->client.start_http();
   self->mqtt.start_mqtt();
   
    bool is_dispenser_executed = false;
    while (true)
    {
        // self->get_rules();
        // if (self->check_rules())
        // {
        //     if (!is_dispenser_executed)
        //     {
        //         self->dispense_food();
        //         is_dispenser_executed = true;
        //     }
        // }
        // else
        // {
        //     is_dispenser_executed = false;
        // }
        k_msleep(CONFIG_APPLICATION_THREAD_PERIOD);
    }
    self->network.wifi_disconnect();
}

void Application::start_application()
{
    k_thread_create(&this->AppTask, APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE, this->app, this, NULL, NULL, CONFIG_APP_THREAD_PRIORITY, 0, K_NO_WAIT);
}
