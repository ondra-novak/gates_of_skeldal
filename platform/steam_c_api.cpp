
#include <chrono>
#include <filesystem>
#ifdef STEAM_ENABLED
#include "platform/steam/steamservice.hpp"
#include "steam_c_api.h"
#include "platform/sdl/BGraph2.h"
#include "libs/memman.h"
#include <fstream>



std::unique_ptr<SteamService> steam_service;

void initialize_steam_client() {

    steam_service = std::make_unique<SteamService>();
    if (!steam_service->is_available()) {
        steam_service.reset();
        set_steam_callback(nullptr);
    } else {
        set_steam_callback([]{
            steam_service->run_callbacks();
        });
    }
}


void shutdown_steam_client() {
    steam_service.reset();
}


int8_t set_achievement(const char * id)
{
    if (steam_service) {
        return steam_service->set_achievement(id)?0:-1;
    }
    return -1;
}

int8_t clear_achievement(const char * id)
{
    if (steam_service) {
        return steam_service->clear_achievement(id)?0:-1;
    }
    return -1;
}

char is_steam_available()
{
    return steam_service?1:0;
}


static uint64_t get_steam_id(const std::filesystem::path &p) {
    std::ifstream f(p, std::ios::in);
    if (!f) return 0;
    uint64_t id;
    f >> id;
    return id;
}

static bool set_steam_id(const std::filesystem::path &p, uint64_t id, time_t publish_time) {
    std::ofstream f(p, std::ios::out|std::ios::trunc);
    f << id << "\n";
    f << publish_time << "\n";
    return !!f;
}



static std::string_view ddl_load(auto fname) {
    int32_t sz;
    auto data = reinterpret_cast<const char *>(afile(fname, 0,&sz));
    if (data) {
        return {data, static_cast<std::size_t>(sz)};
    } else {
        return {};
    }
}

