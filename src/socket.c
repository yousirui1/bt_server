#include "base.h"
#include "packet.h"
#include "client.h"

extern int max_connections;

#define PACKET_LEN_OFFSET 12
#define PACKET_ORDER_OFFSET 4
#define PACKET_TOKEN_OFFSET 20
unsigned short read_packet_order(unsigned char *buf)
{
    return *(uint32_t *)&buf[PACKET_ORDER_OFFSET];
}

uint32_t read_packet_size(unsigned char *buf)
{
    return *(uint32_t *)&buf[PACKET_LEN_OFFSET];
}

unsigned short read_packet_token(unsigned char *buf)
{
    return *(uint16_t *)&buf[PACKET_TOKEN_OFFSET];
}


/* 发送数据 */
int send_msg(const int fd, const char *buf, const int len)
{
    const char *tmp = buf;
    int cnt = 0;
    int send_cnt = 0;
    while (send_cnt != len)
    {
        cnt = send(fd, tmp + send_cnt, len - send_cnt, 0);
        if (cnt < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return ERROR;
        }
        send_cnt += cnt;
    }
    return SUCCESS;
}



/* buf packet 
 * cmd  1000
 * data_type ox00:二进制数据 0x01:json 0x02:protobuf
 */

void set_packet_head(char *buf, int cmd, int data_size, char data_type, int req_flag)
{
    yzy_packet *packet = (yzy_packet *)buf;
    packet->version_chief = 1;
    packet->version_sub = 0;
    packet->service_code = cmd;
    packet->request_code = 0;
    packet->dataSize = data_size;
    packet->dataType = 0x01;
    packet->encoding = 0x00;
    packet->clientType = 0x02;
    if(req_flag)
        packet->reqOrRes = 0x02;
    else
        packet->reqOrRes = 0x01;
    packet->supplementary = 0x00;
}

int send_packet(struct client *cli)
{
    int ret;
    if (!cli || !cli->fd)
        return ERROR;

    ret = send_msg(cli->fd, cli->packet, PACKET_LEN);

    if(cli->token && cli->token_size < DATA_SIZE && cli->token_size > 0)
        ret = send_msg(cli->fd, cli->token, cli->token_size);

    if(cli->data_buf && cli->data_size < DATA_SIZE && cli->data_size > 0)
        ret = send_msg(cli->fd, cli->data_buf, cli->data_size);
    
    return ret;
}


/* 关闭socket */
void close_fd(int fd)
{
    if (fd)
    {
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
    }
}



int create_tcp()
{   
    int fd, sock_opt;
    int keepAlive = 1;    //heart echo open
    int keepIdle = 15;    //if no data come in or out in 15 seconds,send tcp probe, not send ping
    int keepInterval = 3; //3seconds inteval
    int keepCount = 5;    //retry count
    
    fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (fd == -1) 
    {   
        DEBUG("unable to create socket");
        goto run_out;
    }   
    
    sock_opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) != 0)
        goto run_out;
#ifdef _WIN32

#else
    if (fcntl(fd, F_SETFD, 1) == -1) 
    {   
        DEBUG("can't set close-on-exec on server socket!");
        goto run_out;
    }   
    if ((sock_opt = fcntl(fd, F_GETFL, 0)) < -1) 
    {   
        DEBUG("can't set close-on-exec on server socket!");
        goto run_out;
    }   
    if (fcntl(fd, F_SETFL, sock_opt | O_NONBLOCK) == -1) 
    {   
        DEBUG("fcntl: unable to set server socket to nonblocking");
        goto run_out;
    }   
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) != 0)
        goto run_out;
    if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle)) != 0)
        goto run_out;
    if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) != 0)
        goto run_out;
#endif
    return fd; 
run_out:
    close_fd(fd);
    return -1; 
}


/* 连接服务器 */
int connect_server(int fd, const char *ip, int port, int count)
{
    struct sockaddr_in client_sockaddr;
    memset(&client_sockaddr, 0, sizeof client_sockaddr);

    client_sockaddr.sin_family = AF_INET;
    client_sockaddr.sin_addr.s_addr = inet_addr(ip);
    client_sockaddr.sin_port = htons(port);

    while (connect(fd, (struct sockaddr *)&client_sockaddr, sizeof(client_sockaddr)) != 0)
    {
        DEBUG("connection failed reconnection after 1 seconds");
        sleep(1);
        if (!count--)
        {
            return ERROR;
        }
    }
    return SUCCESS;
}


int bind_socket(int fd, int port)
{
    DEBUG("tcp bind port %d", port);

    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof server_sockaddr);

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(port);

    /* internet family-specific code encapsulated in bind_server()  */
    if (bind(fd, (struct sockaddr *)&server_sockaddr,
             sizeof(server_sockaddr)) == -1) 
    {   
        DEBUG("unable to bind");
        goto run_out;
    }   

    /* listen: large number just in case your kernel is nicely tweaked */
    if (listen(fd, max_connections) == -1) 
    {   
        DEBUG("listen max_connect: %d error", max_connections);
        goto run_out;
    }   

    return SUCCESS;
run_out:
    close_fd(fd);
    return ERROR;
}

