
#ifndef _CRTX_DICT_INOUT_H
#define _CRTX_DICT_INOUT_H

typedef int (*write_fct)(void *conn_id, void *data, size_t data_size);
typedef int (*read_fct)(void *conn_id, void *data, size_t data_size);

#define CRTX_MAX_SIGNATURE_LENGTH 128

#ifndef CRTX_DICT_DB_PATH
#define CRTX_DICT_DB_PATH "/var/lib/cortex/dict_db/"
#endif

struct crtx_dict *crtx_read_dict(read_fct recv, void *conn_id);
void crtx_write_dict(write_fct write, void *conn_id, struct crtx_dict *ds);

int crtx_wrapper_read(void *conn_id, void *data, size_t data_size);
int crtx_wrapper_write(void *conn_id, void *data, size_t data_size);

char crtx_read(read_fct recv, void *conn_id, void *buffer, size_t read_bytes);
char *crtx_readfile(char *path);

#endif
