#include "base.h"

int server_s;

pthread_t pthread_tcp, pthread_bt, pthread_track;


static void do_exit()
{
    pthread_join(pthread_tcp, &tret);
    DEBUG("pthread_exit tcp ret: %d", (int *)tret);	
}



static void process_msg(struct client *cli)
{
	switch()
	{
			
	}
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
                if(buf[0] == 'C')       //close client all
                {
                    for(i = 0 ; i <= maxi; i++)
                    {
#if 0
                        if(clients[i] == NULL || clients[i]->fd < 0)
                            continue;

                        FD_CLR(clients[i]->fd, &allset);
                        close_client(clients[i]);
                        clients[i] = NULL;
                        total_connections--;
#endif
                    }
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
            cli->data_size = HEAD_LEN;
            cli->head_buf = (unsigned char *)malloc(HEAD_LEN);
            if(!cli->head_buf)
            {
                DEBUG("fcntl connfd: %d  F_GETFL error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli);
                continue;
            }
#ifdef _WIN32
            memcpy(cli->ip,inet_ntoa(cliaddr.sin_addr), sizeof(cli->ip));
#else
            ret = fcntl(connfd, F_GETFL, 0);
            if(ret < 0)
            {
                DEBUG("fcntl connfd: %d  F_GETFL error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli->head_buf);
                free(cli);
                continue;
            }

            if(fcntl(connfd, F_SETFL, ret | O_NONBLOCK) < 0)
            {
                DEBUG("fcntl connfd: %d F_SETFL O_NONBLOCK error :%s", connfd, strerror(errno));
                close_fd(connfd);
                free(cli->head_buf);
                free(cli);
                continue;
            }
            /* recode client ip */
            if(inet_ntop(AF_INET, &cliaddr.sin_addr, cli->ip, sizeof(cli->ip)) == NULL)
            {
                DEBUG("connfd: %d inet_ntop error ",connfd, strerror(errno));
                close_fd(connfd);
                free(cli->head_buf);
                free(cli);
                continue;
            }
#endif
            FD_SET(connfd, &allset);
            for(i = 0; i < max_connections; i++)
            {
                //if(clients[i] == NULL || (sockfd = clients[i]->fd) < 0)
                if(clients[i] == NULL)
                    clients[i] = cli;
                break;
            }
            total_connections++;
            if(i > maxi)
                maxi = i;
            if(connfd > maxfd)
                maxfd = connfd;

            DEBUG("client index: %d total_connections: %d maxi: %d connfd ip: %s",i, total_connections, maxi, cli->ip);
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
                    if((ret = recv(sockfd, (void *)clients[i]->head_buf + clients[i]->pos,
                                        HEAD_LEN-clients[i]->pos, 0)) <= 0)
                    {
                        if(ret < 0)
                        {
                            if(errno == EINTR || errno == EAGAIN)
                                continue;
                        }
                        DEBUG("client close index: %d ip: %s port %d", clients[i]->index,
                                    clients[i]->ip, clients[i]->port);

                        FD_CLR(clients[i]->fd, &allset);
                        close_client(clients[i]);
                        clients[i] = NULL;
                        total_connections--;
                        continue;
                    }
                    clients[i]->pos += ret;
                    if(clients[i]->pos != HEAD_LEN)
                        continue;

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

                    clients[i]->has_read_head = 1;
                    clients[i]->data_size = read_msg_size(clients[i]->head_buf);
                    clients[i]->pos = 0;


                    if(clients[i]->data_size >= 0 && clients[i]->data_size < CLIENT_BUF)
                    {
                        if(clients[i]->data_buf)
                            free(clients[i]->data_buf);

                        clients[i]->data_buf = (unsigned char*)malloc(clients[i]->data_size + 1);
                        if(!clients[i]->data_buf)
                        {
                            DEBUG("malloc data buf error: %s close client index: %d ip: %s port %d",
                                    strerror(errno), clients[i]->index, clients[i]->ip, clients[i]->port);

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
                        DEBUG("client send size: %d error close client index: %d ip: %s port %d",
                                clients[i]->data_size, clients[i]->index, clients[i]->ip, clients[i]->port);

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
                                DEBUG("client close index: %d ip: %s port %d",
                                        clients[i]->index, clients[i]->ip, clients[i]->port);

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
                            DEBUG("process msg error client index: %d ip: %s port %d",
                                    clients[i]->index, clients[i]->ip, clients[i]->port);

                            FD_CLR(clients[i]->fd, &allset);
                            close_client(clients[i]);
                            clients[i] = NULL;
                            total_connections--;
                            continue;
                        }
                        memset(clients[i]->head_buf, 0, HEAD_LEN);
                        clients[i]->data_size = HEAD_LEN;
                        clients[i]->pos = 0;
                        if(clients[i]->data_buf)
                            free(clients[i]->data_buf);
                        clients[i]->data_buf = NULL;
                        clients[i]->has_read_head = 0;
                    }
                    if(clients[i]->pos > clients[i]->data_size)
                    {
                        DEBUG("loss msg data client index: %d ip: %s port %d", clients[i]->index,
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


void *thread_tcp(void *param)
{
    int ret, listenfd = -1;
    pthread_attr_t st_attr;
    struct sched_param sched;

    listenfd = *(int *)param;

    ret = pthread_attr_init(&st_attr);
    if(ret)
    {
        DEBUG("server thread tcp attr init fail warning");
    }
    ret = pthread_attr_setschedpolicy(&st_attr, SCHED_FIFO);
    if(ret)
    {
        DEBUG("server thread tcp set SCHED_FIFO fail warning");
    }
    sched.sched_priority = SCHED_PRIORITY_TCP;
    pthread_attr_setschedparam(&st_attr, &sched);

    tcp_loop(listenfd);
    return (void *)ret;
}

int init_server()
{
	clients = (struct client **)malloc(sizeof(struct client *) * max_connections);
	if(!clients)
	{
        FAIL("clients malloc error %s", strerror(errno));
        return ERROR;
	}
	
   	memset(clients, 0, sizeof(struct client *) * max_connections);
		
	server_s = create_tcp();
    if(bind_socket(server_s, client_port) == -1) 
    {
        FAIL("bind port %d error", client_port);
        return ERROR;
    } 

    ret = pthread_create(&pthread_tcp, NULL, thread_tcp, &server_s);
    if(0 != ret)
    {
        FAIL("create server tcp thread ret: %d error: %s", ret, strerror(ret));
        return ERROR;
    }
	
	do_exit();
}
