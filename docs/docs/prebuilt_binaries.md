# Prebuilt Binaries

The Brisk library depends on several third-party libraries (such as libpng, zlib, etc.). Even if you're using prebuilt Brisk binaries, these libraries must still be linked to build the final executable.

Below are instructions for acquiring the necessary binaries to build Brisk applications.

## Acquiring Complete Brisk Binaries

You can download the Brisk library binaries along with all required dependencies:

#### For Brisk Releases

In the *assets* section of the release page on GitHub:

https://github.com/brisklib/brisk/releases

These files contain the Brisk binaries and all necessary dependencies.

#### For Automated (Nightly) Builds

GitHub Actions artifacts include archives with header files and static libraries:

https://github.com/brisklib/brisk/actions

These files include only the Brisk binaries. Please download the Brisk dependencies separately by following the instructions in the README.md file provided in the GitHub Actions artifacts archive.

## Acquiring Only Brisk Dependencies

Alternatively, you can choose to build the Brisk library from source.

This approach is useful if you plan to modify Brisk's source code to suit your needs. Refer to the license text for details on your rights and obligations regarding code modification and redistribution.

To download only the prebuilt dependencies, use the following command. Set the `VCPKG_DEFAULT_TRIPLET` or `VCPKG_TARGET_TRIPLET` environment variable to match your system's architecture (see [List of Available Triplets](GettingStarted.md#vcpkg-triplets)).

```bash
cd brisk/repository
cmake -P acquire-deps.cmake
```

After executing this command, the `vcpkg_exported` and `vcpkg_installed` directories will be created.

The script hashes files affecting dependencies (such as library versions and toolchain settings), generates a combined hash, and downloads the corresponding prebuilt binaries from our build server. If specific binaries are not available on our servers, the script will attempt to build them using vcpkg, unless you specify `-DDEP_BUILD=OFF`:

```bash
# Don't try to build, just download
cmake -DDEP_BUILD=OFF -P acquire-deps.cmake
```

### Setting Up Prebuilt Binaries in CMake

Set the `VCPKG_INSTALLED_DIR` CMake variable to the `vcpkg_installed` directory. Additional variables, such as `CMAKE_TOOLCHAIN_FILE` and `VCPKG_TARGET_TRIPLET`, should be configured according to the vcpkg documentation.
