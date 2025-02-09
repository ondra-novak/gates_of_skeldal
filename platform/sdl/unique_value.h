#include <optional>
#include <utility>
#include <type_traits>


template<typename T, typename Deleter>
class unique_value {
public:
    using value_type = T;
    using deleter_type = Deleter;

private:
    std::optional<T> value;
    deleter_type deleter;  

public:
    unique_value() noexcept(std::is_nothrow_default_constructible_v<Deleter>)
        : value(std::nullopt), deleter{} { }

    explicit unique_value(T v, Deleter d = Deleter{}) noexcept
        : value(std::move(v)), deleter(std::move(d)) { }

    // Zakázání kopírování
    unique_value(const unique_value&) = delete;
    unique_value& operator=(const unique_value&) = delete;

    unique_value(unique_value&& other) noexcept
        : value(std::move(other.value)), deleter(std::move(other.deleter))
    {
        other.value.reset();
    }

    unique_value& operator=(unique_value&& other) noexcept {
        if (this != &other) {
            reset(); // Uvolní aktuální hodnotu (volá deleter)
            value = std::move(other.value);
            deleter = std::move(other.deleter);
            other.value.reset();
        }
        return *this;
    }

    ~unique_value() {
        reset();
    }

    T* operator->() {
        return &(*value);
    }
    const T* operator->() const {
        return &(*value);
    }

    T& operator*() {
        return *value;
    }
    const T& operator*() const {
        return *value;
    }

    T& get() {
        return *value;
    }
    const T& get() const {
        return *value;
    }

    bool has_value() const noexcept {
        return value.has_value();
    }

    explicit operator bool() const noexcept {
        return value.has_value();
    }

    std::optional<T> release() noexcept {
        auto temp = std::move(value);
        value.reset();
        return temp;
    }

    void reset() noexcept {
        if (value.has_value()) {
            deleter(*value);
            value.reset();
        }
    }
    void reset(T &&v) noexcept {
        reset();
        value = std::move(v);
    }
    void reset(const T &v) noexcept {
        reset();
        value = v;
    }
};
