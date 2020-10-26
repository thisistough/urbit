final: prev:

let

  isMusl = prev.stdenv.hostPlatform.isMusl;

  optionalsNull = xs: prev.lib.optionals (xs != null) xs;

  overrideStdenv = pkg: pkg.override { stdenv = prev.gcc9Stdenv; };

in prev.lib.optionalAttrs isMusl {
  libsigsegv = prev.libsigsegv.overrideAttrs (old: {
    postConfigure = ''
      substituteInPlace config.h \
        --replace '#define CFG_FAULT "fault-linux-x86_64-old.h"' \
                  '#define CFG_FAULT "fault-linux-i386.h"'
    '';
  });

  secp256k1 = prev.secp256k1.overrideAttrs (old: {
    nativeBuildInputs = optionalsNull old.nativeBuildInputs
      ++ [ prev.buildPackages.stdenv.cc ];
  });

  rhash = overrideStdenv prev.rhash;

  numactl = overrideStdenv prev.numactl;

  lmdb = overrideStdenv prev.lmdb;
}
