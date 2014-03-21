RamDisk
=======

RamDisk for CS552

# To make the kernel module
$> make

# To make the test file for kernel mode
# Define _KERNEL_MODE in source files
$> make kernel

# To make the test file for user mode
# De-define _KERNEL_MODE in source files
$> make user

# To test
$> ./test_fs




