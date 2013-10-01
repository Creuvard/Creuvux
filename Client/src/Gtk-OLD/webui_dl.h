/*
 * Author: Bourdou Sylvain
 * Mail  : sylvain.bourdou@colerix.org
 *
 */

#ifndef WEBUI_DL
#define WEBUI_DL

typedef struct {
	FILE *output;
	char *sha1;
	char *title;
	char *eta;
	float pct;
	float rate;
} Webstruct;

extern void initialize_webui_struct(Webstruct *WebUI);
extern int WebUI_dl(const char *, const char *, const char *, float , float );
#endif /* WEBUI_DL */
