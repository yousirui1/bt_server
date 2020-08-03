#include <chrono>
#include <iostream>
#include <libtorrent/entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include "torrent.h"


using namespace std;
using namespace chrono;
using namespace lt;

time_t last_time;
extern time_t current_time;
extern int pipe_bt[2];

shared_ptr<session> m_ses;
//std::vector<lt::torrent_handle> task_torrent;

char const *state(torrent_status::state_t s)
{
	switch(s)
	{
		case torrent_status::checking_files: return "checking";
		case torrent_status::downloading_metadata: return "dl metadata";
		case torrent_status::downloading: return "finished";
		case torrent_status::finished: return "seeding";
		case torrent_status::seeding: return "allocating";
		case torrent_status::checking_resume_data: return "checking resume";
		default: return "<>";
	}
}

int get_task_info(uint32_t id, struct task_info *info)
{

}

uint32_t add_task(const char *torrent, const char *save_path)
{
	lt::error_code ec;
	add_torrent_params params;
	params.save_path = save_path;
	params.ti = make_shared<torrent_info>(torrent, ec);
	params.download_limit = 0;
	params.upload_limit = 0;
	
	torrent_handle th = m_ses->add_torrent(std::move(params));
	//task_torrent.push_back(th);
	return th.id();	
}

int make_torrent(char *file_path, char *torrent_path, char *track)
{

}

int bt_client(int port)
{
	settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask
        , lt::alert::error_notification
        | lt::alert::storage_notification
        | lt::alert::status_notification);
    pack.set_str(lt::settings_pack::user_agent, "ltclient/""test");

	m_ses = make_shared<session>(pack);

	char buf[1024] = {0};
	int ret, maxfd = -1, nready;
	struct timeval tv;

	fd_set reset, allset;
	FD_ZERO(&allset);
	FD_SET(pipe_bt[1], &allset);

	maxfd = maxfd > pipe_bt[1] ? maxfd : pipe_bt[1];

	last_time = current_time;
	(void)time(&current_time);

	lt::torrent_handle h;
	
	for(;;)
	{
		reset = allset;
		tv.tv_sec = 0;
		tv.tv_usec = 200000;	
		ret = select(maxfd + 1, &reset, NULL, NULL, &tv);
			
        if(ret == -1) 
        {   
            if(errno == EINTR)
                continue;
            else if(errno != EBADF)
            {   
                //cout("select %s error", strerror(ret));
                continue;
            }   
        }  
		(void)time(&current_time);
        /* pipe msg */
        if(FD_ISSET(pipe_bt[1], &reset))
        {  
            ret = read(pipe_bt[1], (void *)buf, sizeof(buf));
            if(ret == 1 && buf[0] == 'S')
            {
				//cout("pipe end bt client thread");
				break;	
            }
            if(--nready <= 0)
                continue;
        } 

		std::vector<lt::alert *> alerts;
		m_ses->pop_alerts(&alerts);
		
		for(lt::alert const *a : alerts)
		{
			if(auto st = lt::alert_cast<lt::state_update_alert>(a))
			{
				if(st->status.empty())
					continue;
			    lt::torrent_status const& s = st->status[0];
                std::cout << "\r" << state(s.state) << " " 
                    << (s.download_payload_rate / 1000) << " kB/s "
                    << (s.total_done / 1000) << " kB ("
                    << (s.progress_ppm / 10000) << "%) downloaded\x1b[K";
                std::cout.flush();
			}
		}
		m_ses->post_torrent_updates();
	}
	return 0;
}


