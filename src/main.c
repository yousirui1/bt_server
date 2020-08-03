#include "base.h"
#include "client.h"
#include "cJSON.h"
#include "packet.h"
#include <limits.h>
#include <ctype.h>
#include <sys/un.h>
#include <linux/rtc.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include "torrent.h"

time_t current_time;
const char program_name[] = "bt_server";
int max_connections = 1024;
struct client **clients = NULL;


/* pipe */
int pipe_event[2];
int pipe_tcp[2];
int pipe_udp[2];
int pipe_bt[2];
int pipe_track[2];

pthread_t pthread_server, pthread_torrent, pthread_track;

int track_flag = 0;
int run_flag = 0;

//const ch
char *track_ip = NULL;
int track_port = 50020;
char tracker[128] = {0};
int torrent_port;

#define  SCHED_PRIORITY_SERVER 0

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

int make_torrent(char *file_path, char *torrent_path, char *track);

static void sig_quit_listen(int e)
{
    char s = 'S';

    write(pipe_tcp[1], &s, sizeof(s));
    DEBUG("kill use signal end !");
}

static void do_exit()
{
	void *tret = NULL;
	//pthread_join(pthread_server, &tret);
	pthread_join(pthread_track, &tret);
    DEBUG("pthread_exit %d event", (int *)tret);
}

static int send_code(struct client *cli, int code)
{
    int ret;

    if(cli->data_buf)
        free(cli->data_buf);
    
    cJSON *root = cJSON_CreateObject();
    char *buf = NULL;
    if(root)
    {   
        cJSON_AddNumberToObject(root, "code", code);
        cJSON_AddStringToObject(root, "msg", "Success");
    }
        
    cli->data_buf = cJSON_Print(root);
    cli->data_size = strlen(cli->data_buf);
    
    set_packet_head(cli->packet, read_packet_order(cli->packet), cli->data_size, JSON_TYPE, 1);
        
    ret = send_packet(cli);
    
    if(root)
       cJSON_Delete(root);
    return ret;
}

static int send_task_state(struct client *cli, struct task_info *info)
{
    int ret;

    if(cli->data_buf)
        free(cli->data_buf);
    
    cJSON *root = cJSON_CreateObject();
    char *buf = NULL;
		
	char total_download[128] = {0};
	char total_size[128] = {0};
	sprintf(total_download, "%lld", info->total_download);
	sprintf(total_size, "%lld", info->total_size);
    if(root)
    {   
        cJSON_AddNumberToObject(root, "code", 0);
        cJSON_AddStringToObject(root, "msg", "Success");
        cJSON_AddStringToObject(root, "state", info->state);
        cJSON_AddNumberToObject(root, "progress", info->progress);
       	cJSON_AddStringToObject(root, "total_download", total_download);
        cJSON_AddNumberToObject(root, "download_rate", info->download_rate);
        cJSON_AddStringToObject(root, "total_size",  total_size);
    }
        
    cli->data_buf = cJSON_Print(root);
    cli->data_size = strlen(cli->data_buf);
    
    set_packet_head(cli->packet, read_packet_order(cli->packet), cli->data_size, JSON_TYPE, 1);
        
    ret = send_packet(cli);
    
    if(root)
       cJSON_Delete(root);
    return ret;
}


static int recv_get_task_state(struct client *cli)
{
	int ret;
	if(!run_flag)
	{
    	cJSON *root = cJSON_Parse((char*)(cli->data_buf));    
    	if(!root)
    	{   
			DEBUG("cJSON_Parse error");
			return send_code(cli, -1);
    	}   
    
		cJSON* torrent_id_json = cJSON_GetObjectItem(root, "torrent_id");
		uint32_t torrent_id;

		sscanf(torrent_id_json->valuestring, "%lld", &torrent_id);

		struct task_info info;
		memset(&info, 0, sizeof(struct task_info));
		//get_task_info(torrent_id, &info);
		
		return send_task_state(cli, &info);		
	}
	return send_code(cli, -1);
}

