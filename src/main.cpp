#include <pthread.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <exception>
#include <iostream>
#include <stop_token>
#include <thread>

#include "app/application_factory.hpp"
#include "app/pulse_application.hpp"

int main() {
    sigset_t signals{};

    if (sigemptyset(&signals) != 0 || sigaddset(&signals, SIGINT) != 0 ||
        sigaddset(&signals, SIGTERM) != 0) {
        std::cerr << "Failed to configure shutdown signals\n";
        return 1;
    }

    // Block signals before creating additional threads.
    sigset_t previous_mask{};
    const int mask_error = pthread_sigmask(SIG_BLOCK, &signals, &previous_mask);

    if (mask_error != 0) {
        std::cerr << "Failed to block shutdown signals\n";
        return 1;
    }

    int exit_code = 0;

    try {
        pulse::PulseApplication app{pulse::ApplicationFactory::createMonitoringService()};

        std::stop_source stop_source;
        std::atomic<bool> signal_error{false};

        // Listen for shutdown signals in a background thread.
        std::jthread signal_thread{[&signals, &signal_error, &stop_source](std::stop_token token) {
            while (!token.stop_requested()) {
                const timespec timeout{.tv_sec = 0, .tv_nsec = 100'000'000};

                const int received_signal = sigtimedwait(&signals, nullptr, &timeout);

                if (received_signal == SIGINT || received_signal == SIGTERM) {
                    stop_source.request_stop();
                    break;
                }

                if (received_signal == -1 && errno != EAGAIN && errno != EINTR) {
                    signal_error.store(true);
                    stop_source.request_stop();
                    break;
                }
            }
        }};

        // Run monitoring on the main thread.
        const auto result = app.run(stop_source.get_token());

        // Stop the signal listener even if monitoring failed.
        signal_thread.request_stop();
        signal_thread.join();

        if (signal_error.load()) {
            std::cerr << "Failed to wait for shutdown signal\n";
            exit_code = 1;
        }

        else if (!result) {
            std::cerr << "Monitoring failed\n";
            exit_code = 1;
        } else {
            std::cout << "Pulse stopped.\n";
        }

    } catch (const std::exception& error) {
        std::cerr << "Pulse failed: " << error.what() << '\n';
        exit_code = 1;
    }

    // Restore the original signal mask.
    const int restore_error = pthread_sigmask(SIG_SETMASK, &previous_mask, nullptr);

    if (restore_error != 0) {
        std::cerr << "Failed to restore signal mask\n";
        return 1;
    }

    return exit_code;
}