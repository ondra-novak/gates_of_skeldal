
#include "steam/isteamfriends.h"
#include "steam/isteamugc.h"
#include "steam/steam_api.h"
#include "steam/steam_api_common.h"
#include "steam/steamtypes.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <linux/limits.h>
#include <memory>
#include <mutex>
#include <strings.h>
#include <type_traits>
#include "steamservice.hpp"


SteamService::SteamService() {
    _available = SteamAPI_Init();
    if (_available) {
        _appid = SteamUtils()->GetAppID();
    }


}

SteamService::~SteamService() {
    SteamAPI_Shutdown();

}

void SteamService::post(std::function<void()> fn) {
    std::lock_guard _(_main_thread_tasks_mutex);
    _main_thread_tasks.push_back(std::move(fn));
}

void SteamService::run_callbacks() {
    SteamAPI_RunCallbacks();
    std::unique_lock lk(_main_thread_tasks_mutex);
    std::size_t idx = 0;
    std::size_t count =_main_thread_tasks.size();
    while (idx < count) {
        auto fn = std::move(_main_thread_tasks[idx]);
        ++idx;
        lk.unlock();
        fn();
        lk.lock();
    }
    _main_thread_tasks.erase(_main_thread_tasks.begin(), _main_thread_tasks.begin()+count);
}

bool SteamService::set_achievement(const char* id) {
    if (!_available) return false;
    post([=]{
        SteamUserStats()->SetAchievement(id);
        SteamUserStats()->StoreStats();
    });
    return true;

}
bool SteamService::clear_achievement(const char* id) {
    if (!_available) return false;
    post([=] {
        if (strcasecmp(id, "all") == 0) {
            unsigned int cnt = SteamUserStats()->GetNumAchievements();
            for (unsigned int i = 0; i < cnt; ++i) {
                SteamUserStats()->ClearAchievement(SteamUserStats()->GetAchievementName(i));
            }
            SteamUserStats()->StoreStats();
            return 0;
        } else {
            if (SteamUserStats() && SteamUserStats()->ClearAchievement(id)) {
                SteamUserStats()->StoreStats();
                return 0;
            } else {
                return -1;
            }
        }
    });
    return true;

}

bool SteamService::create_item( CreateItemCallback callback) {
    if (!_available) {
        return false;
    }
    post([this, callback = std::move(callback)]() mutable {
        _create_item_call.await([callback = std::move(callback)](CreateItemResult_t *result, bool io_failure) {
            if (io_failure) {
                callback(false, 0, false);
            } else {
                callback(result->m_eResult == k_EResultOK, result->m_nPublishedFileId, result->m_bUserNeedsToAcceptWorkshopLegalAgreement);
            }
        }, SteamUGC()->CreateItem(_appid, k_EWorkshopFileTypeCommunity));
    });
    return true;
}

bool SteamService::delete_item(uint64_t id, DeleteItemCallback callback) {
    if (!_available) {
        return false;
    }
    post([this, id, callback = std::move(callback)]() mutable {
        _delete_item_call.await([callback = std::move(callback)](DeleteItemResult_t *result, bool io_failure) {
            if (io_failure) {
                callback(false, 0);
            } else {
                callback(result->m_eResult == k_EResultOK, result->m_nPublishedFileId);
            }
        }, SteamUGC()->DeleteItem(id));
    });
    return true;
}


SteamService::ItemUpdate::ItemUpdate(UGCUpdateHandle_t handle, SteamService *service)
    :_handle(handle), _service(service) {

}

bool SteamService::ItemUpdate::set_title(const std::string &title){
    return SteamUGC()->SetItemTitle(_handle, title.c_str());

}
bool SteamService::ItemUpdate::set_description(const std::string &description){
    return SteamUGC()->SetItemDescription(_handle, description.c_str());
}
bool SteamService::ItemUpdate::set_language(const std::string &language){
    return SteamUGC()->SetItemUpdateLanguage(_handle, language.c_str());
}
bool SteamService::ItemUpdate::set_visibility(ERemoteStoragePublishedFileVisibility visibility){
    return SteamUGC()->SetItemVisibility(_handle,  visibility);
}
bool SteamService::ItemUpdate::set_tags(std::span<const std::string> tags) {

    std::vector<const char *> list;
    list.reserve(tags.size());
    for (const auto &x: tags) list.push_back(x.c_str());

    SteamParamStringArray_t ptags{list.data(), static_cast<int32_t>(list.size())};

    return SteamUGC()->SetItemTags(_handle, &ptags);

}
bool SteamService::ItemUpdate::set_content(std::filesystem::path content_path){
    return SteamUGC()->SetItemContent(_handle, content_path.string().c_str());
}
bool SteamService::ItemUpdate::set_preview(std::filesystem::path preview_path){
    return SteamUGC()->SetItemPreview(_handle, preview_path.string().c_str());
}
bool SteamService::ItemUpdate::submit(std::string change_note, SubmitItemCallback callback){

    _service->post([this, callback=std::move(callback), change_note = std::move(change_note)]() mutable {
        _call.await([callback = std::move(callback)](SubmitItemUpdateResult_t *result, bool io_failure){

            if (io_failure) {
                callback(false, false, -1);
            } else {
                callback(result->m_eResult == k_EResultOK , result->m_bUserNeedsToAcceptWorkshopLegalAgreement, result->m_eResult);
            }
        },SteamUGC()->SubmitItemUpdate(_handle, change_note.empty()?NULL:change_note.c_str()));
    });
    return true;
}
void SteamService::ItemUpdate::get_upload_progress(UpdateProgressCallback cb) {
    _service->post([handle = this->_handle,callback = std::move(cb)] {
        uint64 processed = 0;
        uint64 total = 0;
        auto st = SteamUGC()->GetItemUpdateProgress(handle, &processed, &total);
        callback(st == k_EItemUpdateStatusInvalid, st, processed, total);
    });

}

