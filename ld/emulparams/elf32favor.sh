SCRIPT_NAME=elf
TEMPLATE_NAME=elf
OUTPUT_FORMAT="elf32-favor"
# stolen from moxie for now
TEXT_START_ADDR=0x1000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
ARCH=favor
# put this at the top of 32 bit address space...?
STACK_ADDR=0xF0000000
