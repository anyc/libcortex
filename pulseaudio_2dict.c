/* This file has been generated with hdr2dict.py */
#include <string.h>
// #include "/usr/include/pulse/pulseaudio.h"

#include "dict.h"

char crtx_pa_sink_input_info2dict(struct pa_sink_input_info *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di;

	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "index", ptr->index, sizeof(ptr->index), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "name", ptr->name, strlen(ptr->name), CRTX_DIF_CREATE_DATA_COPY);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "owner_module", ptr->owner_module, sizeof(ptr->owner_module), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "client", ptr->client, sizeof(ptr->client), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "sink", ptr->sink, sizeof(ptr->sink), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "sample_spec", 0, 0, 0);

	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "format", ptr->sample_spec.format, sizeof(ptr->sample_spec.format), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "rate", ptr->sample_spec.rate, sizeof(ptr->sample_spec.rate), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "channels", ptr->sample_spec.channels, sizeof(ptr->sample_spec.channels), 0);
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "channel_map", 0, 0, 0);

	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "channels", ptr->channel_map.channels, sizeof(ptr->channel_map.channels), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "map", 0, 0, 0);

		di2->dict = crtx_init_dict(0, ptr->channel_map.channels, 0);
		{
			int i;
			for (i=0; i < ptr->channel_map.channels; i++)
				crtx_fill_data_item(&di2->dict->items[i], 'i', 0, ptr->channel_map.map[i], sizeof(ptr->channel_map.map[0]), 0);
// 			crtx_fill_data_item(&di2->dict->items[1], 'i', 0, ptr->channel_map.map[1], sizeof(ptr->channel_map.map[1]), 0);
// 			crtx_fill_data_item(&di2->dict->items[2], 'i', 0, ptr->channel_map.map[2], sizeof(ptr->channel_map.map[2]), 0);
// 			crtx_fill_data_item(&di2->dict->items[3], 'i', 0, ptr->channel_map.map[3], sizeof(ptr->channel_map.map[3]), 0);
// 			crtx_fill_data_item(&di2->dict->items[4], 'i', 0, ptr->channel_map.map[4], sizeof(ptr->channel_map.map[4]), 0);
// 			crtx_fill_data_item(&di2->dict->items[5], 'i', 0, ptr->channel_map.map[5], sizeof(ptr->channel_map.map[5]), 0);
// 			crtx_fill_data_item(&di2->dict->items[6], 'i', 0, ptr->channel_map.map[6], sizeof(ptr->channel_map.map[6]), 0);
// 			crtx_fill_data_item(&di2->dict->items[7], 'i', 0, ptr->channel_map.map[7], sizeof(ptr->channel_map.map[7]), 0);
// 			crtx_fill_data_item(&di2->dict->items[8], 'i', 0, ptr->channel_map.map[8], sizeof(ptr->channel_map.map[8]), 0);
// 			crtx_fill_data_item(&di2->dict->items[9], 'i', 0, ptr->channel_map.map[9], sizeof(ptr->channel_map.map[9]), 0);
// 			crtx_fill_data_item(&di2->dict->items[10], 'i', 0, ptr->channel_map.map[10], sizeof(ptr->channel_map.map[10]), 0);
// 			crtx_fill_data_item(&di2->dict->items[11], 'i', 0, ptr->channel_map.map[11], sizeof(ptr->channel_map.map[11]), 0);
// 			crtx_fill_data_item(&di2->dict->items[12], 'i', 0, ptr->channel_map.map[12], sizeof(ptr->channel_map.map[12]), 0);
// 			crtx_fill_data_item(&di2->dict->items[13], 'i', 0, ptr->channel_map.map[13], sizeof(ptr->channel_map.map[13]), 0);
// 			crtx_fill_data_item(&di2->dict->items[14], 'i', 0, ptr->channel_map.map[14], sizeof(ptr->channel_map.map[14]), 0);
// 			crtx_fill_data_item(&di2->dict->items[15], 'i', 0, ptr->channel_map.map[15], sizeof(ptr->channel_map.map[15]), 0);
// 			crtx_fill_data_item(&di2->dict->items[16], 'i', 0, ptr->channel_map.map[16], sizeof(ptr->channel_map.map[16]), 0);
// 			crtx_fill_data_item(&di2->dict->items[17], 'i', 0, ptr->channel_map.map[17], sizeof(ptr->channel_map.map[17]), 0);
// 			crtx_fill_data_item(&di2->dict->items[18], 'i', 0, ptr->channel_map.map[18], sizeof(ptr->channel_map.map[18]), 0);
// 			crtx_fill_data_item(&di2->dict->items[19], 'i', 0, ptr->channel_map.map[19], sizeof(ptr->channel_map.map[19]), 0);
// 			crtx_fill_data_item(&di2->dict->items[20], 'i', 0, ptr->channel_map.map[20], sizeof(ptr->channel_map.map[20]), 0);
// 			crtx_fill_data_item(&di2->dict->items[21], 'i', 0, ptr->channel_map.map[21], sizeof(ptr->channel_map.map[21]), 0);
// 			crtx_fill_data_item(&di2->dict->items[22], 'i', 0, ptr->channel_map.map[22], sizeof(ptr->channel_map.map[22]), 0);
// 			crtx_fill_data_item(&di2->dict->items[23], 'i', 0, ptr->channel_map.map[23], sizeof(ptr->channel_map.map[23]), 0);
// 			crtx_fill_data_item(&di2->dict->items[24], 'i', 0, ptr->channel_map.map[24], sizeof(ptr->channel_map.map[24]), 0);
// 			crtx_fill_data_item(&di2->dict->items[25], 'i', 0, ptr->channel_map.map[25], sizeof(ptr->channel_map.map[25]), 0);
// 			crtx_fill_data_item(&di2->dict->items[26], 'i', 0, ptr->channel_map.map[26], sizeof(ptr->channel_map.map[26]), 0);
// 			crtx_fill_data_item(&di2->dict->items[27], 'i', 0, ptr->channel_map.map[27], sizeof(ptr->channel_map.map[27]), 0);
// 			crtx_fill_data_item(&di2->dict->items[28], 'i', 0, ptr->channel_map.map[28], sizeof(ptr->channel_map.map[28]), 0);
// 			crtx_fill_data_item(&di2->dict->items[29], 'i', 0, ptr->channel_map.map[29], sizeof(ptr->channel_map.map[29]), 0);
// 			crtx_fill_data_item(&di2->dict->items[30], 'i', 0, ptr->channel_map.map[30], sizeof(ptr->channel_map.map[30]), 0);
// 			crtx_fill_data_item(&di2->dict->items[31], 'i', 0, ptr->channel_map.map[31], sizeof(ptr->channel_map.map[31]), 0);
		}
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "volume", 0, 0, 0);

	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "channels", ptr->volume.channels, sizeof(ptr->volume.channels), 0);

		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'D', "values", 0, 0, 0);

		di2->dict = crtx_init_dict(0, ptr->volume.channels, 0);
		{
			int i;
			for (i=0; i < ptr->volume.channels; i++)
				crtx_fill_data_item(&di2->dict->items[i], 'u', 0, ptr->volume.values[i], sizeof(ptr->volume.values[0]), 0);
// 			crtx_fill_data_item(&di2->dict->items[1], 'u', 0, ptr->volume.values[1], sizeof(ptr->volume.values[1]), 0);
// 			crtx_fill_data_item(&di2->dict->items[2], 'u', 0, ptr->volume.values[2], sizeof(ptr->volume.values[2]), 0);
// 			crtx_fill_data_item(&di2->dict->items[3], 'u', 0, ptr->volume.values[3], sizeof(ptr->volume.values[3]), 0);
// 			crtx_fill_data_item(&di2->dict->items[4], 'u', 0, ptr->volume.values[4], sizeof(ptr->volume.values[4]), 0);
// 			crtx_fill_data_item(&di2->dict->items[5], 'u', 0, ptr->volume.values[5], sizeof(ptr->volume.values[5]), 0);
// 			crtx_fill_data_item(&di2->dict->items[6], 'u', 0, ptr->volume.values[6], sizeof(ptr->volume.values[6]), 0);
// 			crtx_fill_data_item(&di2->dict->items[7], 'u', 0, ptr->volume.values[7], sizeof(ptr->volume.values[7]), 0);
// 			crtx_fill_data_item(&di2->dict->items[8], 'u', 0, ptr->volume.values[8], sizeof(ptr->volume.values[8]), 0);
// 			crtx_fill_data_item(&di2->dict->items[9], 'u', 0, ptr->volume.values[9], sizeof(ptr->volume.values[9]), 0);
// 			crtx_fill_data_item(&di2->dict->items[10], 'u', 0, ptr->volume.values[10], sizeof(ptr->volume.values[10]), 0);
// 			crtx_fill_data_item(&di2->dict->items[11], 'u', 0, ptr->volume.values[11], sizeof(ptr->volume.values[11]), 0);
// 			crtx_fill_data_item(&di2->dict->items[12], 'u', 0, ptr->volume.values[12], sizeof(ptr->volume.values[12]), 0);
// 			crtx_fill_data_item(&di2->dict->items[13], 'u', 0, ptr->volume.values[13], sizeof(ptr->volume.values[13]), 0);
// 			crtx_fill_data_item(&di2->dict->items[14], 'u', 0, ptr->volume.values[14], sizeof(ptr->volume.values[14]), 0);
// 			crtx_fill_data_item(&di2->dict->items[15], 'u', 0, ptr->volume.values[15], sizeof(ptr->volume.values[15]), 0);
// 			crtx_fill_data_item(&di2->dict->items[16], 'u', 0, ptr->volume.values[16], sizeof(ptr->volume.values[16]), 0);
// 			crtx_fill_data_item(&di2->dict->items[17], 'u', 0, ptr->volume.values[17], sizeof(ptr->volume.values[17]), 0);
// 			crtx_fill_data_item(&di2->dict->items[18], 'u', 0, ptr->volume.values[18], sizeof(ptr->volume.values[18]), 0);
// 			crtx_fill_data_item(&di2->dict->items[19], 'u', 0, ptr->volume.values[19], sizeof(ptr->volume.values[19]), 0);
// 			crtx_fill_data_item(&di2->dict->items[20], 'u', 0, ptr->volume.values[20], sizeof(ptr->volume.values[20]), 0);
// 			crtx_fill_data_item(&di2->dict->items[21], 'u', 0, ptr->volume.values[21], sizeof(ptr->volume.values[21]), 0);
// 			crtx_fill_data_item(&di2->dict->items[22], 'u', 0, ptr->volume.values[22], sizeof(ptr->volume.values[22]), 0);
// 			crtx_fill_data_item(&di2->dict->items[23], 'u', 0, ptr->volume.values[23], sizeof(ptr->volume.values[23]), 0);
// 			crtx_fill_data_item(&di2->dict->items[24], 'u', 0, ptr->volume.values[24], sizeof(ptr->volume.values[24]), 0);
// 			crtx_fill_data_item(&di2->dict->items[25], 'u', 0, ptr->volume.values[25], sizeof(ptr->volume.values[25]), 0);
// 			crtx_fill_data_item(&di2->dict->items[26], 'u', 0, ptr->volume.values[26], sizeof(ptr->volume.values[26]), 0);
// 			crtx_fill_data_item(&di2->dict->items[27], 'u', 0, ptr->volume.values[27], sizeof(ptr->volume.values[27]), 0);
// 			crtx_fill_data_item(&di2->dict->items[28], 'u', 0, ptr->volume.values[28], sizeof(ptr->volume.values[28]), 0);
// 			crtx_fill_data_item(&di2->dict->items[29], 'u', 0, ptr->volume.values[29], sizeof(ptr->volume.values[29]), 0);
// 			crtx_fill_data_item(&di2->dict->items[30], 'u', 0, ptr->volume.values[30], sizeof(ptr->volume.values[30]), 0);
// 			crtx_fill_data_item(&di2->dict->items[31], 'u', 0, ptr->volume.values[31], sizeof(ptr->volume.values[31]), 0);
		}
	}

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'U', "buffer_usec", ptr->buffer_usec, sizeof(ptr->buffer_usec), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'U', "sink_usec", ptr->sink_usec, sizeof(ptr->sink_usec), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "resample_method", ptr->resample_method, strlen(ptr->resample_method), CRTX_DIF_CREATE_DATA_COPY);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "driver", ptr->driver, strlen(ptr->driver), CRTX_DIF_CREATE_DATA_COPY);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "mute", ptr->mute, sizeof(ptr->mute), 0);

	di = crtx_alloc_item(dict);
