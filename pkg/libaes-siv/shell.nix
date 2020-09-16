let

  pkgs = import ../../default.nix { };

in pkgs.shellFor {
  name = "libaes-siv";
  packages = ps: [ ps.libaes-siv ];
}
