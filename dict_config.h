
struct crtx_config;
typedef char *(*on_new_entry_cb)(struct crtx_config *cfg, char *key, struct crtx_dict *entry);

struct crtx_config {
	char *name;
	struct crtx_dict *data;
	
	on_new_entry_cb new_entry;
};

void crtx_load_config(struct crtx_config *config);
void crtx_store_config(struct crtx_config *config);