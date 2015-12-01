/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "lua.h"
#include "platform.h"
#include "c_string.h"
#include "c_stdlib.h"
#include "c_stdio.h"

#include "flash_fs.h"
#include "user_interface.h"
#include "user_exceptions.h"
#include "user_modules.h"

#include "ets_sys.h"
#include "driver/uart.h"
#include "mem.h"

#ifdef LUA_USE_MODULES_RTCTIME
#include "rtc/rtctime.h"
#endif

#define SIG_LUA 0
#define SIG_UARTINPUT 1
#define TASK_QUEUE_LEN 4
os_event_t *taskQueue;


/* Note: the trampoline *must* be explicitly put into the .text segment, since
 * by the time it is invoked the irom has not yet been mapped. This naturally
 * also goes for anything the trampoline itself calls.
 */
void TEXT_SECTION_ATTR user_start_trampoline (void)
{
   __real__xtos_set_exception_handler (
     EXCCAUSE_LOAD_STORE_ERROR, load_non_32_wide_handler);

#ifdef LUA_USE_MODULES_RTCTIME
  // Note: Keep this as close to call_user_start() as possible, since it
  // is where the cpu clock actually gets bumped to 80MHz.
  rtctime_early_startup ();
#endif
  call_user_start ();
}


/* To avoid accidentally losing the fix for the TCP port randomization
 * during an LWIP upgrade, we've implemented most it outside the LWIP
 * source itself. This enables us to test for the presence of the fix
 * /at link time/ and error out if it's been lost.
 * The fix itself consists of putting the function-static 'port' variable
 * into its own section, and get the linker to provide an alias for it.
 * From there we can then manually randomize it at boot.
 */
static inline void tcp_random_port_init (void)
{
  extern uint16_t _tcp_new_port_port; // provided by the linker script
  _tcp_new_port_port += xthal_get_ccount () % 4096;
}


void task_lua(os_event_t *e){
    char* lua_argv[] = { (char *)"lua", (char *)"-i", NULL };
    NODE_DBG("Task task_lua started.\n");
    switch(e->sig){
        case SIG_LUA:
            NODE_DBG("SIG_LUA received.\n");
            lua_main( 2, lua_argv );
            break;
        case SIG_UARTINPUT:
            lua_handle_input (false);
            break;
        case LUA_PROCESS_LINE_SIG:
            lua_handle_input (true);
            break;
        default:
            break;
    }
}

void task_init(void){
    taskQueue = (os_event_t *)os_malloc(sizeof(os_event_t) * TASK_QUEUE_LEN);
    system_os_task(task_lua, USER_TASK_PRIO_0, taskQueue, TASK_QUEUE_LEN);
}

// extern void test_spiffs();
// extern int test_romfs();

// extern uint16_t flash_get_sec_num();

void nodemcu_init(void)
{
    NODE_ERR("\n");
    // Initialize platform first for lua modules.
    if( platform_init() != PLATFORM_OK )
    {
        // This should never happen
        NODE_DBG("Can not init platform for modules.\n");
        return;
    }

#if defined(FLASH_SAFE_API)
    if( flash_safe_get_size_byte() != flash_rom_get_size_byte()) {
        NODE_ERR("Self adjust flash size.\n");
        // Fit hardware real flash size.
        flash_rom_set_size_byte(flash_safe_get_size_byte());
        // Flash init data at FLASHSIZE - 0x04000 Byte.
        flash_init_data_default();
        // Flash blank data at FLASHSIZE - 0x02000 Byte.
        flash_init_data_blank();
        if( !fs_format() )
        {
            NODE_ERR( "\ni*** ERROR ***: unable to format. FS might be compromised.\n" );
            NODE_ERR( "It is advised to re-flash the NodeMCU image.\n" );
        }
        else{
            NODE_ERR( "format done.\n" );
        }
        fs_unmount();   // mounted by format.
    }
#endif // defined(FLASH_SAFE_API)

    if( !flash_init_data_written() ){
        NODE_ERR("Restore init data.\n");
        // Flash init data at FLASHSIZE - 0x04000 Byte.
        flash_init_data_default();
        // Flash blank data at FLASHSIZE - 0x02000 Byte.
        flash_init_data_blank();
    }

#if defined( BUILD_WOFS )
    romfs_init();

    // if( !wofs_format() )
    // {
    //     NODE_ERR( "\ni*** ERROR ***: unable to erase the flash. WOFS might be compromised.\n" );
    //     NODE_ERR( "It is advised to re-flash the NodeWifi image.\n" );
    // }
    // else
    //     NODE_ERR( "format done.\n" );

    // test_romfs();
#elif defined ( BUILD_SPIFFS )
    fs_mount();
    // test_spiffs();
#endif
    // endpoint_setup();

    // char* lua_argv[] = { (char *)"lua", (char *)"-e", (char *)"print(collectgarbage'count');ttt={};for i=1,100 do table.insert(ttt,i*2 -1);print(i);end for k, v in pairs(ttt) do print('<'..k..' '..v..'>') end print(collectgarbage'count');", NULL };
    // lua_main( 3, lua_argv );
    // char* lua_argv[] = { (char *)"lua", (char *)"-i", NULL };
    // lua_main( 2, lua_argv );
    // char* lua_argv[] = { (char *)"lua", (char *)"-e", (char *)"pwm.setup(0,100,50) pwm.start(0) pwm.stop(0)", NULL };
    // lua_main( 3, lua_argv );
    // NODE_DBG("Flash sec num: 0x%x\n", flash_get_sec_num());

    tcp_random_port_init ();

    task_init();
    system_os_post(LUA_TASK_PRIO,SIG_LUA,'s');
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
#ifdef LUA_USE_MODULES_RTCTIME
    rtctime_late_startup ();
#endif
    // NODE_DBG("SDK version:%s\n", system_get_sdk_version());
    // system_print_meminfo();
    // os_printf("Heap size::%d.\n",system_get_free_heap_size());
    // os_delay_us(50*1000);   // delay 50ms before init uart

    UartBautRate br = BIT_RATE_DEFAULT;

    uart_init (br, br, USER_TASK_PRIO_0, SIG_UARTINPUT);

    #ifndef NODE_DEBUG
    system_set_os_print(0);
    #endif

    system_init_done_cb(nodemcu_init);
}
