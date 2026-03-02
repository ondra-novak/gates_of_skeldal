#pragma once

#include "steam/steam_api_common.h"
#include "steam/steamtypes.h"
#include <concepts>
#include <optional>
#include <stdexcept>

template<typename Result, std::invocable<Result *,bool> Callback>
class GenericSteamCall {
public:

    void await(Callback func, SteamAPICall_t call) {
        if (_callback) {
            throw std::runtime_error("Already waiting for a call result");
        }
        _callback.emplace(std::move(func));
        _call_result.Set(call, this, &GenericSteamCall::on_result);        
    }

protected:
    std::optional<Callback> _callback;
    CCallResult<GenericSteamCall,Result> _call_result;

    void on_result(Result *result, bool io_failure) {
        if (_callback) {
            (*_callback)(result, io_failure);
            _callback.reset();
        }
    }
};
