#pragma once


#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <thread>
#include <array>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>

class ProcessBase {
public:
    using ProcHandle = HANDLE;
    static constexpr auto null_handle = ProcHandle(nullptr);

    class FileHandle {
    public:
        FileHandle(HANDLE h = INVALID_HANDLE_VALUE):_h(h) {};
        ~FileHandle() {if (_h != INVALID_HANDLE_VALUE) CloseHandle(_h);}
        FileHandle(FileHandle &&other): _h(other._h) {other._h = 0;}
        FileHandle &operator=(FileHandle &&other) {
            if (_h != other._h) {
                close();
                _h = other._h;
                other._h = INVALID_HANDLE_VALUE;
            }
            return *this;
        }
        
        explicit operator bool() const {return _h != INVALID_HANDLE_VALUE;}
        operator HANDLE() {return _h;}
        HANDLE release() {auto x = _h; _h = INVALID_HANDLE_VALUE; return x;}
        void close() {
            if (_h != INVALID_HANDLE_VALUE) {
                CloseHandle(_h);
                _h = INVALID_HANDLE_VALUE;
            }
        }
        HANDLE *ref() {return &_h;}
    protected:
        HANDLE _h;
    };


protected:
    std::optional<std::intptr_t> close_handle(ProcHandle h, std::chrono::steady_clock::time_point wait_until = std::chrono::steady_clock::time_point::max()) {
        if (!h) return 0;
        auto timeout = std::max<int64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(wait_until - std::chrono::steady_clock::now()).count(),
                std::numeric_limits<DWORD>::max());
        DWORD out = WaitForSingleObject(h, static_cast<DWORD>(timeout));
        if (out == WAIT_TIMEOUT) return std::nullopt;
        DWORD r;
        GetExitCodeProcess(h, &r);
        CloseHandle(h);
        return r;
    }

    void terminate_process(HANDLE h) {
        if (h == INVALID_HANDLE_VALUE) return;
        TerminateProcess(h,255);
    }

};

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


class ProcessBase {
public:

    using ProcHandle = pid_t;
    static constexpr auto null_handle = ProcHandle(0);

    class FileHandle {
    public:
        FileHandle(int h = -1):_h(h) {};
        ~FileHandle() {close();}
        FileHandle(FileHandle &&other): _h(other._h) {other._h = 0;}
        FileHandle &operator=(FileHandle &&other) {
            if (_h != other._h) {
                close();
                _h = other._h;
                other._h = -1;
            }
            return *this;
        }
        
        explicit operator bool() const {return _h>=0;};
        operator int() {return _h;}
        int release() {int x = _h; _h = -1; return x;}
        void close() {
            if (_h>=0) {::close(_h);_h = -1;}
        }
    protected:
        int _h;
    };

protected:

    std::optional<std::intptr_t> close_handle(ProcHandle h, std::chrono::steady_clock::time_point wait_until = std::chrono::steady_clock::time_point::max()) {
        if (!h) return 0;        
        int sta;
        int opt = wait_until == std::chrono::steady_clock::time_point::max()?0:WNOHANG;
        while (waitpid(h, &sta, opt) == 0) {
            if (std::chrono::steady_clock::now() >= wait_until) return std::nullopt;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (WIFEXITED(sta)) return WEXITSTATUS(sta);
        if (WIFSIGNALED(sta)) return -WTERMSIG(sta);
        return 0;
    }

    void terminate_process(ProcHandle h) {
        if (!h) return;
        kill(h, SIGTERM);
    }

};

#endif

/// Process class, cross platform process management with optional pipes for stdin/stdout/stderr
class Process : public ProcessBase {
public:

    /// default constructor, creates non-joinable process instance
    Process() = default;
    /// destructor closes handle - it always joins the process, if it is still running
    ~Process() {
        ProcessBase::close_handle(_handle);
    }
    /// move constructor and assignment operator, moves the handle and leaves the moved-from instance non-joinable
    Process(Process &&other):_handle(other._handle) {other._handle = null_handle;}
    /// move constructor and assignment operator, moves the handle and leaves the moved-from instance non-joinable
    Process &operator=(Process &&other) {
        if (this != &other) {
            ProcessBase::close_handle(_handle);
            _handle = other._handle;
            other._handle = null_handle;
        }
        return *this;
    }

