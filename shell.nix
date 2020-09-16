# A repository wide shell.nix containing all tools, formatters, and inputs
# required to build any of the C or Haskell packages.
#
# Entering a nix-shell using this derivation will allow you to cd anywhere
# in the ./pkg directory and run the appropriate build tooling.
#
# See the individual ./pkg/* directories for shell.nix derivations that only
# propagate minimal sets of buildInputs for the related package.
# (import ./default.nix { }).shell

let

  nixpkgs = import ./nix/default.nix { };
  pkgs = import ./default.nix { };
  lib = nixpkgs.lib;

  # The non-Haskell packages which build inputs (dependencies) will be
  # propagated into the shell. This combines nixpkgs' mkShell behaviour
  # with Haskell.nix's shellFor.
  #
  # For example, adding urbit here results in gmp, h2o, zlib, etc. being
  # made available, so you can just run make.
  # 
  # Typically the inputs listed here also have a shell.nix in their respective
  # source directory you can use, to avoid the Haskell/GHC dependencies.
  inputsFrom =
    [ pkgs.ent pkgs.ge-additions pkgs.herb pkgs.libaes-siv pkgs.urbit ];

  merge = name: lib.concatLists (lib.catAttrs name inputsFrom);

in pkgs.hs.shellFor {
  # Haskell packages from the stackProject which will have their
  # dependencies available in the shell.
  packages = ps:
    with ps; [
      lmdb-static
      racquire
      terminal-progress-bar
      urbit-atom
      urbit-azimuth
      urbit-eventlog-lmdb
      urbit-king
      urbit-noun
      urbit-noun-core
      urbit-termsize
    ];

  # Build tools to make available in the shell's PATH.
  buildInputs = [
    nixpkgs.cacert
    nixpkgs.stack
    nixpkgs.nixfmt
    nixpkgs.shfmt

    (import nixpkgs.sources.niv { }).niv

    (pkgs.hs.hackageTool {
      name = "ormolu";
      version = "0.1.3.0";
    })

    (pkgs.hs.hackageTool {
      name = "ShellCheck";
      version = "0.7.1";
    })
  ] ++ merge "buildInputs";

  nativeBuildInputs = merge "nativeBuildInputs";
  propagatedBuildInputs = merge "propagatedBuildInputs";
  propagatedNativeBuildInputs = merge "propagatedNativeBuildInputs";

  shellHook = lib.concatStringsSep "\n"
    (lib.catAttrs "shellHook" (lib.reverseList inputsFrom));
}
