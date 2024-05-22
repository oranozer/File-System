#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define blocksize 512
#define entriesfat 4096
#define filelist_items 128
#define filename_len 248

typedef struct {
    unsigned int entries[entriesfat];
} 
FAT; //file allocation table

typedef struct {
    char fileName[filename_len];
    unsigned int firstBlock;
    unsigned int fileSize;
} 
FileEntry;

typedef struct {
    FileEntry entries[filelist_items];
} 
FileList;

FAT fat;
FileList fileList;
FILE *disk;

void FATinitialize(){

for (int a = 1; a < entriesfat; a++) {
        fat.entries[a] = 0x00000000;  // All other entries are 0x00000000
}
fat.entries[0] = 0xFFFFFFFF;  // First entry is always 0xFFFFFFFF
    
    
}

void filelist_initialize(){
for (int i = 0; i < filelist_items; i++) {
    fileList.entries[i].firstBlock = 0; //initialize
    fileList.entries[i].fileSize = 0;   //initialize
    fileList.entries[i].fileName[0] = '\0';  // Empty file name
        
}
}

void writting( const char *name,  const char *source,  const char *destination_name) {
    disk = fopen(name, "r+b"); // open the disk image
    FILE *sourcefile = fopen(source, "rb"); // open the sourcefile

    // search for empty entry in the file list

    int fileindex = -1;
    for (int i = 0; i < filelist_items; i++) {
        if (fileList.entries[i].fileName[0] == '\0') {
            fileindex = i;
            break;
        }
    }

    if (fileindex == -1) {
        printf("there is no empty entry.\n");
        fclose(sourcefile);
        fclose(disk);
        return;
    }

    // Fill in the file list entry
    strncpy(fileList.entries[fileindex].fileName, destination_name, filename_len - 1); //copy the dest file name to list entry.
    fileList.entries[fileindex].fileName[filename_len - 1] = '\0';
    fileList.entries[fileindex].fileSize = 0; //calculate later

    // Write file data to disk
    
    int firstBlock = -1;
    int prevBlock = -1;
    int true =1;
    unsigned char buffer[blocksize]; //temporarily hold the data read from the source file.
    while (true) {
        // Read data from the source file
        size_t bytes_read = fread(buffer, 1, blocksize, sourcefile) ;
        if (bytes_read == 0) break;

        // Find an empty block in the FAT
          int currentBlock = -1;
        for (int i = 1; i < entriesfat; i++) {
            if  (fat.entries[i] == 0x00000000) 
            {
                currentBlock = i;
                break;
            }
        }
        // if does not change the disk is full.
        if (currentBlock == -1) 
         {
            printf("Disk is full. \n");

            fclose(sourcefile);
            fclose(disk);

            return;
         }

        // First Write data to the block
        
         fseek(disk, sizeof(FAT) + sizeof(FileList) + currentBlock * blocksize, SEEK_SET);
         fwrite(buffer, 1, bytes_read, disk);

        // Updating FAT
        if (prevBlock != -1) {
            fat.entries[prevBlock] = currentBlock;
        } else {
            firstBlock = currentBlock;
        }
        prevBlock = currentBlock;

        // Update file list
        fileList.entries[fileindex].fileSize += bytes_read;

        if (bytes_read < blocksize) break;
    }

    //end of the file in FAT.

    fat.entries[prevBlock] =  0xFFFFFFFF ;

    fileList.entries[fileindex].firstBlock = firstBlock;

    //  second Write updated FAT and file list to disk
    fseek(disk, 0, SEEK_SET); //position the file pointer at the start of the disk image.
    
    fwrite(&fat, sizeof(FAT), 1, disk); //write FAT
    fwrite(&fileList, sizeof(FileList), 1, disk); //write FileList

    fclose(sourcefile);
    fclose(disk);

    printf("writting is done.\n");
}

