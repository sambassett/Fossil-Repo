#line 1 "../fossil-1.32/src/sitemap.c"
/*
** Copyright (c) 2014 D. Richard Hipp
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
** This file contains code to implement the sitemap webpage.
*/
#include "config.h"
#include "sitemap.h"
#include <assert.h>

/*
** WEBPAGE:  sitemap
**
** Show an incomplete list of web pages offered by the Fossil web engine.
*/
void sitemap_page(void){
  login_check_credentials();
  style_header("Site Map");
  style_adunit_config(ADUNIT_RIGHT_OK);
  cgi_printf("<p>\n"
         "The following links are just a few of the many web-pages available for\n"
         "this Fossil repository:\n"
         "</p>\n"
         "\n"
         "<ul>\n"
         "<li>%zHome Page</a>\n"
         "  <ul>\n"
         "  <li>%zSearch Project Documentation</a></li>\n"
         "  </ul></li>\n"
         "<li>%zFile Browser</a></li>\n"
         "  <ul>\n"
         "  <li>%zTree-view,\n"
         "       Trunk Check-in</a></li>\n"
         "  <li>%zFlat-view</a></li>\n"
         "  <li>%zFile ages for Trunk</a></li>\n"
         "</ul>\n"
         "<li>%zProject Timeline</a></li>\n"
         "<ul>\n"
         "  <li>%zFirst 10 \n"
         "       check-ins</a></li>\n"
         "  <li>%zAll check-ins with file name\n"
         "       changes</a></li>\n"
         "  <li>%zActivity Reports</a></li>\n"
         "</ul>\n"
         "<li>%zBranches</a></li>\n"
         "<ul>\n"
         "  <li>%zLeaf Check-ins</a></li>\n"
         "  <li>%zList of Tags</a></li>\n"
         "</ul>\n"
         "</li>\n"
         "<li>%zWiki</a>\n"
         "  <ul>\n"
         "    <li>%zWiki Search</a></li>\n"
         "    <li>%zList of Wiki Pages</a></li>\n"
         "    <li>%zRecent activity</a></li>\n"
         "    <li>%zWiki Formatting Rules</a></li>\n"
         "    <li>%zMarkdown Formatting Rules</a></li>\n"
         "    <li>%zSandbox</a></li>\n"
         "    <li>%zList of Attachments</a></li>\n"
         "  </ul>\n"
         "</li>\n"
         "<li>%zTickets</a>\n"
         "  <ul>\n"
         "  <li>%zTicket Search</a></li>\n"
         "  <li>%zRecent activity</a></li>\n"
         "  <li>%zList of Attachments</a></li>\n"
         "  </ul>\n"
         "</li>\n"
         "<li>%zFull-Text Search</a></li>\n"
         "<li>%zLogin/Logout/Change Password</a></li>\n"
         "<li>%zRepository Status</a>\n"
         "  <ul>\n"
         "  <li>%zCollisions on SHA1 hash\n"
         "      prefixes</a></li>\n"
         "  <li>%zList of URLs used to access\n"
         "      this repository</a></li>\n"
         "  <li>%zList of Artifacts</a></li>\n"
         "  </ul></li>\n"
         "<li>On-line Documentation\n"
         "  <ul>\n"
         "  <li>%zList of All Commands and Web Pages</a></li>\n"
         "  <li>%zAll \"help\" text on a single page</a></li>\n"
         "  <li>%zFilename suffix to mimetype map</a></li>\n"
         "  </ul></li>\n"
         "<li>%zAdministration Pages</a>\n"
         "  <ul>\n"
         "  <li>%zPending Moderation Requests</a></li>\n"
         "  <li>%zAdmin log</a></li>\n"
         "  <li>%zStatus of the web-page cache</a></li>\n"
         "  </ul></li>\n"
         "<li>Test Pages\n"
         "  <ul>\n"
         "  <li>%zCGI Environment Test</a></li>\n"
         "  <li>%zList of \"Timewarp\" Check-ins</a></li>\n"
         "  <li>%zList of file renames</a></li>\n"
         "  <li>%zPage to experiment with the automatic\n"
         "      colors assigned to branch names</a>\n"
         "  </ul></li>\n"
         "</ul></li>\n",(href("%R/home")),(href("%R/docsrc")),(href("%R/tree")),(href("%R/tree?type=tree&ci=trunk")),(href("%R/tree?type=flat")),(href("%R/fileage?name=trunk")),(href("%R/timeline?n=200")),(href("%R/timeline?a=1970-01-01&y=ci&n=10")),(href("%R/timeline?n=all&namechng")),(href("%R/reports")),(href("%R/brlist")),(href("%R/leaves")),(href("%R/taglist")),(href("%R/wikihelp")),(href("%R/wikisrch")),(href("%R/wcontent")),(href("%R/timeline?y=w")),(href("%R/wiki_rules")),(href("%R/md_rules")),(href("%R/wiki?name=Sandbox")),(href("%R/attachlist")),(href("%R/reportlist")),(href("%R/tktsrch")),(href("%R/timeline?y=t")),(href("%R/attachlist")),(href("%R/search")),(href("%R/login")),(href("%R/stat")),(href("%R/hash-collisions")),(href("%R/urllist")),(href("%R/bloblist")),(href("%R/help")),(href("%R/test-all-help")),(href("%R/mimetype_list")),(href("%R/setup")),(href("%R/modreq")),(href("%R/admin_log")),(href("%R/cachestat")),(href("%R/test_env")),(href("%R/test_timewarps")),(href("%R/test-rename-list")),(href("%R/hash-color-test")));
  style_footer();
}
