final: prev:

let

  isMusl = prev.stdenv.hostPlatform.isMusl;

  optionalsNull = xs: prev.lib.optionals (xs != null) xs;

  overrideStdenv = pkg: pkg.override { stdenv = prev.gcc9Stdenv; };

in prev.lib.optionalAttrs isMusl {
  libsigsegv = prev.libsigsegv.overrideAttrs (old: {
    patches = optionalsNull old.patches
      ++ [ ../pkgs/libsigsegv/sigcontext-redefined-fix.patch ];
  });

  secp256k1 = prev.secp256k1.overrideAttrs (old: {
    nativeBuildInputs = optionalsNull old.nativeBuildInputs
      ++ [ prev.buildPackages.stdenv.cc ];
  });

  rhash = overrideStdenv prev.rhash;

  numactl = overrideStdenv prev.numactl;

  lmdb = overrideStdenv prev.lmdb;
}
