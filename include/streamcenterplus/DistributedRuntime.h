#pragma once

#include <array>
#include <cstdlib>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef STREAMCENTERPLUS_CLUSTER_MPI
#include <mpi.h>
#endif

namespace streamcenterplus::distributed {

// Own initialization only when the embedding application has not initialized MPI.
class Runtime {
public:
    Runtime() {
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
        int initialized = 0;
        Check(MPI_Initialized(&initialized));
        if (!initialized) {
            int provided = 0;
            Check(MPI_Init_thread(nullptr, nullptr, MPI_THREAD_FUNNELED, &provided));
            ownsMpi_ = true;
            if (provided < MPI_THREAD_FUNNELED) {
                MPI_Abort(MPI_COMM_WORLD, 1);
                throw std::runtime_error("MPI thread support is insufficient.");
            }
        }
        Check(MPI_Comm_rank(MPI_COMM_WORLD, &rank_));
        Check(MPI_Comm_size(MPI_COMM_WORLD, &size_));
#else
        for (const char* key : {"OMPI_COMM_WORLD_SIZE", "PMI_SIZE", "PMIX_SIZE"}) {
            const char* value = std::getenv(key);
            if (value && std::strtol(value, nullptr, 10) > 1) {
                throw std::runtime_error("Multi-rank execution requires an MPI-enabled solver.");
            }
        }
#endif
    }
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    ~Runtime() {
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
        if (ownsMpi_) {
            MPI_Finalize();
        }
#endif
    }
    int rank() const { return rank_; }
    int size() const { return size_; }
    bool coordinator() const { return rank_ == 0; }

    std::string Broadcast(std::string value) const {
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
        unsigned long long count = value.size();
        Check(MPI_Bcast(&count, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD));
        if (count > static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("MPI broadcast payload is too large.");
        }
        value.resize(static_cast<std::size_t>(count));
        Check(MPI_Bcast(value.data(), static_cast<int>(count), MPI_CHAR, 0, MPI_COMM_WORLD));
#endif
        return value;
    }
    template<std::size_t N>
    std::array<double, N> Sum(const std::array<double, N>& values) const {
        auto result = values;
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
        Check(MPI_Allreduce(values.data(), result.data(), static_cast<int>(N), MPI_DOUBLE,
                            MPI_SUM, MPI_COMM_WORLD));
#endif
        return result;
    }
    void Barrier() const {
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
        Check(MPI_Barrier(MPI_COMM_WORLD));
#endif
    }
    template<class Function>
    auto Guard(Function&& function) const -> decltype(function()) {
        try {
            return std::forward<Function>(function)();
        } catch (...) {
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
            // A peer may be inside a collective; local unwinding alone would hang it.
            if (size_ > 1) {
                try { throw; }
                catch (const std::exception& error) {
                    std::cerr << "[MPI rank " << rank_ << "] " << error.what() << '\n';
                }
                catch (...) { std::cerr << "[MPI] Unknown distributed failure\n"; }
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
#endif
            throw;
        }
    }
private:
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
    static void Check(int status) {
        if (status != MPI_SUCCESS) {
            MPI_Abort(MPI_COMM_WORLD, status);
            throw std::runtime_error("MPI runtime operation failed.");
        }
    }
    bool ownsMpi_ = false;
#endif
    int rank_ = 0;
    int size_ = 1;
};

}  // namespace streamcenterplus::distributed
