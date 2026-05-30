{
	description = "Declarations for the environment that this project will use.";

	# Flake inputs
	inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

	# Flake outputs
	outputs = inputs:
		let
			# The systems supported for this flake
			supportedSystems = [
				"x86_64-linux" # 64-bit Intel/AMD Linux
				"aarch64-linux" # 64-bit ARM Linux
				"x86_64-darwin" # 64-bit Intel macOS
				"aarch64-darwin" # 64-bit ARM macOS
			];

			# Helper to provide system-specific attributes
			forEachSupportedSystem = f: inputs.nixpkgs.lib.genAttrs supportedSystems (system: f {
				pkgs = import inputs.nixpkgs { inherit system; };
			});
		in
		{
			devShells = forEachSupportedSystem ({ pkgs }: {
				default = pkgs.mkShell {
					# The Nix packages provided in the environment
					# Add any you need here
					packages = with pkgs; [
						gcc
						bear
						gdb
						gum
						universal-ctags
						cppcheck
						doxygen
						clang-tools  # provides clang-format and clangd
					] ++ lib.optionals stdenv.isLinux [ valgrind ];

					# Set any environment variables for your dev shell
					env = { };

					# Add any shell logic you want executed any time the environment is activated
					shellHook = ''
					'';
				};
			});
		};
}