static int recv_make_torrent(struct client *cli)
{
    cJSON *root = cJSON_Parse((char*)(cli->data_buf));    
    if(!root)
    {   
		DEBUG("make_torrent cJSON_Parse error");
		return send_code(cli, -1);
    }   
	 
    cJSON* file_path = cJSON_GetObjectItem(root, "file_path");
    cJSON* torrent_path = cJSON_GetObjectItem(root, "torrent_path");

	if(make_torrent(file_path->valuestring, torrent_path->valuestring , tracker) == SUCCESS)
	{
		DEBUG("make file: %s torrent success ", file_path->valuestring);
		return send_code(cli, SUCCESS);
	}
	else
	{
		DEBUG("make file: %s torrent error ", file_path->valuestring);
		return send_code(cli, -1);
	}	
}

static int recv_del_task(struct client *cli)
{
   	int ret;
    cJSON *root = cJSON_Parse((char*)(cli->data_buf));    
    if(!root)
    {   
		DEBUG("del_task cJSON_Parse error");
		return send_code(cli, -1);
    }   
    
	cJSON* torrent_id = cJSON_GetObjectItem(root, "torrent_id");
		
	//ret = del_task(torrent_id->valueint);
	if(ret == SUCCESS)
		return send_code(cli, SUCCESS);
	else
		return send_code(cli, -1);
}

static int send_add_task(struct client *cli, uint32_t torrent_id)
{
	int ret;

    if(cli->data_buf)
        free(cli->data_buf);
    
    cJSON *root = cJSON_CreateObject();
    char buf[128] = {0};
	sprintf(buf, "%lld", torrent_id);
    if(root)
    {   
        cJSON_AddNumberToObject(root, "code", 0);
        cJSON_AddStringToObject(root, "msg", "Success");
		//cJSON_AddNumberToObject(root, "torrent_id", torrent_id);
		cJSON_AddStringToObject(root, "torrent_id", buf);
    }
        
    cli->data_buf = cJSON_Print(root);
    cli->data_size = strlen(cli->data_buf);
    
    set_packet_head(cli->packet, read_packet_order(cli->packet), cli->data_size, JSON_TYPE, 1);
        
    ret = send_packet(cli);
    
    if(root)
       cJSON_Delete(root);
    return ret;		
}

static int recv_add_task(struct client *cli)
{
	cJSON *root = cJSON_Parse((char*)(cli->data_buf));
	if(!root)
	{
		DEBUG("cJSON_Parse error");
		return send_code(cli, -1);
	}	
	DEBUG("%s", cli->data_buf);

	cJSON* torrent = cJSON_GetObjectItem(root, "torrent");
    cJSON* save_path = cJSON_GetObjectItem(root, "save_path");
	
	if(torrent && save_path)
	{
		uint32_t torrent_id = add_task(torrent->valuestring, save_path->valuestring);
		return send_add_task(cli, torrent_id);
	}
	else
	{
		DEBUG("torrent or save_path is NULL error");
		return send_code(cli, -1);	
	}
}


static int recv_set_tracker(struct client *cli)
{
	cJSON *root = cJSON_Parse((char*)(cli->data_buf));
	if(!root)
	{
		DEBUG("cJSON_Parse error");
		return send_code(cli, -1);
	}	

	cJSON* timeout = cJSON_GetObjectItem(root, "time_out");
    cJSON* ip = cJSON_GetObjectItem(root, "ip");
    cJSON* port = cJSON_GetObjectItem(root, "port");

	if(ip)
	{
		track_ip = strdup(ip->valuestring);
		sprintf(tracker, "%s:%d", track_ip, track_port);
		DEBUG("tracker %s", tracker);
	}

	return send_code(cli, SUCCESS);
}

static int process_msg(struct client *cli)
{
	int ret;
	DEBUG("process_msg %d", read_packet_order(cli->packet));
	switch(read_packet_order(cli->packet))
	{
		case SET_TRACKER:
			ret = recv_set_tracker(cli);
			break;
		case ADD_TASK:
			ret = recv_add_task(cli);
			break;	
		case DEL_TASK:
			ret = recv_del_task(cli);
			break;	
		case MAKE_TORRETN:
			ret = recv_make_torrent(cli);
			break;	
		case GET_TASK_STATE:
			ret = recv_get_task_state(cli);
			break;		
	}
	return ret;
}

