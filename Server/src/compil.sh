#!/usr/bin/env sh
CFG=/usr/bin/mysql_config

#sh -c "gcc -o creuvuxd -g -Wall -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Werror-implicit-function-declaration -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Wconversion -Wsign-compare -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wnested-externs -Winline -Wlong-long -I/usr/local/include -L/usr/local/lib -lz -lsqlite3 -lcrypto `$CFG --cflags` -lssl network.c create_database.c  creuvard.c  creuvuxd.c  database_utils.c  server_conf.c log.c SSL_sendfile.c sync.c get.c SSL_loadfile.c put.c msg.c crv_admin.c crv_mysql.c `$CFG --libs`"


#sh -c "gcc -o creuvuxd -g -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -I/usr/local/include -L/usr/local/lib -lz -lsqlite3 -lcrypto `$CFG --cflags` -lssl network.c create_database.c  creuvard.c  creuvuxd.c  server_conf.c log.c SSL_sendfile.c sync.c get.c SSL_loadfile.c put.c crv_mysql.c crv_string.c `$CFG --libs` -lpthread -pthread -lssl -lcrypto"

echo "gcc -o creuvuxd -g -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -I/usr/local/include -L/usr/local/lib -lz -lsqlite3 -lcrypto `$CFG --cflags` -lssl network.c create_database.c  creuvard.c  creuvuxd.c  server_conf.c log.c SSL_sendfile.c sync.c get.c SSL_loadfile.c put.c crv_mysql.c crv_string.c `$CFG --libs` -lpthread -pthread -lssl -lcrypto"

echo `$CFG --cflags`
echo `$CFG --libs`
