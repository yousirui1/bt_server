#ifndef __TORRENT_H__
#define __TORRENT_H__

#ifdef __cplusplus
extern "C" 
{
#endif

struct task_info
{
	uint32_t task_id;
	char state[12];			// 1 checking 2 downing 3 finished 4 seeding
	unsigned int progress;
	unsigned long long total_download;
	unsigned long long download_rate;
	unsigned long long total_size;	
};

int bt_client(int port);
uint32_t add_task(const char *torrent, const char *save_path);
int get_task_info(uint32_t id, struct task_info *info);
int make_torrent(char *file_path, char *torrent_path, char *track);

#ifdef __cplusplus
}
#endif





#endif
