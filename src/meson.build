src = [
  'nix-uncached.cc',
]

executable('nix-uncached', src,
           dependencies : [
             nix_main_dep,
             nix_store_dep,
             nix_expr_dep,
             nix_cmd_dep,
             boost_dep,
             nlohmann_json_dep,
             threads_dep
           ],
           install: true,
           cpp_args: ['-std=c++17', '-fvisibility=hidden'])
