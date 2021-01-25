#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>



/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */

#define FILENAME_MAXLEN 8  // including the NULL char
#define MAX_BLK 8

/* 
 * inode 
 */

typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [MAX_BLK];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;

/* 
 * directory entry
 */

typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;


// ------------------------------------ my globals ---------------------------------
#define BLOCK_SIZE 1024
#define BLOCK_COUNT 128
#define TOTAL_INODES (BLOCK_SIZE - BLOCK_COUNT) / sizeof(inode)
int myfs;
int indent;

/*
 * functions
 */

// create file -- 			CR()
// copy file -- 			CP()
// remove/delete file -- 	DL()
// move a file -- 			MV()
// list file info -- 		LL()
// create directory -- 		CD()
// remove a directory -- 	RD()

// -------------------------------- INIT FILE SYSTEM ------------------------------------
int FS() {
/* Creates myfs file desriptor with initial state. Initial state has null-characters filled
 and only root directory with a dot dir-entry
*/
	myfs = open("./myfs", O_RDWR | O_CREAT); // create it if not found
	if (myfs == -1) {
		printf("error: cannot create ./myfs\n");
		return 1;
	}

	ftruncate(myfs, BLOCK_SIZE * BLOCK_COUNT); // filled with null characters '\0'
	
	char VerySimpleFileSystem = 'S'; // first byte in myfs indentifies this filesystem format with char 'S'
	write(myfs, (char*)&VerySimpleFileSystem, 1);

	char dBitmap = (char)1; // first block alloted to root directory
	write(myfs, (char*)&dBitmap, 1);

	struct inode rootinode;
	rootinode.dir = 1;  // boolean value. 1 if it's a directory.
	strcpy(rootinode.name, "/");
	rootinode.size = sizeof(dirent);  // actual file/directory size in bytes.
	rootinode.blockptrs[0] = 1;  // direct pointers to blocks containing file's content.
	rootinode.used = 1;  // boolean value. 1 if the entry is in use.
	rootinode.rsvd = 0;  // reserved for future use

	lseek(myfs, BLOCK_COUNT, SEEK_SET);
	write(myfs, (char*)&rootinode, sizeof(inode));

	struct dirent rootdirent;

	strcpy(rootdirent.name, "."); // dot direntry for root dir
	rootdirent.namelen = 1;  // length of entry name
	rootdirent.inode = 0; 

	lseek(myfs, BLOCK_SIZE, SEEK_SET);
	write(myfs, (char*)&rootdirent, sizeof(dirent));
	return myfs;
}

int GetParentInode(char *filename) {
/*
 Returns the last directory containing the target file/dir specified in absolute path filename
*/
	if (strcmp(filename,"/")==0 ) return -2;
	char idir[100];
	int dirnode = 0;
	struct inode rdirinode;
	struct inode rinode;
	struct dirent rdirent;
	
	while (sscanf(filename, "/%[^/]%s", idir, filename) == 2) {
		lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
		read(myfs, (char*)&rdirinode, sizeof(inode));
		int dirsize = rdirinode.size;

		for(int i=0; i<dirsize; i+=sizeof(dirent)) {
			lseek(myfs, BLOCK_SIZE * rdirinode.blockptrs[0] + i, SEEK_SET);
			read(myfs, (char*)&rdirent, sizeof(dirent));
			if (strcmp(rdirent.name, idir) == 0) {
				
				lseek(myfs, BLOCK_COUNT + rdirent.inode * sizeof(inode), SEEK_SET);
				read(myfs, (char*)&rinode, sizeof(inode));
				if (rinode.dir == 1) goto DIR_MATCHED;
			}
		}
		printf("error: the directory %s in the given path does not exist\n", idir);
		return -1; 
	DIR_MATCHED:
		dirnode = rdirent.inode;
	}
	sscanf(filename, "/%s", filename);
	return dirnode;
}

