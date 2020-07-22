#include "base.h"
#include "client.h"
#include "packet.h"
#include "cJSON.h"

int server_s;
time_t current_time;
struct client m_client;

const char *server_ip = "127.0.0.1";
int server_port = 50022;

const char program_name[] = "test";

int max_connections = 1024;
uint32_t torrent_id = -1;

typedef enum BT_CMD {
    START_BT= 2000,
    STOP_BT,
    SET_TRACKER,
    ADD_TASK,
    DEL_TASK,
    MAKE_TORRETN,
    GET_TASK_STATE,

    TASK_END = 8000,
}BT_CMD;


static void send_make_torrent(struct client *cli)
{
	char *data = &cli->data_buf[PACKET_LEN];
	//set_packet_head(cli);
	char *tmp = NULL;
	int len = 0;

	char file_path[128] = {0};
	char torrent_path[128] = {0};

	DEBUG("file_path: ");
	gets(file_path);

	DEBUG("torrent_path: ");
	gets(torrent_path);
	
	cJSON *root = cJSON_CreateObject();
    char *buf = NULL;
    if(root)
    {
        cJSON_AddStringToObject(root, "file_path", file_path);
        cJSON_AddStringToObject(root, "torrent_path", torrent_path);
    }

	tmp = cJSON_Print(root);
	len = strlen(tmp);

	memcpy(data, tmp, len);
	DEBUG("data %s", data);
	
	set_packet_head(cli->data_buf, MAKE_TORRETN, len, JSON_TYPE, 0);
	write(server_s, cli->data_buf, len + PACKET_LEN);
}


static void send_add_task(struct client *cli)
{
	char *data = &cli->data_buf[PACKET_LEN];
	char *tmp = NULL;
	int len = 0;

	char torrent_path[128] = {0};
	char save_path[128] = {0};


	DEBUG("torrent_path: ");
	gets(torrent_path);

	DEBUG("save_path: ");
	gets(save_path);

	cJSON *root = cJSON_CreateObject();
    char *buf = NULL;
    if(root)
    {
        cJSON_AddStringToObject(root, "torrent", torrent_path);
        cJSON_AddStringToObject(root, "save_path", save_path);
    }

	tmp = cJSON_Print(root);
	len = strlen(tmp);

	memcpy(data, tmp, len);
	DEBUG("data %s", data);
	
	set_packet_head(cli->data_buf, ADD_TASK, len, JSON_TYPE, 0);
	write(server_s, cli->data_buf, len + PACKET_LEN);
}

static void send_del_task(struct client *cli)
{
	char *data = &cli->data_buf[PACKET_LEN];
	char *tmp = NULL;
	int len = 0;

	cJSON *root = cJSON_CreateObject();
    char *buf = NULL;
    if(root)
    {
        cJSON_AddNumberToObject(root, "torrent_id", torrent_id);
    }

	tmp = cJSON_Print(root);
	len = strlen(tmp);

	memcpy(data, tmp, len);
	DEBUG("data %s", data);
	
	set_packet_head(cli->data_buf, DEL_TASK, len, JSON_TYPE, 0);
	write(server_s, cli->data_buf, len + PACKET_LEN);
}

static void send_get_task_state(struct client *cli)
{
	char *data = &cli->data_buf[PACKET_LEN];
	char *tmp = NULL;
	int len = 0;


	cJSON *root = cJSON_CreateObject();
    char buf[128] = {0};
	sprintf(buf, "%lld", torrent_id);
    if(root)
    {
        cJSON_AddStringToObject(root, "torrent_id", buf);
    }

	tmp = cJSON_Print(root);
	len = strlen(tmp);

	memcpy(data, tmp, len);
	DEBUG("data %s", data);
	
	set_packet_head(cli->data_buf, GET_TASK_STATE, len, JSON_TYPE, 0);
	write(server_s, cli->data_buf, len + PACKET_LEN);
}

static void send_set_tracker(struct client *cli)
{
	char *data = &cli->data_buf[PACKET_LEN];
	char *tmp = NULL;
	int len = 0;

	cJSON *root = cJSON_CreateObject();
    char *buf = NULL;
    if(root)
    {
        cJSON_AddStringToObject(root, "track_ip", "192.168.1.11");
        cJSON_AddNumberToObject(root, "track_port", 1337);
    }

	tmp = cJSON_Print(root);
	len = strlen(tmp);

	memcpy(data, tmp, len);
	DEBUG("tmp %s", tmp);
	DEBUG("data %s", data);
	
	set_packet_head(cli->data_buf, SET_TRACKER, len, JSON_TYPE, 0);
	write(server_s, cli->data_buf, len + PACKET_LEN);
}


char wait_input()
{
    char ic;
    char temp;

    fflush(stdin);

    ic = getchar();
    temp = getchar();

    fflush(stdin);

    return ic;
}

void tcp_loop()
{
	char input;
	struct client *cli = &m_client;
	char *data = &cli->data_buf[PACKET_LEN];
	for(;;)
	{
		input = wait_input();
		switch(input)
		{
			case 'm':
				send_make_torrent(cli);
				break;
			case 's':
				send_set_tracker(cli);
				break;
			case 'g':
				send_get_task_state(cli);
				break;
			case 'a':
				send_add_task(cli);
				break;
			case 'd':
				//send_del_task(cli, torrent_id);
				break;
		}
		read(server_s, m_client.data_buf, DATA_SIZE);
		DEBUG("data %s", data);
		switch(read_packet_order(m_client.data_buf))
		{
			case ADD_TASK:
			{	
				cJSON *root = cJSON_Parse((char*)(data));
				cJSON *torrent_id_json = cJSON_GetObjectItem(root, "torrent_id");
				if(torrent_id_json)
				{
					//torrent_id = torrent_id_json->valueint;	
					sscanf(torrent_id_json->valuestring, "%lld", &torrent_id);
					DEBUG("------ %lld -------", torrent_id);
				}
				break;
			}
		}

	}
}

int main(int argc, char *argv[])
{
	int ret;
    server_s = create_tcp();
    if(server_s == -1) 
    {   
        DEBUG("create client fd error");
        return ERROR;   
    }   
	DEBUG("server %s server_port %d",server_ip, server_port);
    ret = connect_server(server_s, server_ip, server_port, -1);
    if(0 != ret)
    {   
        DEBUG("connect server ip: %s port: %d timeout 10s", server_ip, server_port);
        return ERROR;
    }    
    memset(&m_client, 0, sizeof(struct client));
    m_client.fd = server_s;
    m_client.head_buf = malloc(HEAD_LEN + 1); 
    m_client.packet = malloc(sizeof(PACKET_LEN));
       
	m_client.data_buf = malloc(DATA_SIZE);
	m_client.data_size = DATA_SIZE;
      
	tcp_loop();		
    return SUCCESS;
}
