#ifndef __TLIGHT_CONFIG_H__
#define __TLIGHT_CONFIG_H__

#define TL_PARAM_SYS_NAME	"SystemName"
#define TL_PARAM_COM		"ComPort"
#define TL_PARAM_COM_CFG	"ComCfg"
#define TL_PARAM_WD			"WDInterval"
#define TL_PARAM_TIMEOUT	"Timeout"
#define TL_PARAM_MODULE		"Module"
#define TL_PARAM_MASTER		"Master"
#define TL_PARAM_GATE		"Gate"
#define TL_PARAM_LAMP		"Lamp"
#define TL_PARAM_TIMER		"Timer"

#define TL_CMD_DELAY			"Delay"
#define TL_CMD_LOOP				"Loop"
#define TL_CMD_TIMER_CDOWN		"countdown"
#define TL_CMD_TIMER_DISPLAY	"display"
#define TL_CMD_TIMER_RED		"red"
#define TL_CMD_TIMER_GREEN		"green"

int tl_parse_cfg_file(char *szFileName, tl_global_t *pGlobal);
int tl_parse_prog_file(char *szFileName, tl_program_t **ppProgram);

#endif /* __TLIGHT_CONFIG_H__ */
