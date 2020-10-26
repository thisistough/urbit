final: prev:

let

  isAarch64 = prev.stdenv.hostPlatform.isAarch64;

in prev.lib.optionalAttrs isAarch64 {
  libsigsegv = prev.libsigsegv.overrideAttrs (old: {
    preConfigure = ''
      header "patching configure to set CFG_FAULT=fault-linux-arm.h"

      sed -i 's/^CFG_FAULT=$/CFG_FAULT=fault-linux-arm.h/' configure
    '';
  });
}
