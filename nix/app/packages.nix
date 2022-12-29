{
  inputs,
  cell,
}: rec {
  default = nix-uncached;
  nix-uncached = inputs.nixpkgs.callPackage ./default.nix {
    inherit (inputs.nix.packages) nix;
    src = inputs.std.incl inputs.self [
      "${inputs.self}/src"
      "${inputs.self}/meson.build"
    ];
  };
}
