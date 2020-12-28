

# FsMega
FsMega is a Total Commander plugin that allows you to access files stored in Mega cloud accounts.

The plugin works for both 32-bit and 64-bit variants of Total Commander, on Windows 7 and newer.

## Building
1. Download the Mega SDK from https://github.com/meganz/sdk
2. Follow the instructions to build the Mega SDK, both in 32-bit and 64-bit variants.
3. Create a folder that will contain the Mega SDK and all the required dependencies. This folder needs to have the following structure:
	- include: copy the contents of the include folder from the Mega SDK
	- lib32: will contain the libraries for the 32-bit version of the Mega SDK
		- Debug: copy the debug version of Mega.lib, and also its dependencies (cryptopp, libsodium, sqlite, zlib, etc.)
		- Release: copy the release version of Mega.lib, and also its dependencies
	- lib64: will contain the libraries for the 64-bit version of the Mega SDK
		- Debug: copy the debug version of Mega.lib, and also its dependencies
		- Release: copy the release version of Mega.lib, and also its dependencies
4. Create an environment variable called `MEGA_SDK`, its value should point to the folder that you created in step 3.
5. Get an API key from https://mega.nz/sdk. Create an environment variable called `MEGA_API_KEY`, its value should be your API key
6. Open FsMega.sln with Visual Studio 2019, and build the plugin. If the two environment variables are correctly defined, it should build the plugin without problems.
7. Once you have built the Release version of the plugin, for both 32-bit and 64-bit, you can run the `pack.cmd` file to build a zip archive that contains the plugin. You can open this archive with Total Commander and it will ask you to install the plugin.