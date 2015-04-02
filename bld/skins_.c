#line 1 "../fossil-1.32/src/skins.c"
/*
** Copyright (c) 2009 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** Implementation of the Setup page for "skins".
*/
#include "config.h"
#include <assert.h>
#include "skins.h"

/*
** An array of available built-in skins.
**
** To add new built-in skins:
**
**    1.  Pick a name for the new skin.  (Here we use "xyzzy").
**
**    2.  Install files skins/xyzzy/css.txt, skins/xyzzy/header.txt,
**        and skins/xyzzy/footer.txt into the source tree.
**
**    3.  Rerun "tclsh makemake.tcl" in the src/ folder in order to
**        rebuild the makefiles to reference the new CSS, headers, and footers.
**
**    4.  Make an entry in the following array for the new skin.
*/
static struct BuiltinSkin {
  const char *zDesc;    /* Description of this skin */
  const char *zLabel;   /* The directory under skins/ holding this skin */
  int whiteForeground;  /* True if this skin uses a light-colored foreground */
  char *zSQL;           /* Filled in at run-time with SQL to insert this skin */
} aBuiltinSkin[] = {
  { "Default",                           "default",           0, 0 },
  { "Plain Gray, No Logo",               "plain_gray",        0, 0 },
  { "Khaki, No Logo",                    "khaki",             0, 0 },
  { "Black & White, Menu on Left",       "black_and_white",   0, 0 },
  { "Shadow boxes & Rounded Corners",    "rounded1",          0, 0 },
  { "Enhanced Default",                  "enhanced1",         0, 0 },
  { "San Francisco Modern",              "etienne1",          0, 0 },
  { "Eagle",                             "eagle",             1, 0 },
  { "Xekri",                             "xekri",             0, 0 },
};

/*
** Alternative skins can be specified in the CGI script or by options
** on the "http", "ui", and "server" commands.  The alternative skin
** name must be one of the aBuiltinSkin[].zLabel names.  If there is
** a match, that alternative is used.
**
** The following static variable holds the name of the alternative skin,
** or NULL if the skin should be as configured.
*/
static struct BuiltinSkin *pAltSkin = 0;
static char *zAltSkinDir = 0;

/*
** Invoke this routine to set the alternative skin.  Return NULL if the
** alternative was successfully installed.  Return a string listing all
** available skins if zName does not match an available skin.  Memory
** for the returned string comes from fossil_malloc() and should be freed
** by the caller.
**
** If the alternative skin name contains one or more '/' characters, then
** it is assumed to be a directory on disk that holds override css.txt,
** footer.txt, and header.txt.  This mode can be used for interactive
** development of new skins.
*/
char *skin_use_alternative(const char *zName){
  int i;
  Blob err = BLOB_INITIALIZER;
  if( strchr(zName, '/')!=0 ){
    zAltSkinDir = fossil_strdup(zName);
    return 0;
  }
  for(i=0; i<ArraySize(aBuiltinSkin); i++){
    if( fossil_strcmp(aBuiltinSkin[i].zLabel, zName)==0 ){
      pAltSkin = &aBuiltinSkin[i];
      return 0;
    }
  }
  blob_appendf(&err, "available skins: %s", aBuiltinSkin[0].zLabel);
  for(i=1; i<ArraySize(aBuiltinSkin); i++){
    blob_append(&err, " ", 1);
    blob_append(&err, aBuiltinSkin[i].zLabel, -1);
  }
  return blob_str(&err);
}

/*
** Look for the --skin command-line option and process it.  Or
** call fossil_fatal() if an unknown skin is specified.
*/
void skin_override(void){
  const char *zSkin = find_option("skin",0,1);
  if( zSkin ){
    char *zErr = skin_use_alternative(zSkin);
    if( zErr ) fossil_fatal("%s", zErr);
  }
}

