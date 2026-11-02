#include "ramfs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define FILES 20
#define HANDLES 24
#define CAPACITY (512u*1024u)

typedef struct {char name[128];uint8_t *data;size_t size,capacity;int readonly,refs;} File;
typedef struct {File *file;size_t offset;int flags;} Handle;
static File files[FILES];
static Handle handles[HANDLES];
static int find(const char *name) {int i;for(i=0;i<FILES;i++)if(!strcmp(files[i].name,name))return i;return -1;}
static int valid_name(const char *name) {return name && !strncmp(name,"/ram/",5) && name[5] && strlen(name)<128;}
static Handle *handle(int fd) {return fd>=0 && fd<HANDLES && handles[fd].file ? &handles[fd] : NULL;}
int ramfs_mount_wad(const char *name,const uint8_t *data,size_t size) {
    int i;
    if(!valid_name(name)||!data||size>INT_MAX)return -EINVAL;
    if(find(name)>=0)return -EEXIST;
    for(i=0;i<FILES;i++)if(!files[i].name[0])break;
    if(i==FILES)return -ENOSPC;
    strcpy(files[i].name,name);files[i].data=(uint8_t *)data;files[i].size=size;files[i].readonly=1;return 0;
}
int ramfs_open(const char *name,int flags) {
    int i,fd;File *f;
    if(!valid_name(name))return -EINVAL;
    for(fd=0;fd<HANDLES;fd++)if(!handles[fd].file)break;
    if(fd==HANDLES)return -EMFILE;
    i=find(name);
    if(i<0) {
        if(!(flags&O_CREAT))return -ENOENT;
        for(i=0;i<FILES;i++)if(!files[i].name[0])break;
        if(i==FILES)return -ENOSPC;
        strcpy(files[i].name,name);
    } else if((flags&O_CREAT)&&(flags&O_EXCL))return -EEXIST;
    f=&files[i];
    if(f->readonly && ((flags&O_ACCMODE)!=O_RDONLY || (flags&O_TRUNC)))return -EROFS;
    if((flags&O_TRUNC) && (flags&O_ACCMODE)!=O_RDONLY)f->size=0;
    handles[fd]=(Handle){f,(flags&O_APPEND)?f->size:0,flags};f->refs++;return fd;
}
int ramfs_close(int fd) {Handle *h=handle(fd);if(!h)return -EBADF;h->file->refs--;h->file=NULL;return 0;}
int ramfs_read(int fd,void *data,size_t size) {
    Handle *h=handle(fd);size_t n;
    if(!h||(h->flags&O_ACCMODE)==O_WRONLY)return -EBADF;
    n=h->offset<h->file->size ? h->file->size-h->offset : 0;if(n>size)n=size;
    if(n)memcpy(data,h->file->data+h->offset,n);
    h->offset+=n;return (int)n;
}
int ramfs_write(int fd,const void *data,size_t size) {
    Handle *h=handle(fd);File *f;size_t end,capacity;uint8_t *p;
    if(!h||(h->flags&O_ACCMODE)==O_RDONLY)return -EBADF;
    f=h->file;if(f->readonly)return -EROFS;
    if(h->flags&O_APPEND)h->offset=f->size;
    if(h->offset>CAPACITY||size>CAPACITY-h->offset)return -ENOSPC;
    if(!size)return 0;
    end=h->offset+size;
    if(end>f->capacity) {
        capacity=(end+4095)&~(size_t)4095;
        p=realloc(f->data,capacity);if(!p)return -ENOMEM;
        f->data=p;f->capacity=capacity;
    }
    if(h->offset>f->size)memset(f->data+f->size,0,h->offset-f->size);
    memcpy(f->data+h->offset,data,size);h->offset=end;if(end>f->size)f->size=end;return (int)size;
}
int ramfs_seek(int fd,int offset,int whence) {
    Handle *h=handle(fd);int64_t pos;
    if(!h)return -EBADF;
    if(whence!=SEEK_SET&&whence!=SEEK_CUR&&whence!=SEEK_END)return -EINVAL;
    pos=(whence==SEEK_SET?0:whence==SEEK_CUR?(int64_t)h->offset:(int64_t)h->file->size)+offset;
    if(pos<0||pos>INT_MAX)return -EINVAL;
    h->offset=(size_t)pos;return (int)pos;
}
int ramfs_size(int fd) {Handle *h=handle(fd);return h?(int)h->file->size:-EBADF;}
int ramfs_unlink(const char *name) {
    int i=find(name);if(i<0)return -ENOENT;
    if(files[i].readonly)return -EROFS;
    if(files[i].refs)return -EBUSY;
    free(files[i].data);memset(&files[i],0,sizeof(File));return 0;
}
int ramfs_rename(const char *oldname,const char *newname) {
    int i=find(oldname),j=find(newname),r;
    if(!valid_name(newname))return -EINVAL;
    if(i<0)return -ENOENT;
    if(i==j)return 0;
    if(files[i].readonly)return -EROFS;
    if(j>=0){r=ramfs_unlink(newname);if(r<0)return r;}
    strcpy(files[i].name,newname);return 0;
}
