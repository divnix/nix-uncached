#include <nix/attr-path.hh>
#include <nix/config.h>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <nix/filetransfer.hh>
#include <nix/thread-pool.hh>

using namespace nix;

// Safe to ignore - the args will be static.
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif __clang__
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
struct OpArgs : MixCommonArgs, MixJSON {
  std::vector<std::string> paths;

  OpArgs() : MixCommonArgs("nix-uncached") {
    expectArgs({.label = "paths", .optional = false, .handler = {&paths}});

    addFlag({
        .longName = "help",
        .description = "show usage information",
        .handler = {[&]() {
          printf("USAGE: nix-uncached [options] paths...\n\n");
          for (const auto &[name, flag] : longFlags) {
            if (hiddenCategories.count(flag->category)) {
              continue;
            }
            printf("  --%-20s %s\n", name.c_str(), flag->description.c_str());
          }
          ::exit(0);
        }},
    });
  }
};

static OpArgs opArgs;

int main(int argc, char **argv) {
  return handleExceptions(argv[0], [&]() {
    initNix();
    initGC();

    opArgs.parseCmdline(argvToStrings(argc, argv));
    
    auto store = openStore();

    if (opArgs.paths.empty()) throw UsageError("no paths passed");

    ThreadPool pool(fileTransferSettings.httpConnections);

    std::function<void(StorePath)> queryPath;
    queryPath = [&](const StorePath & req) {
        SubstitutablePathInfos infos;
        store->querySubstitutablePathInfos({{req, std::nullopt}}, infos);

        if (infos.empty()) {
            std::cout << store->printStorePath(req) << '\n';
        }
    };

    for (auto & str : opArgs.paths) {
        auto path = store->followLinksToStorePath(str);

        pool.enqueue(std::bind(queryPath, path));
    }

    pool.process();
  });
}
