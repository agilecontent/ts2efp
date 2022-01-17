
# UDP -> TS -> EFP -> TS -> UDP

This example uses EFP ([ElasticFramingProtocol](https://github.com/Unit-X/efp)) 

[Kissnet](https://github.com/Ybalrid/kissnet) for sending/recieving UDP packets

and a patched [Akanchis TS-Demuxer](https://github.com/unit-x/mpegts) for muxing/demuxing MPEG-TS.

## Description

This simple example 'ts2efp' expects a H264 video stream and at least one AAC stream in the MPEG-TS data 127.0.0.1:8080. The example maps one video and one audio to EFP and sends the EFP data to 127.0.0.1:8090.

The example 'efp2ts' expects EFP arriving at 127.0.0.1:8090 and multiplexes the data to mpeg-ts and sends it to 127.0.0.1:8100.

## Build


**Release:**

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

***Debug:***

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

**Run the system:**

1. Start a MPEG-TS generator (Example script streamts.sh)
2. start the TS -> EFP program (ts2efp)
3. start the EFP -> TS program (efp2ts)
4. start ffmplay or any other mpeg-ts player udp://127.0.0.1:8100


## License

*MIT*

Read *LICENSE.md* for details