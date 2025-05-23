# DroneEngage Camera Module

This module uses native webrtc to stream videos from multiple camera to [WebClient](https://github.com/DroneEngage/droneengage_webclient "Weblient").


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/youtube_video_streaming.png?raw=true)](https://www.youtube.com/watch?v=hf5onZ2-7V4)

# Compile The Code


## for X86

first you need webrtc compiled code to get libwebrtc.a 
you need to use branch 

`git ls-remote https://chromium.googlesource.com/external/webrtc --heads branch-heads/4606`

this is an old branch and you need **Ubuntu 18.04** to build it.

Once you have **libwebrtc.a ** just put it in  **/lib/webrtc94-local/lib/x64/Debug** and run 

you can download a compiled version from [here](https://drive.google.com/file/d/10KvinexvvDRUgd6afGLAbMCgCr_h8yM6/view?usp=sharing "here")
`make debug`

## for Arm

We compile de_camera as part of the WEBRTC project itself. We compiled it as an example next to the examples that already exist.

use BUILD.gn file in this project to overwrite the example BUILD.gn in webrtc source, and add the current project file inside **src** in a folder called de_camera similar to:

/home/webrtc/webrtc/webrtc-checkout/src/examples/de_camera


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/virtual_machine_de_camera.png?raw=true)](https://github.com/DroneEngage/droneengage_camera/blob/master/res/virtual_machine_de_camera.png?raw=true)

then compile webrtc and it will compile project de_camera as part of the project.
then you can use it in RPI or JetsonNano

## To Install Scripts for Camera

Please check [Script Section](https://github.com/DroneEngage/droneengage_camera/blob/master/scripts/README.md "Script Section")




    sudo apt-get install nlohmann-json3-dev
    sudo apt-get install libjsoncpp-dev # Install the development package
    sudo apt-get install libjpeg-dev  # Install the development package
    sudo apt-get install libx11-dev libexpat1-dev 

    sudo apt-get install clang 
    sudo apt-get install libc++-dev


## How to prepare WEBRTC environment

On Ubuntu 18.04

    conda create -n webrtc_4606 python=2.7
    conda env activate webrtc_4606
    
    
    mkdir webrtc_4606
    
    wget https://raw.githubusercontent.com/webrtc-uwp/chromium-build/master/install-build-deps.sh
    chmod +x ./install-build-deps.sh
    ./install-build-deps.sh --no-chromeos-fonts
    
    fetch --nohooks webrtc
    git checkout refs/remotes/branch-heads/4606

### To Compile for X86

    python3 build/linux/sysroot_scripts/install-sysroot.py --arch=x86
    
    gn gen out/x64_4606 --args='is_debug=true  rtc_include_tests=false treat_warnings_as_errors=false use_rtti=true is_component_build=false  enable_iterator_debugging=false   is_clang=false use_sysroot=false linux_use_bundled_binutils=false use_custom_libcxx=false use_custom_libcxx_for_host=false target_os="linux"  target_cpu="x64"'


### To compile for RPI-4 / RPI-Z2 W

    python3 build/linux/sysroot_scripts/install-sysroot.py --arch=arm64
    
    python build/linux/sysroot_scripts/install-sysroot.py --arch=arm64
    gn gen out/arm64_72 --args='is_debug=false  enable_iterator_debugging=false   treat_warnings_as_errors=false rtc_include_tests=false target_os="linux"  target_cpu="arm64" is_clang=true rtc_exclude_audio_processing_module=true rtc_use_h264=true ' 
    
