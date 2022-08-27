# TFS
A tag based file system written in fuse

## OVERVIEW
tfs acts as an overlay for given directory and creates a enviroment that enforces tag based file browsing
### usage
```./tfs DIRECTORY_TO_USE_AS_ROOT MOUNT_POINT```

## BUILDING
### dependancies
* libfuse3 (building from source recommended)
* libsqlite3
* libssl
### Makefile
Adjust the makefile to point to the appropriate libfuse3 location and simply
```
mkdir obj bin example
make
```
**AS OF NOW BUILDING THE UTILITIES (I.E lstag, mktag, rmtag, tag) DOES NOT INSTALL THEM, HOWEVER DUE TO THE FACT THAT tfs ASSUMES NO SUBDIRECTORIES, RUNNING THE UTILIES BY AS ../mktag WILL BE SUFFICE.**

## UTILITIES
tfs provides a three utilities analogous to GNU's ls, mkdir and rm (lstag, mktag, rmtag respectfully) and "tag" which attaches tags to given files
### mktag, rmtag
creates and removes given tags
```
../mktag TAGS_TO_CREATE...
../rmtag TAGS_TO_REMOVE...
```
### lstag
shows files that have ALL given tags attached
```
../lstag TAGS_TO_LIST...
```
### tag
attaches tags to a given file
```
../tag FILE TAG_TO_ATTACH...
```

## PREVIEW
### creating files
<img width="1968" alt="FILE CREATION" src="https://user-images.githubusercontent.com/4114533/187020304-5fcbb480-dafd-45fc-a294-274f1059b015.png">
tfs calculates a file's sha256 hash on creation and sets its filename as such, as a small side effect of this procedure the directory tfs manages never contains duplicate files.

### listing tags

an example output of lstag
<img width="868" alt="LSTAG" src="https://user-images.githubusercontent.com/4114533/187020398-8f02b687-611a-49c9-96b7-2550ff04d42d.png">

lstag with multiple tags as parameters
<img width="870" alt="SEARCHING TAGS" src="https://user-images.githubusercontent.com/4114533/187020425-62cf845f-081e-4e35-b7c6-a95aa9bbf18c.png">

### tagging files
<img width="1192" alt="TAGGING" src="https://user-images.githubusercontent.com/4114533/187020468-a7c5437e-ef07-4587-8a29-a88d421dbeed.png">

### removing tags
<img width="880" alt="REMOVE TAGS" src="https://user-images.githubusercontent.com/4114533/187020472-17765641-b5de-4909-abb5-d5e62917f929.png">

