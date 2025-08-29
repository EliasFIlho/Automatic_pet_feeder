#include "Application.hpp"
#include "Storage.hpp"

#include "RTC.hpp"

K_THREAD_STACK_DEFINE(APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE);

Application::Application()
{
}

Application::~Application()
{
}


//TODO: Move this MOCKED function to a test enviroment
#if !CONFIG_MOCK_TEST
int Application::get_rules()
{
    // Goto to filesystem
    // Populate rules struct
    // If cant return erro code (TODO)
}
#else
int Application::get_rules()
{
    this->rules.period = WEEKLY;
    if (this->rules.period == WEEKLY)
    {
        this->rules.week_days = 127; // All days
    }
    else
    { // Specifc year day
        this->rules.date.day = 27;
        this->rules.date.month = 8;
        this->rules.date.year = 2025;
    }

    this->rules.time.hour = 22;
    this->rules.time.minutes = 0;
    this->rules.amount = 90;

    return 0;
}
#endif

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
    this->network.connect_to_wifi(ssid, psk);
    return true;
}

void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);

    // MAIN APP LOOP
    printk("App Task created!!\r\n");
    self->init_wifi();
    self->rtc.sync_time();
    self->client.start_http();
    bool is_dispenser_executed = false;
    while (true)
    {
        self->get_rules();
        if (self->check_rules())
        {
            if (!is_dispenser_executed)
            {
                self->dispense_food();
                is_dispenser_executed = true;
            }
        }else{
            is_dispenser_executed = false;
        }
        k_msleep(CONFIG_APPLICATION_THREAD_PERIOD);
    }
    self->network.wifi_disconnect();
}

void Application::start_application()
{
    k_thread_create(&this->AppTask, APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE, this->app, this, NULL, NULL, CONFIG_APP_THREAD_PRIORITY, 0, K_NO_WAIT);
}
