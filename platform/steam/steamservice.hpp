#pragma once


#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <span>
#include "generic_callback.hpp"
#include "steam/isteamugc.h"


class SteamService {
public:

    using CreateItemCallback = std::function<void(bool success, uint64_t id, bool needLegalAgreement)>;
    using DeleteItemCallback = std::function<void(bool success, uint64_t id)>;
    using SubmitItemCallback = std::function<void(bool success, bool needLegalAgreement, int steamErrorCode)>;
    using UpdateProgressCallback = std::function<void(bool success, int stage, uint64_t bytes_uploaded, uint64_t bytes_total)>;
    using QuerySubscribedCall = GenericSteamCall<SteamUGCQueryCompleted_t, std::function<void(SteamUGCQueryCompleted_t *, bool)> >;

    using CreateItemCall = GenericSteamCall<CreateItemResult_t, std::function<void(CreateItemResult_t *, bool)> >;
    using DeleteItemCall = GenericSteamCall<DeleteItemResult_t, std::function<void(DeleteItemResult_t *, bool)> >;
    using SubmitItemCall = GenericSteamCall<SubmitItemUpdateResult_t, std::function<void(SubmitItemUpdateResult_t *, bool)> >;

    struct UGCItem {
        std::string title;
        std::string author;
        std::string download_location;     
        uint64_t id;   
    };

    using QueryUGCCallback = std::function<void(std::span<const UGCItem> list)>;

    class ItemUpdate {
    public:
        ItemUpdate(UGCUpdateHandle_t handle, SteamService *service);
        bool set_title(const std::string & title);
        bool set_description(const std::string & description);
        bool set_language(const std::string & language);
        bool set_visibility(ERemoteStoragePublishedFileVisibility visibility);
        bool set_tags(std::span<const std::string> tags);
        bool set_content(std::filesystem::path content_path);
        bool set_preview(std::filesystem::path preview_path);
        bool submit(std::string  change_note, SubmitItemCallback callback);
        void get_upload_progress(UpdateProgressCallback cb);
    protected:
        UGCUpdateHandle_t _handle;
        SteamService *_service;
        SubmitItemCall _call;        
    };

    

    using StartItemUpdateCallback = std::function<void(std::unique_ptr<ItemUpdate> ptr)>;

    SteamService();
    ~SteamService();
    bool is_available() const;
    
    void post(std::function<void()> fn);
    void run_callbacks();

    bool set_achievement(const char* id);
    bool clear_achievement(const char* id);

    bool create_item( CreateItemCallback callback) ;
    bool delete_item(uint64_t id, DeleteItemCallback callback) ;
    bool start_item_update(uint64_t id, StartItemUpdateCallback callback);

    bool query_ugc(QueryUGCCallback cb);

    void activate_game_overlay_to_web_page(std::string url);
    bool is_overlay_enabled() const;
    size_t get_install_callback_counter() ;


    class UpdateInstallEvent {
    public:
        UpdateInstallEvent(SteamService *me):_me(me) {}
    private:
        SteamService *_me;
        STEAM_CALLBACK( UpdateInstallEvent, OnItemInstalled, ItemInstalled_t );
    };
    class SubscribeChange {
    public:
        SubscribeChange(SteamService *me):_me(me) {}
    private:
        SteamService *_me;
        STEAM_CALLBACK( SubscribeChange, OnUserSubscribedItemsListChanged, UserSubscribedItemsListChanged_t );
    };


protected:
    bool _available = false;
    AppId_t _appid = {};
    std::vector<std::function<void()>> _main_thread_tasks;
    std::recursive_mutex _main_thread_tasks_mutex;  

    QuerySubscribedCall _query_subscribed_call;
    CreateItemCall _create_item_call;
    DeleteItemCall _delete_item_call;    
    std::atomic<size_t> _install_counter = {};
    UpdateInstallEvent _update_install_event;
    SubscribeChange _subscribe_change_event;
    class QUGCState;
};