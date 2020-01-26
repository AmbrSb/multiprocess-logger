#pragma once

#include <thread>
#include <future>
#include <chrono>
#include <string>
#include <type_traits>

#include <sys/wait.h>
#include <sys/types.h>

#include <glog/logging.h>

#define SERVICE_INSTANCE_NAME  "_SRV"s
#define SERVICE(s) (!(strcmp(s, SERVICE_INSTANCE_NAME)))
#define MONITOR(s) (!SERVICE(s))
#define NOINTR(e)                                               \
        ({                                                      \
            int err_;                                           \
            while ((err_ = (e)) == -1 && errno == EINTR);       \
            err_;                                               \
        })
#define CHILD(p)    (0 == p)
#define PARENT(p)   (p > 0)

using namespace std::literals;

class ExecFailed: std::exception {};
class ForkFailed: std::exception {};

/**
 * This class runs a given executable as child process and
 * monitors its execution. If the process is terminated for
 * any reason, it is restarted by the service manager.
 * 
 */
class ServiceManager {
public:
    enum ServiceStatus {
        /// The service has not been run yet.
        kNotStarted = 0x0000001,
        /// The service was started but has now crashed.
        kCrashed    = 0x0000010,
        /// The service finished running, normally.
        kFinished   = 0x0000100,
        /// The service is running but an error has occured.
        kError      = 0x0001000,
        /// The service is stopped due to user request.
        kStopped    = 0x0010000,
        /// The service status is unknown.
        kUnknown    = 0x0100000,
        /// The service is running normally.
        kRunning    = 0x1000000,
        /// The service is not running.
        kNotRunning = 0x0111111,
    };

    /**
     * @brief Construct a new Service Manager object.
     * 
     * @param exec The path of the service executable.
     * @param argv The arguments to pass to the service process.
     */
    ServiceManager(std::string exec, char *const argv[]);

    ServiceManager(ServiceManager const&) = delete;
    ServiceManager(ServiceManager&&) = delete;
    ServiceManager& operator=(ServiceManager const&) = delete;
    ServiceManager& operator=(ServiceManager&&);
    /**
     * @brief Destroy the Service Manager object and stop the
     * underlying service instance.
     */
    ~ServiceManager();

    /**
     * This is the main entry point to start the ServiceManager
     * operation.
     * 
     * @return ServiceManager& Returns a reference to the current
     * ServiceManager instance.
     */
    ServiceManager& Activate();
    /**
     * Stop the service and the monitoring thread.
     */
    void Stop();
    /**
     * Get the status of service managed by this ServiceManager instance
     * 
     * @return ServiceStatus
     */
    ServiceStatus GetStatus();
    /**
     * A call to this function blocks until the service stops executing
     * for any reason.
     */
    void Wait();

private:
    /**
     * If set to true, the monitoring service will stop executing.
     */
    bool terminate_service_;
    /**
     * When is service is in kRunning status, the pid_ is set to
     * the PID of the child process that is executing the service.
     */
    pid_t pid_;
    /**
     * The current status of the service.
     */
    ServiceStatus status_;
    /**
     * The service child process has been started.
     */
    volatile bool started_ = false;
    /**
     * The std::thread that is running the monitoring daemon thread
     * that monitors the child service process.
     */
    std::thread monitor_;
    /**
     * Protects the ServiceManager from multiple concurrent stop()ings.
     */
    std::mutex stop_lock_;
    /**
     * The path of the service executable.
     */
    std::string exec_;
    /**
     * The arguments that should be passed to the service process.
     */
    char** argv_ = nullptr;
    /**
     * Figure out the current status of the child process that is
     * executing the service, if any.
     * @return ServiceStatus 
     */
    ServiceStatus DetectServiceProcessStatus();
    void Monitor();
    /**
     * fork() and exec() the executable as a child process.
     * 
     * @return ServiceManager& Returns a reference to the current
     * ServiceManager instance.
     */
    ServiceManager& Start();
};

ServiceManager::ServiceManager(std::string exec, char *const argv[])
    : terminate_service_ {false},
      pid_ {-1},
      status_{kNotStarted},
      exec_{exec},
      argv_{const_cast<char**>(argv)}
{
}

ServiceManager&
ServiceManager::operator=(ServiceManager&& other)
{
    pid_               = other.pid_;
    other.pid_         = -1;
    terminate_service_ = other.terminate_service_;
    status_            = other.status_;
    monitor_           = std::move(other.monitor_);

    return *this;
}

void
ServiceManager::Wait()
{
    while(DetectServiceProcessStatus() & kRunning) {
        std::this_thread::sleep_for(1000ms);
    }
    LOG(INFO) << "Waiting finished";
}

auto
ServiceManager::DetectServiceProcessStatus() -> ServiceStatus
{
    int status = 0;
    pid_t p = NOINTR(waitpid(pid_, &status, WNOHANG));
    if (p == pid_) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* The child has terminated */
            if (WEXITSTATUS(status) == 0)
                return kFinished;
            else
                return kCrashed;
        }
    } else if (p == 0) {
        return kRunning;
    }
    return kUnknown;
}

ServiceManager&
ServiceManager::Activate()
{
    monitor_ = std::thread{&ServiceManager::Monitor, this};
    while (!started_)
        std::this_thread::sleep_for(10ms);
    return *this;
}

void
ServiceManager::Monitor()
{

#define CANCEL_MONIT_IN_CHILD     \
        if (in_child_)            \
            return;               \

    LOG(INFO) << "Monitor main thread started.";
    while (true) {
        if (terminate_service_) {
            break;
        } else {
            if (status_ == kNotStarted)
                Start();
            status_ = DetectServiceProcessStatus();
            if (status_ & kNotRunning)
                Start();
        }
        std::this_thread::sleep_for(10ms);
    }
    LOG(INFO) << "Monitor main thread finished.";
}

ServiceManager&
ServiceManager::Start()
{
    LOG(INFO) << "fork()ing the service process.";
    pid_ = fork();
    if (CHILD(pid_)) {
        int ret = execv(exec_.c_str(), argv_);
        if (ret != 0) {
            PLOG(ERROR) << "Service exec() failed";
            throw ExecFailed{};
        }
    } else if (PARENT(pid_)) {
        started_ = true;
    } else {
        PLOG(ERROR) << "Service fork() failed";
        throw ForkFailed{};
    }
    return *this;
}

void
ServiceManager::Stop()
{
    if (pid_ == -1)
        return;

    std::lock_guard{stop_lock_};
    if (terminate_service_)
        return;

    LOG(INFO) << "Trying to stop service instance (" << pid_ << ")";
    terminate_service_ = true;
    if (monitor_.joinable())
        monitor_.join();
    int ret = kill(pid_, SIGTERM);
    if (0 == ret) {
        status_ = kStopped;
        LOG(INFO) << "Successfully stoped service instance (" << pid_ << ")";
    } else {
        LOG(ERROR) << "Failed to stop the service instance (" << pid_ << ")";
    }
}

inline
ServiceManager::~ServiceManager()
{
    Stop();
}
