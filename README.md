# File-System
a simple user mode file system (FS) that employs a file allocation table (FAT)

## Compile

```bash
gcc myfs.c -o myfs -lm
```

## Create a Disk Image
```bash
dd if=/dev/zero of=disk.image bs=2146304 count=1
```
## Commands
1. Formatting: Overwrites the disk header with an empty FAT and an empty file list.
```bash
./myfs disk –format
```
2. Writing: Copies a file to the disk.
```bash
./myfs disk -write source_file destination_file
```
3. Reading: Copies a file from the disk.
```bash   
./myfs disk -read source_file destination_file
```
4. Deleting: Deletes a file in the disk.
```bash
./myfs disk -delete file
```
5. Listing: Prints all visible files
```bash
./myfs disk –list
```
6. Sorting: Sorts files by their sizes in an ascending order.
```bash
./myfs disk –sorta
```
7. Renaming: Renames a file in the disk.
```bash
./myfs disk –rename source_file new_name
```
8. Duplicating: Duplicates a file in the disk.
```bash
./myfs disk –duplicate source_file
```
9. Searching: Searches for a file in the disk, and gives “YES” or “NO” result.
```bash
./myfs disk -search source_file
```
10. Hiding: Hides a file in the disk.
```bash
./myfs disk –hide source_file
```
11. Unhiding: Unhides a file in the disk.
```bash
./myfs disk –unhide source_file
```
12. Printing File List: Prints File List to the “filelist.txt” file.
```bash
./myfs disk –printfilelist
```
13. Printing FAT: Prints FAT to the “fat.txt” file.
```bash
./myfs disk –printfat
```
14. Disk Defragmentation: Merges fragmented files into one contiguous space or block.
```bash
./myfs disk –defragment
```
