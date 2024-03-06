#okay so this is a bit of a mess.
#bear with me...
#assembling
AS := nasm
ASFLAGS := -f elf
ASFILES := $(wildcard kernel/boot/*.s kernel/asm/*.s)
ASOBJECTS := $(ASFILES:.s=.s.o)
#Compiling C++
CXX := g++
CXXFLAGS := -c -nostdlib -nostdinc -fno-builtin -fno-stack-protector -ffreestanding -m32 -I./kernel/include
CXXFILES := $(wildcard *.cc kernel/*.cc kernel/core/*.cc kernel/module/*.cc)
CXXOBJECTS := $(CXXFILES:.cc=.o)
CXXHEADERS := $(wildcard *.h kernel/include/*.h)
DEPENDS := $(CXXFILES:.cc=.d)

#linking
LD := ld
LDFLAGS := -m elf_i386 -T linker.ld

.PHONY: clean run

#final product will be in build
all: gnOSis.bin
#the final image of the OS
gnOSis.bin: $(ASOBJECTS) $(CXXOBJECTS)
	$(LD) $(LDFLAGS) -o build/$@ $^

#run the kernel
run: build/gnOSis.bin
	@qemu-system-i386 -kernel 'build/gnOSis.bin' 

#Assembly -> object files
%.s.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

#C++ -> object
%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

#Generate dependancy files so make grabs updated headers
%.d: %.cc
	$(CXX) -MM -MD -I./kernel/include -o $@ $<


#Throw it all in the bin
clean: 
	@rm -f $(ASOBJECTS)
	@rm -f $(CXXOBJECTS)
	@rm -f $(DEPENDS)