void formatting(const char *name)     {

    disk = fopen(name, "r+b"); //open file
    
    // Initialize FAT and File List
    FATinitialize();
    filelist_initialize();

    // Write FAT and File List to disk
    fseek(disk, 0, SEEK_SET); //from the beginning
    
    fwrite(&fat, sizeof(FAT), 1, disk); // write the initialized FAT structure to the disk
    fwrite(&fileList, sizeof(FileList), 1, disk); //write the initialized file list


    fclose(disk);
}



void reading( const char *name,  const char *source_name,  const char *destPath)   {
    
    disk =  fopen(name, "r+b") ; //Opening the Disk File
    // Find the file in the file list
    int fileindex = -1;

    for (int i = 0; i < filelist_items; i++) {
        if (strcmp(fileList.entries[i].fileName, source_name) == 0) //search for fileName
        //take source file name
        {
            fileindex = i;
            break;
        }
    }

    FILE *destFile = fopen(destPath, "wb"); // open destination file
    
    // Read the file data from the disk
    unsigned int currentBlock = fileList.entries[fileindex].firstBlock; //the starting block of the file.
    unsigned char buffer[blocksize];
    //Read data from the disk into a buffer and then write the buffer to the destination file.
    while (currentBlock != 0xFFFFFFFF) // continue until the end of the file
    {
        fseek(disk, sizeof(FAT) + sizeof(FileList) + currentBlock * blocksize, SEEK_SET); //move the file pointer within the disk image to the correct position .
        
        fread(buffer, 1, blocksize, disk); //read blocksize bytes of data from the disk image into the buffer.
        fwrite(buffer, 1, blocksize, destFile); //write the data stored in buffer to the destination file.

        currentBlock = fat.entries[currentBlock];
    }

    fclose(destFile);
    fclose(disk);
    printf("File read from disk successfully.\n");
}

void deleting( const char *name,  const char *deletingfilename) 
{

    disk = fopen(name, "r+b");

    // Find the file in the file list
    int index = -1;
    for (int i = 0; i < filelist_items; i++) 
     
     {
        if (strcmp(fileList.entries[i].fileName, deletingfilename) == 0) // compare each file name in the list with fileName.
        {
            index = i ;
            break;
        }
    }


    // Free the blocks in FAT
    unsigned int currentBlock = fileList.entries[index].firstBlock;
    while (currentBlock != 0xFFFFFFFF) 
        {
        unsigned int nextBlock = fat.entries[currentBlock];
          
        fat.entries[currentBlock] = 0x00000000;
        currentBlock = nextBlock;
        }

    // Clear the file list entry
    fileList.entries[index].fileSize = 0;
    fileList.entries[index].fileName[0] = '\0';
    fileList.entries[index].firstBlock = 0;
    

    // Write updated FAT and file list to disk
    fseek(disk, 0, SEEK_SET);

    fwrite(&fat, sizeof(FAT), 1, disk);
    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);

    printf("File deleted successfully.\n");
}

void Listing(const char *name) {
        disk = fopen(name, "r+b");

    printf("File Name\t File Size (Bytes)\n");
    printf("//////////////////////////////\n");
    for (int i = 0; i < filelist_items; i++) {
        if (fileList.entries[i].fileName[0] != '\0'&& fileList.entries[i].fileName[0] != '.') // later hiding with adding "."
        {
              printf("%s\t%u\n", fileList.entries[i].fileName, fileList.entries[i].fileSize);
        }
    }

    fclose(disk);
}

void Sorting(const char *name) {

        disk = fopen(name, "r+b");
    
    // file sorting
    for (int a = 0; a < filelist_items - 1; a++) 
    {
        for (int j = 0; j < filelist_items - a - 1; j++) {
            if (fileList.entries[j].fileSize > fileList.entries[j + 1].fileSize) {
                FileEntry temp = fileList.entries[j];
                fileList.entries[j] = fileList.entries[j + 1];
                fileList.entries[j + 1] = temp;
            }
        }
    }

    // Write updated file list to disk
    fseek(disk, sizeof(FAT), SEEK_SET);

    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);
    printf("Files sorted by size.\n");
}

