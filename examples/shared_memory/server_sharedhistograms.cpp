//
// Created by Vetle Wegner Ingeberg on 28/04/2026.
//

#include <histogram/SharedHistograms.h>
#include <sys/fcntl.h>
#include <csignal>
#include <thread>
#include <chrono>
#include <random>

#include <sys/mman.h>   // shm_unlink
#include <cerrno>

char leaveprog = 'n';

void keyb_int(int sig_num)
{
    if (sig_num == SIGINT || sig_num == SIGQUIT || sig_num == SIGTERM) {
        printf("\n\nLeaving...\n");
        leaveprog = 'y';
    }
}



void CleanupPosixShm(const char* name)
{
    // Best-effort: remove any stale object with this name.
    // Ignore failure if it doesn't exist.
    if (shm_unlink(name) == -1 && errno != ENOENT) {
        // Optional: log or handle unexpected errors.
        perror("shm_unlink");
    }
}

int main() {

    signal(SIGINT, keyb_int); // set up interrupt handler (Ctrl-C)
    signal(SIGQUIT, keyb_int);
    signal(SIGTERM, keyb_int);
    signal(SIGPIPE, SIG_IGN);

    // First step is to clean up if the last run crashed.
    CleanupPosixShm("/test");

    auto creator = SharedHistograms::Create("test", 503316480, 30, true);

    auto hist = creator.Create1D("hist", "hist", 65536, 0, 65536, "Channel [ch]");

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<double> dist{1332., 35.0};

    while (leaveprog == 'n') {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        unsigned int x = gen() & 0xFFFF;
        hist->Fill(x);

        double y = dist(gen);
        hist->Fill(y);
    }

}
