final: prev: {
  h2o = prev.h2o.overrideAttrs (_old: {
    version = final.sources.h2o.rev;
    src = final.sources.h2o;
    outputs = [ "out" "dev" "lib" ];
  });

  libsigsegv = prev.libsigsegv.overrideAttrs (old: {
    patches = optionalsNull old.patches ++ [
      ../pkgs/libsigsegv/disable-stackvma_fault-linux-arm.patch
      ../pkgs/libsigsegv/disable-stackvma_fault-linux-i386.patch
      ../pkgs/libsigsegv/disable-stackvma_fault-linux-x86_64-old.patch
      ../pkgs/libsigsegv/undefined-stack_top.patch
    ];
  });

  curlMinimal = prev.curl.override {
    http2Support = false;
    scpSupport = false;
    gssSupport = false;
    ldapSupport = false;
    brotliSupport = false;
  };

  # perl = prev.perl.overrideAttrs (old:
  #   prev.lib.optionalAttrs prev.stdenv.hostPlatform.isDarwin {
  #     # Fix no-sys-dirs-5.31.patch failing to be applied on darwin.
  #     patches = [ ];
  #   });
}