// 	crtx_fill_data_item(di, 'p', "proplist", ptr->proplist, 0, CRTX_DIF_DONT_FREE_DATA);
	crtx_fill_data_item(di, 'D', "proplist", 0, 0, 0);
	crtx_pa_proplist2dict(ptr->proplist, &di->dict);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "corked", ptr->corked, sizeof(ptr->corked), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "has_volume", ptr->has_volume, sizeof(ptr->has_volume), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "volume_writable", ptr->volume_writable, sizeof(ptr->volume_writable), 0);

	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "format", 0, 0, 0);
	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "encoding", ptr->format->encoding, sizeof(ptr->format->encoding), 0);
		
		di2 = crtx_alloc_item(dict);
// 		crtx_fill_data_item(di2, 'p', "plist", ptr->format->plist, 0, CRTX_DIF_DONT_FREE_DATA);
		crtx_fill_data_item(di2, 'D', "plist", 0, 0, 0);
		crtx_pa_proplist2dict(ptr->format->plist, &di2->dict);
	}
	
	
	return 0;
}

char crtx_pa_card_info2dict(struct pa_card_info *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "index", ptr->index, sizeof(ptr->index), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "name", ptr->name, strlen(ptr->name), CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "owner_module", ptr->owner_module, sizeof(ptr->owner_module), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "driver", ptr->driver, strlen(ptr->driver), CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "n_profiles", ptr->n_profiles, sizeof(ptr->n_profiles), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "profiles", 0, 0, 0);
	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 's', "name", ptr->profiles->name, strlen(ptr->profiles->name), CRTX_DIF_DONT_FREE_DATA);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 's', "description", ptr->profiles->description, strlen(ptr->profiles->description), CRTX_DIF_DONT_FREE_DATA);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "n_sinks", ptr->profiles->n_sinks, sizeof(ptr->profiles->n_sinks), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "n_sources", ptr->profiles->n_sources, sizeof(ptr->profiles->n_sources), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "priority", ptr->profiles->priority, sizeof(ptr->profiles->priority), 0);
	}
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "active_profile", 0, 0, 0);
	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 's', "name", ptr->active_profile->name, strlen(ptr->active_profile->name), CRTX_DIF_DONT_FREE_DATA);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 's', "description", ptr->active_profile->description, strlen(ptr->active_profile->description), CRTX_DIF_DONT_FREE_DATA);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "n_sinks", ptr->active_profile->n_sinks, sizeof(ptr->active_profile->n_sinks), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "n_sources", ptr->active_profile->n_sources, sizeof(ptr->active_profile->n_sources), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "priority", ptr->active_profile->priority, sizeof(ptr->active_profile->priority), 0);
	}
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'p', "proplist", ptr->proplist, 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "n_ports", ptr->n_ports, sizeof(ptr->n_ports), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'p', "ports", ptr->ports, 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'p', "profiles2", ptr->profiles2, 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "active_profile2", 0, 0, 0);
	{
		di->dict = crtx_init_dict(0, 0, 0);
		struct crtx_dict *dict = di->dict;
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 's', "name", ptr->active_profile2->name, strlen(ptr->active_profile2->name), CRTX_DIF_DONT_FREE_DATA);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 's', "description", ptr->active_profile2->description, strlen(ptr->active_profile2->description), CRTX_DIF_DONT_FREE_DATA);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "n_sinks", ptr->active_profile2->n_sinks, sizeof(ptr->active_profile2->n_sinks), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "n_sources", ptr->active_profile2->n_sources, sizeof(ptr->active_profile2->n_sources), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'u', "priority", ptr->active_profile2->priority, sizeof(ptr->active_profile2->priority), 0);
		
		di2 = crtx_alloc_item(dict);
		crtx_fill_data_item(di2, 'i', "available", ptr->active_profile2->available, sizeof(ptr->active_profile2->available), 0);
	}
	
	return 0;
}




