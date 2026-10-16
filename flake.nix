{
  description = "Development environment for a Bison-based JSX parser";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nix-vite-plus.url = "github:ryoppippi/nix-vite-plus";
  };

  outputs = { nixpkgs, nix-vite-plus, ... }:
    let
      systems = [
        "aarch64-darwin"
        "x86_64-darwin"
        "aarch64-linux"
        "x86_64-linux"
      ];
      forEachSystem = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forEachSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              bison
              cmake
              ninja
              llvmPackages.clang
              llvmPackages.clang-tools
              nix-vite-plus.packages.${system}.vp
            ];

            shellHook = ''
              export BISON_PKGDATADIR="${pkgs.bison}/share/bison"
              eval "$(vp env print)"
            '';
          };
        });
    };
}
