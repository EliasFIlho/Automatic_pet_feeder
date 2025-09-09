#include "Application.hpp"
#include "Storage.hpp"
#include "RTC.hpp"
#include "Storage.hpp"
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/kernel.h>

#define WTD_TIMEOUT_THRESHOLD 500

struct k_sem update_rules;

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

static const struct json_obj_descr rules_json_obj[] = {

    JSON_OBJ_DESCR_OBJECT(Rules_t, date, rules_specific_date),
    JSON_OBJ_DESCR_OBJECT(Rules_t, time, rules_time),
    JSON_OBJ_DESCR_PRIM(Rules_t, period, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, week_days, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, amount, JSON_TOK_UINT),

};

Application::Application()
{
    k_sem_init(&update_rules, 0, 1);
}

Application::~Application()
{
}

void print_rules(const Rules_t *r)
{
    printk("=== Scheduler Rule ===\n");
    printk("Date     : %04u-%02u-%02u\n",
           r->date.year,
           r->date.month,
           r->date.day);

    printk("Time     : %02u:%02u\n",
           r->time.hour,
           r->time.minutes);

    printk("Period   : %s\n",
           (r->period == WEEKLY) ? "WEEKLY" : "SPECIF");

    printk("Weekdays : 0x%02X\n", r->week_days); /* bitmask if you use it that way */
    printk("Amount   : %u\n", r->amount);
    printk("======================\n");
}

// TODO: Create error code returns
int Application::get_rules()
{
    char rules_buff[500];
    Storage &fs = Storage::getInstance();
    fs.read_data(RULES_ID, rules_buff, sizeof(rules_buff));
    int ret = json_obj_parse(rules_buff, sizeof(rules_buff), rules_json_obj, ARRAY_SIZE(rules_json_obj), &this->rules);
    if (ret < 0)
    {
        printk("Error to parse rules: %d\n\r", ret);
    }
    else
    {
        printk("Parser return value: %d\n\r", ret);
        print_rules(&this->rules);
    }
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
        printk("Connect to wifi return < 0\r\n");
        return false;
    }
    return true;
}

void Application::app(void *p1, void *, void *)
{
    int ret;
    auto *self = static_cast<Application *>(p1);

    if (!self->init_wifi())
    {
        // TODO: Handle with this error and implement Offline mode :p (this may take longer since i'll need a hardware to store the time)
        printk("ERROR TO INIT WIFI\n\rRuning Offline Mode");
    }
    else
    {
        // TODO: Check for error
        self->rtc.sync_time();
        self->mqtt.start_mqtt();
    }

    bool is_dispenser_executed = false;

      //TODO: Implement a callback to handle watchdog errors - like rescheduler task or something
    int task_wdt_id = task_wdt_add(CONFIG_APPLICATION_THREAD_PERIOD + WTD_TIMEOUT_THRESHOLD, NULL, NULL);

    /*At first this will called only one time in the start of task to fill the rules struct,
    and then just will be called again if mqtt says that has received a update.
    for future work it will be created a circular buffer to store N rules.
    */

    // TODO: After set errors codes, check this to avoid start the application if rules are not avaliable
    self->get_rules();
    while (true)
    {
        if (k_sem_take(&update_rules, K_NO_WAIT) == 0)
        {
            self->get_rules();
        }

        if (self->check_rules())
        {
            if (!is_dispenser_executed)
            {
                self->dispense_food();
                is_dispenser_executed = true;
            }
        }
        else
        {
            is_dispenser_executed = false;
        }
      
        ret = task_wdt_feed(task_wdt_id);
        if (ret != 0)
        {
            printk("Error to feed watchdog task in APP\n\r");
        }
        else
        {
            printk("APP: Feed watchdog task\n\r");
        }
        k_msleep(CONFIG_APPLICATION_THREAD_PERIOD);
    }
    self->network.wifi_disconnect();
}

void Application::start_application()
{
    k_thread_create(&this->AppTask, APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE, this->app, this, NULL, NULL, CONFIG_APP_THREAD_PRIORITY, 0, K_NO_WAIT);
}