static void put_file(const std::filesystem::path &target, std::string_view text) {
    std::ofstream f(target, std::ios::out|std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open: "+target.string());
    f.write(text.data(), text.length());    
}

static void create_ini(const std::filesystem::path &target, std::string_view title, std::string_view author, std::string_view base_lang) {
    std::ofstream f(target, std::ios::trunc);
    if (!f) throw std::runtime_error("Failed to open file for content:" + target.string());
    f << "[description]\n"
         "name=" << title  <<"\n"
         "lang=" << base_lang << "\n"
         "author=" << author << "\n";
    if (!f) throw std::runtime_error("Failed to write file for cotent:" + target.string());

}

static void cycle_get_state(workshop_update_cb callback, void *context, std::shared_ptr<SteamService::ItemUpdate> ptr, std::chrono::steady_clock::time_point next_call) {
    auto now = std::chrono::steady_clock::now();    
    if (now > next_call) {        
        ptr->get_upload_progress([=](bool success, int stage, uint64_t bytes_uploaded, uint64_t bytes_total) {
            auto next = now +std::chrono::milliseconds(500);
            if (success) {
                if (stage > 0) {
                    const char *msg = NULL;
                    switch (stage) {
                        case k_EItemUpdateStatusPreparingConfig: msg = "Processing configuration data";break;
                        case k_EItemUpdateStatusPreparingContent: msg = "Reading and processing content files";break;
                        case k_EItemUpdateStatusUploadingContent: msg = "Uploading content changes to Steam";break;
                        case k_EItemUpdateStatusUploadingPreviewFile: msg = "Iploading new preview file image";break;
                        case k_EItemUpdateStatusCommittingChanges: msg = "Committing all changes";break;                
                    }
                    callback(0,msg,bytes_uploaded,bytes_total,context);
                    steam_service->post([=]{cycle_get_state(callback,context,ptr,next);});
                } 
            } else {
                steam_service->post([=]{cycle_get_state(callback,context,ptr,next);});
            }
        });
    } else {
        steam_service->post([=]{cycle_get_state(callback,context,ptr,next_call);});
    }
}


void continue_publish(std::filesystem::path content_path, std::filesystem::path state_path, workshop_update_cb cb, uint64 id, void *context) {
    steam_service->start_item_update(id, [=](std::shared_ptr<SteamService::ItemUpdate> ptr){
        try {
            if (!ptr) {
                cb(-1,"ERROR: Failed to initiate item update (StartItemUpdate)",0,0,context);
            } else {
                cb(0,"Preparing request",0,0,context);
                std::filesystem::path target = state_path.parent_path()/"depot";
                std::filesystem::create_directories(target);
                auto inifile = target/"info.ini";
                auto content  = target/"content.ddl";
                auto in_game_preview = target/"preview.hi";
                auto steam_preview = state_path.parent_path()/"preview.hi";       

                std::vector<std::string> stags;
                auto tags=ddl_load(".TAGS");
                auto itr = tags.data();
                while (itr != tags.data()+tags.size()) {
                    stags.push_back(itr);
                    itr = strchr(itr,0)+1;
                }
                auto visibility=ddl_load(".VIS");
                auto title=ddl_load(".TITLE");
                auto desc=ddl_load(".DESC");
                auto ulang=ddl_load(".ULANG");
                auto clang=ddl_load(".CLANG");
                auto blang=ddl_load(".BLANG");
                auto author=ddl_load(".AUTHOR");
                auto imgctxr=ddl_load("imgctx");
                auto image=ddl_load("image");
                auto ingame_image =ddl_load(".PRVIMG");
                auto changelog =ddl_load(".CHANGELOG");
                
                stags.emplace_back(clang);
                if (imgctxr == "image/jpeg") steam_preview.replace_extension(".jpg");
                else if (imgctxr == "image/png") steam_preview.replace_extension(".png");
                else {
                    cb(-1,"Preview image: unsupported media type",0,0,context);
                    return;
                }

                put_file(steam_preview, image);
                put_file(in_game_preview, ingame_image);
                create_ini(inifile, title, author, blang);
                std::filesystem::rename(content_path, content);

                ptr->set_content(target);
                ptr->set_description(std::string(desc));
                ptr->set_language(std::string{ulang});
                ptr->set_preview(steam_preview);
                ptr->set_tags(stags);
                ptr->set_title(std::string{title});
                ptr->set_visibility((ERemoteStoragePublishedFileVisibility)visibility[0]);
                cb(0,"Submiting request",0,0,context);                
                ptr->submit(std::string(changelog), [=](bool success, bool needLegalAgreement, int steamErrorCode) {                                    
                    std::filesystem::remove_all(target);
                    std::filesystem::remove(steam_preview);
                    time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                    if (!success || steamErrorCode  != 1) {
                        const char *message;
                        switch (steamErrorCode) {
                            default:
                            case k_EResultFail: message="Generic failure.";break;
                            case k_EResultInvalidParam: message="Either the provided app ID is invalid or doesn't match the consumer app ID of the item or, you have not enabled ISteamUGC for the provided app ID on the Steam Workshop Configuration App Admin page. The preview file is smaller than 16 bytes.";break;
                            case k_EResultAccessDenied: message="You don't own a license for the provided app ID.";break;
                            case k_EResultFileNotFound: message="Failed to get the workshop info for the item or failed to read the preview file.";break;
                            case k_EResultLockingFailed: message="Failed to aquire UGC Lock.";break;
                            case k_EResultLimitExceeded: message="The preview image is too large, it must be less than 1 Megabyte; or there is not enough space available on the user's Steam Cloud.";break;
                        }
                        cb(-1,message,0,0,context);

                    } else {
                        const char *message = needLegalAgreement?"SUCCESS: Upload successful! To make your item public, please accept the agreement on the item's page.":"SUCCESS: Upload successful!";
                        set_steam_id(state_path, id, t);   
                        cb(1,message,100,100,context);
                        if (needLegalAgreement) {
                            char weburl[100];
                            snprintf(weburl,sizeof(weburl),"https://steamcommunity.com/sharedfiles/filedetails/?id=%llu", (unsigned long long)id);
                            steam_service->activate_game_overlay_to_web_page(weburl);
                        }
                    }
                });
                cycle_get_state(cb, context, std::move(ptr), std::chrono::steady_clock::now()+std::chrono::milliseconds(500));
                



            }
        } catch (std::exception &e) {
            auto er = std::string("ERROR: ").append(e.what());            
            cb(-1, er.c_str(),0,0,context);
        }
    });
}


void steam_upload_to_workshop(const char *file, workshop_update_cb callback, void *context) {

    std::filesystem::path ddl_path = file;
    std::filesystem::path publish_path = ddl_path;
    std::filesystem::path state_path = ddl_path;
    publish_path.replace_extension(".publish");
    state_path.replace_extension(".steam");


    if (!steam_service) {
        callback(-1,"ERROR: Steam Client is not running", 0,0,context);
        return;
        
    }

    if (!add_patch_file(publish_path.string().c_str())) {
        return;
    }

    
    auto id = get_steam_id(state_path);
    
    if (id == 0) {
        callback(0,"Creating workshop item", 0,0,context);;                    
        steam_service->create_item([=](bool success, uint64_t id, bool ){
            if (success) {
                if (set_steam_id(state_path, id, 0)) {
                    continue_publish(ddl_path, state_path, callback, id, context);
                } else {
                    callback(-1,"ERROR: Failed to store workshop steam ID - check for write permissions", 0,0,context);;                    
                }
            } else {
                callback(-1,"ERROR: Create workshop item rejected (Permission denied)", 0,0,context);;                    
            }
        });
    } else {
        continue_publish(ddl_path, state_path, callback, id, context);
    }
}

size_t get_install_callback_counter() {
    if (steam_service) return steam_service->get_install_callback_counter();
    else return 0;
}

#else
void initialize_steam_client() {}
int8_t set_achievement(auto id) {return -1;}
int8_t clear_achievement(auto id) {return -1;}
char is_steam_available() {return 0;}
void steam_upload_to_workshop(const char *file, workshop_update_cb callback, void *context) {
    callback(-1,"ERROR: Steam is not compiled", 0,0,context);;                    
}
size_t get_install_callback_counter() {return 0;}
#endif

