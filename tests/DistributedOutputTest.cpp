#include <streamcenterplus/DistributedRuntime.h>
#include <streamcenterplus/DistributedOwnership.h>
#include <streamcenterplus/RankAwareOutput.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace output = streamcenterplus::rank_output;

void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Function> void Reject(Function function) {
    bool rejected = false;
    try { function(); } catch (const std::exception&) { rejected = true; }
    Require(rejected, "Invalid distributed input was accepted.");
}
void Write(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << value;
    out.close();
    Require(static_cast<bool>(out), "Fixture write failed.");
}

int main(int argc, char** argv) {
    // A second solver in the same host process must reuse the MPI session.
    { streamcenterplus::distributed::Runtime first; first.Barrier(); }
    { streamcenterplus::distributed::Runtime second; second.Barrier(); }
    streamcenterplus::distributed::Runtime runtime;
    return runtime.Guard([&] {
        const auto hardware = streamcenterplus::DetectHardwareAwareness();
#ifdef STREAMCENTERPLUS_CLUSTER_MPI
        // A serial executable may run inside a larger Slurm allocation; its
        // scheduler metadata need not equal a one-process Runtime communicator.
        Require(hardware.mpiRank == runtime.rank() && hardware.mpiWorldSize == runtime.size(),
                "Hardware probe disagrees with initialized MPI communicator.");
        Require(hardware.mpiLocalRank == runtime.localRank(), "Hardware probe lost the node-local MPI rank.");
        MPI_Comm local = MPI_COMM_NULL;
        Require(MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED,
                                   runtime.rank(), MPI_INFO_NULL, &local) == MPI_SUCCESS,
                "Could not query shared-memory communicator.");
        int expectedLocalRank = -1;
        MPI_Comm_rank(local, &expectedLocalRank);
        MPI_Comm_free(&local);
        Require(runtime.localRank() == expectedLocalRank, "Runtime cached an incorrect node-local rank.");
#endif
        streamcenterplus::HardwareAwareness unmasked;
        unmasked.mpiLocalRank = 3;
        Require(streamcenterplus::PreferredCudaDeviceOrdinal(unmasked) == 3,
                "Unmasked CUDA selection collapsed node-local ranks to device zero.");
        unmasked.cudaVisibleDevices = {"0", "1"};
        Require(streamcenterplus::PreferredCudaDeviceOrdinal(unmasked) == 1,
                "CUDA selection ignored visible device remapping.");
        std::cout << "rank " << runtime.rank() << ", local rank " << hardware.mpiLocalRank << '\n';
        if (argc != 2) throw std::runtime_error("Expected fixture directory.");
        const fs::path root = argv[1];
        if (runtime.coordinator()) {
            fs::remove_all(root);
            fs::create_directories(root);
        }
        runtime.Barrier();
        const auto planPath = root / ("plan-" + std::to_string(runtime.rank()) + ".scpa");
        const std::string validPlan =
            "rank_count = 2\nassignment_count = 1\n"
            "assignment.0.path = mesh.vtu\nassignment.0.kind = cell_block\n"
            "assignment.0.rank = 0\nassignment.0.range_start = 0\n"
            "assignment.0.range_end = 4\n";
        Write(planPath, validPlan);
        namespace partition = streamcenterplus::partition;
        const auto plan = partition::LoadPlan(planPath);
        const auto assigned = partition::BuildOwnership(plan, 0, partition::OwnershipLocation::Cells);
        const auto empty = partition::BuildOwnership(plan, 1, partition::OwnershipLocation::Cells);
        Require(partition::Contains(assigned, 0) && partition::Contains(assigned, 3) &&
                !partition::Contains(assigned, 4) && !partition::Contains(empty, 0) &&
                empty.ownedItemCount() == 0, "Empty or boundary ownership is incorrect.");
        for (const auto& replacement : {
                 std::pair<std::string, std::string>{"rank_count = 2", "rank_count = 2garbage"},
                 {"rank_count = 2", "rank_count = 0"},
                 {"assignment_count = 1", "assignment_count = 999999999"},
                 {"assignment.0.rank = 0", "assignment.0.rank = 2"},
                 {"assignment.0.range_end = 4", "assignment.0.range_end = -1"},
                 {"assignment.0.range_end = 4", "assignment.0.range_end = 4suffix"}}) {
            auto invalid = validPlan;
            invalid.replace(invalid.find(replacement.first), replacement.first.size(), replacement.second);
            Write(planPath, invalid);
            Reject([&] { partition::LoadPlan(planPath); });
        }
        Write(planPath, validPlan + "rank_count = 3\n");
        Reject([&] { partition::LoadPlan(planPath); });
        Write(planPath, "rank_count = 2\nassignment_count = 1\nassignment.0.kind = file\n"
                        "assignment.0.range_start = -1\nassignment.0.range_end = -1\n");
        Require(!partition::LoadPlan(planPath).assignments.front().hasRange(),
                "File-level partition lost its absent-range sentinel.");
        auto context = output::MakeContext(root, runtime.rank(), runtime.size(), true, "run-new");
        // Serial test still exercises a two-shard assembly without an MPI dependency.
        if (runtime.size() == 1) context = output::MakeContext(root, 0, 2, true, "run-new");
        Write(context.effectiveOutputDir / "payload.vtu", "closed-payload");
        output::WriteRankShardManifest(context, "fixture", "plan-A", {"payload.vtu"});
        output::ValidateRankShardManifest(context, context.rank, "fixture", "plan-A");
        auto stale = context;
        stale.runId = "run-stale";
        Reject([&] { output::ValidateRankShardManifest(stale, context.rank, "fixture", "plan-A"); });
        Reject([&] { output::ValidateRankShardManifest(context, context.rank, "fixture", "plan-B"); });
        Reject([&] { output::CheckedPayload(context.effectiveOutputDir, "../escape"); });
        const auto total = runtime.Sum<4>({double(runtime.rank() + 1), 0, 0, 1});
        Require(total[0] == runtime.size() * (runtime.size() + 1) / 2.0 &&
                total[3] == runtime.size(), "MPI sum/count mismatch.");
        runtime.Barrier();
        if (runtime.coordinator()) {
            if (runtime.size() == 1) {
                Reject([&] { output::WriteAssemblyManifest(context, "fixture", "plan-A"); });
                auto other = output::MakeContext(root, 1, 2, true, "run-new");
                Write(other.effectiveOutputDir / "rank_manifest.scpa.partial", "schema = partial\n");
                Reject([&] { output::WriteAssemblyManifest(context, "fixture", "plan-A"); });
                Write(other.effectiveOutputDir / "payload.vtu", "closed-payload");
                output::WriteRankShardManifest(other, "fixture", "plan-A", {"payload.vtu"});
                Write(other.effectiveOutputDir / "rank_manifest.scpa", "schema = incomplete\n");
                Reject([&] { output::WriteAssemblyManifest(context, "fixture", "plan-A"); });
                output::WriteRankShardManifest(other, "fixture", "plan-A", {"payload.vtu"});
            }
            output::WriteAssemblyManifest(context, "fixture", "plan-A");
            Require(fs::is_regular_file(root / "assembly/rank_assembly.scpa"), "Assembly missing.");
            fs::remove(context.effectiveOutputDir / "payload.vtu");
            Reject([&] { output::ValidateRankShardManifest(context, 0, "fixture", "plan-A"); });
        }
        runtime.Barrier();
        std::cout << "rank " << runtime.rank() << ": distributed output checks passed\n";
        return 0;
    });
}
