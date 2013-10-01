#!/bin/bash

#gcc -I/usr/local/include/libxml2 -I/usr/local/include -L/usr/local/lib -lxml2 -I/usr/local/include -I/usr/local/include/libxml2 -L/usr/local/lib -lxslt -lz -liconv -lm -lxml2 -o creuvux -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -I/usr/local/include -L/usr/local/lib -lsqlite3 creuvux.c client_conf.c  client_conf.h  creuvard.c  creuvard.h help.h network.c  network.h sync.c sync.h thread.h get.c get.h put.c put.h SSL_sendfile.c SSL_sendfile.h help.c msg.c msg.h -lssl -lcrypto -pthread list.c list.h find.c find.h info.c info.h list_grp.c list_grp.h list_cat.c list_cat.h

gcc -o creuvux -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -I/usr/local/include -L/usr/local/lib -lsqlite3 creuvux.c client_conf.c  client_conf.h  creuvard.c  creuvard.h help.h network.c  network.h sync.c sync.h thread.h get.c get.h upload.c upload.h SSL_sendfile.c SSL_sendfile.h help.c msg.c msg.h -lssl -lcrypto -pthread -lz list.c list.h find.c find.h info.c info.h list_grp.c list_grp.h list_cat.c list_cat.h -lcurl `xml2-config --cflags` `xml2-config --libs`