    /// Defines pipes for process creation, 
    /** if the pointer is set to non-nullptr, the process will be created with the corresponding pipe connected to stdin/stdout/stderr.
       The file pointers are opened in the opposite direction of the stream (input is opened for output and vice versa) because they 
        are used by the parent process to write to child's stdin and read from child's stdout/stderr. */
    struct Pipes {
        /// set to pointer to existing FILE pointer to initialize stdin of the process. The file is opened for output!
        FILE **input = nullptr;
        /// set to pointer to existing FILE pointer to initialize stdout of the process, The file is opened for input!
        FILE **output = nullptr;
        /// set to pointer to existing FILE pointer to initialize stderr of the process, The file is opened for input!
        FILE **error = nullptr;
        ///enforce binary stream (windows only)
        bool input_binary = false;
        ///enforce binary stream (windows only)
        bool output_binary = false;
        ///enforce binary stream (windows only)
        bool error_binary = false;
    };

    
    /// creates a process with given path and arguments, optionally with pipes for stdin/stdout/stderr
    /**
     * @param process_path The path to the executable file to run. If process name doesn't contain a slash, it will be searched in PATH environment variable.
     * @param arguments The list of arguments to pass to the process. The first argument is not executable, the function will prepend the executable path to the arguments list. 
                        The arguments are passed as string views, so they don't need to be null-terminated.                    
                        Arguments must have UTF-8 encoding, they will be converted to wide strings on Windows. 
     * @param pipes Optional struct defining pipes for stdin/stdout/stderr. If a pointer is set to non-nullptr,
                         the process will be created with the corresponding pipe connected to stdin/stdout/stderr.
                         The file pointers are opened in the opposite direction of the stream (input is opened for
                         output and vice versa) because they are used by the parent process to write to child's stdin
                         and read from child's stdout/stderr.
    */
    Process(std::filesystem::path process_path, std::span<const std::u8string_view> arguments, Pipes pipes = Pipes{nullptr,nullptr, nullptr,false,false,false}) {
#ifdef _WIN32
        std::basic_ostringstream<char8_t, std::char_traits<char8_t>, std::allocator<char8_t>> argbld;
        std::wstring path = L"\"" + process_path.wstring() + L"\"";
        for (const auto &arg : arguments) {            
            size_t sp = arg.find_first_of(u8" \t\n\v\f\r");
            argbld << u8" ";
            if (sp == arg.npos) argbld << arg; else argbld << '"' << arg  << '"';
        }
        auto cmdline = path + to_unicode(std::move(argbld).str());

        STARTUPINFOW si = { sizeof(si) };
        DWORD pflags = 0;
        FileHandle h_input, h_output, h_error;
        

        if (pipes.error || pipes.input || pipes.output) {
            pflags = CREATE_NO_WINDOW;
            if (pipes.input) {
                auto p = make_pipes();
                DuplicateHandle(GetCurrentProcess(), p[0], GetCurrentProcess(), h_input.ref(), 0, TRUE, DUPLICATE_SAME_ACCESS);
                *pipes.input = handle2file(p[1].release(), pipes.input_binary);
            } else {
                h_input = createNulDevice();
            }
            if (pipes.output) {
                auto p = make_pipes();
                DuplicateHandle(GetCurrentProcess(), p[1], GetCurrentProcess(), h_output.ref(), 0, TRUE, DUPLICATE_SAME_ACCESS);
                *pipes.output = handle2file(p[0].release(), pipes.output_binary);
            } else {
                h_output = createNulDevice();
            }
            if (pipes.error) {
                auto p = make_pipes();
                DuplicateHandle(GetCurrentProcess(), p[1], GetCurrentProcess(), h_error.ref(), 0, TRUE, DUPLICATE_SAME_ACCESS);
                *pipes.error = handle2file(p[0].release(), pipes.error_binary);
            } else {
                h_error = createNulDevice();
            }
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdInput = h_input;
            si.hStdOutput = h_output;
            si.hStdError = h_error;
        }

        PROCESS_INFORMATION pi;

        if (!CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, TRUE, pflags, nullptr, nullptr, &si, &pi)) {
            throw std::system_error(GetLastError(), std::system_category(), "CreateProcessW failed");
        }
        CloseHandle(pi.hThread);
        _handle = pi.hProcess;
#else 
        auto control = make_pipes();
        int fsta = fork();
        if (fsta < 0) {            
            throw std::system_error(errno, std::system_category(), "fork failed");        
        }

        std::array<FileHandle,2> p_stdin, p_stdout, p_stderr;
        if (pipes.input) {
            p_stdin = make_pipes();
            *pipes.input = fdopen(p_stdin[1].release(), "w");            
        }
        if (pipes.output) {
            p_stdout = make_pipes();
            *pipes.output = fdopen(p_stdout[0].release(), "r");            
        }
        if (pipes.error) {
            p_stderr = make_pipes();
            *pipes.error = fdopen(p_stderr[0].release(), "r");            
        }