int ScanDir(int dirnode, char *filename, int isdir, int *block, int *mynode) {
/*
 Searches target file/dir name in the given directory. Returns its inode if found.
*/
	struct inode rinode;
	struct dirent rdirent;
	int ret = -1;

	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&rinode, sizeof(inode));
	int dirsize = rinode.size;
	*block = rinode.blockptrs[0];
	for(int i=0; i<dirsize; i+=sizeof(dirent)) {
		lseek(myfs, BLOCK_SIZE * (*block) + i, SEEK_SET);
		read(myfs, (char*)&rdirent, sizeof(dirent));
		if (strcmp(rdirent.name, filename) == 0 ) {
			ret = -2;
			*mynode = rdirent.inode;
			struct inode rinode;
			lseek(myfs, BLOCK_COUNT + rdirent.inode * sizeof(inode), SEEK_SET);
			read(myfs, (char*)&rinode, sizeof(inode));
			if (rinode.dir == isdir) return i;
	}	}
	return ret;
}

int DeleteDirEntry(int dirnode, int direntry) {
/*
 Deletes dir-entry in the given directory and also subtracts its size.
*/

	struct inode rinode;
	struct dirent rdirent;

	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&rinode, sizeof(inode));
	int dirsize = rinode.size;
	int blockj = rinode.blockptrs[0];

	if (direntry != dirsize - sizeof(dirent)) {
		lseek(myfs, BLOCK_SIZE * blockj + dirsize - sizeof(dirent), SEEK_SET);
		read(myfs, (char*)&rdirent, sizeof(dirent));

		lseek(myfs, BLOCK_SIZE * blockj + direntry, SEEK_SET);
		write(myfs, (char*)&rdirent, sizeof(dirent));
	}

	rinode.size = dirsize - sizeof(dirent);
	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&rinode, sizeof(inode));
	return 0;
}

int DangerAddDirEntry(int dirnode, int mynode, char *filename) {
/*
 Adds dir-entry in the given directory and also adds its size. This function unlike ScanDirAndUpdate()
 does not checks if a file of same name already exist in the directory.
*/

	struct inode rinode;
	struct dirent rdirent;

	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&rinode, sizeof(inode));
	int dirsize = rinode.size;
	int blockj = rinode.blockptrs[0];

	strcpy(rdirent.name, filename);
	rdirent.namelen = strlen(filename);
	rdirent.inode = mynode;

	lseek(myfs, BLOCK_SIZE * blockj + dirsize, SEEK_SET);
	write(myfs, (char*)&rdirent, sizeof(dirent));

	rinode.size = dirsize + sizeof(dirent);
	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&rinode, sizeof(inode));
	return 0;
}

int ScanDirAndUpdate(int dirnode, int mynode, char *filename, int isdir) {
/*
 Searches target file/dir name in the given directory. Then writes it into directory if not found.
*/
	struct inode rinode;
	struct inode rinode2;
	struct dirent rdirent;

	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&rinode, sizeof(inode));
	int dirsize = rinode.size;
	int blockj = rinode.blockptrs[0];
	for(int i=0; i<dirsize; i+=sizeof(dirent)) {
		lseek(myfs, BLOCK_SIZE * blockj + i, SEEK_SET);
		read(myfs, (char*)&rdirent, sizeof(dirent));
		if (strcmp(rdirent.name, filename) == 0) {

			
			lseek(myfs, BLOCK_COUNT + rdirent.inode * sizeof(inode), SEEK_SET);
			read(myfs, (char*)&rinode2, sizeof(inode));
			if (rinode2.dir == isdir) {
				if (isdir == 1) printf("error: the directory already exists\n");
				else printf("error: the file already exists\n");
				return -1;
			}
	}	}

	lseek(myfs, BLOCK_SIZE * blockj + dirsize, SEEK_SET);
	rdirent.namelen = strlen(filename);
	strcpy(rdirent.name, filename);
	rdirent.inode = mynode;
	write(myfs, (char*)&rdirent, sizeof(dirent));

	rinode.size += sizeof(dirent);
	lseek(myfs, BLOCK_COUNT + dirnode * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&rinode, sizeof(inode));	
	return 0;
}

int FindEmptyInode() {
/*
 Searches for an unused inode address
*/
	struct inode rinode[TOTAL_INODES];

	lseek(myfs, BLOCK_COUNT, SEEK_SET);
	read(myfs, (char*)rinode, TOTAL_INODES * sizeof(inode));
	for (int i=0; i<TOTAL_INODES; i++) {
		if (rinode[i].used == 0) return i;
	}
	printf("error: all inodes occupied\n");
	return -1;
}

