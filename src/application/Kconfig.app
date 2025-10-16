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

config MOCK_TEST
    bool "Enable application to use mocked functions"
    default y
endmenu