        if (fsta == 0) {            
            std::string path = process_path.string();
            std::size_t needsz = path.size()+1;
            for (const auto &x: arguments) needsz += x.size()+1;
            char *args = new char[needsz];
            char **arglst = new char *[arguments.size()+2];
            char *iter = args;
            arglst[0] = args;
            iter = std::copy(path.begin(), path.end(), iter);
            *iter++=0;
            int idx = 1;
            for (const auto &x: arguments) {
                arglst[idx++] = iter;
                iter = std::copy(x.begin(), x.end(), iter);
                *iter++=0;
            }
            arglst[idx] = 0;

            if (p_stdin[0]) dup2(p_stdin[0], STDIN_FILENO);
            if (p_stdout[1]) dup2(p_stdout[1], STDOUT_FILENO);
            if (p_stderr[1]) dup2(p_stderr[1], STDERR_FILENO);            

            execvp(path.c_str(), arglst);
            int e = errno;
            write(control[1], &e, sizeof(e));
            _exit(0);
        }
        int res;
        int cnt = read(control[0],&res, sizeof(res));
        if (cnt == res) {
            throw std::system_error(res, std::system_category(), "exec failed");
        }
        _handle = fsta;
#endif
    }

    /// joins the process and returns its exit code. If the process is still running, function will block until it finishes. If the process is not joinable, throws an exception.
    intptr_t join() {
        auto h = _handle;
        if (h == null_handle) throw std::runtime_error("Process instance is not joinable");
        _handle = null_handle;
        return *ProcessBase::close_handle(h);
    }    
    /// tries to join the process and returns its exit code. If the process is still running, returns std::nullopt. If the process is not joinable, throws an exception.
    /**
     * @param dur The maximum duration to wait for the process to finish. If the process is still running after this duration, returns std::nullopt. If the process is not joinable, throws an exception.
     * @return The exit code of the process if it finished within the specified duration, std::nullopt if the process is still running after the duration, or throws an exception if the process is not joinable.
     */
    std::optional<intptr_t> join(std::chrono::steady_clock::duration dur) {
        auto h = _handle;
        if (h == null_handle) throw std::runtime_error("Process instance is not joinable");
        std::optional<intptr_t> res = ProcessBase::close_handle(h,std::chrono::steady_clock::now()+dur);
        if (res.has_value()) _handle = null_handle;
        return res;
    }
    

    /// checks if the process is joinable, i.e. if it is still running or has not been joined yet. If the process is not joinable, returns false.
    bool joinable() const {
        return _handle != null_handle;
    }

    /// terminates the process. If the process is not joinable, throws an exception.
    /**
        Only sends signal to the process, it is not guaranteed that the process will actually terminate. If the process is still running after calling this function, 
                it can be joined with join() or join(timeout) to get its exit code. If the process is not joinable, throws an exception.
        * @throws std::runtime_error if the process instance is not joinable (i.e. if it is still running or has not been joined yet).
     */
    void terminate() {
        if (_handle == null_handle) throw std::runtime_error("Process instance is not joinable");
        ProcessBase::terminate_process(_handle);

    }

    Process(std::filesystem::path process_path, std::initializer_list<std::u8string_view> arguments, Pipes pipes = Pipes{nullptr,nullptr, nullptr})
        :Process(std::move(process_path), std::span<const std::u8string_view>(arguments.begin(), arguments.size()),pipes) {}

protected:
    ProcHandle _handle = null_handle;
 
#ifdef _WIN32
    std::array<FileHandle, 2> make_pipes() {
        HANDLE read, write;        
        if (!CreatePipe(&read, &write, NULL, 0)) {
            throw std::system_error(GetLastError(),std::system_category(), "Create pipe failed");
        }
        return {FileHandle(read), FileHandle(write)};
    }

    std::wstring to_unicode(std::u8string_view data ) {
        std::wstring result;
        int need = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size()), nullptr, 0);
        if (need <= 0) throw std::system_error(GetLastError(), std::system_category(), "MultiByteToWideChar failed");
        result.resize(need);
        if (!MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size()), result.data(), static_cast<int>(result.size()))) {
            throw std::system_error(GetLastError(), std::system_category(), "MultiByteToWideChar failed");
        }
        return result;
    }

    FILE *handle2file(HANDLE f, bool binary) {
        int fd = _open_osfhandle((intptr_t)f, 0);
        if (fd < 0) throw std::system_error(errno, std::system_category(), "_open_osfhandle failed");
        FILE *res = _fdopen(fd,binary?"rb+":"rt+");
        if (!res) {
            int e = errno;
            _close(fd);
            throw std::system_error(e, std::system_category(), "_fdopen failed");
        }
        return res;
    }

    HANDLE createNulDevice() {
        SECURITY_ATTRIBUTES attrs = {0};
        attrs.nLength = sizeof(attrs);
        attrs.bInheritHandle = TRUE;
        HANDLE h = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,&attrs, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            throw std::system_error(GetLastError(), std::system_category(), "Failed to open NUL device");
        }
        return h;
    }

#else 
    std::array<FileHandle, 2> make_pipes() {
        int pp[2];
        if (::pipe2(pp,O_CLOEXEC)) throw std::system_error(errno, std::system_category(), "failed pipe2");
        return {pp[0],pp[1]};
    }

#endif


};
