## single core 
### cp -r singlecore/sw/* $EE6470/riscv-vp/sw/
### cp -r singlecore/platform/* $EE6470/riscv-vp/vp/src/platform/basic-acc
### cd $EE6470/riscv-vp/vp/build
### cmake ..
### make install
### cd $EE6470/riscv-vp/sw
### make 
### make sim
## multi core 
### cp -r mulcore/sw/* $EE6470/riscv-vp/sw/
### cp -r mulcore/platform/* $EE6470/riscv-vp/vp/src/platform/tiny32-mc/
### cd $EE6470/riscv-vp/vp/build
### cmake ..
### make install
### cd $EE6470/riscv-vp/sw
### make 
### make sim
