#include <future>
#include <nix/attr-path.hh>
#include <nix/config.h>
#include <nix/filetransfer.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>

using namespace nix;

// Safe to ignore - the args will be static.
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif __clang__
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
struct OpArgs : MixCommonArgs {
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

    if (!isatty(STDIN_FILENO)) {
      std::string word;
      while (std::cin >> word) {
        opArgs.paths.push_back(word);
      }
    }
    if (opArgs.paths.empty())
      throw UsageError("no paths passed");

    StorePaths storePaths;
    for (auto &str : opArgs.paths) {
      storePaths.emplace_back(store->followLinksToStorePath(str));
    }

    StringSet pathsS;
    std::exception_ptr exc;
    std::vector<std::future<ref<const ValidPathInfo>>> futures;

    fileTransferSettings.tries = 1;
    fileTransferSettings.enableHttp2 = true;

    for (auto &sub : getDefaultSubstituters()) {
      if (!settings.useSubstitutes)
        break;

      if (sub->storeDir != store->storeDir)
        continue;
      if (!sub->wantMassQuery)
        continue;

      for (auto &path : storePaths)
        futures.push_back(std::async(
            [=](StorePath path) { return sub->queryPathInfo(path); }, path));
    }

    for (auto &fut : futures) {
      try {
        auto info = fut.get();
        pathsS.emplace(store->printStorePath(info->path));
      } catch (InvalidPath &) {
        continue;
      } catch (...) {
        exc = std::current_exception();
      }
    }

    if (exc)
      std::rethrow_exception(exc);

    StorePathSet substitutablePaths;
    for (auto &str : pathsS) {
      substitutablePaths.emplace(store->followLinksToStorePath(str));
    }

    for (auto &path : storePaths) {
      if (substitutablePaths.find(path) == substitutablePaths.end())
        std::cout << store->printStorePath(path) << '\n';
    }
    std::cout << std::endl;
  });
}
