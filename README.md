
# UDP -> TS -> EFP Example

This example uses EFP ([ElasticFramingProtocol](https://bitbucket.org/unitxtra/efp/src/master/)) 

kissnet for recieving UDP packets

and [Akanchis TS-Demuxer](https://github.com/andersc/mpegts) for extracting the ES-streams from MPEG-TS

## Description

This simple example expects a H264 video stream and at least one AAC stream in the MPEG-TS data. The example maps one video and one audio to EFP just as a boiler plate example.

## Build


**All architectures:**

```
cmake .
make
```

**Run the system:**

```
1. If you got no MPEG-TS UDP source use the included streamts.sh script.
2. run ./ts2efp
```

Expected result->


ts2efp prints out information about the data parsed.

## License

*MIT*

Read *LICENCE.md* for details


 