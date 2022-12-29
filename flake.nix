{
  description = "Hydra's builtin hydra-eval-jobs as a standalone";

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
}
