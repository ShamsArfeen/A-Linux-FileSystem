# A-Linux-FileSystem

## Create a filesyntax: CR [filename] [size]

This command should create a file titled filename of the given size. The filename will bean absolute path. If there’s not enough space in the disk, it outputs an error saying ”not enough space”, otherwise it creates a file of the required size.

If a directory in the given path does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory.

If a file with a given pathname already exist, it gives an error ”the file already exists”.

## Delete a filesyntax: DL [filename]

This command deletes a file titled filename. The filename will be an absolute path. 

If a directory in the given path does not exist, it outputs an error message saying ”the directory XXX in the given path does not exist” where XXX is the name of the missing directory. 

If a file with a given pathname does not exist, it gives an error ”the file does not exist”.
