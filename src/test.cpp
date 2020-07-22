#include <iostream>
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_info.hpp"
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include <chrono>
 
using namespace std;
using namespace chrono;

//int64_t downloadsize = 0;

std::vector<lt::torrent_handle> taks_torrent;
 

bool is_absolute_path(std::string const &f)
{
	if(f.empty())
		return false;
	if(f[0] == '/')
		return true;
	return false;
}


std::string path_append(std::string const & lhs, std::string const &rhs)
{
	if(lhs.empty() || lhs == ".")
		return rhs;
	if(rhs.empty() || rhs == ".") 
		return lhs;
	
#define TORRENT_SEPARATOR "/"
	bool need_sep = lhs[lhs.size() - 1] != '/';

	return 	lhs + (need_sep?TORRENT_SEPARATOR:"") + rhs;
}


std::string make_absolute_path(std::string const &p)
{
	if(is_absolute_path(p))
		return p;
	std::string ret;
	char *cwd = ::getcwd(nullptr, 0);
	ret = path_append(cwd, p);
	std::free(cwd);
	return ret;
}

bool is_resume_file(std::string const & s)
{
	static std::string const hex_digit = "0123456789abcdef";
	if(s.size() != 40 + 7)
		return false;
	if(s.substr(40) != ".resume")
		return false;
	//if(char const c : s.substr(0, 40))
	for (char const c : s.substr(0, 40))
	{
		if(hex_digit.find(c) == std::string::npos)
			return false;
	}
	return true;
}

int start_bt(const char *save_path, const char *torrent_path, const int port, uint64_t physic_offset)
{
	


}



char const* state(lt::torrent_status::state_t s)
{
    switch (s) {
    case lt::torrent_status::checking_files: return "checking";
    case lt::torrent_status::downloading_metadata: return "dl metadata";
    case lt::torrent_status::downloading: return "downloading";
    case lt::torrent_status::finished: return "finished";
    case lt::torrent_status::seeding: return "seeding";
    case lt::torrent_status::allocating: return "allocating";
    case lt::torrent_status::checking_resume_data: return "checking resume";
    default: return "<>";
    }   
}

time_t current_time, last_time;
// return the name of a torrent status enum
bool handle_alter(lt::session& ses, lt::alert* a, lt::torrent_handle &th)
{
    using namespace lt; 
 
    if (add_torrent_alert *p = alert_cast<add_torrent_alert>(a))
    {   
        if (p->error)
        {
            std::fprintf(stderr, "failed to add torrent:%s %s\n"
                ,p->params.ti?p->params.ti->name().c_str():p->params.name.c_str()
            ,p->error.message().c_str());
        }
    }
    else if (torrent_finished_alert *p = alert_cast<torrent_finished_alert>(a))
    {
        cout << "torrent finished" << endl;
    }
    else if (state_update_alert *p = alert_cast<state_update_alert>(a))
    {
        char cinfo[1024] = { 0x00 };
        std::vector<std::int64_t> file_progress;
        th.file_progress(file_progress); // 获取文件下载进度
        int const idx = static_cast<int>(0);
        if(!p->status.empty())
        {
            lt::torrent_status s = th.status(lt::torrent_handle::query_save_path);

            if(current_time - last_time > 3)
            {
            	cout << state(s.state) << " download rate " << s.download_payload_rate / 1000 << "KB /s, total_download " << s.total_done / 1000 << "KB, uprate " << s.upload_rate / 1000 << "KB /s, total_up " << s.total_upload / 1000
                << "KB, progress " << s.progress << " progress_ppm " << s.progress_ppm << " progress " << s.progress_ppm / 10000 << "  " << file_progress[0] * 100 / downloadsize << endl;
            	cout << cinfo << endl;
            	std::cout.flush();
            	last_time = current_time;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[])
{


}

int start_bt(char *torrent, char *save_path, uint64_t physic_offset)
//int main(int argc, char*argv[])
{
    int nTimedOut = 200000; //设置下载超时时间
    //std::string save_path("/dev/vdc/");//保存文件路径
    int torrent_upload_limit = 100000 * 1000; //上传速度限制
    int torrent_download_limit = 100000*1000; //下载速度限制 单位是字节

    //BT下载客户端代码
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask
        , lt::alert::error_notification
        | lt::alert::storage_notification
        | lt::alert::status_notification);
    pack.set_str(lt::settings_pack::user_agent, "ltclient/""test");
    lt::session ses(pack);

    //设置下载参数
    lt::add_torrent_params params;
    params.save_path = save_path;
    //params.save_path = argv[2];
    params.ti = std::make_shared<lt::torrent_info>(torrent);
    //params.ti = std::make_shared<lt::torrent_info>(argv[1]);
    params.download_limit = torrent_download_limit;
    params.upload_limit = torrent_upload_limit;

    auto start = system_clock::now();
    lt::torrent_handle th = ses.add_torrent(params);
    static auto end = system_clock::now();
    static auto duration = duration_cast<microseconds>(end - start);;

    //params.ti->set_physicaldrive_offset(2099367 * 512);   
    params.ti->set_physicaldrive_offset(physic_offset);
    (void)time(&current_time);
    last_time = current_time;

    while (true)
    {
        downloadsize = params.ti->files().file_size(0); //下载文件的总大小
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        (void)time(&current_time);
        for (auto a : alerts)
        {
        	::handle_alter(ses, a, th);
        }

        std::vector<std::int64_t> file_progress;
        th.file_progress(file_progress); // 获取文件下载进度
        int const idx = static_cast<int>(0);
        bool const complete = file_progress[idx] == downloadsize; //判断文件是否下载完成
        if (complete)
        {   
            ses.post_torrent_updates();
            cout << "\ndownload is complete" << endl;
            break;
        }
        end = system_clock::now();
        duration = duration_cast<microseconds>(end - start);
#if 0
        if ((double(duration.count())*microseconds::period::num / microseconds::period::den) > 100 && file_progress[idx] == 0)
        {
            cout << "download failed,check" << endl;
            //break;
        }
#endif
        if ((double(duration.count())*microseconds::period::num / microseconds::period::den) > nTimedOut)//判断是否超时
        {
            cout << "download timed out" << endl;
            break;
        }
        ses.post_torrent_updates();
    }   
    std::string title = params.ti->files().file_name(0).to_string();
        
        cout << "download "<<title<<" cost " << double(duration.count())*microseconds::period::num / microseconds::period::den<<"s"<<endl;  
        
        
    while (1)
    {   
        char c = getchar();
        if (c == 'p')
            break;
    }   
    return 0;
}    
