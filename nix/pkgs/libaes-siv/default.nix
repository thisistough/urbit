{ lib, stdenv, openssl, enableParallelBuilding ? true }:

stdenv.mkDerivation {
  name = "libaes-siv";
  src = lib.cleanSource ../../../pkg/libaes-siv;

  buildInputs = [ openssl ];

  installFlags = [ "PREFIX=$(out)" ];

  inherit enableParallelBuilding;
}
