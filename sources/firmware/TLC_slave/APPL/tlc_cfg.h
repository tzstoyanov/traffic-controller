#ifndef TLC_CFG_H
#define TLC_CFG_H

/*
 * FRAME :<id:11> <len:4> <data:0-64>
 * <id>  :<cmd:3><address:8>
 */

// address
#define TLC_ADDR_INVALID    0x00
#define TLC_ADDR_ALL        0xFF


// commands
#define TLC_CMD_CONFIG          1
#define TLC_CMD_STATE           2
#define TLC_CMD_WD_REQ          3
#define TLC_CMD_WD_RESP         4
#define TLC_CMD_WD_RESP_FRAG    5

/*
 * config FRAME: <<001><address>> <len:4> [<cfg:4><len:4> <data:0-56>]
 */
#define TLC_CFG_QUERY_INTERVAL      1
#define TLC_CFG_SUB_MODULES_ADD     2
#define TLC_CFG_SUB_MODULES_REMOVE  3

/*
 * config param QUERY_INTERVAL: <0001> <0001> <query_interval>
 */

/*
 * config param SUB_MODULES: <0010> <size:4> <modules: bytes up to 7>
 */

/*
 * state FRAME: <<010> <address>> <len:4> <type:3> <data:5-61>
 */
#define TLC_TYPE_MASTER     0
#define TLC_TYPE_GATEWAY    1
#define TLC_TYPE_LAMP       2
#define TLC_TYPE_TIMER      3

/*
 * state FRAME lamp: <<010><address>> <0001> <010><res:1><lamps state:4>
 */

/*
 * state FRAME timer: <<010><address>> <len:4> <011><color:2><action:3><action data>
 */
#define TLC_TIMER_COLOR_GREEN     0
#define TLC_TIMER_COLOR_RED       1

#define TLC_TIMER_ACTION_COUNT      0   // 1 byte count reload
#define TLC_TIMER_ACTION_STATIC     1   // 1 byte segment data

/*
 * frame WD_REQ <<011> <address>> <0001> <resp address:8>
 */

/*
 * frame WD_RESP lamp  <<100> <address>> <0011> <slave address:8> <010> <status:1><error:4> <:4><lamps state:4>
 * frame WD_RESP timer <<100> <address>> <0100> <slave address:8> <011> <status:1><error:4> <:3><color:2><action:3><action data:8>
 * frame WD_RESP GW    <<100> <address>> <len:> [<slave address:8> <type:3> <status:1><error/state:4>]
 * frame WD_RESP GW    <<101> <address>> <len:> [<slave address:8> <type:3> <status:1><error/state:4>]
 */

#define TLC_STATUS_OK       0
#define TLC_STATUS_FAIL     1

#define TLC_ERROR_OK        0
#define TLC_ERROR_INIT      1
#define TLC_ERROR_OUTPUT    2
#define TLC_ERROR_COMM      3       // ?!?
#define TLC_ERROR_SLAVE     4       // ?!?
#define TLC_ERROR_TIMEOUT   5

#endif