int FindEmptyBlocks(int *blocks, int n) {
/*
 Searches for n number of unused blocks
*/
	int iblock=1;
	char isoccupied[BLOCK_COUNT];
	lseek(myfs, 0, SEEK_SET);
	read(myfs, isoccupied, 128);
	for (int i=0; i<n; i++) {
		while(iblock <= BLOCK_COUNT) {
			if (isoccupied[iblock] != (char)1) goto FOUND_BLOCK;
			iblock++;
		}
		printf("error: not enough space\n");
		return -1;
FOUND_BLOCK:
		blocks[i] = iblock;
		iblock++;
	}
	return 0;
}

// -------------------------------- CREATE DIRECTORY ------------------------------------
int CD( char *dirname) {
/*
 Creates a new directory specified in absolute path after checking for all restrictions.
*/

	int dirnode = GetParentInode(dirname); // only continue if there is no missing directory in path
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: root directory already present\n"); return 1;}
	
	int myblock;
	int ret = FindEmptyBlocks(&myblock, 1); // only continue if free blocks available
	if (ret == -1) return 1;

	int mynode = FindEmptyInode(); // only continue if free inode available
	if (mynode == -1) return 1;
	
	ret = ScanDirAndUpdate(dirnode, mynode, dirname, 1); // only continue if target was not already present in last directory
	if (ret == -1) return 1;


	struct inode directorynode;
	directorynode.dir = 1;  // boolean value. 1 if it's a directory.
	strcpy(directorynode.name, dirname);
	directorynode.size = 0;  // actual file/directory size in bytes.
	directorynode.blockptrs[0] = myblock;  // direct pointers to blocks containing file's content.
	directorynode.used = 1;  // boolean value. 1 if the entry is in use.
	directorynode.rsvd = 0;  // reserved for future use

	char one = (char)1;
	lseek(myfs, myblock, SEEK_SET);
	write(myfs, &one, 1);

	lseek(myfs, BLOCK_COUNT + mynode * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&directorynode, sizeof(inode));

	ScanDirAndUpdate(mynode, mynode, ".", 1);
	ScanDirAndUpdate(mynode, dirnode, "..", 1);

	return 0;
}

// -------------------------------- CREATE FILE ------------------------------------
int CR(char *filename, int size) {
/*
 Creates a new file specified in absolute path after checking for all restrictions.
*/

	struct inode filenode;

	int dirnode = GetParentInode(filename); // only continue if there is no missing directory in path
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: root directtory is invalid filename\n"); return 1;}
	
	int blockcount = (size/BLOCK_SIZE) + (size%BLOCK_SIZE!=0);
	if (blockcount > MAX_BLK) { // file size cannot be greater than MAX_BLK blocks
		int maxblk = MAX_BLK;
		printf("error: filesize exceeding limit of %d KB\n", maxblk);
		return 1; }

	int ret = FindEmptyBlocks(filenode.blockptrs, blockcount); // only continue if free blocks available
	if (ret == -1) return 1;

	int mynode = FindEmptyInode(); // only continue if free inode available
	if (mynode == -1) return 1;
	
	ret = ScanDirAndUpdate(dirnode, mynode, filename, 0); // only continue if target was not already present in last directory
	if (ret == -1) return 1;


	filenode.dir = 0;  // boolean value. 1 if it's a directory.
	strcpy(filenode.name, filename);
	filenode.size = size;  // actual file/directory size in bytes.
	filenode.used = 1;  // boolean value. 1 if the entry is in use.
	filenode.rsvd = 0;  // reserved for future use

	lseek(myfs, BLOCK_COUNT + mynode * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&filenode, sizeof(inode));

	char one = (char)1;
	char blockdata[BLOCK_SIZE];
	int buffsize = BLOCK_SIZE;
	for (int i=0; i<blockcount; i++) { // fills each block with a-z characters repeated
		lseek(myfs, filenode.blockptrs[i], SEEK_SET);
		write(myfs, &one, 1);

		if (size>BLOCK_SIZE) size-=BLOCK_SIZE;
		else buffsize = size;

		for (int k=0; k<buffsize; k++) 
			blockdata[k] = (char) (97 + ((k + i * BLOCK_SIZE) % 26));

		lseek(myfs, BLOCK_SIZE * filenode.blockptrs[i], SEEK_SET);
		write(myfs, blockdata, buffsize); 
	}

	return 0; //mynode;
}


