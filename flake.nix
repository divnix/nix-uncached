{
  description = "Query Nix Cache Status";

  inputs.nixpkgs.follows = "nix/nixpkgs";
  inputs.std.url = "github:divnix/std";
  inputs.nix.url = "github:nixos/nix/2.12-maintenance";

  outputs = {
    self,
    nixpkgs,
    std,
    nix,
  } @ inputs:
    std.growOn {
      inherit inputs;
      systems = std.inputs.flake-utils.lib.defaultSystems;
      cellsFrom = ./nix;
      cellBlocks = [
        (std.installables "packages" {ci.build = true;})
        (std.devshells "devshells" {ci.build = true;})
      ];
    } {
      packages = std.harvest self [
        ["app" "packages"]
      ];
      devShells = std.harvest self [
        ["automation" "devshells"]
      ];
    };
  nixConfig = {
    allow-import-from-derivation = false;
    extra-substituters = [
      "https://cache.divnix.com"
    ];
    extra-trusted-public-keys = [
      "cache.divnix.com:sx7ojBrBUtdNmAMzNhiucTX+pqLzNTs4ISNb5qhh5OI="
    ];
  };
}
