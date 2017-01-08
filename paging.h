#ifndef PAGING_H
#define PAGING_H

typedef unsigned int PageDirectoryEntry;

void init_paging(const PageDirectoryEntry pd[1024]);

void init_identity_paging();

#endif  // PAGING_H
