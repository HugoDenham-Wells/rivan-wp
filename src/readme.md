# Work Task for Rivan
Dev / build / test setup: 
* Ubuntu 24.04 (in WSL)
* ~~With qemu-user aarch64 emulation per this guide: https://azeria-labs.com/arm-on-x86-qemu-user/~~

# TODO
- [x] Do house keeping stuff - organise dirs, etc. 
- [x] Setup GH repo
- [x] Get SIMPLE_RT running and/or building - maybe via qemu user mode emulation? See this guide: https://azeria-labs.com/arm-on-x86-qemu-user/. Doesn't work right now - hopefully can get an x86 binary or try with full system emulation (slow...). Update - got x86 binary, will go ahead with that. 
- [ ] Play with it - where is its data going? 
- [x] Set up build system - get to hello_world for CMD.
- [ ] Build CMD
    - [ ] ...
- [ ] Build XCP_CMD
    - [ ] ...
- [ ] Build XCP_DAQ
    - [ ] ...
- [ ] ???
- [ ] Profit

# Requirements Spec
We will have 4 executables running simultaneously
1. RT_EXE  - The realtime executable which is a proxy for the controls software, this is what we are extracting data from 
2. CMD - RT_EXE connection manager 
3. XCP_CMD - XCP controller for the RT_EXE 
4. XCP_DAQ - The data subscriber (proxy for database) 

## CMD 
1. Manages the connection to the RT_EXE via TCP port 17725 (hard coded), robust to 
connection loss or restart of RT_EXE 
2. Retains setup state of XCP_CMD setup in case of RT_EXE or CMD restart 
3. Transparently sends XCP_CMD packets to the RT_EXE and returns the response to XCP_CMD 
4. Pushes all DAQ packets to the XCP_DAQ process 
 
## XCP_CMD 
1. Sets up the DAQ list of the on the RT_EXE to stream the “measurements” 
SIMPLE_RT_Y.Q_1 and SIMPLE_RT_Y.Q_2, once this complete and the DAQ list is 
streaming - the process exits 

## XCP_DAQ 
1. Prints DAQ data to stdout in decoded form 
2. Persistent data transfer between CMD and XCP_DAQ, i.e. if either process fails while the other is running data cannot be lost

# Questions
* How to handle ever-growing cache of XCP_CMD packets?

# Write up
## Future changes:
With 2 weeks I would need to better understand the use and context of the apps in larger project - how bulletproof are they, who is using them, how are they being used, what's the roadmap in to the larger system. Can gold plate it and sink unbounded time, but does this bring us to 1MW? Anyway, here's the low-hanging fruit: 
* See inline TODOs for low-level stuff
* CMD is a bit of a mess - state machine was already becoming unwieldy + lots of edge cases to catch. A refactor to simplify and streamline would go a long way. Right now I'm sure there are fault conditions we would need to handle that by way of CMDs structure are quite hard to grok. 
* XCP_CMD just bit bashes DAQ packets together and everything is hardcoded. Could make this generic to allow arbitrary DAQs + a UI to support this? 
* A few magic numbers floating around and stylistic things to clean up
* Could do with some doc, linting, static analysis, and *definitely* unit tests for critical stuff
* If this is going to live for a while I would make it more modular - shared headers / modules / defines for maintainability. Right now a few versions of the truth floating around (e.g. hardcoded names of apps in a few locations)
## Fault Handling:
We handle:
* RT_SIMPLE restarts (although CMD just hangs around waiting for it, so it does leave the clients unattended to)
* CMD restarts - XCP_CMD commands are cached and are re-sent to RT_SIMPLE on restart. But it does rely on an initial execution of XCP_CMD.
We don't handle:
* XCP_CMD DAQ setup being interrupted mid-stream
* XCP_CMD stored state being corrupted
* Fault conditions lie: XCP_CMD flooding us with data
* Any sort of security concerns
* Any OS-level auto-restart stuff (e.g. daemons)