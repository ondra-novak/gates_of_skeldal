#include "ugc.h"
#include "config.h"
#include "process.hpp"
#include <cstdio>

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <string_view>
#include <memory>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

extern "C" {
    #include <libs/event.h>
}

#ifdef STEAM_ENABLED
#include "steam/steamservice.hpp"
#include "steam_c_api.hpp"
#endif
#include "steam_c_api.h"

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

struct IniContent {
    std::string name;
    std::string author;
    std::string lang;
    std::string content;
};

static std::optional<IniContent> parse_ini(const std::filesystem::path &ini) {
    std::optional<IniContent> res;
    if (std::filesystem::is_regular_file(ini)) {
        INI_CONFIG *cfg = ini_open(reinterpret_cast<const char *>(ini.u8string().c_str()));
        if (cfg) {
            const INI_CONFIG_SECTION *section = ini_section_open(cfg, "description");
            res.emplace();
            res->name = toKEYBCS2(ini_get_string(section, "name", ""));
            res->author = toKEYBCS2(ini_get_string(section, "author", ""));
            res->lang = ini_get_string(section, "lang", "CZ");
            res->content = ini_get_string(section, "content", "content.ddl");
            ini_close(cfg);
        }
    }
    return res;


}

std::optional<UGCItem> parse_ugc(const std::filesystem::path &entry, std::unordered_set<std::string> &strings) {
    std::optional<UGCItem> item;
    const auto ini = entry/"info.ini";
    auto ini_data = parse_ini(ini);
    if (ini_data) {
        std::hash<std::string> hasher;
        item.emplace();
        auto addstr = [&](auto &&s){return strings.insert(std::move(s)).first->c_str();};
        item->ddl_path = addstr((entry/ini_data->content).string());
        item->author = addstr(ini_data->author);
        item->lang = addstr(ini_data->lang);
        item->name = addstr(ini_data->name);
        item->id = hasher(entry.filename().string());
    }
    return item;
     
}

struct UGCGetContext {
    std::vector<UGCItem> items;
    std::unordered_set<std::string> strings;
    void (*callback)(const UGCItem *items, unsigned int count, void *context);
    void *context;
};

void UGC_GetList(const char *ugc_user_path, 
                 const char *ugc_dlc_path,
                 void (*callback)(const UGCItem *items, unsigned int count, void *context), void *context) {
    
    std::array<std::filesystem::path, 2> paths = {ugc_user_path, ugc_dlc_path};
    std::error_code ec;
    UGCGetContext ctx;
    ctx.callback = callback;
    ctx.context = context;
    for (const auto &p: paths) {
        auto iter = std::filesystem::directory_iterator(p,ec);
        if (ec == std::error_code{}) {
            auto end = std::filesystem::directory_iterator();
            for (const auto &entry: std::ranges::subrange(iter,end)) {
                if (entry.is_directory(ec)) {    
                    auto item = parse_ugc(entry.path(), ctx.strings);
                    if (item) {
                        ctx.items.push_back(*item);
                    }
                }
            }
        }
    }

#ifdef STEAM_ENABLED
    if (steam_service) {
        UGCGetContext *st = new UGCGetContext(std::move(ctx));
        
        steam_service->query_ugc([st](std::span<const SteamService::UGCItem> list){
            for (const auto &x: list) {
                auto item = parse_ugc(x.download_location, st->strings);
                if (item) {
                    if (!item->author || item->author[0] == 0) {
                        item->author = st->strings.insert(toKEYBCS2(x.author.c_str())).first->c_str();                        
                    }
                    if (!x.title.empty()) {
                        item->name = st->strings.insert(toKEYBCS2(x.title.c_str())).first->c_str();                        
                    }
                    item->id = x.id;
                    st->items.push_back(*item);
                }
            }
            post_to_event_thread([](void *ctx){
                UGCGetContext *st = (UGCGetContext *)ctx;
                st->callback(st->items.data(), static_cast<unsigned int>(st->items.size()), st->context);
                delete st;
            }, st);
        });
        return;
    }
#endif
    ctx.callback(ctx.items.data(), static_cast<unsigned int>(ctx.items.size()), ctx.context);
}

size_t get_install_callback_counter() {
#ifdef STEAM_ENABLED
    if (steam_service) return steam_service->get_install_callback_counter();
#endif
    return 0;
}

char is_steam_workshop_browser_available() {
#ifdef STEAM_ENABLED
    if (steam_service) return steam_service->is_overlay_enabled();
#endif
    return false;
}


static std::string read_command(FILE *f) {
    std::string ln;
    ln.resize(1024);
    char *r = fgets(ln.data(), static_cast<int>(ln.size()), f);
    if (!r) return {};
    ln.resize(strlen(r));
    while (!ln.empty() && std::isspace(ln.back())) ln.pop_back();
    return ln;
}

void open_steam_workshop() {
#ifdef STEAM_ENABLED
 if (steam_service) return steam_service->activate_game_overlay_to_web_page("https://steamcommunity.com/app/3533830/workshop/");
#endif
}

static std::thread editor_thread;
static std::atomic<bool> editor_exited = false;


static void upload_callback(int result, const char *message, uint64_t upload_bytes, uint64_t total_bytes, void *context) {
    FILE *out = (FILE *)context;
    fprintf(out, "{\"running\":%s,\"error\":%s,\"sent\":%llu,\"total\":%llu,\"message\":\"%s\"})\r\n",
        result==0?"true":"false", result < 0?"true":"false", 
        static_cast<unsigned long long>(upload_bytes),
        static_cast<unsigned long long>(total_bytes),
        message
    );
    fflush(out);
}


void start_editor() {
#ifdef STEAM_ENABLED
    if (!steam_service) return;

    editor_exited = false;

    editor_thread = std::thread([&]{

        FILE *commands = nullptr;
        FILE *status = nullptr;
        #ifdef _WIN32
        const char *binary_name = "mapedit_server.exe";
        #else
        const char *binary_name = "./mapedit_server";
        #endif

        try {
            Process mapedit(binary_name,{u8"-o"}, {&status,&commands, nullptr});
            std::string cmd= read_command(commands);
            steam_service->activate_game_overlay_to_web_page(cmd);
            while (!cmd.empty()) {
                if (cmd.substr(0,4) == "PUB:") {
                    const char *pkg = cmd.c_str()+4;
                    steam_upload_to_workshop(pkg, upload_callback, status);
                }
                cmd = read_command(commands);
            }
        } catch (...) {

        }
        fclose(commands);
        fclose(status);
        editor_exited = true;
    });

    editor_thread.detach();

#endif
}

char did_editor_exit() {
    return editor_exited.load()?1:0;
}