/*
** The following routines return the various components of the skin
** that should be used for the current run.
*/
const char *skin_get(const char *zWhat){
  const char *zOut;
  char *z;
  if( zAltSkinDir ){
    char *z = mprintf("%s/%s.txt", zAltSkinDir, zWhat);
    if( file_isfile(z) ){
      Blob x;
      blob_read_from_file(&x, z);
      fossil_free(z);
      return blob_str(&x);
    }
    fossil_free(z);
  }
  if( pAltSkin ){
    z = mprintf("skins/%s/%s.txt", pAltSkin->zLabel, zWhat);
    zOut = builtin_text(z);
    fossil_free(z);
  }else{
    zOut = db_get(zWhat, 0);
    if( zOut==0 ){
      z = mprintf("skins/default/%s.txt", zWhat);
      zOut = builtin_text(z);
      fossil_free(z);
    }
  }
  return zOut;
}
int skin_white_foreground(void){
  int rc;
  if( pAltSkin ){
    rc = pAltSkin->whiteForeground;
  }else{
    rc = db_get_boolean("white-foreground",0);
  }
  return rc;
}

/*
** For a skin named zSkinName, compute the name of the CONFIG table
** entry where that skin is stored and return it.
**
** Return NULL if zSkinName is NULL or an empty string.
**
** If ifExists is true, and the named skin does not exist, return NULL.
*/
static char *skinVarName(const char *zSkinName, int ifExists){
  char *z;
  if( zSkinName==0 || zSkinName[0]==0 ) return 0;
  z = mprintf("skin:%s", zSkinName);
  if( ifExists && !db_exists("SELECT 1 FROM config WHERE name=%Q", z) ){
    free(z);
    z = 0;
  }
  return z;
}

/*
** Return true if there exists a skin name "zSkinName".
*/
static int skinExists(const char *zSkinName){
  int i;
  if( zSkinName==0 ) return 0;
  for(i=0; i<sizeof(aBuiltinSkin)/sizeof(aBuiltinSkin[0]); i++){
    if( fossil_strcmp(zSkinName, aBuiltinSkin[i].zDesc)==0 ) return 1;
  }
  return db_exists("SELECT 1 FROM config WHERE name='skin:%q'", zSkinName);
}

/*
** Construct and return an string of SQL statements that represents
** a "skin" setting.  If zName==0 then return the skin currently
** installed.  Otherwise, return one of the built-in skins designated
** by zName.
**
** Memory to hold the returned string is obtained from malloc.
*/
static char *getSkin(const char *zName){
  const char *z;
  char *zLabel;
  static const char *azType[] = { "css", "header", "footer" };
  int i;
  Blob val;
  blob_zero(&val);
  for(i=0; i<sizeof(azType)/sizeof(azType[0]); i++){
    if( zName ){
      zLabel = mprintf("skins/%s/%s.txt", zName, azType[i]);
      z = builtin_text(zLabel);
      fossil_free(zLabel);
    }else{
      z = db_get(azType[i], 0);
      if( z==0 ){
        zLabel = mprintf("skins/default/%s.txt", azType[i]);
        z = builtin_text(zLabel);
        fossil_free(zLabel);
      }
    }
    blob_appendf(&val,
       "REPLACE INTO config(name,value,mtime) VALUES(%Q,%Q,now());\n",
       azType[i], z
    );
  }
  return blob_str(&val);
}

/*
** Respond to a Rename button press.  Return TRUE if a dialog was painted.
** Return FALSE to continue with the main Skins page.
*/
static int skinRename(void){
  const char *zOldName;
  const char *zNewName;
  int ex = 0;
  if( P("rename")==0 ) return 0;
  zOldName = P("sn");
  zNewName = P("newname");
  if( zOldName==0 ) return 0;
  if( zNewName==0 || zNewName[0]==0 || (ex = skinExists(zNewName))!=0 ){
    if( zNewName==0 ) zNewName = zOldName;
    style_header("Rename A Skin");
    if( ex ){
      cgi_printf("<p><span class=\"generalError\">There is already another skin\n"
             "named \"%h\".  Choose a different name.</span></p>\n",(zNewName));
    }
    cgi_printf("<form action=\"%s/setup_skin\" method=\"post\"><div>\n"
           "<table border=\"0\"><tr>\n"
           "<tr><td align=\"right\">Current name:<td align=\"left\"><b>%h</b>\n"
           "<tr><td align=\"right\">New name:<td align=\"left\">\n"
           "<input type=\"text\" size=\"35\" name=\"newname\" value=\"%h\">\n"
           "<tr><td><td>\n"
           "<input type=\"hidden\" name=\"sn\" value=\"%h\">\n"
           "<input type=\"submit\" name=\"rename\" value=\"Rename\">\n"
           "<input type=\"submit\" name=\"canren\" value=\"Cancel\">\n"
           "</table>\n",(g.zTop),(zOldName),(zNewName),(zOldName));
    login_insert_csrf_secret();
    cgi_printf("</div></form>\n");
    style_footer();
    return 1;
  }
  db_multi_exec(
    "UPDATE config SET name='skin:%q' WHERE name='skin:%q';",
    zNewName, zOldName
  );
  return 0;
}

