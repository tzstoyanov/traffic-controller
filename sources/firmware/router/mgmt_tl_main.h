#ifndef __TLIGHT_MGMT_MAIN_H__
#define __TLIGHT_MGMT_MAIN_H__

#define TL_MGMT_LOG_PREFIX	"TL MGMT"

int get_client_fd();

int tl_mgmt_connect();
int tl_mgmt_send(int iFd, uint8_t iCommand, uint8_t *pData, uint32_t iLen);
int tl_mgmt_get(int iFd, uint8_t *pData, uint32_t iLen);

void print_help_program();
void process_program(char *szArgument, char *szValue);

void print_help_status();
void process_status(char *szArgument, char *szValue);

void print_help_debug();
void process_debug(char *szArgument, char *szValue);

#endif /* __TLIGHT_MGMT_MAIN_H__ */
