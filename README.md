# A-Linux-FileSystem

The program creates a snapshot of filesystem in a file "myfs" and supports 7 basic filesystem commands (see bellow). The layout of the filesystem is stored according to the following scheme:

## Filesystem Layout:

The 128 blocks are divided into 1 super block and 127 data blocks. The superblock contains the 128 byte free block list where each byte contains a boolean value indicating whether that particular block is free or not. Just after the free block list, in the super block, we have the inode table containing the 16 inodes themselves. Each inode is 56 bytes in size and contains metadata about the stored files/directories as indicated by the data structure "inode".

## How to Run:
Simply compile using gcc and run the executable. The executable will take a filename as a cmd argument which will contain commands to execute.
``gcc main.c -o vfs.out``
``./vfs.out [CMDFILE.txt]``

# Supported Commands:

The invoked prompt supports the following commands:

## Create a file: CR [filename] [size]

This command should create a file titled filename of the given size. The filename will bean absolute path. If there’s not enough space in the disk, it outputs an error saying ”not enough space”, otherwise it creates a file of the required size.

If a directory in the given path does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory.

If a file with a given pathname already exist, it gives an error ”the file already exists”.

## Delete a file: DL [filename]

This command deletes a file titled filename. The filename will be an absolute path. 

If a directory in the given path does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory. 

If a file with a given pathname does not exist, it gives an error ”the file does not exist”.

## Copy a file: CP [srcname] [dstname]

This command should copy a file titled srcname to a file titled dstname. The srcname and dstname will be an absolute paths.

If there’s not enough space in the disk, it outputs an error saying ”not enough space”, otherwise it creates a copy of the source file at the destination.

If a directory in the given paths does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory. 

If a file with a given pathname already exist, it overwrites it. If  either srcname or dstname is a directory, it gives an errory saying ”can’t handle directories”.

## Move a file: MV [srcname] [dstname] 

This command should move a file titled srcname to a file titled dstname. The srcname and dstname will be an absolute paths. This command does not fail due to space limitations.

If a directory in the given paths does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory. If a file with a given pathname already exists, it should overwrite it. 

If either srcname or dstname is a directory, it gives an errory saying ”can’t handle directories”.
 

## Create a directory: CD [dirname]

This command creates an  empty  directory  at  the  path  indicated by dirname. The dirname will be an absolute path. 

If a directory in the given path does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory. 

If a directory with the given name with the given path exists, it should give an errory message saying ”the directory already exists”.

## Remove a directory: DD [dirname]

This command should remove the directory at the path indicated by dirname. The dirname will be an absolute path. This is a recursive operation, it removes everything inside the directory from the file system.

If a directory in the given path does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory. If a directory with the given name at the given path does not exist, it gives an errory message saying ”the directory does not exist”.

## List all files: LL 

This command should list all the files/directories on the hard disk along with their sizes.
