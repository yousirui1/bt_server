#ifndef __BT_SERVER_H__
#define __BT_SERVER_H__


#ifdef __cplusplus
extern "C" 
{
#endif

int make_torrent(char *file_path, char *torrent_path, char *track);
//int make_torrent(int argc, char const* argv[], char *track);
int add_torrent(const char *torrent, const char *save_path, int down_flag);
void bt_loop();

#ifdef __cplusplus
}

#endif
#endif



