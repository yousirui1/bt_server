
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/create_torrent.hpp"
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include "libtorrent/session.hpp"
#include <chrono>

#include <functional>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>

//#include "bt.h"
#include "torrent.h"

using namespace std;
using namespace std::placeholders;
using namespace chrono;


std::vector<lt::torrent_handle> taks_torrent;
std::shared_ptr<lt::session> ses;

time_t current_time, last_time;

char const *esc(char const *code)
{   
    // this is a silly optimization
    // to avoid copying of strings
    enum { num_strings = 200 };
    static char buf[num_strings][20];
    static int round_robin = 0;
    char* ret = buf[round_robin];
    ++round_robin;
    if (round_robin >= num_strings) round_robin = 0;
    ret[0] = '\033';
    ret[1] = '[';
    int i = 2;
    int j = 0;
    while (code[j]) ret[i++] = code[j++];
    ret[i++] = 'm';
    ret[i++] = 0;
    return ret;
}

char const *timestamp()
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


void print_alert(lt::alert const *a, std::string &str)
{   
    using namespace lt;
    
    if (a->category() & alert_category::error)
    {    
        str += esc("31");
    }    
    else if (a->category() & (alert_category::peer | alert_category::storage))
    {    
        str += esc("33");
    }    
    str += "["; 
    str += timestamp();
    str += "] ";
    str += a->message();
    str += esc("0");
   
   // if (g_log_file)
    //    std::fprintf(g_log_file, "[%s] %s\n", timestamp(),  a->message().c_str());
    printf("[%s] %s\n", timestamp(),  a->message().c_str());
}

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
    
    return  lhs + (need_sep?TORRENT_SEPARATOR:"") + rhs;
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

uint32_t add_task(const char *torrent, const char *save_path)
{
    /* 设置下载参数 */
	lt::error_code ec;
    lt::add_torrent_params params;
    params.save_path = save_path;
    params.ti = std::make_shared<lt::torrent_info>(torrent, ec);
    params.download_limit = 0;
    params.upload_limit = 0;

#if 0
    /* 先读取本地记录 */
    if(load_file())
    {

    }
#endif
    //lt::torrent_handle 
    //if(physic_offset)
        ;//params.ti->set_physicaldrive_offset(physic_offset);
	lt::torrent_handle th = ses->add_torrent(params);
    taks_torrent.push_back(th);
	std::cout<<"handle" <<th.id()<<endl;
	//printf("handle id %d", th.id());
	return th.id();
}

bool handle_alert(lt::alert *a, lt::torrent_handle &th)
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
#if 0
        char cinfo[1024] = { 0x00 };
        std::vector<std::int64_t> file_progress;
        th.file_progress(file_progress); // 获取文件下载进度
        int const idx = static_cast<int>(0);
        if(!p->status.empty())
        {
            lt::torrent_status s = th.status(lt::torrent_handle::query_save_path);
#if 0
			//cout<<"torrent_id "<<th.id();
            if(current_time - last_time > 1)
            {
#if 1
                cout << state(s.state) << " download rate " << s.download_payload_rate / 1000 << "KB /s, total_download " << s.total_done / 1000 << "KB, uprate " << s.upload_rate / 1000 << "KB /s, total_up " << s.total_upload / 1000
                << "KB, progress " << s.progress << " progress_ppm " << s.progress_ppm << " progress " << s.progress_ppm / 10000 << "  " << endl;
                cout << cinfo << endl;
#endif
                std::cout.flush();
                last_time = current_time;
            }
#endif
        }
#endif
    }
    return true;
}

void pop_alerts(lt::torrent_handle &torrent)
{
    std::vector<lt::alert*> alerts;
    ses->pop_alerts(&alerts);
    for(auto a : alerts)
    {
        if(::handle_alert(a, torrent))
            continue;

        /* if we didn't handle the  alert, print it to the log */
        std::string event_string;
        print_alert(a, event_string);
    }
}

int get_task_info(uint32_t id, struct task_info *info)
{
	for(auto torrent : taks_torrent)
	{
		if(id != torrent.id())
		{
			cout<<"torrent.id"<<torrent.id()<<endl;
			cout<<id<<id<<endl;
			continue;
		}
		lt::torrent_status s = torrent.status(lt::torrent_handle::query_save_path);
		cout << state(s.state) << " download rate " << s.download_payload_rate / 1000 << "KB /s, total_download " << 
		s.total_done / 1000 << "KB, uprate " << s.upload_rate / 1000 << "KB /s, total_up " << s.total_upload / 1000
        << "KB, progress " << s.progress << " progress_ppm " << s.progress_ppm << " progress " << s.progress_ppm / 10000 <<endl;
		std::cout.flush();	

		strcpy(info->state, state(s.state));	
		info->progress = s.progress_ppm / 10000;
		info->total_download = s.total_upload / 1024/ 1024;
		info->download_rate = s.download_payload_rate / 1024 / 1024;
		info->total_size = s.total_wanted / 1024 / 1024;
	}
}

