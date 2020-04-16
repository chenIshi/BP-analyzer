# BP-analyzer
Using Intel Pintool to analyze branch prediction

# Quick start guide
1. Download Pintool from source
2. Copy bp_analyzer to pin/source/tools/MyPinTool, then modify `makefile.rule` script target
3. Run `make TARGET=intel64`
4. Take `tar` as testbench, run with `../../../pin -t obj-intel64/bp_analyzer.so -- /bin/tar`  
The output log name will be `output.log` in current directory