void close_client(struct client *cli)
{
	if(cli->fd)
		close(cli->fd);
	if(cli->packet)
		free(cli->packet);
	if(cli->data_buf)
		free(cli->data_buf);
	if(cli)
		free(cli);	
}

static void tcp_loop(int listenfd)
{
    int maxfd = 0, connfd, sockfd;
    int nready, ret, i, maxi = 0;
    int total_connections = 0;
    char buf[DATA_SIZE] = {0};          //pipe msg buf
    char *tmp = &buf[HEAD_LEN];

    struct client *cli = NULL;

    fd_set reset, allset;

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    struct timeval tv; 
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    FD_SET(pipe_tcp[0], &allset);

    maxfd = maxfd > listenfd ? maxfd : listenfd;
    maxfd = maxfd > pipe_tcp[0] ? maxfd : pipe_tcp[0];
    for(;;)
    {   
        tv.tv_sec = 1;
        reset = allset;
        ret = select(maxfd + 1, &reset, NULL, NULL, &tv);
        if(ret == -1) 
        {   
             if(errno == EINTR)
                continue;
            else if(errno != EBADF)
            {   
                FAIL("select %d %s ", errno, strerror(errno));
                return;
            }
        }
		DEBUG("select %d", ret);	
        nready = ret;
        /* pipe msg */
        if(FD_ISSET(pipe_tcp[0], &reset))
        {
            ret = recv(pipe_tcp[0], (void *)buf, sizeof(buf), 0);
            if(ret >= HEAD_LEN)
            {
                //process_server_pipe(buf, ret);
            }
            if(ret == 1)
            {
                if(buf[0] == 'S')
                {
                    DEBUG("event thread send stop msg");
                    break;
                }
           }
            if(--nready <= 0)
                continue;
        }
        /* new connect */
        if(FD_ISSET(listenfd, &reset))
        {
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
            if(connfd < 0)
                continue;
            cli = (struct client *)malloc(sizeof(struct client));
            if(!cli)
            {
                DEBUG("new connect and malloc struct client error :%s", strerror(errno));
                continue;
            }
            memset(cli, 0, sizeof(struct client));
            cli->fd = connfd;
            cli->data_size = PACKET_LEN;
            //cli->head_buf = (unsigned char *)malloc(HEAD_LEN);
            cli->packet = (unsigned char *)malloc(PACKET_LEN);
            if(!cli->packet)
            {
                DEBUG("fcntl connfd: %d  F_GETFL error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli);
                continue;
            }
            ret = fcntl(connfd, F_GETFL, 0);
            if(ret < 0)
            {
                DEBUG("fcntl connfd: %d  F_GETFL error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli->packet);
                free(cli);
                continue;
            }

            if(fcntl(connfd, F_SETFL, ret | O_NONBLOCK) < 0)
            {
                DEBUG("fcntl connfd: %d F_SETFL O_NONBLOCK error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli->packet);
                free(cli);
                continue;
            }
            /* recode client ip */
            if(inet_ntop(AF_INET, &cliaddr.sin_addr, cli->ip, sizeof(cli->ip)) == NULL)
            {
                DEBUG("connfd: %d inet_ntop error ",connfd, strerror(errno));
                close_fd(connfd);
                free(cli->packet);
                free(cli);
                continue;
            }
            FD_SET(connfd, &allset);
            for(i = 0; i < max_connections; i++)
            {
                if(clients[i] == NULL)
                    clients[i] = cli;
                break;
            }
            total_connections++;
            if(i > maxi)
                maxi = i;
            if(connfd > maxfd)
                maxfd = connfd;
            if(--nready <= 0)
                continue;
        }
        /* client msg */
        for(i = 0; i <= maxi; i++)
        {
            if(clients[i] == NULL || (sockfd = clients[i]->fd) < 0)
                continue;
            if(FD_ISSET(sockfd, &reset))
            {
                if(clients[i]->has_read_head == 0)
                {
                    if((ret = recv(sockfd, (void *)clients[i]->packet + clients[i]->pos,
                                        PACKET_LEN - clients[i]->pos, 0)) <= 0)
                    {
                        if(ret < 0)
                        {
                            if(errno == EINTR || errno == EAGAIN)
                                continue;
                        }

                        FD_CLR(clients[i]->fd, &allset);
                        close_client(clients[i]);
                        clients[i] = NULL;
                        total_connections--;
                        continue;
                    }
                    clients[i]->pos += ret;
                    if(clients[i]->pos != PACKET_LEN)
                        continue;
#if 0
                    if(read_msg_syn(clients[i]->head_buf) != DATA_SYN)
                    {
                        DEBUG(" %02X client send SYN flag error close client index: %d ip: %s port %d",
                            read_msg_syn(clients[i]->head_buf), clients[i]->index, clients[i]->ip,
                            clients[i]->port);

                        FD_CLR(clients[i]->fd, &allset);
                        close_client(clients[i]);
                        clients[i] = NULL;
                        total_connections--;
                        continue;
                    }
#endif
					DEBUG("1111111111111111111111111111111");

                    clients[i]->has_read_head = 1;
					clients[i]->data_size = read_packet_size(clients[i]->packet) + read_packet_token(clients[i]->packet);
                    clients[i]->pos = 0;

                    if(clients[i]->data_size >= 0 && clients[i]->data_size < CLIENT_BUF)
                    {
                        if(clients[i]->data_buf)
                            free(clients[i]->data_buf);

                        clients[i]->data_buf = (unsigned char*)malloc(clients[i]->data_size + 1);
                        if(!clients[i]->data_buf)
                        {
                            DEBUG("malloc data buf error: %s close client: ip: %s port %d",
                                    strerror(errno),  clients[i]->ip, clients[i]->port);

                            FD_CLR(clients[i]->fd, &allset);
                            close_client(clients[i]);
                            clients[i] = NULL;
                            total_connections--;
                            continue;
                        }
                        memset(clients[i]->data_buf, 0, clients[i]->data_size);
                    }
                    else 
                    {
                        DEBUG("client send size: %d error close  ip: %s port %d",
                                clients[i]->data_size, clients[i]->ip, clients[i]->port);

                        FD_CLR(clients[i]->fd, &allset);
                        close_client(clients[i]);
                        clients[i] = NULL;
                        total_connections--;
                        continue;
                    }
                }

                if(clients[i]->has_read_head == 1)
                {
                    if(clients[i]->pos < clients[i]->data_size)
                    {
                        if((ret = recv(sockfd,clients[i]->data_buf+clients[i]->pos,
                                        clients[i]->data_size - clients[i]->pos,0)) <= 0)
                        {
                            if(ret < 0)
                            {
                                if(errno == EINTR || errno == EAGAIN)
                                    continue;
                                //DEBUG("client ip: %s port %d",
                                 //        clients[i]->ip, clients[i]->port);

                                FD_CLR(clients[i]->fd, &allset);
                                close_client(clients[i]);
                                clients[i] = NULL;
                                total_connections--;
                                continue;
                            }
                        }
                        clients[i]->pos += ret;
                    }

                    if(clients[i]->pos == clients[i]->data_size)
                    {
                        if(process_msg(clients[i]))
                        {
                            //DEBUG("process msg error : ip: %s port %d",
                             //        clients[i]->ip, clients[i]->port);

                            FD_CLR(clients[i]->fd, &allset);
                            close_client(clients[i]);
                            clients[i] = NULL;
                            total_connections--;
                            continue;
                        }
                        memset(clients[i]->packet, 0, PACKET_LEN);
                        clients[i]->data_size = PACKET_LEN;
                        clients[i]->pos = 0;
                        if(clients[i]->data_buf)
                            free(clients[i]->data_buf);
                        clients[i]->data_buf = NULL;
                        clients[i]->has_read_head = 0;
                    }
                    if(clients[i]->pos > clients[i]->data_size)
                    {
                        DEBUG("loss msg data : ip: %s port %d",
                                clients[i]->ip, clients[i]->port);
                        FD_CLR(clients[i]->fd, &allset);
                        close_client(clients[i]);
                        clients[i] = NULL;
                        total_connections--;
                        continue;
                    }
                }
                if(--nready <= 0)
                    break;
            }
        }
    }
    DEBUG("thread server tcp end");
}

#define SCHED_PRIORITY_SERVER 0



void init_config()
{


}

int init_pipe()
{
    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_track) < 0)
    {
        DEBUG("create client bt err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_track[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_track[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_bt) < 0)
    {
        DEBUG("create client bt err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_bt[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_bt[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_tcp) < 0)
    {
        DEBUG("create server pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_tcp[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_tcp[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_udp) < 0)
    {
        DEBUG("create client pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_udp[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_udp[1], F_SETFL, O_NONBLOCK);

    /* create pipe to give main thread infomation */
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipe_event) < 0)
    {
        DEBUG("create client pipe err %s", strerror(errno));
        return ERROR;
    }

    fcntl(pipe_event[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_event[1], F_SETFL, O_NONBLOCK);
    return SUCCESS;
}
void *thread_torrent(void *param)
{
    int ret = SUCCESS;
    pthread_attr_t st_attr;
    struct sched_param sched;
	
	int port = *(int *)param;
    
    ret = pthread_attr_init(&st_attr);
    if(ret)
    {    
        DEBUG("ThreadUdp attr init warning ");
    }    
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {    
        DEBUG("ThreadUdp set SCHED_FIFO warning");
    }    
    sched.sched_priority = SCHED_PRIORITY_SERVER;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

	bt_client(port);
    return (void *)ret;
}

void *thread_track(void *param)
{
    int ret = SUCCESS;
    pthread_attr_t st_attr;
    struct sched_param sched;
	
	int port = *(int *)param;
    
    ret = pthread_attr_init(&st_attr);
    if(ret)
    {    
        DEBUG("ThreadUdp attr init warning ");
    }    
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {    
        DEBUG("ThreadUdp set SCHED_FIFO warning");
    }    
    sched.sched_priority = SCHED_PRIORITY_SERVER;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);     //线程可以被取消掉
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);//立即退出

   	start_track_server(port);
    return (void *)ret;
}

void *thread_server(void *param)
{
    int ret = SUCCESS;
    pthread_attr_t st_attr;
    struct sched_param sched;
	
	int listenfd = *(int *)param;
    
    ret = pthread_attr_init(&st_attr);
    if(ret)
    {    
        DEBUG("ThreadUdp attr init warning ");
    }    
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {    
        DEBUG("ThreadUdp set SCHED_FIFO warning");
    }    
    sched.sched_priority = SCHED_PRIORITY_SERVER;
    ret = pthread_attr_setschedparam(&st_attr, &sched);

    tcp_loop(listenfd);
    return (void *)ret;
}

int main(int argc, char *argv[])
{
	int ret;
	int server_s;
	srandom(time(NULL) + getpid());

	(void)time(&current_time);
	signal(SIGPIPE, SIG_IGN);
	//signal(SIGINT, SIG_IGN);	

	struct sigaction act;
    act.sa_handler = sig_quit_listen;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, 0); 
	
	init_logs();
    init_config();
    init_pipe();

    server_s = create_tcp();
    if(bind_socket(server_s, 50022) == -1) 
    {   
        FAIL("bind port %d error", 20001);
        return ERROR;
    }    

    clients = (struct client **)malloc(sizeof(struct client *) * max_connections);
    if(!clients)
    {
        FAIL("clients malloc error %s", strerror(errno));
        return ERROR;
    }
    memset(clients, 0, sizeof(struct client *) * max_connections);

	ret = pthread_create(&pthread_server, NULL, thread_server, (void *)&server_s);
	if(SUCCESS != ret)
	{
		DIE("create server thread ret %d error %s", ret, strerror(ret));
	}

	ret = pthread_create(&pthread_torrent, NULL, thread_torrent, (void *)&torrent_port);
	if(SUCCESS != ret)
	{
		DIE("create server thread ret %d error %s", ret, strerror(ret));
	}

	ret = pthread_create(&pthread_track, NULL, thread_track, (void *)&track_port);
	if(SUCCESS != ret)
	{
		DIE("create server thread ret %d error %s", ret, strerror(ret));
	}

	do_exit();
	return ret;
}
