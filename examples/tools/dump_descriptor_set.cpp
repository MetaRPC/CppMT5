/*══════════════════════════════════════════════════════════════════════════════
 FILE: dump_descriptor_set.cpp - TOOL: export the gRPC contract as a FileDescriptorSet

 PURPOSE:
   Serializes the complete mt5_term_api gRPC contract (every proto file and all its
   transitive dependencies) from the compiled-in descriptor pool into a single
   google.protobuf.FileDescriptorSet (.desc). That .desc is the version-independent
   source of truth: CMake feeds it to `protoc` at build time, so the C++/gRPC stubs
   always match the protobuf the user has installed.

   This is a developer/build tool, not part of the SDK. Run it only to (re)produce
   proto/mt5_term_api.desc when the contract changes.

 USAGE:  ./dump_descriptor_set <out.desc>
══════════════════════════════════════════════════════════════════════════════*/

#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

using google::protobuf::DescriptorPool;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;
using google::protobuf::FileDescriptorSet;

// The non-WKT proto files that make up the MT5 contract (the ones we generate stubs for).
// Their transitive dependencies (google/protobuf/*, google/api/*) are pulled in automatically.
static const std::vector<std::string> kSeeds = {
    "mrpc-mt5-error.proto",
    "mt5-term-api-account-helper.proto",   "mt5-term-api-account-information.proto",
    "mt5-term-api-accounts.proto",         "mt5-term-api-admin.proto",
    "mt5-term-api-charts.proto",           "mt5-term-api-connection.proto",
    "mt5-term-api-demo-account.proto",     "mt5-term-api-gui.proto",
    "mt5-term-api-health-check.proto",     "mt5-term-api-internal-charts.proto",
    "mt5-term-api-market-info.proto",      "mt5-term-api-public-account.proto",
    "mt5-term-api-quote-history.proto",    "mt5-term-api-server-timezone.proto",
    "mt5-term-api-subscriptions.proto",    "mt5-term-api-trade-functions.proto",
    "mt5-term-api-trade-stats.proto",      "mt5-term-api-trading-helper.proto",
    "mt5-term-api-vnc.proto",
};

static void addWithDeps(const FileDescriptor* fd, std::set<std::string>& seen,
                        FileDescriptorSet& out) {
    if (!fd) return;
    std::string name(fd->name());
    if (seen.count(name)) return;
    seen.insert(name);
    for (int i = 0; i < fd->dependency_count(); ++i)   // dependencies first (topological order)
        addWithDeps(fd->dependency(i), seen, out);
    fd->CopyTo(out.add_file());
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: %s <out.desc>\n", argv[0]); return 2; }

    const DescriptorPool* pool = DescriptorPool::generated_pool();
    FileDescriptorSet set;
    std::set<std::string> seen;

    for (const std::string& name : kSeeds) {
        const FileDescriptor* fd = pool->FindFileByName(name);
        if (!fd) { std::fprintf(stderr, "ERROR: %s not in the generated pool\n", name.c_str()); return 1; }
        addWithDeps(fd, seen, set);
    }

    std::ofstream os(argv[1], std::ios::binary);
    if (!os) { std::fprintf(stderr, "ERROR: cannot write %s\n", argv[1]); return 1; }
    if (!set.SerializeToOstream(&os)) { std::fprintf(stderr, "ERROR: serialize failed\n"); return 1; }

    std::printf("wrote %s: %d files\n", argv[1], set.file_size());
    return 0;
}
