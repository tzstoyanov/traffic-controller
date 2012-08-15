#ifndef __TLIGHT_CONFIG_H__
#define __TLIGHT_CONFIG_H__

#define TL_PARAM_SYS_NAME	"SystemName"
#define TL_PARAM_COM		"ComPort"
#define TL_PARAM_COM_CFG	"ComCfg"
#define TL_PARAM_WD			"WDInterval"
#define TL_PARAM_DEADCOUNT	"DeadCount"
#define TL_PARAM_TIMEOUT	"Timeout"
#define TL_PARAM_MODULE		"Module"
#define TL_PARAM_MASTER		"Master"
#define TL_PARAM_GATE		"Gate"
#define TL_PARAM_LAMP		"Lamp"
#define TL_PARAM_TIMER		"Timer"

#define TL_COM_SERIAL_PREFIX	"/dev/"

typedef struct
{
	char *szCrossName;
	tl_com_t stPort;
	int32_t iWdInterval;
	int32_t iDeadCount;
	tl_controler_t *arrDevices[TL_MAX_CONTROLERS];
	struct timeval stStartTime;
}tl_global_config_t;

int tl_parse_cfg_file(char *szFileName, tl_global_config_t*pGlobal, 
							tl_new_controler_callback_t fNewControler);
int tl_get_next_token(char **pszLine, char **pszString);
tl_global_config_t *tl_get_global_dbase();
void tl_daemonzie(int iSem);

#endif /* __TLIGHT_CONFIG_H__ */
