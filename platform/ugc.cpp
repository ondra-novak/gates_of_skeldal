#include "ugc.h"
#include "config.h"

#include <algorithm>
#include <ranges>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
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


static std::filesystem::path ugc_local_path;

void UGCSetLocalFoler(const char *path) {
    ugc_local_path = reinterpret_cast<const char8_t *>(path);
}


void UGC_GetList(void (*callback)(const UGCItem *items, unsigned int count, void *context), void *context) {
    std::unordered_set<std::string> strings;
    std::vector<UGCItem> items;
    try {    
        for(const auto &entry : std::ranges::subrange(std::filesystem::directory_iterator(ugc_local_path), std::filesystem::directory_iterator())) {
            if (entry.is_directory()) {
                const auto ddl_path = entry.path()/"content.ddl";
                const auto ini = entry.path()/"info.ini";
                const auto stamp = entry.path()/"stamp";
                if (std::filesystem::is_regular_file(ddl_path) && std::filesystem::is_regular_file(ini)) {
                    const INI_CONFIG *cfg = ini_open(reinterpret_cast<const char *>(ini.u8string().c_str()));
                    if (cfg) {
                        const INI_CONFIG_SECTION *section = ini_section_open(cfg, "description");
                        std::string name = toKEYBCS2(ini_get_string(section, "name", "noname"));
                        std::string author = toKEYBCS2(ini_get_string(section, "author", "noname"));
                        std::string lang = ini_get_string(section, "lang", "CZ");
                        auto ddl8 = ddl_path.u8string();
                        auto niter = strings.insert(name);
                        auto aiter = strings.insert(author);
                        auto liter = strings.insert(lang);
                        auto diter = strings.insert({reinterpret_cast<const char *>(ddl8.c_str()), ddl8.size()});
                        time_t tplay = 0;
                        if (std::filesystem::is_regular_file(stamp)) {
                            auto tp = std::filesystem::last_write_time(stamp);
                            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - std::filesystem::file_time_type::clock::now()
                                + std::chrono::system_clock::now());
                            tplay = std::chrono::system_clock::to_time_t(sctp);
                        }

                        items.push_back(UGCItem{
                            niter.first->c_str(), 
                            diter.first->c_str(),
                            aiter.first->c_str(),
                            liter.first->c_str(),
                            tplay
                        });                        
                    }
                }                
            }
        }        
    } catch (...) {
    }
    std::sort(items.begin(), items.end(), [&](const UGCItem &a, const UGCItem &b) {
        return b.last_played - a.last_played;
    });
    callback(items.data(), items.size(), context);
    
}

void UGC_StartPlay(const char *ddl_path) {
    std::filesystem::path p = ddl_path;
    auto stamp = p.parent_path()/"stamp";
    std::ofstream(stamp, std::ios::out|std::ios::trunc);
}