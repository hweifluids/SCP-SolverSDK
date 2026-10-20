#include <streamcenterplus/DistributedRuntime.h>
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
    Require(rejected, "Invalid shard was accepted.");
}
void Write(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << value;
    out.close();
    Require(static_cast<bool>(out), "Fixture write failed.");
}

int main(int argc, char** argv) {
    streamcenterplus::distributed::Runtime runtime;
    return runtime.Guard([&] {
        if (argc != 2) throw std::runtime_error("Expected fixture directory.");
        const fs::path root = argv[1];
        if (runtime.coordinator()) {
            fs::remove_all(root);
            fs::create_directories(root);
        }
        runtime.Barrier();
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
