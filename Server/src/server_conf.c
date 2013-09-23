/**
 * \file server_conf.c
 * \brief Fichier contenant 2 fonctions de configurations du serveur.
 * \author Sylvain Bourdou
 * \version 0.81
 * \date 25 Février 2013
 *
 * Fonction de creuvard
 *
 */
/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "server_conf.h"
#include "creuvard.h"

#define debug() fprintf(stderr, "DEBUG: fonction:%s() file:%s line:%d\n", __FUNCTION__, __FILE__, __LINE__);

/**
 * \fn void initialize_server_options ( ServerOptions *options)
 * \brief Initialise les options du serveur.
 *
 * \param options Struct contenant toutes les variables d'environement.
 */
void initialize_server_options ( ServerOptions *options)
{
  memset(options, 0, sizeof(*options));
  options->user = NULL;
  options->path = NULL;
  options->bandwidth = 100; /* Default value 100 KBps */
  options->chroot_directory = NULL;
  options->config = crv_strdup("/etc/creuvuxd/creuvuxd.conf");;
  options->listen_addrs = NULL;
  options->host = NULL;
  options->num_ports = 1664;
  options->debug = 1;
  options->pid = NULL;
  options->sec = -1;
  options->passphrase = NULL;
  options->upload_directory = crv_strdup("/");
  options->public_directory = crv_strdup("/");
  options->address_family = AF_INET;
  options->stat_repport = crv_strdup("repport.html");
  options->db_min = 0;
  options->db_max = 1000;
	options->ipdb = NULL;
	options->portdb = 3306;
	options->dbname = NULL;
	options->dbuser = NULL;
	options->dbpassw = NULL;
}

/* Server configuration options. */
ServerOptions options;

void Free_options(void)
{
	if (options.user != NULL)
		crv_free(options.user);

	if (options.path != NULL)
		crv_free(options.path);

	if (options.chroot_directory != NULL)
		crv_free(options.chroot_directory);
	
	if (options.config != NULL)
		crv_free(options.config);

	if (options.listen_addrs != NULL)
		crv_free(options.listen_addrs);

	if (options.host != NULL)
		crv_free(options.host);

	if (options.pid != NULL)
		crv_free(options.pid);

	if (options.passphrase != NULL)
		crv_free(options.passphrase);

	if (options.upload_directory != NULL)
		crv_free(options.upload_directory);

	if (options.public_directory != NULL)
		crv_free(options.public_directory);

	if (options.stat_repport != NULL)
		crv_free(options.stat_repport);
	
	if (options.ipdb != NULL)
		crv_free(options.ipdb);
	
	if (options.dbname != NULL)
		crv_free(options.dbname);
	
	if (options.dbuser != NULL)
		crv_free(options.dbuser);
	
	if (options.dbpassw != NULL)
		crv_free(options.dbpassw);

}
