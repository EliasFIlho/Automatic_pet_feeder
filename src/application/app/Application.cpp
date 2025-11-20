#include "Application.hpp"

#define WTD_TIMEOUT_THRESHOLD 5000
#define RULES_TEMP_BUFF 250

Application::Application(IClock &clk, IMotor &motor, IStorage &fs, IWatchDog &guard, IJson &json, ITaskRunner &runner) : _clk(clk), _motor(motor), _fs(fs), _guard(guard), _json(json), _runner(runner)
{
}

Application::~Application()
{
}

void Application::on_network_event(NetworkEvent evt)
{
    switch (evt)
    {
    case NetworkEvent::WIFI_CONNECTED:
        // Enable RTC sync using SNTP
        break;
    case NetworkEvent::WIFI_DISCONNECTED:
        // Disable RTC sync and go offline mode
        break;
    case NetworkEvent::MQTT_DISCONNECTED:
        // Check if is relevant
        break;
    case NetworkEvent::MQTT_NEW_DATA:
        // Refresh the scheduller rules buffer with storage new data
        this->get_rules();
        break;
    default:
        break;
    }
}

int32_t Application::get_rules()
{
    char rules_buff[RULES_TEMP_BUFF];
    int32_t ret;
    ret = this->_fs.read_data(RULES_ID, rules_buff, sizeof(rules_buff));
    if (ret < 0)
    {
        return ret;
    }
    else
    {
        this->_json.parse(rules_buff, &this->rules);
        return 0;
    }
}

void Application::dispense_food()
{
    this->_motor.move_for(this->rules.amount);
}

bool Application::is_time_match()
{
    if (this->_clk.get_hour() != this->rules.time.hour)
    {
        return false;
    }
    else if (this->_clk.get_minute() != this->rules.time.minutes)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool Application::is_date_match()
{
    if (this->_clk.get_day() != this->rules.date.day)
    {
        return false;
    }
    else if (this->_clk.get_month() != this->rules.date.month)
    {
        return false;
    }
    else if (this->_clk.get_year() != this->rules.date.year)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool Application::is_week_days_match()
{
    uint8_t week_day = this->_clk.get_week_day();
    week_day = this->rules.week_days & (1 << week_day);
    if (week_day)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Application::check_rules()
{
    if (this->rules.period == WEEKLY)
    {

        if (!this->is_week_days_match())
        {
            return false;
        }
        else if (!this->is_time_match())
        {
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
            return false;
        }
        else if (!this->is_time_match())
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

void Application::step()
{
    if (this->check_rules())
    {
        if (!this->is_dispenser_executed)
        {
            this->dispense_food();
            this->is_dispenser_executed = true;
        }
    }
    else
    {
        this->is_dispenser_executed = false;
    }
}

void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);

    self->_clk.sync_time();
    self->_motor.init();
    self->task_wdt_id = self->_guard.create_and_get_wtd_timer_id(CONFIG_APPLICATION_THREAD_PERIOD + WTD_TIMEOUT_THRESHOLD);
    self->is_dispenser_executed = false;
    self->get_rules();

    while (true)
    {
        self->step();
        self->_guard.feed(self->task_wdt_id);
        self->_runner.sleep(CONFIG_APPLICATION_THREAD_PERIOD);
    }
}

//TODO: i've add this runner to avoid kernel modules inside application, but this seems too much...I'll create a interface for application so i can mock for tests in host machine
void Application::init_application()
{
    this->_runner.create_task(Application::app, this);
}