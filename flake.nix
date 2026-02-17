{
  description = "NorthstarLauncher - MSVC cross build";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    flake-utils.url = "github:numtide/flake-utils";

    treefmt-nix.url = "github:numtide/treefmt-nix";

    self.submodules = true; # flakes don't copy submodles by default so we need this
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ ];
          config = {
            allowUnsupportedSystem = true;
            allowUnfree = true;
            microsoftVisualStudioLicenseAccepted = true;
          };
        };

        cross = pkgs.pkgsCross.x86_64-windows;

        base = cross.windows.sdk;
        arch = "x64";

        toolchainFile =
          let
            MSVC_INCLUDE = "${base}/crt/include";
            MSVC_LIB = "${base}/crt/lib";
            WINSDK_INCLUDE = "${base}/sdk/Include";
            WINSDK_LIB = "${base}/sdk/Lib";
            mkArgs = args: builtins.concatStringsSep " " args;
            linker = mkArgs [
              "/manifest:no"
              "-libpath:${MSVC_LIB}"
              "-libpath:${WINSDK_LIB}/ucrt/${arch}"
              "-libpath:${WINSDK_LIB}/um/${arch}"
              "-libpath:${WINSDK_LIB}/${arch}"
              "-libpath:${MSVC_LIB}/${arch}"
            ];
            compiler = mkArgs [
              "/vctoolsdir ${cross.windows.sdk}/crt"
              "/winsdkdir ${cross.windows.sdk}/sdk"
              "/EHs"
              "-D_CRT_SECURE_NO_WARNINGS"
              "--target=x86_64-windows-msvc"
              "-fms-compatibility-version=19.11"
              "-imsvc ${MSVC_INCLUDE}"
              "-imsvc ${WINSDK_INCLUDE}/ucrt"
              "-imsvc ${WINSDK_INCLUDE}/shared"
              "-imsvc ${WINSDK_INCLUDE}/um"
              "-imsvc ${WINSDK_INCLUDE}/winrt"
            ];
          in
          pkgs.writeText "WindowsToolchain.cmake" ''
            set(CMAKE_SYSTEM_NAME Windows)
            set(CMAKE_SYSTEM_VERSION 10.0)
            set(CMAKE_SYSTEM_PROCESSOR x86_64)
            set(IS_NIX_ENV 1) # for a check somewhere down the line

            set(CMAKE_C_COMPILER "clang-cl")
            set(CMAKE_CXX_COMPILER "clang-cl")
            set(CMAKE_AR "llvm-lib")
            set(CMAKE_LINKER "lld-link")
            set(CMAKE_RC_COMPILER "llvm-rc")

            set(CMAKE_C_FLAGS "${compiler}")
            set(CMAKE_CXX_FLAGS "${compiler}")
            set(CMAKE_EXE_LINKER_FLAGS "${linker}")

            set(CMAKE_C_STANDARD_LIBRARIES "${compiler}")
            set(CMAKE_CXX_STANDARD_LIBRARIES "${compiler}")
            set(CMAKE_SHARED_LINKER_FLAGS "${linker}")
            set(CMAKE_MODULE_LINKER_FLAGS "${linker}")

            set(CMAKE_C_COMPILER_WORKS 1)
            set(CMAKE_CXX_COMPILER_WORKS 1)

            message(STATUS "MSVC_LIB: ${MSVC_LIB}")
            message(STATUS "WINSDK_LIB: ${WINSDK_LIB}")

            include_directories(${MSVC_INCLUDE})
            include_directories(${WINSDK_INCLUDE}/ucrt)
            include_directories(${WINSDK_INCLUDE}/shared)
            include_directories(${WINSDK_INCLUDE}/um)
            include_directories(${WINSDK_INCLUDE}/winrt)


            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

            set(CMAKE_VERBOSE_MAKEFILE ON)
          '';

        mkBuildDir = /* bash */ ''
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${toolchainFile} -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        '';
    	mkBuildDirShell = /*bash*/ ''
    		${pkgs.bear} -- ${mkBuildDir} -DCMAKE_EXPORT_COMPILE_COMMANDS=1
    	'';

        lib = pkgs.lib;

        # Eval the treefmt modules from ./treefmt.nix
        treefmtEval =
          pkgs:
          treefmt-nix.lib.evalModule pkgs (
            { ... }:
            {
              # Used to find the project root
              projectRootFile = "flake.nix";

              # Add formaters for some other langs
              # programs.clang-format.enable = true; # doesn't format correctly yet
              programs.nixfmt.enable = true;

              # settings
              settings.formater.clang-format.args = [
                "-i"
                "--style=file"
                "--exclude=primedev/include primedev/*.cpp primedev/*.h"
              ];
            }
          );
      in
      {
        # for `nix fmt`
        formatter = (treefmtEval pkgs).config.build.wrapper;
        # for `nix flake check`
        checks = {
          formatting = (treefmtEval pkgs).config.build.check self;
        };

        packages = {
          northstar = pkgs.stdenv.mkDerivation {
            pname = "NorthstarLauncher";
            version = "0.0.0";

            src = self;

            nativeBuildInputs = with pkgs; [
              cross.buildPackages.cmake
              cross.buildPackages.ninja
              cross.buildPackages.msitools
              llvmPackages.clang-unwrapped
              llvmPackages.bintools-unwrapped
              perl
              pkg-config
              git
            ];

            buildInputs = [
              # cross.zlib # TODO: need upstream fixes for this # but also northstar is vendoring zlib so let it vendor it self
              cross.windows.sdk
            ];

            dontUseCmakeConfigure = true;
            phases = [
              "unpackPhase"
              "patchPhase"
              "postPatch"
              "buildPhase"
              "installPhase"
            ];

            postPatch =
              let
                zlib = pkgs.fetchFromGitHub {
                  owner = "R2Northstar";
                  repo = "zlib";
                  rev = "9f0f2d4f9f1f28be7e16d8bf3b4e9d4ada70aa9f";
                  hash = "sha256-PL6lH7I4qGduaVTR1pGfXUjpZp41kUERvGrqERmSoNQ=";
                };
              in
              ''
                mkdir -p $TMPDIR/cloned
                zlib_src=$TMPDIR/cloned/zlib

                cp -r ${zlib} "$zlib_src"

                chmod +rw "$zlib_src"

                substituteInPlace primedev/thirdparty/minizip/CMakeLists.txt \
                  --replace "clone_repo(zlib https://github.com/madler/zlib)" "
                  set(ZLIB_SOURCE_DIR $zlib_src)
                  set(ZLIB_BINARY_DIR $zlib_src)
                  	"
              '';

            buildPhase = ''
              mkdir -p build

              ${mkBuildDir}

              cmake --build build/
            '';

            installPhase = ''
              mkdir -p $out
              cp -r build/game $out
            '';

            meta = {
              description = "Northstar launcher";
              homepage = "https://northstar.tf/";
              license = lib.licenses.mit;
              mainProgram = "NorthstarLauncher";
              platforms = [ "x86_64-linux" ];
              maintainers = [ ];
            };
          };
          default = self.packages.${system}.northstar;
        };

        devShells = {
          no-auto-build = pkgs.mkShellNoCC {
            nativeBuildInputs = with pkgs; [
              cross.buildPackages.cmake
              cross.buildPackages.ninja
              cross.buildPackages.msitools
              llvmPackages.clang-unwrapped
              llvmPackages.bintools-unwrapped
              perl
              cross.zlib
              pkg-config

              (pkgs.writeShellScriptBin "rebuild-ns" ''
                rm -rf build/
                mkdir -p build

                ${mkBuildDirShell}
                cmake --build build/
              '')
              (pkgs.writeShellScriptBin "init-ns" mkBuildDirShell)
              (pkgs.writeShellScriptBin "build-ns" "cmake --build build/")
            ];

            buildInputs = [
              cross.windows.sdk
            ];

            shellHook = ''
              cp -f ${pkgs.writeText ".clangd" ''
                CompileFlags:
                  CompilationDatabase: "cmake"
              ''} .clangd
              echo "Northstar shell init"
              echo "    init-ns: setups the build dir for cmake and stuff"
              echo "    build-ns: incrementally re/builds northstar"
              echo "    rebuild-ns: builds northstar"
            '';
          };
          default = self.devShells.${system}.no-auto-build;
        };
      }
    );
}
