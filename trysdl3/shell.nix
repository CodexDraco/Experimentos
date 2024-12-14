{
  pkgs ? import <nixpkgs> {}
}:
let SDL3 = pkgs.callPackage ../../nixpkgs/SDL3/package.nix {};
in
pkgs.stdenv.mkDerivation {
  name = "trysdl3";
  nativeBuildInputs = with pkgs; [
    pkg-config
    cmake
    ninja
  ];
  buildInputs = [
    SDL3
  ];
}
