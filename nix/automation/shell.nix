{
  pkgs,
  nix-uncached,
  nix,
}: let
  inherit (pkgs) lib stdenv;
in
  nix-uncached.overrideAttrs (old: {
    src = null;

    nativeBuildInputs =
      old.nativeBuildInputs
      ++ [
        pkgs.treefmt
        pkgs.llvmPackages.clang # clang-format
        pkgs.alejandra
        pkgs.nodePackages.prettier
        pkgs.bear

        (pkgs.python3.withPackages (ps: [
          ps.pytest
          ps.black
        ]))
      ];

    NODE_PATH = "${pkgs.nodePackages.prettier-plugin-toml}/lib/node_modules";

    shellHook = lib.optionalString stdenv.isLinux ''
      export NIX_DEBUG_INFO_DIRS="${pkgs.curl.debug}/lib/debug:${nix.debug}/lib/debug''${NIX_DEBUG_INFO_DIRS:+:$NIX_DEBUG_INFO_DIRS}"
    '';
  })
