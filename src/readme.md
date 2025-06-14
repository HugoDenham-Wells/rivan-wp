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
