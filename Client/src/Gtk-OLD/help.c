#include <stdio.h>
#include "help.h"


void
welcom (void)
{
  fprintf (stdout, 
  "    ____                                 \n"
  "   / ___|_ __ ___ _   ___   ___   ___  __\n"
  "  | |   | '__/ _ \\ | | \\ \\ / / | | \\ \\/ /\n"
  "  | |___| | |  __/ |_| |\\ V /| |_| |>  < \n"
  "   \\____|_|  \\___|\\__,_| \\_/  \\__,_/_/\\_\\   v%s\n"
  "  \n", CREUVUX_VERSION);
}


void
print_help (void)
{
	
	fprintf (stdout, 
	"Usage:\n "
    "\t\"\033[0;35mhelp\033[0;m\" print this help\n " \
    "\t\"\033[0;35msync\033[0;m\" Download & Update data list\n " \
    "\t\"\033[0;35mlist\033[0;m\" Print list of datas\n " \
	"\t\"\033[0;35mls\033[0;m\" Print list of datas\n " \
	"\t\"\033[0;35mlist cat\033[0;m\" Print all categories\n " \
	"\t\"\033[0;35mlist group\033[0;m\" Print all group\n " \
	"\t\"\033[0;35minfo <id>\033[0;m\" Print infos about file (Ex: \"info 2\")\n " 
    "\t\"\033[0;35mget <id>\033[0;m\" Download file <id> (Ex: \"get 2\")\n " \
    "\t\"\033[0;35mput </path/file.ext> \033[0;m\" (Ex: \"put /home/bar/foo.iso \")\n " \
	"\t\"\033[0;35mfind text\033[0;m\" Printf file(s) which contain string \"text\" \n " \
	"\t\"\033[0;35mmsg <id>\033[0;m\" Send comment about file <id>\n " \
	"\t\"\033[0;35mlscat\033[0;m\" Print file(s) from select category\n " \
	"\t\"\033[0;35mlsgrp\033[0;m\" Print file(s) from select groups\n " \
//	"\t\"tail\" Print 34 last uploaded data"
/*	"\t\"show_server\" Print informations about servers\n " */
    "\t\"\033[0;35mexit\033[0;m\" Quit Creuvux\n" \
	"\t\"\033[0;35mquit\033[0;m\" Quit Creuvux\n");


}
