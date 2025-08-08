#include "ugc.h"
#include "config.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string_view>


#include <memory>

std::wstring toWideChar(std::string_view text) {
    unsigned int codepoint = 0;
    unsigned int bytes = 0;
    std::wstring out;
    for (char c : text) {
        if ((c & 0x80) == 0) out.push_back(c);
        else {
            if ((c & 0xC0) == 0x80) {
                codepoint = (codepoint << 6) | (c & 0x3F);
            } else if ((c & 0xE0) == 0xC0) {
                bytes=2; codepoint = c & 0x1F;
            } else if ((c & 0xF0) == 0xE0) {
                bytes=3; codepoint = c & 0x0F;
            } else if ((c & 0xF8) == 0xF0) {
                bytes=4; codepoint = c & 0x07;
            }
            --bytes;
            if (!bytes) out.push_back(static_cast<wchar_t>(codepoint));
        }
    }
    return out;
}

constexpr std::pair<int,int> cztable[]={
	    {0x010C,'\x80'},
        {0x00FC,'u'},
        {0x00E9,'\x82'},
        {0x010F,'\x83'},
        {0x00E4,'a'},
        {0x010E,'\x85'},
        {0x0164,'\x86'},
        {0x010D,'\x87'},
        {0x011B,'\x88'},
        {0x011A,'\x89'},
        {0x0139,'L'},
        {0x00CD,'\x8B'},
        {0x013E,'l'},
        {0x013A,'l'},
        {0x00C4,'A'},
        {0x00C1,'\x8F'},
        {0x00C9,'\x90'},
        {0x017E,'\x91'},
        {0x017D,'\x92'},
        {0x00F4,'o'},
        {0x00F6,'o'},
        {0x00D3,'\x95'},
        {0x016F,'\x96'},
        {0x00DA,'\x97'},
        {0x00FD,'\x98'},
        {0x00D6,'O'},
        {0x00DC,'U'},
        {0x0160,'\x9b'},
        {0x013D,'L'},
        {0x00DD,'\x9d'},
        {0x0158,'\x9e'},
        {0x0165,'\x9f'},
        {0x00E1,'\xA0'},
        {0x00ED,'\xA1'},
        {0x00F3,'\xA2'},
        {0x00FA,'\xA3'},
        {0x0148,'\xA4'},
        {0x0147,'\xA5'},
        {0x016E,'\xA6'},
        {0x00D4,'O'},
        {0x0161,'\xA8'},
        {0x0159,'\xA9'},
        {0x0155,'r'},
        {0x0154,'R'},
        {0x00BC,'\xAC'},
        {0x00A7,'\xAD'},
        {0x00AB,'<'},
        {0x00BB,'>'},
};


std::string toKEYBCS2(const char *text) {
    auto wstr = toWideChar(text);
    std::string out;
    out.resize(wstr.size());
    std::transform(wstr.begin(), wstr.end(), out.begin(), [](wchar_t c){
        if (c <= 0x80) return static_cast<char>(c);
        auto iter = std::find_if(std::begin(cztable), std::end(cztable), [c](const auto &x){return x.first == c;});
        if (iter == std::end(cztable)) return '_';
        return static_cast<char>(iter->second);
    });
    return out;
}


struct UGCItemEx : UGCItem {
    std::unique_ptr<char[]> text_data;
    std::filesystem::path _stamp_file;
};

struct tag_UGCManager {
    std::vector<UGCItemEx> _list;
};

UGCManager *UGC_create() {
    return new UGCManager;
}
void UGC_Destroy(UGCManager *inst) {
    delete inst;
}

static std::filesystem::path ugc_local_path;

void UGCSetLocalFoler(const char *path) {
    ugc_local_path = reinterpret_cast<const char8_t *>(path);
}

size_t UGC_Fetch(UGCManager *manager) {

    manager->_list.clear();
    std::error_code ec;
    auto iter = std::filesystem::directory_iterator(ugc_local_path,ec);
    if (ec == std::error_code()) {
        auto fend = std::filesystem::directory_iterator();
        while (iter != fend) {
            const auto &entry = *iter;
            if (entry.is_directory()) {
                auto entry_path =std::filesystem::weakly_canonical(entry.path()) ;
                auto info_path =  entry_path / "content.ini";
                if (std::filesystem::is_regular_file(info_path)) {
                    INI_CONFIG *cfg = ini_open(reinterpret_cast<const char *>(info_path.u8string().c_str()));
                    if (cfg) {
                        const INI_CONFIG_SECTION *section = ini_section_open(cfg, "description");
                        std::string title = toKEYBCS2(ini_get_string(section, "title", NULL));
                        std::string author = toKEYBCS2(ini_get_string(section, "author", "unknown author"));
                        const INI_CONFIG_SECTION *files = ini_section_open(cfg, "files");
                        const char *ddl = ini_get_string(files, "ddlfile", NULL);
                        if (ddl && !title.empty()) {
                            UGCItemEx r;
                            std::filesystem::path ddlpath = entry_path / ddl;
                            auto pstr = ddlpath.u8string();
                            ddl = reinterpret_cast<const char *>(pstr.c_str());

                            auto tlen = title.size()+1;
                            auto alen = author.size()+1;
                            auto dlen = std::strlen(ddl)+1;

                            r.text_data = std::make_unique<char[]>(tlen+alen+dlen);
                            char *c = r.text_data.get();
                            memcpy(c, title.c_str(), tlen);r.name = c;c+=tlen;
                            memcpy(c, author.c_str(), alen);r.author = c;c+=alen;
                            memcpy(c, ddl, dlen);r.ddl_path= c;c+=alen;

                            auto stampfile = entry_path  / "stamp";
                            std::filesystem::file_time_type tp = std::filesystem::last_write_time(stampfile, ec);
                            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - std::filesystem::file_time_type::clock::now()
                                + std::chrono::system_clock::now());
                            r.last_played = std::chrono::system_clock::to_time_t(sctp);
                            r._stamp_file = std::move(stampfile);
                            manager->_list.push_back(std::move(r));
                        }
                        ini_close(cfg);
                    }
                }
            }
            ++iter;
        }

    }
    std::sort(manager->_list.begin(), manager->_list.end(), [](const UGCItem &a, const UGCItem &b){
        return a.last_played > b.last_played;
    });

    return manager->_list.size();

}
UGCItem UGC_GetItem(UGCManager *manager, size_t pos) {
    return manager->_list[pos];
}

void UGC_StartPlay(UGCManager *manager, size_t pos) {
    std::ofstream out(manager->_list[pos]._stamp_file, std::ios::out|std::ios::trunc);
}



