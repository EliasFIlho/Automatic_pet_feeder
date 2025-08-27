#include "Application.hpp"
#include "Storage.hpp"

#include "RTC.hpp"

#define APP_TASK_STACK_SIZE 4096
#define APP_TASK_PRIORITY 5
K_THREAD_STACK_DEFINE(APP_STACK_AREA, APP_TASK_STACK_SIZE);

Application::Application()
{
}

Application::~Application()
{
}

void Application::get_rules()
{
}

void Application::dispense_food(int amount)
{
}
bool Application::check_date()
{
    return true;
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
    this->network.connect_to_wifi(ssid, psk);
    return true;
}

void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);

    // MAIN APP LOOP
    self->init_wifi();
    self->rtc.sync_time();
    self->network.wifi_disconnect();
    while (1)
    {
        printk("App Task creation works, nice\r\n");
        int date_ret = self->rtc.get_day();
        printk("DAY - [%d]\n\r",date_ret);
        // date_ret = self->rtc.get_month();
        // date_ret = self->rtc.get_day();
        // date_ret = self->rtc.get_week_day();
        // date_ret = self->rtc.get_hour();
        // date_ret = self->rtc.get_minute();
        k_msleep(1000);
    }
}

void Application::start_application()
{
    k_thread_create(&this->AppTask, APP_STACK_AREA, APP_TASK_STACK_SIZE, this->app, this, NULL, NULL, APP_TASK_PRIORITY, 0, K_NO_WAIT);
}
