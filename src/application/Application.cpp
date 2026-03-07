#include "Application.hpp"

#define WTD_TIMEOUT_THRESHOLD 5000

K_THREAD_STACK_DEFINE(APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE);
LOG_MODULE_REGISTER(APPLICATION_LOG);

Application::Application(IRTC &clk, IMotor &motor, IStorage &fs, IWatchDog &guard, ISchedulerRules &rules) : _clk(clk), _motor(motor), _fs(fs), _guard(guard), _rules(rules)
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

// TODO: Now read_rule does not return nothing, but i need to deal with a possible FS error
int32_t Application::get_rules()
{
    this->_rules.read_rules(this->rules);
    this->isRulesAvaliable = true;
    return 0;
}

void Application::process_rules()
{
    for (auto &i : this->rules)
    {
        if (i.state == RULE_STATE::WAITING)
        {
            LOG_INF("Running Rule %d", i.fs_index);
            this->dispense_food(i.rule.amount);
            i.state == RULE_STATE::EXECUTED;
        }
    }
}

void Application::dispense_food(const uint8_t amount)
{
    this->_motor.move_for(amount);
}

bool Application::is_time_match(const TimeRule_t &time)
{
    if (this->_clk.get_hour() != time.hour)
    {
        return false;
    }

    if (this->_clk.get_minute() != time.minutes)
    {
        return false;
    }

    return true;
}

bool Application::is_date_match(const SpecifcDateRule_t &date)
{
    if (this->_clk.get_day() != date.day)
    {
        return false;
    }
    if (this->_clk.get_month() != date.month)
    {
        return false;
    }
    if (this->_clk.get_year() != date.year)
    {
        return false;
    }

    return true;
}

bool Application::is_week_days_match(const uint8_t week_days_mask)
{
    uint8_t week_day = this->_clk.get_week_day();
    return ((week_days_mask & (1 << week_day)) != 0) ? true : false;
}

bool Application::check_rules(const Rules_t &rule)
{
    if (!this->is_time_match(rule.time))
    {
        return false;
    }

    if (rule.period == WEEKLY)
    {

        return this->is_week_days_match(rule.week_days);
    }
    if (rule.period == SPECIF)
    {
        return this->is_date_match(rule.date);
    }

    return false;
}

uint8_t Application::select_rules_to_execute()
{
    uint8_t idx = 0;
    for (int i = 0; i < this->_rules.get_number_of_rules(); i++)
    {
        if (this->check_rules(this->rules[i].rule))
        {
            if (this->rules[i].state == RULE_STATE::READY)
            {
                this->rules[i].state = RULE_STATE::WAITING;
                idx++;
            }
        }
        else if (this->rules[i].state == RULE_STATE::EXECUTED && this->rules[i].rule.period == WEEKLY)
        {
            this->rules[i].state = RULE_STATE::READY;
        }
    }

    return idx; /*If > 0 means some rule is waiting, this will tell the sate machine to execute the actuator
    (this is only needed bcs if no rule is waiting that means i dont need to check again)*/
}

void Application::app(void *p1, void *, void *)
{
    auto *self = static_cast<Application *>(p1);
    self->state = APP_STATES::INIT;
    self->task_wdt_id = self->_guard.create_and_get_wtd_timer_id(CONFIG_APPLICATION_THREAD_PERIOD + WTD_TIMEOUT_THRESHOLD);
    self->_rules.init();

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

            if (self->select_rules_to_execute())
            {
                self->state = APP_STATES::PROCESS;
            }
            else
            {
                self->state = APP_STATES::IDLE;
            }
            break;
        case APP_STATES::PROCESS:

            self->process_rules();

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