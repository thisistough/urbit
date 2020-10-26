final: prev:

let

  optionalsNull = xs: prev.lib.optionals (xs != null) xs;

in {
  h2o = prev.h2o.overrideAttrs (_old: {
    version = final.sources.h2o.rev;
    src = final.sources.h2o;
    outputs = [ "out" "dev" "lib" ];
  });

  libsigsegv = prev.libsigsegv.overrideAttrs (old: {
    patches = optionalsNull old.patches ++ [
      ../pkgs/libsigsegv/disable-stackvma_fault-linux-arm.patch
      ../pkgs/libsigsegv/disable-stackvma_fault-linux-i386.patch
    ];
  });

  curlMinimal = prev.curl.override {
    http2Support = false;
    scpSupport = false;
    gssSupport = false;
    ldapSupport = false;
    brotliSupport = false;
  };
}
