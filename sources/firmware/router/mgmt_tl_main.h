#ifndef __TLIGHT_MGMT_MAIN_H__
#define __TLIGHT_MGMT_MAIN_H__

#define TL_MGMT_LOG_PREFIX	"TL MGMT"

int get_client_fd();

void print_help_program();
void process_program(char *szArgument, char *szValue);

void print_help_status();
void process_status(char *szArgument, char *szValue);

void print_help_debug();
void process_debug(char *szArgument, char *szValue);

#endif /* __TLIGHT_MGMT_MAIN_H__ */
