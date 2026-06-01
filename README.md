# Small LAN server environment for Linux

- backend server running on C, build on top of Linux network API, no third-party dependencies
- frontend browser UI running on vanilla Html + Css + JS

> While the server itself is for Linux only, Windows users can easily run it inside [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
>
> - If you use Windows 10, I would recommend downgrading to WSL version 1 in order avoid network connection loss issues when downloading anything. 
> - On Windows 11 both version 1 and 2 are fine

If you face connection issues between your local machines, you might need to pick a VPN tool such as
- ZeroTier
- TailScale
- WireGuard

or similar


## Table of contents

- [<u>Features</u>](#features)
- [<u>Running the server</u>](#serverconf)
- [<u>Building with make</u>](#building-with-make)
- [<u>Adding a new script</u>](#adding-a-new-script)
- [<u>Improvements</u>](#improvements)
- [<u>References</u>](#references)


## Features

Might be missing something here, but these should cover the most important ones:

#### Server
- uploading, downloading and deleting (POST, GET, DELETE)
- arbitrary file sizes and formats
    - transfer operations are streamed in chunks: this allows seamless delivery on even large file sizes
downloading/previewing
- scripting support
    - scripts APIs are created in `script_api.c`
    - each has 
        - name
        - description
        - script execution path
        - a struct for args
    - each arg requires
        - name
        - type (either `"text"` or `"select"`)
        - description
        - required check: `1` if this field has to be filled, `0` if optional
        - if type is `"select"`, list of pre-defined choices; otherwise `NULL` for free user input
    - final struct is then used for building a json response. With this data, javascript can dynamically generate a simple web form for each script 
- threaded client operations
    - this means file uploading, downloading and running scripts don't directly interfere
     with one another. 
        - sometimes script outputs might get buffered, but should mostly work as expected. Not entirely sure why this
        happens, but seems to be related to server load when multiple expensive operations run concurrently e.g. a few large 
        downloads mixed with a longer script or two
    - simple mutex+locking for preventing race conditions with uploads, deletes and job worker threads
- cookie-based token authentication: this serves as a small but nonetheless useful security layer
- `config/server.conf` file for updating port and auth token
    - **remember to update the token for actual use and do not upload it to the server!**
- routes are listed in `src/http/routing.c`. Here you can change which routes require auth permissions (change `AUTH` to `PUBLIC`)

#### Web UI
- login/logout with auth token. Cookies don't define Max-Age/Expires attribute which means they get deleted when current sessions ends. This can mean various lengths, even infinite lifetime; see [this MDN section](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Cookies#removal_defining_the_lifetime_of_a_cookie) for more details
- fully asynchronous to take advantage of server-side threading
- run scripts and see their output in a window. Multiple scripts can be run simultaneously
    - small extra feature: change terminal text color between 6 pre-defined colors
- files:
    - uploading supports multiple files (browse+select or drag & drop) and displays progress
        - each upload queue includes its own progress bar with a cancellation button
    - display all uploaded server files in a list-like structure which auto-updates on file changes. Files include metadata such as
        - name
        - type
        - size
        - upload date (technically 'last modified' but files are always copied and override existing ones with same name)
    - each file has `Preview`, `Download` and `Delete` buttons
        - previews don't automatically work for all file formats, but do support the common ones. Both `src/http/mime.c` and `public/app.js -> TEXT_FORMATS` can be used for expanding this functionality
    - simple search and sorting (name/type/size/date + ascending/descending)
- run scripts and see their output in a separate window. 
    - multiple scripts can be run simultaneously and their output will not interfere with one another
    - small extra feature: 6 pre-defined text colors which you can easily edit in index.html (button visuals) + style.css (the text itself)

        
## Running the server

#### server.conf

To set port and login password token, edit `config/server.conf`:

    PORT=8080
    TOKEN=secret-token

Make sure to update TOKEN value: even though this is just a lan server tool, it never hurts to have an additional safety layer.

#### start the server

You can simply run server like with `make server` (this executes ./bin/lan-server)

You can also move lan-server file elsewhere, just make sure you have all required directory dependencies then run it as executable.

For example if your root is called `my-server` then you need

```
my-server/  
  lan-server (executable)
  config/ (server.conf defines port + auth token)
  uploads/ (file storage)
  public/ (Web UI; optional, but most likely want this)
```

Also it's important you run executable relative to root dir, otherwise server can't find config file. For example: if your executable is still at "bin/lan-server", you need to use command `./bin/lan-server` from root; simply cd'ing into "bin" and running ./lan-server fails.

=> to keep it simple: either use `make server` or have executable in root dir.

#### connecting to server

You can first test connecting via ip normally. If this doesn't work, you can always rely on VPNs listed at the start of this document.

In general:

- if you run server on localhost (= host and users on same machine), use `http://localhost:PORT_NUMBER/`
- otherwise make sure your host server machine and main PC are in same local network, then connect to host via local IPv4 address e.g. `http://192.168.1.1:PORT_NUMBER/`
    
    Here you replace the ipv4 and port your host address + the port you defined in server.conf. To find ip, use one of the following commands:
    - Windows: 
        - cmd -> `ipconfig`, find line `IPv4 Address ... : 192.168.x.x`
        - powershell -> `Get-NetIPAddress`
    - Linux:
        - bash -> `hostname -I`


## Building with make

Project uses a simple Makefile:

- `make build` builds src into `bin/lan-server`
- `make server` starts the server with `./bin/lan-server`

This makes modification of server source files very simple.

Make sure to `chmod +x lan-server` for file execution rights


## Adding a new script

In this example I use my [PixelRay](https://github.com/j-miet/PixelRay) ray tracer to add a server script which can generate gif animations from json and lua files. It reads the inputs from server's `uploads` directory and produces the output gif into same location.

---

In `api/script_api.c`, the are two structs: `ScriptField script_test` and `ScriptEntry scripts`:

```C
ScriptField script_test[] = {{"input", "text", "Write something here", 1, {NULL}},
                            {"message", "select", "Pick a message", 0, {"hello!", "bye!", NULL}},
                            {NULL}};

ScriptEntry scripts[] = {
    { "test", "./scripts/test.sh", "Test script", script_test},
    { NULL, NULL, NULL, NULL}
};
```

- ScriptEntry lists all available scripts. Each requires name, path, description and a ScriptField struct for passed args. Final entry must be a list of NULLs so that the build-in json parser knows where to stop parsing.
- ScriptField lists each args as a list. List includes 
    - arg name
    - type ("text" for free input, "select" for a dropdown select with pre-defined values)
    - description
    - required check: 1 is input is required, 0 if optional
    - "select" type options. If type is "text", then replace this array with `{NULL}`. Otherwise list all options and finish with a NULL value e.g. in script_test, two values are used, "hello!" and "bye!", which generates array like this: `{"hello!", "bye!", NULL}`

Then ScriptEntry is build into a json string which gets fed to web ui page where app.js builds the scripting UI dynamically based on these values.

Now we'll add a new script called **pixelrayGif** into ScriptEntry: this script runs the PixelRay executable which requires inputs for
- static scene creation file (json)
- script file for animations (lua)
- amount of animation frames (integer)

Script API can't perform further type validations, it only sees string values.  The script itself should perform safety checks if necessary (more on this later).

In this case, each input field should take a free input so all will use type "text". Each is also required in order to run the script so set the 4. value as `1` which corresponds to boolean true.

Thus ScriptField becomes like this:

```C
ScriptField pixelrayGif[] = {{"input", "text", "scene file <scene>.json", 1, {NULL}},
                             {"lua", "text", "lua script file <script>.lua", 1, {NULL}},
                             {"frames", "text", "animation frame count", 1, {NULL}},
                             {NULL}};
```
Script itself runs as `pixelrayGif.sh` so ScriptEntry becomes like this:


```C
ScriptEntry scripts[] = {
    { "test", "./scripts/test.sh", "Test script", script_test},
    { "pixelrayGif", "./scripts/pixelrayGif.sh", "Generate a gif animation", pixelrayGif},
    { NULL, NULL, NULL, NULL}
};
```

where `pixelrayGif.sh` looks like this:

```bash
#!/bin/bash

# custom 'external/PixelRay' dir added for PixelRay executable and copied temp files
if [ ! -d "external/PixelRay" ]; then
    echo "Couldn't find PixelRay dir"
    exit 1
fi

cd "./external/PixelRay"

# use basename: this parses path and ends up with only the file name in order to avoid paths like "../"
# this script will not check validity of file formats (scene is .json, lua is .lua) because PixelRay does this already
SCENE=$(basename "$1")
LUA=$(basename "$2")

# abort script if user tries to input 'pixelray' in any casing. This is to later prevent
# - overriding executable when temp file gets copied
# - deletion of executable when temp is removed (technically this can never even happen if override condition is properly checked)
if [ "${SCENE,,}" = "pixelray" ] || [ "{$LUA,,}" = "pixelray" ]; then
    echo "Can't use name 'pixelray' in scene or script files"
    exit 1
fi

# check that passed frame value is an integer
FRAMES="$3"
if [[ ! $FRAMES =~ ^-?[0-9]+$ ]]; then
    echo "frames must be an integer value"
    exit 1
fi

# to avoid complicating file paths and adding possible security issues: 
# - allow only file names in paths (this was done earlier with basename)
# - copy the scene and script files from 'uploads' dir into 'external/PixelRay'
# - run script using direct file names
# - move output gif to 'uploads'
# - clean up by deleting PixelRay's auto-generated frame images in 'frames' dir + the temp copies of used scene and lua script
cp ../../uploads/$SCENE $SCENE
if [ $? != 0 ]; then
    echo "Scene file copying failed"
    exit 1
fi

cp ../../uploads/$LUA $LUA
if [ $? != 0 ]; then
    echo "Lua script file copying failed"
    exit 1
fi

chmod +x PixelRay
./PixelRay -i $SCENE -s $LUA $FRAMES -g -u 0

mv "outputGIF.gif" "../../uploads"

if [ ! -d "frames" ]; then
    echo "Couldn't find frames dir after script completion!"
    exit 1
else
    rm -r "frames"
fi

rm -- "$SCENE"
rm -- "$LUA"
```

After a script has been added: 
- close and rebuild the server with `make build`
- reopen the server
- refresh web UI and you should see the button `pixelrayGif` under scripts. Clicking this opens the auto-generated input form similarly to `test` script where you can pass args and execute the script

## Improvements

Things that could be added/improved:

#### Priority

- css: add login.html styling + a couple smaller improvements to index.html
    - also try to move most of the inline css from index.html and app.js into style.css
        for better control
- move file searching to server-side
- pagination (or some other measure to limit loaded file count if server eventually has hundreds/thousands of files)
- mobile-friendly web UI
- scripting API: add queueing and cancellation
- some unit tests + an integration test or two

#### Secondary

- add sql database support, either a self-build minimal db engine or just sqLite, and then
    - file tags for custom grouping
    - server history logs: uploads and jobs
    - a separate page for bookmarks with add, delete and tags
- add file IDs so urls don't include names anymore
- mobile-friendly web UI
- scripting API: replace polling with websockets
- file thumbnails/icons (easy to add simple "string" icons, but what about a more versatile system)
- upgrade from http to https


## References

#### C (network) programming
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [The Open Group](https://pubs.opengroup.org/onlinepubs/7908799/)
- [Linux man-pages](https://man7.org/linux/man-pages/)
- [cppreference](https://en.cppreference.com/c/header) (standard lib headers)

#### Web

[MDN Web docs](https://developer.mozilla.org/en-US/docs/Web)

In particular
- [HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [Response status codes](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status)
- [Common media types (MIME)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/MIME_types/Common_types)