// ---------------------------------- RECURSIVE DELETE --------------------------
void RD(int node) {
/*
 Deletes whole part of filesystem tree branching from given inode.
*/
	struct inode rinode;
	lseek(myfs, BLOCK_COUNT + node * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&rinode, sizeof(inode));

	rinode.used = 0;
	lseek(myfs, BLOCK_COUNT + node * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&rinode, sizeof(inode));
	
	char nullchar = '\0';
	char blockdata[BLOCK_SIZE];
	for (int k=0; k<BLOCK_SIZE; k++) blockdata[k] = nullchar;

	if (rinode.dir == 0) {

		int isize=rinode.size;
		int blockcount = rinode.size / BLOCK_SIZE;
		if (rinode.size > blockcount * BLOCK_SIZE) blockcount+=1;

		for(int i=0; i<blockcount; i++) {
			int j = rinode.blockptrs[i];

			lseek(myfs, j, SEEK_SET);
			write(myfs, &nullchar, 1);

			lseek(myfs, BLOCK_SIZE * j, SEEK_SET);
			if (isize>BLOCK_SIZE) {write(myfs, blockdata, BLOCK_SIZE); isize-=BLOCK_SIZE;}
			else write(myfs, blockdata, isize);
		}
	}
	else {

		lseek(myfs, rinode.blockptrs[0], SEEK_SET);
		write(myfs, &nullchar, 1);


		struct dirent rdirent;
		for (int i= 2*sizeof(dirent); i<rinode.size; i+=sizeof(dirent)) {
			// printf("%d\n", i);
			lseek(myfs, BLOCK_SIZE * rinode.blockptrs[0] + 2 * sizeof(dirent), SEEK_SET);
			read(myfs, &rdirent, sizeof(dirent));
			if (strcmp(rdirent.name,".")!=0 && strcmp(rdirent.name,"..")!=0) 
				RD( rdirent.inode);
		}

		lseek(myfs, BLOCK_SIZE * rinode.blockptrs[0], SEEK_SET);
		write(myfs, &blockdata, BLOCK_SIZE);
}	}


// -------------------------------- DELETE FILE ------------------------------------
int DL(char *filename) {
/*
 Deletes file specified in given absolute path after chacking for all restrictions
*/

	int dirnode = GetParentInode(filename);// checks if path valid
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: root directory is invalid argument\n"); return 1;}

	int block, mynode;
	int entry = ScanDir(dirnode, filename, 0, &block, &mynode); // searches for targets inode
	if (entry == -1 || entry == -2) {
		printf("error: the file does not exist\n");
		return 1;
	}
	
	DeleteDirEntry(dirnode, entry); 

	RD(mynode);
	return 0;
}


// -------------------------------- DELETE DIRECTORY ------------------------------------
int DD( char *dirname) {
/*
 Deletes directory with its contents after chacking for all restrictions
*/

	int dirnode = GetParentInode(dirname); // checks if path valid
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: cannot delete root directory\n"); return 1;}

	int block, mynode;
	int entry = ScanDir(dirnode, dirname, 1, &block, &mynode); // searches for targets inode
	if (entry == -1 || entry == -2) {
		printf("error: the directory does not exist\n");
		return 1;
	}
	
	DeleteDirEntry(dirnode, entry); 

	RD(mynode);
	return 0;
}


