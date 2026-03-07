menu "Application options"

config APP_THREAD_STACK_SIZE
    int "Define application thread stack size"
    default 1024

config APP_THREAD_PRIORITY
    int "Define application thread priority"
    default 5

config APPLICATION_THREAD_PERIOD
    int "Define the period in ms of te application thread"
    default 5000

config MAX_SCHEDULER_RULES
    int "Define the max of rules that the application will handle"
    default 5
endmenu