//int main(int argc, char *argv[])
int start_bt(const int port)
{
    /* BT 下载客户端代码 */
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask
        , lt::alert::error_notification
        | lt::alert::storage_notification
        | lt::alert::status_notification);
    pack.set_str(lt::settings_pack::user_agent, "yzy/""bt");

    ses = std::make_shared<lt::session>(pack);

	(void)time(&current_time);
	last_time = current_time;
	//add_task("win10.torrent", "./");	
    for(;;)
    {
        if(taks_torrent.size() == 0)
        {
            usleep(500000);
        }
		(void)time(&current_time);
        for(auto torrent : taks_torrent)
        {
            pop_alerts(torrent);
        }
        ses->post_torrent_updates();
    }
}




#define SUCCESS 0
#define ERROR 1

std::vector<char> load_file(std::string const &filename)
{
	std::fstream in;
	in.exceptions(std::ifstream::failbit);
	in.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
	in.seekg(0, std::ios_base::end);
	size_t const size = size_t(in.tellg());
	in.seekg(0, std::ios_base::beg);
	std::vector<char> ret(size);
	in.read(ret.data(), ret.size());
	return ret;
}

std::string branch_path(std::string const &f)
{
	if(f.empty())
		return f;
#ifdef TORRENT_WINDOWS
	if(f == "\\\\") return "";
#endif
	if(f == "/") return "";
	
	int len = int(f.size());
	/* if the last character is /o r \ ignore it */
	if(f[len - 1] == '/' || f[len-1] == '\\') 
		--len;
	while(len > 0)
	{
		--len;
		if(f[len] == '/' || f[len] == '\\')
			break;
	}
	if(f[len] == '/' || f[len] == '\\') ++len;
	return std::string(f.c_str(), len);
}

bool file_filter(std::string const &f)
{
	if(f.empty())
		return false;
	
	char const *first = f.c_str();
	char const *sep = strrchr(first, '/');
#if defined(TORRENT_WINDOWS) || defined(TORRENT_OS2)
    char const* altsep = strrchr(first, '\\');
    if (sep == nullptr || altsep > sep) sep = altsep;
#endif

	/* if there is no parent path, just set 'sep'
	 * to point to the filename, if there is a 
	 * parent path, skip the '/' character */
	if(sep == nullptr) sep = first;
	else ++sep;

	/* return false if the first character of the filename is a */
	if(sep[0] == '.')
		return false;
	std::cerr<<f<<"\n";
	return true;
}

int make_torrent(char *file_path, char *torrent_path, char *track)
{
	std::string creator_str = "libtorrent";
	std::string comment_str;
		
	std::vector<std::string> web_seeds;
	std::vector<std::string> trackers;
	std::vector<std::string> collections;
	std::vector<lt::sha1_hash> similar;
	int pad_file_limit = -1;
	int piece_size = 0;
	lt::create_flags_t flags = {};
	std::string root_cert;
	std::string outfile = torrent_path;
	std::string merklefile;
	std::string track_ip = track;

	std::cerr<<torrent_path;
	
#ifdef TORRENT_WINDOWS
	//outfile = "a.torrent";
#endif

	std::string full_path = file_path;
	lt::file_storage fs;
	lt::add_files(fs, full_path, file_filter, flags);
	if(fs.num_files() == 0)
	{
		//DEBUG("file size error");
		return ERROR;
	}		
	
	lt::create_torrent t(fs, piece_size, pad_file_limit, flags);
	
	std::string http_track ="http://"+ track_ip +"/announce";
    std::string udp_track ="udp://"+ track_ip + "/announce";
	
	trackers.push_back(http_track);

	int tier = 0;	
	for(std::string const& tr:trackers)
	{
		if(tr == "-") ++ tier;
		else t.add_tracker(tr, tier);
	}
	for(std::string const & ws : web_seeds)
		t.add_url_seed(ws);

	for(std::string const & c : collections)
		t.add_collection(c);
	

  	for (lt::sha1_hash const& s : similar)
        t.add_similar_torrent(s);

	auto const num = t.num_pieces();
	lt::set_piece_hashes(t, branch_path(full_path), [num]
		(lt::piece_index_t const p) {
		std::cerr<<"\r"<<p<<"/"<<num;
	});
       
    t.set_creator(creator_str.c_str());
    if(!comment_str.empty())
    {   
        t.set_comment(comment_str.c_str());
    }   

    if (!root_cert.empty()) {
        std::vector<char> const pem = load_file(root_cert);
        t.set_root_cert(std::string(&pem[0], pem.size()));
    }   

    // create the torrent and print it to stdout
    std::vector<char> torrent;
    lt::bencode(back_inserter(torrent), t.generate());
    if (!outfile.empty()) {
        std::fstream out;
        out.exceptions(std::ifstream::failbit);
        out.open(outfile.c_str(), std::ios_base::out | std::ios_base::binary);
        out.write(torrent.data(), torrent.size());
    }   
    else {
        std::cout.write(torrent.data(), torrent.size());
    }   

    if (!merklefile.empty()) {
        std::fstream merkle;
        merkle.exceptions(std::ifstream::failbit);
        merkle.open(merklefile.c_str(), std::ios_base::out | std::ios_base::binary);
        auto const& tree = t.merkle_tree();
        merkle.write(reinterpret_cast<char const*>(tree.data()), tree.size() * 20);
    }   
    return SUCCESS;
    
}

