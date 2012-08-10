#ifndef __TLIGHT_LIB_COM_H__
#define __TLIGHT_LIB_COM_H__

int tl_com_sym_open(tl_com_t *pCom);
int tl_com_read_packet(tl_com_t *pCom);
int tl_com_send(tl_com_t *pCom);

#endif /* __TLIGHT_LIB_COM_H__ */
