{
  inputs,
  cell,
}: {
  default = import ./shell.nix {
    inherit (inputs.nixpkgs) pkgs;
    inherit (inputs.nix.packages) nix;
    inherit (inputs.cells.app.packages) nix-uncached;
  };
}
