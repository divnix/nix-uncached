{
  stdenv,
  lib,
  nix,
  meson,
  cmake,
  ninja,
  pkg-config,
  boost,
  nlohmann_json,
  src,
}:
stdenv.mkDerivation {
  inherit src;

  pname = "nix-uncached";
  version = "2.13.3";
  buildInputs = [
    nlohmann_json
    nix
    boost
  ];
  nativeBuildInputs = [
    meson
    pkg-config
    ninja
    # nlohmann_json can be only discovered via cmake files
    cmake
  ];

  meta = {
    description = "Query Nix caches efficiently";
    homepage = "https://github.com/divnix/nix-uncached";
    license = lib.licenses.gpl3;
    maintainers = with lib.maintainers; [nrdxp];
    platforms = lib.platforms.unix;
  };
}