void Renaming(const char *name, const char *source_name, const char *updated_name) {
    disk = fopen(name, "r+b");
    
    // Finding the file in the file list
    int fileindex = -1;
    for (int i = 0; i < filelist_items; i++) {
        if (strcmp(fileList.entries[i].fileName, source_name) == 0) {
            fileindex = i;
            break;
        }
    }

        if (fileindex == -1) {
        printf("File not found.\n");
        fclose(disk);
        return;
    }

    // Rename the file
    
    strncpy(fileList.entries[fileindex].fileName, updated_name, filename_len - 1);
    fileList.entries[fileindex].fileName[filename_len - 1] = '\0';

    // Write updated file list to disk
    fseek(disk, sizeof(FAT), SEEK_SET);

    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);
    printf("File is renamed.\n");
}

void duplicating(const char *name, const char *source_name) {
    disk = fopen(name, "r+b");
    
    // Find the file in the file list

    int fileindex = -1;
        for (int i = 0; i < filelist_items; i++) {
            if (strcmp(fileList.entries[i].fileName, source_name) == 0) {
            fileindex = i;
            break;
        }
    }

    if (fileindex == -1) {
        printf("File not found.\n");
        fclose(disk);
        return;
    }

    char newFileName[filename_len];
    

    // Find an empty entry in the file list
    int newfileindex = -1;
    for (int i = 0; i < filelist_items; i++) {
        if (fileList.entries[i].fileName[0] == '\0') {
            newfileindex = i;
            break;
        }
    }

    if (newfileindex == -1) {
        printf("File list is full.\n");
        fclose(disk);
        return;
    }

    // Copy file list entry
    strncpy(fileList.entries[newfileindex].fileName, newFileName, filename_len - 1);
    fileList.entries[newfileindex].fileName[filename_len - 1] = '\0';
    fileList.entries[newfileindex].fileSize = fileList.entries[fileindex].fileSize;

    // Copy file data block by block
    unsigned int currentBlock = fileList.entries[fileindex].firstBlock;
    int firstBlock = -1, prevBlock = -1, newBlock;
    while (currentBlock != 0xFFFFFFFF) {
        // Find an empty block in the FAT
        newBlock = -1;
        for (int i = 1; i < entriesfat; i++) {
            if (fat.entries[i] == 0x00000000) {
                newBlock = i;
                break;
            }
        }

        if (newBlock == -1) {
            printf("Disk is full.\n");
            fclose(disk);
            return;
        }

        unsigned char buffer[blocksize];
        fseek(disk, sizeof(FAT) + sizeof(FileList) + currentBlock * blocksize, SEEK_SET);
        fread(buffer, 1, blocksize, disk);

        fseek(disk, sizeof(FAT) + sizeof(FileList) + newBlock * blocksize, SEEK_SET);
        fwrite(buffer, 1, blocksize, disk);

        if (prevBlock != -1) {
            fat.entries[prevBlock] = newBlock;
        } else {
            firstBlock = newBlock;
        }
        prevBlock = newBlock;

        currentBlock = fat.entries[currentBlock];
    }

    // the end of the new file in FAT
    fat.entries[prevBlock] = 0xFFFFFFFF;
    fileList.entries[newfileindex].firstBlock = firstBlock;

    // update FAT and file list
    fseek(disk, 0, SEEK_SET);

    fwrite(&fat, sizeof(FAT), 1, disk);
    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);
    printf("File duplicated.\n");
}

void searching(const char *name, const char *searchname) 
{
    disk = fopen(name, "r+b");
    
    int find = 0;
    for (int i = 0; i < filelist_items; i++) {
        if (strcmp(fileList.entries[i].fileName, searchname) == 0) 
        {
            find = 1;
            break;
        }
      }

       if (find) {
        printf("YES\n");
      } else {
        printf("NO\n");
       }

    fclose(disk);
}

