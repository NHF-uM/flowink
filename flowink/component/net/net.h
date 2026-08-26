#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>

void wifi_init(void);
void wifi_deinit1(void);

void http_set_revc_buf(uint8_t *bmp_buf);

extern struct k_sem sem_http_data_uping;

extern int http_server_start(void);
extern void http_server_stop(void);


#endif
