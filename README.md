# Bazel Support Plugin for QtCreator

## Description

This plugin adds Bazel project management support to QtCreator.

## Features

- Project navigation with sources grouping by build target;
- C++ code model;
- Build configurations supporting Bazel's "compilation modes";
- Advanced selection of build targets: individual targets, entire packages, or packages recursively;
- Running executables;

## How to Build

### Dependencies

- Install Conan package manager
- Fetch external source code as shown below:

```sh
git -C 3rd_party/bazel config core.sparseCheckout true
cp 3rd_party/bazel_sparse-checkout.txt .git/modules/3rd_party/bazel/info/sparse-checkout
git submodule update --force --checkout 3rd_party/bazel
```

### Configure and start the build
Create a build directory and run

```sh
conan install <path_to_plugin_source>
cmake \
  -D CMAKE_PREFIX_PATH=<path_to_qt_sdk>;<path_to_qtcreator_sdk> \
  -D CMAKE_BUILD_TYPE=RelWithDebInfo \
  <path_to_plugin_source>
cmake --build .
```

where `<path_to_qtcreator_sdk>` is the relative or absolute path to a Qt Creator build directory, 
or to a combined binary and development package (Windows / Linux), or to the 
`Qt Creator.app/Contents/Resources/` directory of a combined binary and development package (macOS), 
and `<path_to_plugin_source>` is the relative or absolute path to this plugin directory.

When setting up a build in QtCreator you can use this to let the IDE put actual paths:
`-D CMAKE_PREFIX_PATH:STRING=%{Qt:QT_INSTALL_PREFIX};%{IDE:ResourcePath}`

### Troubleshooting

- Make sure to run `conan install` before running CMake; otherwise it will miss packages!
- Make sure to use PRECISELY THE SAME version of Qt as the one Qt Creator was built against (see 
  about dialog). Otherwise you may get configuration errors from CMake.
- Make sure QtCreator's build kit has Qt SDK properly set up.
- Export `QT_LOGGING_RULES=qtc.extensionsystem\*=true` before running QtCreator to test the plugin 
  to see extra logs from the plugin manager.
- Qt Creator's CMake target helpers are shit and don't regenerate the plugin metadata JSON when
  the template file changes. So run CMake manually to update it!

## How to Use

Run a compatible Qt Creator with the additional command line argument

    -pluginpath <path_to_plugin>

where `<path_to_plugin>` is the path to the resulting plugin library in the build directory
(`<plugin_build>/lib/qtcreator/plugins` on Windows and Linux,
`<plugin_build>/Qt Creator.app/Contents/PlugIns` on macOS).

You might want to add `-temporarycleansettings` (or `-tcs`) to ensure that the opened Qt Creator
instance cannot mess with your user-global Qt Creator settings.

When building and running the plugin from Qt Creator, you can use these values as the 
`Command line arguments` field in the run settings. On Windows and Linux:

    -pluginpath "%{buildDir}/lib/qtcreator/plugins" -tcs

or this on Mac OS:

    -pluginpath "%{buildDir}/Qt Creator.app/Contents/PlugIns" -tcs

## License
This software is licensed under [The MIT License](https://opensource.org/licenses/mit-license.php).
A copy of its text is included in the LICENSE.txt file nearby.
