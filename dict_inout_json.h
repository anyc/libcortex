
#ifndef _CRTX_DICT_INOUT_JSON_H
#define _CRTX_DICT_INOUT_JSON_H

void crtx_dict_parse_json(struct crtx_dict *dict, char *string);
char crtx_dict_json_from_file(struct crtx_dict **dict, char *dictdb_path, char *id);

#endif
