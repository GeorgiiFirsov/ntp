# ntp

Windows API has a very beautiful [Thread Pool API][1] that allows to execute 
work tasks, wait for waitable handles or completion ports and schedule timer 
callbacks very efficiently.

This API is designed for C programming language, but C++ provides more useful 
features, that make working with such APIs easier. This library is a wrapper 
for Thread Pool API, that provides a C++ abstraction and makes easier to 
manage all threadpool resources.

For further information about internal API refer to [Microsoft documentation][2].

## Installation

TODO :)

## Usage

Usage of `ntp` is as simple as possible.
Just add the following lines in your CMakeLists.txt:
```cmake
target_link_libraries(your-project PRIVATE ntp)
target_include_directories(your-project PRIVATE "/path/to/ntp/include")
```
After that you can just include header and use library features:
```cpp
 #include "ntp.hpp"
```

## Examples

### Basic workers

```cpp
#include "ntp.hpp"

void DoWork()
{
    ntp::SystemThreadPool pool;
	 
    pool.SubmitWork([]() {
        // Do some long operation.
    });
	 
    // Do some other work.
	 
    pool.WaitWorks();
}
```

### External cancellation

```cpp
#include "ntp.hpp"

void DoWork(const std::vector<Callable>& tasks)
{
    //
    // For RpcTestCancel refer to: https://learn.microsoft.com/en-us/windows/win32/api/rpcdce/nf-rpcdce-rpctestcancel
    //

    ntp::SystemThreadPool pool(RpcTestCancel);

    for (const auto& task : tasks)
    {
        pool.SumbitWork(task);
    }

    // Do some other work.
	
    const bool completed = pool.WaitWorks();
    if (!completed)
    {
        //
        // If WaitWorks returns false, then external cancellation test was 
        // triggered while tasks were awaited.
        //

        HandleIncompleteWork();
    }
}
```

### Cleanup on callback exit

Callbacks may optionally accept `PTP_CALLBACK_INSTANCE` as their first argument.
It allows user to call the following functions:
- [`FreeLibraryWhenCallbackReturns`][3]
- [`LeaveCriticalSectionWhenCallbackReturns`][4]
- [`ReleaseMutexWhenCallbackReturns`][5]
- [`ReleaseSemaphoreWhenCallbackReturns`][6]
- [`SetEventWhenCallbackReturns`][7]

```cpp
#include "ntp.hpp"

void DoWork(HANDLE event)
{
    ntp::SystemThreadPool pool;

    pool.SubmitWork([event](PTP_CALLBACK_INSTANCE instance) {
        SetEventWhenCallbackReturns(instance, event);

        // Do some work. After callback ends event will be set.
    });

    // Do further work...
}
```

### Basic wait callbacks

```cpp
#include "ntp.hpp"

class ProcessManager final
{
public:
    void LaunchProcess(const wchar_t* command_line)
    {
        STARTUPINFO si{ sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi{};

        //
        // Don't handle errors for simplicity
        //

        CreateProcess(nullptr, command_line, nullptr, nullptr, 
            FALSE, 0, nullptr, nullptr, &si, &pi);
			 
        //
        // Set wait callback
        //

        pool_.SubmitWait(pi.hProcess, OnProcessCompleted, 
            pi.hProcess /* Pass handle to callback too */); // (1)
    }

private:
    static void OnProcessCompleted(TP_WAIT_RESULT wait_result, HANDLE process)
    {
        if (wait_result == WAIT_OBJECT_0)
        {
            // Process has been ended
        }

        //
        // Close process handle, passed as parameter at (1)
        //

        CloseHandle(process);
    }

private:
    ntp::SystemThreadPool pool_;
}
```

### External logger support

`ntp` library supports custom logger callback, that accepts severity and a message string.
Severity has values from `ntp::logger::Severity` enumeration.

```cpp
#include "ntp.hpp"

void TraceCallback(ntp::logger::Severity severity, const wchar_t* message)
{
    if (severity >= ntp::logger::Severity::kNormal)
    {
        //
        // Skip ntp::logger::Severity::kExtended
        //

        std::wcerr << message << L'\n';
    }
}

int main()
{
    ntp::logger::SetLogger(TraceCallback);

    // Do work with thread pools
}
```


## TODO

- [X] Work callbacks (`PTP_WORK`)
- [X] Wait callbacks (`PTP_WAIT`)
- [X] Timer callbacks (`PTP_TIMER`)
- [ ] IO callbacks (`PTP_IO`)
- [ ] Tests
- [ ] Cute documentation
- [X] Support for `PTP_INSTANCE` as optional first argument in callbacks
- [ ] (Optional) Alpc callbacks (`PTP_ALPC`)

[1]: https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pool-api
[2]: https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pools
[3]: https://learn.microsoft.com/ru-ru/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-freelibrarywhencallbackreturns
[4]: https://learn.microsoft.com/ru-ru/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-leavecriticalsectionwhencallbackreturns
[5]: https://learn.microsoft.com/ru-ru/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-releasemutexwhencallbackreturns
[6]: https://learn.microsoft.com/ru-ru/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-releasesemaphorewhencallbackreturns
[7]: https://learn.microsoft.com/ru-ru/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-seteventwhencallbackreturns
