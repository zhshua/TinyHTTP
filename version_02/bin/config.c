#include "config.h"
#include <string.h>

void init_conf (struct conf_http_t *conf)
{
	char buf[128] = { '\0' };
	char key[128] = { '\0' };
	char value[128] = { '\0' };
	FILE *fp;
	if((fp = fopen(CONF_PATH, "r")) < 0)
	{
		perror("fail to open conf file, and will be use default conf\n");
		init_default_conf(conf);
		return ;
	}

	while(fgets(buf, 128, fp) != NULL)
	{
		sscanf(buf, "%s %s", key, value);
		if(strcmp(key, "PORT") == 0)
			conf->port = atoi(value);
		if(strcmp(key, "THREAD_NUM") == 0)
			conf->thread_num = atoi(value);
		if(strcmp(key, "LISTEN_NUM") == 0)
			conf->listen_num = atoi(value);
		if(strcmp(key, "FILE_DIR") == 0)
			strcpy(conf->doc, value);
		if(strcmp(key, "EVENT_LIST") == 0)
			conf->event_list = atoi(value);
	}
	fclose(fp);
	return ;
}

void init_default_conf(struct conf_http_t *conf)
{
	conf->port = default_port;
	conf->thread_num = default_thread_num;
	conf->listen_num = default_listen_num;
	strcpy (conf->doc, default_file_dir);
	conf->event_list = default_event_list;
	return ;
}