/*
** Respond to a Save button press.  Return TRUE if a dialog was painted.
** Return FALSE to continue with the main Skins page.
*/
static int skinSave(const char *zCurrent){
  const char *zNewName;
  int ex = 0;
  if( P("save")==0 ) return 0;
  zNewName = P("svname");
  if( zNewName && zNewName[0]!=0 ){
  }
  if( zNewName==0 || zNewName[0]==0 || (ex = skinExists(zNewName))!=0 ){
    if( zNewName==0 ) zNewName = "";
    style_header("Save Current Skin");
    if( ex ){
      cgi_printf("<p><span class=\"generalError\">There is already another skin\n"
             "named \"%h\".  Choose a different name.</span></p>\n",(zNewName));
    }
    cgi_printf("<form action=\"%s/setup_skin\" method=\"post\"><div>\n"
           "<table border=\"0\"><tr>\n"
           "<tr><td align=\"right\">Name for this skin:<td align=\"left\">\n"
           "<input type=\"text\" size=\"35\" name=\"svname\" value=\"%h\">\n"
           "<tr><td><td>\n"
           "<input type=\"submit\" name=\"save\" value=\"Save\">\n"
           "<input type=\"submit\" name=\"cansave\" value=\"Cancel\">\n"
           "</table>\n",(g.zTop),(zNewName));
    login_insert_csrf_secret();
    cgi_printf("</div></form>\n");
    style_footer();
    return 1;
  }
  db_multi_exec(
    "INSERT OR IGNORE INTO config(name, value, mtime)"
    "VALUES('skin:%q',%Q,now())",
    zNewName, zCurrent
  );
  return 0;
}

