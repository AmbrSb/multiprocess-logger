#include <glog/logging.h>

#include "service_manager.hpp"

namespace {

ServiceManager* gsm;

extern "C"
void HandleSIGINT(int)
{
    gsm->Stop();
}

/**
 * Installs a signal handler to handle SIGINT (for Ctrl+C)
 * key strokes for cleanly shutdown the service.
 * 
 */
void
InstallSignalHandlers()
{
    if (SIG_ERR == signal(SIGINT, HandleSIGINT))
        PLOG(FATAL) << "Could not install signal handler for Ctrl-C!";
    else
        LOG(INFO) << "SIGINT handler installed.";
}

}

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;
    InstallSignalHandlers();

    /**
     * Simply create a ServiceManager instance with argv[1] as the
     * path of the the executable file of the service. The reset of
     * argv's are passed as arguments of the service.
     */
    gsm = new ServiceManager{argv[1], argv + 1};
    /**
     * Now activate the service manager. This starts the monitoring
     * thread and starts an instance of the service child process.
     * Then Wait() on the service manager, which blocks until the
     * service is stopped.
     * 
     */
    gsm->Activate().Wait();

    google::ShutdownGoogleLogging();
    return 0;
}