void hiding(const char *name, const char *fileName) {
    disk = fopen(name, "r+b");

    // Find the file in the file list
    int fileindex = -1;
    for (int i = 0; i < filelist_items; i++) {
        if (strcmp(fileList.entries[i].fileName, fileName) == 0) {
            fileindex = i;
            break;
        }
    }

    if (fileindex == -1) {
        printf("File not found.\n");
        fclose(disk);
        return;
    }

    // Hide the file by prepending a '.' to its name
    char newName[filename_len]; //space for new name

    snprintf(newName, filename_len, ".%s", fileList.entries[fileindex].fileName) == filename_len; 
    strncpy(fileList.entries[fileindex].fileName, newName, filename_len - 1);
    fileList.entries[fileindex].fileName[filename_len - 1] = '\0';

    // Write updated file list to disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);
    printf("File hidden successfully.\n");
}

void unhiding(const char *name, const char *fileName) {
    disk = fopen(name, "r+b");
    
    // Find the hidden file in the file list
    int fileindex = -1;
    char hiddenName[filename_len];//space for new name
    snprintf(hiddenName, filename_len, ".%s", fileName) == filename_len;

    for (int i = 0; i < filelist_items; i++) {
        if (strcmp(fileList.entries[i].fileName, hiddenName) == 0) {
            fileindex = i;
            break;
        }
    }

    if (fileindex == -1) {
        printf("File not found.\n");
        fclose(disk);
        return;
    }

    // Unhide the file by removing the '.' from its name
    strncpy(fileList.entries[fileindex].fileName, fileName, filename_len - 1);
    fileList.entries[fileindex].fileName[filename_len - 1] = '\0';

    // Write updated file list to disk
    fseek(disk, sizeof(FAT), SEEK_SET);
    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);
    printf("File unhidden successfully.\n");
}

void printing(const char *name)
   {
    disk = fopen(name, "r+b");
    
    FILE *filelistFile = fopen("filelist.txt", "w");
    

    fprintf(filelistFile, "File Name\tFirst Block\tFile Size (Bytes)\n");
    fprintf(filelistFile, "///////////////////////////////////////////////////////\n");
    for (int i = 0; i < filelist_items; i++) {
        fprintf(filelistFile, "%s\t%u\t%u\n", fileList.entries[i].fileName, fileList.entries[i].firstBlock, fileList.entries[i].fileSize);
    }

    fclose(filelistFile);
    fclose(disk);
    printf("filelist.txt is created.\n");
}

void printfat(const char *name) {
    disk = fopen(name, "r+b");
    
    FILE *fatFile = fopen("fat.txt", "w");

    fprintf(fatFile, "Entry\tValue\n");
    fprintf(fatFile, "///////////////\n");

    for (int i = 0; i < entriesfat; i++) {
        fprintf(fatFile, "%04d\t0x%08X\n", i, fat.entries[i]);
    }

    fclose(fatFile);
    fclose(disk);
    printf("fat.txt is created.\n");
}
 