// -------------------------------- MOVE FILE ------------------------------------
int MV(char *srcname, char *destname) {
/*
 Moves and renames the given file. Only directory entries are changed in the process and the target 
 filename in the inode is renamed. Blocks containing source file data are not touched. If a file of 
 the same name already exists in the destination, it is deleted.
*/

	int dirnode = GetParentInode(srcname); // checks if path valid
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: root directory is invalid srcname\n"); return 1;}

	int block, mynode;
	int entry = ScanDir(dirnode, srcname, 0, &block, &mynode); // searches for targets inode
	if (entry == -1) {
		printf("error: the file does not exist\n");
		return 1;
	}
	else if (entry == -2) {
		printf("error: cannot handle directories\n");
		return 1;
	}
	

	int dirnode2 = GetParentInode(destname); // checks if path valid
	if (dirnode2 == -1) return 1;
	if (dirnode2 == -2) {printf("error: root directory is invalid destname\n"); return 1;}

	int block2, mynode2;
	int entry2 = ScanDir(dirnode2, destname, 0, &block2, &mynode2); // searches if targets inode already there
	if (entry2 == -2) {
		printf("error: cannot handle directories\n");
		return 1;
	}
	else {

		struct inode rinode;
		lseek(myfs, BLOCK_COUNT + mynode * sizeof(inode), SEEK_SET);
		read(myfs, (char*)&rinode, sizeof(inode));

		strcpy(rinode.name,destname);
		lseek(myfs, BLOCK_COUNT + mynode * sizeof(inode), SEEK_SET); // rename the name attribute in src file's inode
		write(myfs, (char*)&rinode, sizeof(inode));

		if (entry2 == -1) { // no file of same name exist
			DangerAddDirEntry(dirnode2, mynode, destname); // just update directory entries

			DeleteDirEntry(dirnode, entry);
			return 0;
		}
		else { // already existing file is deleted

			struct dirent rdirent;
			lseek(myfs, BLOCK_SIZE * block2 + entry2, SEEK_SET);
			read(myfs, (char*)&rdirent, sizeof(dirent));

			RD(rdirent.inode); // delete already existing file
			rdirent.inode = mynode; // just change 'inode' attribute of this dir-entry

			lseek(myfs, BLOCK_SIZE * block2 + entry2, SEEK_SET);
			write(myfs, (char*)&rdirent, sizeof(dirent));

			DeleteDirEntry(dirnode, entry);

			return 0;
		}
	}
}

int CopyBlocks(int *src, int *dest, int count) {
/*
 Copies data from given source blocks into destination blocks.
*/
	char blockdata[BLOCK_SIZE];

	for (int i=0; i<count; i++) {

		lseek(myfs, BLOCK_SIZE * src[i], SEEK_SET);
		read(myfs, blockdata, BLOCK_SIZE);

		lseek(myfs, BLOCK_SIZE * dest[i], SEEK_SET);
		write(myfs, blockdata, BLOCK_SIZE);
	}
	return 0;
}

// -------------------------------- COPY FILE ------------------------------------
int CP(char *srcname, char *destname) {
/*
 Creates a new copy of source file and also gives it the specified location and name. Already existing
 file of the same name is deleted.
*/

	int dirnode = GetParentInode(srcname);
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: root directory is invalid srcname\n"); return 1;}

	int block, mynode;
	int entry = ScanDir(dirnode, srcname, 0, &block, &mynode);
	if (entry == -1) {
		printf("error: the file does not exist\n");
		return 1;
	}
	else if (entry == -2) {
		printf("error: cannot handle directories\n");
		return 1;
	}

	dirnode = GetParentInode(destname);
	if (dirnode == -1) return 1;
	if (dirnode == -2) {printf("error: root directory is invalid destname\n"); return 1;}

	int block2, mynode2;
	int entry2 = ScanDir(dirnode, destname, 0, &block2, &mynode2);

	// creating copy of the file
	struct inode rinode;
	lseek(myfs, BLOCK_COUNT + mynode * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&rinode, sizeof(inode));

	int blockcount = (rinode.size/BLOCK_SIZE) + (rinode.size%BLOCK_SIZE != 0);
	int blocks[blockcount];
	for (int i=0; i<blockcount; i++) blocks[i] = rinode.blockptrs[i];

	strcpy(rinode.name, destname);
	mynode = FindEmptyInode();
	if (mynode == -1) return 1;
	int ret = FindEmptyBlocks(rinode.blockptrs, blockcount);
	if (ret == -1) return 1;

	if (entry2 == -2) {
		printf("error: cannot handle directories\n");
		return 1;
	}
	else if (entry2 != -1) { // already existing file is deleted

		struct dirent rdirent;
		lseek(myfs, BLOCK_SIZE * block2 + entry2, SEEK_SET);
		read(myfs, (char*)&rdirent, sizeof(dirent));

		RD(rdirent.inode);
		DeleteDirEntry(dirnode, entry2); 
	}

	lseek(myfs, BLOCK_COUNT + mynode * sizeof(inode), SEEK_SET);
	write(myfs, (char*)&rinode, sizeof(inode));
	CopyBlocks(blocks, rinode.blockptrs, blockcount); // copies data from blocks
	DangerAddDirEntry(dirnode, mynode, destname);

	return 0;
}