bool SteamService::start_item_update(uint64_t id, StartItemUpdateCallback callback) {
    post([this, id, callback = std::move(callback)]{
        auto handle = SteamUGC()->StartItemUpdate(_appid, id);
        auto ptr = std::make_unique<ItemUpdate>(handle, this);
        callback(std::move(ptr));
    });
    return true;
}

template<typename Pointer, auto x>
requires(std::is_member_function_pointer_v<decltype(x)>)
struct MemberCaller {
    Pointer p;
    template<typename ... Args>
    auto operator()(Args && ... args) const {
        return ((*p).*x)(std::forward<Args>(args)...);
    }
};


class SteamService::QUGCState : public std::enable_shared_from_this<QUGCState> {
public:
    QUGCState(QueryUGCCallback cb, SteamService *svc):_cb(std::move(cb)), _svc(svc) {}


    void entry_point() {
        auto iugc = SteamUGC();
        uint32_t ugc_count = iugc->GetNumSubscribedItems();
        std::vector<PublishedFileId_t> subscribed(ugc_count);
        //all subscribed items
        ugc_count = iugc->GetSubscribedItems(subscribed.data(), subscribed.size());
        if (ugc_count == 0) {
            _cb(_result);
            return;
        }
        subscribed.resize(ugc_count);
        //create list of downloaded items
        _downloaded.reserve(ugc_count);
        for (const auto &x: subscribed) {
            auto st = iugc->GetItemState(x);
            if ((st & k_EItemStateInstalled) && !(st & k_EItemStateDisabledLocally)) {
                _downloaded.push_back(x);
            }
        }        
        //create query - ask for all downloaded items
        auto h = iugc->CreateQueryUGCDetailsRequest(_downloaded.data(),_downloaded.size());
        //submit query
        _details_awaiter.await({shared_from_this()},iugc->SendQueryUGCRequest(h));
        //coroutine contines by details_ready

    }


    void details_ready(const SteamUGCQueryCompleted_t *result, bool io_failure) {
        //query complete
        auto iugc = SteamUGC();
        //if failed, return empty result
        if (io_failure) {
            _cb(_result);
            return;
        }
        //prepare results
        _result.resize(result->m_unNumResultsReturned);
        SteamUGCDetails_t details;
        uint64 size_on_disk;
        uint32 timestamp;

        char folder_buffer[PATH_MAX];


        _owners.resize(result->m_unNumResultsReturned);
        //process all resultrs
        for (uint32 i = 0; i < result->m_unNumResultsReturned; ++i) {
            iugc->GetQueryUGCResult(result->m_handle, i, &details);
            auto &item = _result[i];
            //title
            item.title = details.m_rgchTitle;
            item.id = details.m_nPublishedFileId;

            iugc->GetItemInstallInfo(details.m_nPublishedFileId,&size_on_disk,folder_buffer,sizeof(folder_buffer),&timestamp);
            //download location
            item.download_location = folder_buffer;
            //list of owners
            _owners[i] = details.m_ulSteamIDOwner;
        }

        SteamUGC()->ReleaseQueryUGCRequest(result->m_handle);

        ///request owners info
        std::vector<uint64> ownset  = _owners;
        std::sort(ownset.begin(), ownset.end());
        ownset.erase(std::unique(ownset.begin(), ownset.end()), ownset.end());

        for (auto &x: ownset ){
            SteamFriends()->RequestUserInformation(x, true);
        }
        _timeout = std::chrono::steady_clock::now()+std::chrono::seconds(2);
        author_resolve_cycle();
    }

    static bool invalid_name(std::string_view n) {
        return n.empty() || n == "[unknown]";
    }

    void author_resolve_cycle() {
        bool done = true;
        for (size_t i = 0; i < _result.size(); ++i) {
            auto &r = _result[i];
            auto &u =  _owners[i];
            if (r.author.empty()) {
                std::string n = SteamFriends()->GetFriendPersonaName(u);
                if (!invalid_name(n))  {
                    r.author = n;
                } else {
                    done = false;
                }
            }
        }
        if (done || std::chrono::steady_clock::now() > _timeout) {
            _cb(_result);
            return;
        }
        _svc->post([me = shared_from_this()]{me->author_resolve_cycle();});
    }


protected:
    QueryUGCCallback _cb;
    SteamService *_svc;
    std::vector<UGCItem> _result;
    std::vector<uint64> _owners;
    std::vector<PublishedFileId_t> _downloaded;
    std::chrono::steady_clock::time_point _timeout;


    GenericSteamCall< SteamUGCQueryCompleted_t,
            MemberCaller<std::shared_ptr<QUGCState>, &QUGCState::details_ready> >_details_awaiter;

};

bool SteamService::query_ugc(QueryUGCCallback cb) {
    if (!_available) return false;
    auto state = std::make_shared<QUGCState>(std::move(cb), this);
    post([state]{state->entry_point();});
    return true;
}

bool SteamService::is_available() const {
    return _available;
}

void SteamService::activate_game_overlay_to_web_page(std::string url) {
    post([=]{
        SteamFriends()->ActivateGameOverlayToWebPage(url.c_str());
    });
}