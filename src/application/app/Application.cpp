#include "Application.hpp"

#define WTD_TIMEOUT_THRESHOLD 500

Application::Application(IClock &clk, IMotor &motor, IStorage &fs, IWatchDog &guard, IJson &json, ITaskRunner &runner) : _clk(clk), _motor(motor), _fs(fs), _guard(guard), _json(json), _runner(runner)
{
    this->is_dispenser_executed = false;
}

Application::~Application()
{
}


int32_t Application::get_rules()
{
    char rules_buff[250];
    int32_t ret;
    ret = this->_fs.read_data(RULES_ID, rules_buff, sizeof(rules_buff));
    if(ret < 0){
        return ret;
    }else{
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

    this->_guard.feed(this->task_wdt_id);
}


void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);

    self->_clk.sync_time();
    self->_motor.init();
    self->task_wdt_id = self->_guard.create_and_get_wtd_timer_id(CONFIG_APPLICATION_THREAD_PERIOD + WTD_TIMEOUT_THRESHOLD);
    
    self->get_rules();
    while (true)
    {
        self->step();
        self->_runner.sleep(CONFIG_APPLICATION_THREAD_PERIOD);
    }
}

void Application::init_application()
{
    this->_runner.create_task(Application::app, this);
}