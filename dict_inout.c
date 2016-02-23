
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

#include <linux/limits.h>

#include "core.h"
#include "dict.h"
#include "dict_inout.h"
#include "dict_inout_json.h"

struct serialized_dict_item {
	uint64_t key_length;
	uint64_t payload_size;
	
	uint8_t type;
	uint8_t flags;
};

struct serialized_dict {
	uint32_t signature_length;
};


int crtx_wrapper_read(void *conn_id, void *data, size_t data_size) {
	return read(*(int*) conn_id, data, data_size);
}

int crtx_wrapper_write(void *conn_id, void *data, size_t data_size) {
	return write(*(int*) conn_id, data, data_size);
}

void crtx_write_dict(write_fct write, void *conn_id, struct crtx_dict *ds) {
	struct crtx_dict_item *di;
	struct serialized_dict sdict;
	struct serialized_dict_item sdi;
	char *s;
	int ret;
	
	#ifdef DEBUG
	// suppress valgrind warning
	memset(&sdi, 0, sizeof(struct serialized_dict_item));
	#endif
	
	sdict.signature_length = strlen(ds->signature);
	
	ret = write(conn_id, &sdict, sizeof(struct serialized_dict));
	if (ret < 0) {
		printf("writing event failed: %s\n", strerror(errno));
		return;
	}
	
	ret = write(conn_id, ds->signature, sdict.signature_length);
	if (ret < 0) {
		printf("writing event failed: %s\n", strerror(errno));
		return;
	}
	
	s = ds->signature;
	di = ds->items;
	while (*s) {
		sdi.key_length = strlen(di->key);
		
		sdi.type = di->type;
		sdi.flags = di->flags;
		
		switch (*s) {
			case 'u':
			case 'i':
				sdi.payload_size = sizeof(uint32_t);
				break;
			case 's':
			case 'D':
				sdi.payload_size = di->size;
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return;
		}
		
		ret = write(conn_id, &sdi, sizeof(struct serialized_dict_item));
		if (ret < 0) {
			printf("writing event failed: %s\n", strerror(errno));
			return;
		}
		
		ret = write(conn_id, di->key, sdi.key_length);
		if (ret < 0) {
			printf("writing event failed: %s\n", strerror(errno));
			return;
		}
		
		switch (*s) {
			case 'u':
			case 'i':
				ret = write(conn_id, &di->uint32, sdi.payload_size);
				if (ret < 0) {
					printf("writing event failed: %s\n", strerror(errno));
					return;
				}
				break;
			case 's':
				ret = write(conn_id, di->pointer, sdi.payload_size);
				if (ret < 0) {
					printf("writing event failed: %s\n", strerror(errno));
					return;
				}
				break;
			case 'D':
				crtx_write_dict(write, conn_id, di->ds);
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return;
		}
		
		di++;
		s++;
	}
}


char crtx_read(read_fct read, void *conn_id, void *buffer, size_t read_bytes) {
	int n;
	
	n = read(conn_id, buffer, read_bytes);
	if (n < 0) {
		printf("error while reading from socket: %s\n", strerror(errno));
		return 0;
	} else
	if (n != read_bytes) {
		printf("did not receive enough data (%d != %zu), aborting\n", n, read_bytes);
		return 0;
	}
	
	return 1;
}

struct crtx_dict *crtx_read_dict(read_fct read, void *conn_id) {
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	struct serialized_dict sdict;
	struct serialized_dict_item sdi;
	char *s;
	char ret;
	uint8_t i;
	
	
	ret = crtx_read(read, conn_id, &sdict, sizeof(struct serialized_dict));
	if (!ret) {
		return 0;
	}
	
	if (sdict.signature_length >= CRTX_MAX_SIGNATURE_LENGTH) {
		printf("Too long signature (%u >= %u)\n", sdict.signature_length, CRTX_MAX_SIGNATURE_LENGTH);
		
		return 0;
	}
	
	ds = (struct crtx_dict*) calloc(1, sizeof(struct crtx_dict) + sizeof(struct crtx_dict_item)*sdict.signature_length);
	
	ds->signature = (char*) malloc(sdict.signature_length+1);
	ret = crtx_read(read, conn_id, ds->signature, sdict.signature_length);
	if (!ret) {
		free_dict(ds);
		return 0;
	}
	ds->signature[sdict.signature_length] = 0;
	ds->signature_length = sdict.signature_length;
	
	i = 0;
	s = ds->signature;
	di = ds->items;
	while (*s) {
		ret = crtx_read(read, conn_id, &sdi, sizeof(struct serialized_dict_item));
		if (!ret) {
			free_dict(ds);
			return 0;
		}
		
		di->type = sdi.type;
		di->flags = sdi.flags | DIF_KEY_ALLOCATED;
		
		di->key = (char*) malloc(sdi.key_length+1);
		ret = crtx_read(read, conn_id, di->key, sdi.key_length);
		if (!ret) {
			free_dict(ds);
			return 0;
		}
		di->key[sdi.key_length] = 0;
		
		switch (*s) {
			case 'u':
			case 'i':
				if (sdi.payload_size != sizeof(uint32_t)) {
					free_dict(ds);
					return 0;
				}
				
				ret = crtx_read(read, conn_id, &di->uint32, sdi.payload_size);
				if (!ret) {
					free_dict(ds);
					return 0;
				}
				break;
			case 's':
				di->string = (char*) malloc(sdi.payload_size+1);
				ret = crtx_read(read, conn_id, di->string, sdi.payload_size);
				if (!ret) {
					free_dict(ds);
					return 0;
				}
				di->string[sdi.payload_size] = 0;
				break;
			case 'D':
				di->ds = crtx_read_dict(read, conn_id);
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return 0;
		}
		
		di++;
		s++;
		i++;
	}
	
	return ds;
}

char *crtx_readfile(char *path) {
	FILE *f;
	size_t size, read_size;
	char *s;
	
	f = fopen(path, "r");
	if (!f) {
		printf("error reading file \"%s\": %s\n", path, strerror(errno));
		return 0;
	}
	
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	s = (char*) malloc(size + 1);
	read_size = fread(s, 1, size, f);
	if (read_size != size) {
		printf("error while reading file %s\n", path);
		return 0;
	}
	
	fclose(f);
	
	return s;
}

void crtx_load_dict(struct crtx_dict *dict, char *dictdb_path) {
	char *s;
	char file_path[PATH_MAX];
	char *path;
	
	if (!dictdb_path)
		dictdb_path = CRTX_DICT_DB_PATH;
	
	path = getenv("CRTX_DICTDB_PATH");
	if (path)
		dictdb_path = path;
	
	snprintf(file_path, PATH_MAX, "%s%s.json", dictdb_path?dictdb_path:"", dict->id);
	
	s = crtx_readfile(file_path);
	if (!s)
		return;
	
	dict = crtx_init_dict(0, 0, 0);
	
	#ifdef WITH_JSON
	crtx_load_dict_json(dict, s);
	#endif
	
	crtx_print_dict(dict);
}

void crtx_store_dict(struct crtx_dict *dict) {
// 	int f = open(config->name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
// 	if (f == -1) {
// 		printf("error %s\n", strerror(errno));
// 		return;
// 	}

// 	send_dict(crtx_wrapper_write, &f, d);

// 	close(f);
}