void defragment(const char *name) {
    disk = fopen(name, "r+b");
    
    unsigned char buffer[blocksize];
    unsigned int newFAT[entriesfat];
    unsigned int newBlockIndex = 1;

    for (int i = 0; i < entriesfat; i++) {
        newFAT[i] = 0x00000000;
    }
    newFAT[0] = 0xFFFFFFFF;

    for (int i = 0; i < filelist_items; i++) {
        if (fileList.entries[i].fileName[0] != '\0') {
             
             unsigned int currentBlock = fileList.entries[i].firstBlock;
            unsigned int firstNewBlock = newBlockIndex;
             unsigned int prevNewBlock = 0;

            while (currentBlock != 0xFFFFFFFF) {
                fseek(disk, sizeof(FAT)+sizeof(FileList) + currentBlock * blocksize, SEEK_SET);
                fread(buffer, 1, blocksize, disk);

                fseek(disk, sizeof(FAT) + sizeof(FileList) + newBlockIndex * blocksize, SEEK_SET);
                fwrite(buffer, 1, blocksize, disk);

                if (prevNewBlock != 0) {
                        newFAT[prevNewBlock] = newBlockIndex;
                }

                    prevNewBlock = newBlockIndex;
                    currentBlock = fat.entries[currentBlock];
                    newBlockIndex++;
            }

                newFAT[prevNewBlock] = 0xFFFFFFFF;
                fileList.entries[i].firstBlock = firstNewBlock;
        }
    }

    fseek(disk, 0, SEEK_SET);

    fwrite(newFAT, sizeof(newFAT), 1, disk);
     
    fwrite(&fileList, sizeof(FileList), 1, disk);

    fclose(disk);
    printf("Disk is defragmented.\n");
}





int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./myfs disk command [args...]\n");
        return 1;
    }

    const char *name = argv[1];
    
    const char *command = argv[2];

    // Check if the disk exists before performing any operations
    FILE *diskCheck = fopen(name, "r+b");
    if (diskCheck == NULL) {
        if (strcmp(command, "-format") == 0) {
            formatting(name);
        } 

    } else {
        // Read the FAT and File List if the disk exists
        fread(&fat, sizeof(FAT), 1, diskCheck);
        fread(&fileList, sizeof(FileList), 1, diskCheck);
        fclose(diskCheck);
    }



    if (strcmp(command, "-format") == 0) {
        formatting(name);
    } else if (strcmp(command, "-write") == 0) {
        if (argc != 5) {
            printf("Usage: ./myfs disk -write source_file destination_file\n");
            return 1;
        }
        writting(name, argv[3], argv[4]);
    } else if (strcmp(command, "-read") == 0) {
        if (argc != 5) {
            printf("Usage: ./myfs disk -read source_file destination_file\n");
            return 1;
        }
        reading(name, argv[3], argv[4]);
    } else if (strcmp(command, "-delete") == 0) {
        if (argc != 4) {
            printf("Usage: ./myfs disk -delete file\n");
            return 1;
        }
        deleting(name, argv[3]);
    } else if (strcmp(command, "-list") == 0) {
        Listing(name);
    } else if (strcmp(command, "-sorta") == 0) {
        Sorting(name);
    } else if (strcmp(command, "-rename") == 0) {
        if (argc != 5) {
            printf("Usage: ./myfs disk -rename source_file new_name\n");
            return 1;
        }
        Renaming(name, argv[3], argv[4]);
    } else if (strcmp(command, "-duplicate") == 0) {
        if (argc != 4) {
            printf("Usage: ./myfs disk -duplicate source_file\n");
            return 1;
        }
        duplicating(name, argv[3]);
    } else if (strcmp(command, "-search") == 0) {
        if (argc != 4) {
            printf("Usage: ./myfs disk -search file\n");
            return 1;
        }
        searching(name, argv[3]);
    } else if (strcmp(command, "-hide") == 0) {
        if (argc != 4) {
            printf("Usage: ./myfs disk -hide file\n");
            return 1;
        }
        hiding(name, argv[3]);
    } else if (strcmp(command, "-unhide") == 0) {
        if (argc != 4) {
            printf("Usage: ./myfs disk -unhide file\n");
            return 1;
        }
        unhiding(name, argv[3]);
    } else if (strcmp(command, "-printfilelist") == 0) {
        printing(name); }
        else if (strcmp(command, "-printfat") == 0) {
        printfat(name);
    } else if (strcmp(command, "-defragment") == 0) {
        defragment(name);}
        else {
        printf("Unknown: %s\n", command);
        return 1;
    } 
      

    return 0;
}
