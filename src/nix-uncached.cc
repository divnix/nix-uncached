#include <algorithm>
#include <future>
#include <nix/attr-path.hh>
#include <nix/config.h>
#include <nix/derivations.hh>
#include <nix/filetransfer.hh>
#include <nix/parsed-derivations.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <nlohmann/json.hpp>

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

    using InputPath = std::string;
    std::exception_ptr exc;
    std::map<InputPath, StringSet> queryPaths;
    std::map<InputPath, StringSet> substitutablePaths;
    std::map<InputPath, std::vector<std::future<ref<const ValidPathInfo>>>>
        futures;

    fileTransferSettings.tries = 1;
    fileTransferSettings.enableHttp2 = true;

    for (auto &storePath : storePaths) {
      StorePathSet paths;
      store->computeFSClosure(storePath, paths, false, true);

      // for sanity, only query remotely buildable paths that have a known
      // deriver
      for (auto &p : paths) {
        auto deriver = store->queryPathInfo(p)->deriver;
        if (deriver.has_value()) {
          if (store->isValidPath(deriver.value())) {
            auto drv = store->derivationFromPath(deriver.value());
            auto parsedDrv = ParsedDerivation(deriver.value(), drv);
            if (!parsedDrv.getBoolAttr("preferLocalBuild"))
              queryPaths[store->printStorePath(storePath)].insert(
                  store->printStorePath(p));
          }
        }
      }
    }

    for (auto &sub : getDefaultSubstituters()) {
      debug("considering '%s' for mass query", sub->getUri());

      if (!settings.useSubstitutes)
        break;

      if (sub->storeDir != store->storeDir) {
        debug("discarding '%s' for mass query since '%s' != '%s'", sub->getUri(), sub->storeDir, store->storeDir);
        continue;
      }

      if (!sub->wantMassQuery) {
        debug("discarding '%s' for mass query since doesn not want mass query", sub->getUri());
        continue;
      }

      debug("arming '%s' for mass query", sub->getUri());

      for (auto &map : queryPaths) {
        for (auto &path : map.second)
          futures[map.first].push_back(std::async(
              [=](std::string path) {
                return sub->queryPathInfo(sub->parseStorePath(path));
              },
              path));
      }
    }

    for (auto &map : futures) {
      for (auto &fut : map.second) {
        try {
          auto info = fut.get();
          substitutablePaths[map.first].emplace(store->printStorePath(info->path));
          debug("found '%s'", store->printStorePath(info->path));
        } catch (InvalidPath & e) {
          continue;
        } catch(const std::exception& e) {
          debug("Unknown expection: '%s'", e.what());
          continue;
        }
      }
    }

    std::map<InputPath, StringSet> uncachedPaths;

    for (auto &map : queryPaths) {
      if (substitutablePaths.count(map.first))
        std::set_difference(map.second.begin(), map.second.end(),
                            substitutablePaths[map.first].begin(),
                            substitutablePaths[map.first].end(),
                            std::inserter(uncachedPaths[map.first],
                                          uncachedPaths[map.first].begin()));
    }

    nlohmann::json out(uncachedPaths);
    std::cout << out.dump();
  });
}