void printTree(int node) {
/*
 Prints all filesystem contents with tree hierarchy
*/
	struct inode Rirnode;
	lseek(myfs, BLOCK_COUNT + node * sizeof(inode), SEEK_SET);
	read(myfs, (char*)&Rirnode, sizeof(inode));
	for(int i=0;i<indent;i++) printf("|   ");
	if(Rirnode.dir==1) printf("[%s], size=%d ", Rirnode.name, Rirnode.size);
	else printf("(%s), size=%d ", Rirnode.name, Rirnode.size);
	printf("blocks={");
	int blockcount = Rirnode.size / BLOCK_SIZE;
	if (Rirnode.size > blockcount * BLOCK_SIZE) blockcount+=1;
	for (int i=0; i<blockcount; i++) printf("%d,", Rirnode.blockptrs[i]);
	printf("}\n");
	indent+=1;
	if (Rirnode.dir == 1) {
		for (int i=0; i<Rirnode.size; i+=sizeof(dirent)) {
			struct dirent Rdirent;
			lseek(myfs, BLOCK_SIZE * Rirnode.blockptrs[0] + i, SEEK_SET);
			read(myfs, (char*)&Rdirent, sizeof(dirent));
			if (strcmp(Rdirent.name,".")!=0 && strcmp(Rdirent.name,"..")!=0) printTree(Rdirent.inode);
		}
	}
	indent-=1;
}

void DEBUG() {
	printf("\n-------- Printing Filesystem Tree --------\n");
	indent=0; 
	printTree( 0);
	printf("--------------------------------------------\n");
}

void LL() {
	printf("\n");
	struct inode allnode[TOTAL_INODES];
	lseek(myfs, BLOCK_COUNT, SEEK_SET);
	read(myfs, allnode, BLOCK_SIZE - BLOCK_COUNT);
	for(int i=0; i<TOTAL_INODES; i++) {
		if (allnode[i].used==1) printf("%s %d\n", allnode[i].name, allnode[i].size);
	}
}

void parse_and_execute(char* iLin) {
	char Com[3], argA[1000], argB[1000], Rem[1000];
	int Num;
	sscanf(iLin,"%[^ \n] %[^\n]", Com, Rem);

	int ret = 0;

	if (strcmp(Com, "DB") == 0) { 		// prints filesystem tree hierarchy
		DEBUG();
	}
	else if (strcmp(Com, "FS") == 0) { 	// creates new filesystem with only root directory
		FS();
	}
	else if (strcmp(Com, "LL") == 0) {
		//LL();
		DEBUG();
	}
	else if (strcmp(Com, "CR") == 0) {
		sscanf(Rem, "%[^ ] %d\n", argA, &Num);
		ret = CR(argA, Num);
	}
	else if (strcmp(Com, "CD") == 0) {
		sscanf(Rem, "%[^\n]", argA);
		ret = CD(argA);
	}
	else if (strcmp(Com, "MV") == 0) {
		sscanf(Rem, "%[^ ] %[^\n]", argA, argB);
		ret = MV(argA, argB);
	}
	else if (strcmp(Com, "CP") == 0) {
		sscanf(Rem, "%[^ ] %[^\n]", argA, argB);
		ret = CP(argA, argB);
	}
	else if (strcmp(Com, "DL") == 0) {
		sscanf(Rem, "%[^\n]", argA);
		ret = DL(argA);
	}
	else if (strcmp(Com, "DD") == 0) {
		sscanf(Rem, "%[^\n]", argA);
		ret = DD(argA);
	}
	
	if (ret == 1) {
		char ans;
		printf("failed: %s\ndo you wish to continue? [y/n]", iLin);
		fscanf(stdin, "%s", &ans);
		if (ans == 'n') exit(0);
	}
	else printf("success: %s\n", iLin);
}


/*
 * main
 */
int main (int argc, char* argv[]) {

	FILE * stream = fopen(argv[1], "r");

	myfs = open("./myfs", O_RDWR); // read-write enabled
	if (myfs == -1) myfs = FS();

	char iLin[1000];
	while ( fgets(iLin, 1000, stream) ){
		if (iLin[0] != '#' && iLin[0] != '\n')
			parse_and_execute(iLin);
  	}

	fclose(stream);
	close(myfs);
	return 0;
}

