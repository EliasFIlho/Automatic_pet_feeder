#include "Application.hpp"

#define WTD_TIMEOUT_THRESHOLD 5000

K_THREAD_STACK_DEFINE(APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE);
LOG_MODULE_REGISTER(APPLICATION_LOG);

Application::Application(IRTC &clk, IMotor &motor, IStorage &fs, IWatchDog &guard) : _clk(clk), _motor(motor), _fs(fs), _guard(guard)
{
}

Application::~Application()
{
}

void Application::Update(Events evt)
{
    switch (evt)
    {
    case Events::MQTT_NEW_DATA:
        // Refresh the scheduller rules buffer with storage new data
        this->shouldUpdateRules = true;
        break;
    default:
        break;
    }
}

int32_t Application::get_rules()
{
    int32_t ret;
    ret = this->_fs.read_buffer(RULES_ID, &this->rules, sizeof(this->rules));
    if (ret < 0)
    {
        return ret;
    }
    else
    {
        this->isRulesAvaliable = true;
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

void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);
    self->state = APP_STATES::INIT;
    self->task_wdt_id = self->_guard.create_and_get_wtd_timer_id(CONFIG_APPLICATION_THREAD_PERIOD + WTD_TIMEOUT_THRESHOLD);

    while (true)
    {

        switch (self->state)
        {
        case APP_STATES::INIT:

            self->_motor.init();
            self->isDispenserExecuted = false;
            self->shouldUpdateRules = false;
            self->state = APP_STATES::LOAD_RULES;
            break;
        case APP_STATES::LOAD_RULES:

            self->isRulesAvaliable = false;
            self->get_rules();
            self->state = APP_STATES::CHECK_RULES;
            break;
        case APP_STATES::CHECK_RULES:

            if (!self->isRulesAvaliable)
            {
                self->state = APP_STATES::IDLE;
                break;
            }

            if (self->check_rules())
            {
                self->state = APP_STATES::PROCESS;
            }
            else
            {
                self->isDispenserExecuted = false; // Reset dispenser flag
                self->state = APP_STATES::IDLE;
            }
            break;
        case APP_STATES::PROCESS:
            if (!self->isDispenserExecuted)
            {
                self->dispense_food();
                self->isDispenserExecuted = true;
            }
            self->state = APP_STATES::IDLE;
            break;
        case APP_STATES::IDLE:
            if (self->shouldUpdateRules)
            {
                self->state = APP_STATES::LOAD_RULES;
                self->shouldUpdateRules = false;
            }
            else
            {
                self->state = APP_STATES::CHECK_RULES;
            }

            self->_guard.feed(self->task_wdt_id);
            k_msleep(CONFIG_APPLICATION_THREAD_PERIOD);
            break;
        default:
            break;
        }
    }
}

void Application::init_application()
{
    k_thread_create(&this->app_thread, APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE, Application::app, this, NULL, NULL, CONFIG_APP_THREAD_PRIORITY, 0, K_NO_WAIT);
}