/*
** WEBPAGE: setup_skin
*/
void setup_skin(void){
  const char *z;
  char *zName;
  char *zErr = 0;
  const char *zCurrent = 0;  /* Current skin */
  int i;                     /* Loop counter */
  Stmt q;
  int seenCurrent = 0;

  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }
  db_begin_transaction();
  zCurrent = getSkin(0);
  for(i=0; i<sizeof(aBuiltinSkin)/sizeof(aBuiltinSkin[0]); i++){
    aBuiltinSkin[i].zSQL = getSkin(aBuiltinSkin[i].zLabel);
  }

  /* Process requests to delete a user-defined skin */
  if( P("del1") && (zName = skinVarName(P("sn"), 1))!=0 ){
    style_header("Confirm Custom Skin Delete");
    cgi_printf("<form action=\"%s/setup_skin\" method=\"post\"><div>\n"
           "<p>Deletion of a custom skin is a permanent action that cannot\n"
           "be undone.  Please confirm that this is what you want to do:</p>\n"
           "<input type=\"hidden\" name=\"sn\" value=\"%h\" />\n"
           "<input type=\"submit\" name=\"del2\" value=\"Confirm - Delete The Skin\" />\n"
           "<input type=\"submit\" name=\"cancel\" value=\"Cancel - Do Not Delete\" />\n",(g.zTop),(P("sn")));
    login_insert_csrf_secret();
    cgi_printf("</div></form>\n");
    style_footer();
    return;
  }
  if( P("del2")!=0 && (zName = skinVarName(P("sn"), 1))!=0 ){
    db_multi_exec("DELETE FROM config WHERE name=%Q", zName);
  }
  if( skinRename() ) return;
  if( skinSave(zCurrent) ) return;

  /* The user pressed one of the "Install" buttons. */
  if( P("load") && (z = P("sn"))!=0 && z[0] ){
    int seen = 0;

    /* Check to see if the current skin is already saved.  If it is, there
    ** is no need to create a backup */
    zCurrent = getSkin(0);
    for(i=0; i<sizeof(aBuiltinSkin)/sizeof(aBuiltinSkin[0]); i++){
      if( fossil_strcmp(aBuiltinSkin[i].zSQL, zCurrent)==0 ){
        seen = 1;
        break;
      }
    }
    if( !seen ){
      seen = db_exists("SELECT 1 FROM config WHERE name GLOB 'skin:*'"
                       " AND value=%Q", zCurrent);
      if( !seen ){
        db_multi_exec(
          "INSERT INTO config(name,value,mtime) VALUES("
          "  strftime('skin:Backup On %%Y-%%m-%%d %%H:%%M:%%S'),"
          "  %Q,now())", zCurrent
        );
      }
    }
    seen = 0;
    for(i=0; i<sizeof(aBuiltinSkin)/sizeof(aBuiltinSkin[0]); i++){
      if( fossil_strcmp(aBuiltinSkin[i].zDesc, z)==0 ){
        seen = 1;
        zCurrent = aBuiltinSkin[i].zSQL;
        db_multi_exec("%s", zCurrent/*safe-for-%s*/);
        break;
      }
    }
    if( !seen ){
      zName = skinVarName(z,0);
      zCurrent = db_get(zName, 0);
      db_multi_exec("%s", zCurrent/*safe-for-%s*/);
    }
  }

  style_header("Skins");
  if( zErr ){
    cgi_printf("<p><font color=\"red\">%h</font></p>\n",(zErr));
  }
  cgi_printf("<p>A \"skin\" is a combination of\n"
         "<a href=\"setup_editcss\">CSS</a>,\n"
         "<a href=\"setup_header\">Header</a>, and\n"
         "<a href=\"setup_footer\">Footer</a> that determines the look and feel\n"
         "of the web interface.</p>\n"
         "\n");
  if( pAltSkin ){
    cgi_printf("<p class=\"generalError\">\n"
           "This page is generated using an skin override named\n"
           "\"%h\".  You can change the skin configuration\n"
           "below, but the changes will not take effect until the Fossil server\n"
           "is restarted without the override.</p>\n"
           "\n",(pAltSkin->zLabel));
  }
  cgi_printf("<h2>Available Skins:</h2>\n"
         "<table border=\"0\">\n");
  for(i=0; i<sizeof(aBuiltinSkin)/sizeof(aBuiltinSkin[0]); i++){
    z = aBuiltinSkin[i].zDesc;
    cgi_printf("<tr><td>%d.<td>%h<td>&nbsp;&nbsp;<td>\n",(i+1),(z));
    if( fossil_strcmp(aBuiltinSkin[i].zSQL, zCurrent)==0 ){
      cgi_printf("(Currently In Use)\n");
      seenCurrent = 1;
    }else{
      cgi_printf("<form action=\"%s/setup_skin\" method=\"post\">\n"
             "<input type=\"hidden\" name=\"sn\" value=\"%h\" />\n"
             "<input type=\"submit\" name=\"load\" value=\"Install\" />\n",(g.zTop),(z));
      if( pAltSkin==&aBuiltinSkin[i] ){
        cgi_printf("(Current override)\n");
      }
      cgi_printf("</form>\n");
    }
    cgi_printf("</tr>\n");
  }
  db_prepare(&q,
     "SELECT substr(name, 6), value FROM config"
     " WHERE name GLOB 'skin:*'"
     " ORDER BY name"
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char *zN = db_column_text(&q, 0);
    const char *zV = db_column_text(&q, 1);
    i++;
    cgi_printf("<tr><td>%d.<td>%h<td>&nbsp;&nbsp;<td>\n"
           "<form action=\"%s/setup_skin\" method=\"post\">\n",(i),(zN),(g.zTop));
    if( fossil_strcmp(zV, zCurrent)==0 ){
      cgi_printf("(Currently In Use)\n");
      seenCurrent = 1;
    }else{
      cgi_printf("<input type=\"submit\" name=\"load\" value=\"Install\">\n"
             "<input type=\"submit\" name=\"del1\" value=\"Delete\">\n");
    }
    cgi_printf("<input type=\"submit\" name=\"rename\" value=\"Rename\">\n"
           "<input type=\"hidden\" name=\"sn\" value=\"%h\">\n"
           "</form></tr>\n",(zN));
  }
  db_finalize(&q);
  if( !seenCurrent ){
    i++;
    cgi_printf("<tr><td>%d.<td><i>Current Configuration</i><td>&nbsp;&nbsp;<td>\n"
           "<form action=\"%s/setup_skin\" method=\"post\">\n"
           "<input type=\"submit\" name=\"save\" value=\"Save\">\n"
           "</form>\n",(i),(g.zTop));
  }
  cgi_printf("</table>\n");
  style_footer();
  db_end_transaction(0);
}
