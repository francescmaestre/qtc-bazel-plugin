# Bazel Support Plugin for QtCreator

## Description

This plugin adds Bazel project management support to QtCreator.

## Features

- Project navigation with sources grouping by build target;
- C++ code model;
- Build configurations supporting Bazel's "compilation modes";
- Advanced selection of build targets: individual targets, entire packages, or packages recursively;
- Running executables;

## How to build the plugin

### Dependencies

- Install Conan package manager
- Fetch external source code as shown below:

```sh
git -C 3rd_party/bazel config core.sparseCheckout true
cp 3rd_party/bazel_sparse-checkout.txt .git/modules/3rd_party/bazel/info/sparse-checkout
git submodule update --force --checkout 3rd_party/bazel
```

### Configure and start the build
Create a build directory, enter it, then run

```sh
conan install <path_to_plugin_source>
cmake -D CMAKE_PREFIX_PATH=<path_to_qt_sdk>;<path_to_qtcreator_sdk> <path_to_plugin_source>
cmake --build .
```

Here, `<path_to_qtcreator_sdk>` should point to the Qt Creator SDK directory. On Mac OS this is
`.../Qt Creator.app/Contents/Resources/`. 
And `<path_to_plugin_source>` is the relative or absolute path to this repo directory.

When setting up a build in QtCreator you can use this to let the IDE put actual paths:
`-D CMAKE_PREFIX_PATH:STRING=%{Qt:QT_INSTALL_PREFIX};%{IDE:ResourcePath}`

### Troubleshooting

- Make sure to run `conan install` before running CMake; otherwise it will miss packages!
- Make sure to use PRECISELY THE SAME version of Qt as the one Qt Creator was built against (see its
  "About" dialog). Otherwise you may get configuration errors from CMake.
- Make sure QtCreator's build kit has Qt SDK properly set up.
- Export `QT_LOGGING_RULES=qtc.extensionsystem\*=true` before running QtCreator to test the plugin 
  to see extra logs from the plugin manager.
- Qt Creator's CMake target helpers are shit and don't regenerate the plugin metadata JSON when
  the template file changes. So run CMake manually to update it!

### Dev testing
Run a compatible Qt Creator with the additional command line argument

    -pluginpath <path_to_plugin>

where `<path_to_plugin>` is the path to the resulting plugin library in the build directory
(`<build_dir>/lib/qtcreator/plugins` on Windows and Linux,
`<build_dir>/Qt Creator.app/Contents/PlugIns` on macOS).

You might want to add `-temporarycleansettings` (or `-tcs`) to ensure that the opened Qt Creator
instance cannot mess with your user-global Qt Creator settings.

When building and running the plugin from Qt Creator, you can use these values as the 
`Command line arguments` field in the run settings. On Windows and Linux:

    -pluginpath "%{buildDir}/lib/qtcreator/plugins" -tcs

or this on Mac OS:

    -pluginpath "%{buildDir}/Qt Creator.app/Contents/PlugIns" -tcs

## Installing the plugin
Run this command to install to the IDE plugins location:
```sh
cmake --install <path_to_build_dir>
```

Alternatively, you can install from the IDE itself. Open the "About Plugins" dialog window, click 
the "Install Plugin" button, point the input to the built plugin DLL/SO/DyLib file, click "Continue"
and chose where you want to install the plugin.

## Usage

### Opening a Bazel project
1. Open the File menu
2. Select the "Open file or project" item
3. Navigate the open dialog to the project
4. Select the WORKSPACE file and click Open
5. Back in the IDE window select build kits and compile modes and click "Configure Project"
Once your project completes parsing you should see it appear in the project explorer. Bazel files 
and directories containing packages should be marked with the Bazel logo. Packages should contain 
buildable targets, with the later listing its source files.

### Building Bazel targets
You should be able to trigger a build right after opening a project. By default your build 
configuration is created to build all project targets. To change that, go to the Projects view, 
expand the build step in your build configuration, and use the Targets list to check/uncheck your 
project's packages or individual targets to your liking. A checked package builds all targets 
underneath (corresponds to "/...:all" in Bazel's CLI), whereas a partially checked package builds 
just its immediate targets (corresponds to ":all"). You can also specify additional CLI arguments 
via the input field nearby.

### Running Bazel targets
The plugin automatically creates run configurations per each executable target of your project. Just
select the one you need from the Build/Run selector, or in the "Run Suttings" of the Projects view.
There you can also specify command line arguments if needed.

### Debugging Bazel targets
...

## License
This software is licensed under [The MIT License](https://opensource.org/licenses/mit-license.php).
A copy of its text is included in the LICENSE.txt file nearby.
