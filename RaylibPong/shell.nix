{ pkgs ? import <nixpkgs> {} }:
pkgs.llvmPackages_16.libcxxStdenv.mkDerivation {
  name = "env";
  nativeBuildInputs = with pkgs; [
    clang-tools_16
    xorg.libX11
    xorg.libXrandr
    xorg.libXi
    xorg.libXcursor
    xorg.libXinerama
    mesa
    libglvnd
    glxinfo
  ];
  #buildInputs = with pkgs; [
  #  raylib
  #];
}
