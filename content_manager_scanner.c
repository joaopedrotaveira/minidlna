/**
 * @file content_manager_scanner.c
 *
 * @date 26 Jun 2012
 * @author Joao Pedro Taveira
 */

#include "config.h"

#ifdef P2P_SUPPORT

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <poll.h>

#include "upnpglobalvars.h"
#include "log.h"

#include <yaul/util.h>
#include <yaul/fetch_manager.h>
#include <yaul/simple_list_t.h>
#include <http_engine/http_fetch_method.h>
#include <http_engine/simple_fetch_engine.h>

#include <peer_client/content_manager_client.h>

//#define SARACEN_CONTENT_MANAGER "http://localhost:8092/manifest/list"
#define SARACEN_CONTENT_MANAGER "http://saracen.inov.pt:8092/manifest/list"

//static time_t next_pl_fill = 0;

int is_stream_descriptor_file(const char * path){
	int ret = 0;
	manifest_v2_t *manifest = manifest_v2_new_from_file(path);

	if(manifest){
		ret = 1;
		manifest_v2_free(manifest);
	}
	DPRINTF(E_INFO, L_P2P,  "Checking if %s is stream descriptor: %d\n",path,ret);
	return ret;
}

int content_manager_download_manifests(const char *content_manager_url){
	fetch_t *fetch = NULL;
	simple_fetch_engine_context_t *context = NULL;
	simple_list_t *manifests_list = NULL;
	simple_list_t *link = NULL;
	char *manifest_url = NULL;

	if((context = simple_fetch_engine_context_new(content_manager_url)) == NULL){
		DPRINTF(E_WARN, L_P2P,  "Failed to create simple fetch engine context\n");
		return -1;
	}

	simple_fetch_engine_context_set_mode(context,SIMPLE_FETCH);

	if((manifests_list = content_manager_client_get_manifests_list(content_manager_url)) == NULL){
		DPRINTF(E_WARN, L_P2P,  "Failed to create simple fetch engine context\n");
		simple_fetch_engine_context_free(context);
		return -1;
	}

	link = manifests_list;
	while(link){
		manifest_url = link->data;
		if(manifest_url){
			if((fetch = fetch_manager_run_engine("simple-fetch",manifest_url,FETCH_ON_MEMORY,"content",context)) == NULL){
				DPRINTF(E_WARN, L_P2P,  "Failed to download file: %s\n",manifest_url);
			} else {
				char *local_file = fetch_get_local_file(fetch);
				if(local_file && !yaul_file_exists(local_file)){
					FILE *file = NULL;
					ssize_t fetch_length = 0;
					DPRINTF(E_INFO, L_P2P,  "Saving manifest file: %s\n",local_file);
					const void * fetch_bytes = fetch_get_bytes(fetch,&fetch_length);
					if((file = fopen(local_file,"w+b")) == NULL){
						DPRINTF(E_ERROR, L_P2P,  "Failed to opening file: %s: %s\n",local_file,strerror(errno));
					} else	if(fwrite(fetch_bytes,sizeof(uint8_t),fetch_length,file) != fetch_length){
						DPRINTF(E_ERROR, L_P2P,  "Failed writing to file: %s: %s\n",local_file, strerror(errno));
						if(file) fclose(file);
					} else {
						DPRINTF(E_INFO, L_P2P,  "New manifest file: %s\n",local_file);
						if(file) fclose(file);
					}
					fetch_print(fetch);
				} else if(local_file){
					DPRINTF(E_INFO, L_P2P,  "Manifest file already exists: %s\n",local_file);
				} else {
					DPRINTF(E_ERROR, L_P2P,  "Local file name is null\n");
				}
				if(local_file){
					free(local_file);
					local_file = NULL;
				}
				if(fetch){
					fetch_free(fetch);
					fetch = NULL;
				}
			}
		}
		link = link->next;
	}
	simple_list_foreach(manifests_list,(simple_list_func)free,NULL);
	simple_list_free(manifests_list);
	return 1;
}

void *
start_content_manager_scanner(){
//	int length = 0;
//	struct pollfd pollfds[1];
//	int timeout = 10000;

	http_register_fetch_method();
	simple_fetch_engine_register();

	DPRINTF(E_INFO, L_P2P,  "Starting content manager scanner\n");

	while( scanning )
	{
		if( quitting )
			goto quitting;
		sleep(1);
	}
	if (setpriority(PRIO_PROCESS, 0, 19) == -1)
		DPRINTF(E_WARN, L_P2P,  "Failed to reduce content manager scanner thread priority\n");
	sqlite3_release_memory(1<<31);

	while( !quitting )
	{
		DPRINTF(E_INFO, L_P2P,  "BL BLA BLA LA ...\n");
//		length = poll(pollfds, 0, timeout);
//		if (!length) {
//			if (next_pl_fill && (time(NULL) >= next_pl_fill)) {
				//sleep(15);
				content_manager_download_manifests(SARACEN_CONTENT_MANAGER);
				sleep(15);
//				next_pl_fill = 0;
//			}

//			continue;
//		} else if (length < 0) {
//			if ((errno == EINTR) || (errno == EAGAIN))
//				continue;
//			else
//				DPRINTF(E_ERROR, L_P2P, "read failed!\n");
//		} else {
//
//		}
	}
quitting:

	return 0;
}

#endif /* P2P_SUPPORT */
