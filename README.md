# hyperion-webos_libvtcapture

### What is this? 
This is a fork from Mariotaku's hyperion-webos (https://github.com/webosbrew/hyperion-webos) to support the newer TV generation starting early 2020.
On earlyer TVs libvt and libgm could be used, to capture the video and UI of the TV. Now LG started to deploy a new library called libvtcapture. For UI libhalgal is used. 
This executeable captures both buffers, combines them and after that sends it to the hyperion flatbuffer server.

I'm not a real programmer and all this stuff is really new to me. The only reason this exists, is because I just badly wanted it for any cost. Feel free to create an issue or pull request if you can make things better.  

Take a look here, if you are just looking for an app that can simply be installed on your TV: https://github.com/TBSniller/piccap

### How to use
First you should check if your TV has the libraries. You can check it with  
`find / | grep libvtcapture`
If you get something like this
>/usr/lib/libvtcapture.so.1
>/usr/lib/libvtcapture.so.1.0.0  

You should be good to go. **I think root will be also requierd, I never tested it without.**  

Since LG started to do some more permission handling, the binary must be able to talk to the libvtcapture library. Take a look at the `setlibvtcapturepermission.sh` to get an idea of that, change the binary-path to your needs and run it, before starting the executeable. 

 Usage:  
 > hyperion-webos -a ADDRESS [OPTION]...  
 >  -x, --width      |     Width of video frame (default 360)  
>  -y, --height     |     Height of video frame (default 180)  
>  -a, --address    |     IP address of Hyperion server  
>  -p, --port     |       Port of Hyperion flatbuffers server (default 19400)  
>  -f, --fps      |       Framerate for sending video frames (default unlimited)  
>  -V, --no-video   |     Video will not be captured  
>  -G, --no-gui     |     GUI/UI will not be captured  


### How to build
You have to install the NDK from here: https://github.com/webosbrew/meta-lg-webos-ndk/releases 
And after that do the first steps: https://github.com/webosbrew/meta-lg-webos-ndk#development-instructions

You should then be able to compile it like `cmake --build /home/USER/vscode/hyperion-webos_libvtcapture/build --config Debug --target hyperion-webos_libvtcapture -- -j 10`  
Specifying the target is neccessary.

### Not working
Limiting the FPS isn't supported at this time.
