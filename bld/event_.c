#line 1 "../fossil-1.32/src/event.c"
/*
** Copyright (c) 2010 D. Richard Hipp
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
** This file contains code to do formatting of event messages:
**
**     Technical Notes
**     Milestones
**     Blog posts
**     New articles
**     Process checkpoints
**     Announcements
**
** Do not confuse "event" artifacts with the "event" table in the
** repository database.  An "event" artifact is a technical-note: a
** wiki- or blog-like essay that appears on the timeline.  The "event"
** table records all entries on the timeline, including tech-notes.
**
** (2015-02-14):  Changing the name to "tech-note" most everywhere.
*/
#include "config.h"
#include <assert.h>
#include <ctype.h>
#include "event.h"

/*
** Output a hyperlink to an technote given its tagid.
*/
void hyperlink_to_event_tagid(int tagid){
  char *zId;
  zId = db_text(0, "SELECT substr(tagname, 7) FROM tag WHERE tagid=%d",
                     tagid);
  cgi_printf("[%z%S</a>]\n",(href("%R/technote/%s",zId)),(zId));
  free(zId);
}

/*
** WEBPAGE: technote
** WEBPAGE: event
**
** Display a "technical note" or "tech-note" (formerly called an "event").
**
** PARAMETERS:
**
**  name=ID          // Identify the tech-note to display. ID must be complete
**  aid=ARTIFACTID   // Which specific version of the tech-note.  Optional.
**  v=BOOLEAN        // Show details if TRUE.  Default is FALSE.  Optional.
**
** Display an existing event identified by EVENTID
*/
void event_page(void){
  int rid = 0;             /* rid of the event artifact */
  char *zUuid;             /* UUID corresponding to rid */
  const char *zId;    /* Event identifier */
  const char *zVerbose;    /* Value of verbose option */
  char *zETime;            /* Time of the tech-note */
  char *zATime;            /* Time the artifact was created */
  int specRid;             /* rid specified by aid= parameter */
  int prevRid, nextRid;    /* Previous or next edits of this tech-note */
  Manifest *pTNote;        /* Parsed technote artifact */
  Blob fullbody;           /* Complete content of the technote body */
  Blob title;              /* Title extracted from the technote body */
  Blob tail;               /* Event body that comes after the title */
  Stmt q1;                 /* Query to search for the technote */
  int verboseFlag;         /* True to show details */
  const char *zMimetype = 0;  /* Mimetype of the document */


  /* wiki-read privilege is needed in order to read tech-notes.
  */
  login_check_credentials();
  if( !g.perm.RdWiki ){
    login_needed(g.anon.RdWiki);
    return;
  }

  zId = P("name");
  if( zId==0 ){ fossil_redirect_home(); return; }
  zUuid = (char*)P("aid");
  specRid = zUuid ? uuid_to_rid(zUuid, 0) : 0;
  rid = nextRid = prevRid = 0;
  db_prepare(&q1,
     "SELECT rid FROM tagxref"
     " WHERE tagid=(SELECT tagid FROM tag WHERE tagname GLOB 'event-%q*')"
     " ORDER BY mtime DESC",
     zId
  );
  while( db_step(&q1)==SQLITE_ROW ){
    nextRid = rid;
    rid = db_column_int(&q1, 0);
    if( specRid==0 || specRid==rid ){
      if( db_step(&q1)==SQLITE_ROW ){
        prevRid = db_column_int(&q1, 0);
      }
      break;
    }
  }
  db_finalize(&q1);
  if( rid==0 || (specRid!=0 && specRid!=rid) ){
    style_header("No Such Tech-Note");
    cgi_printf("Cannot locate a technical note called <b>%h</b>.\n",(zId));
    style_footer();
    return;
  }
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  zVerbose = P("v");
  if( !zVerbose ){
    zVerbose = P("verbose");
  }
  if( !zVerbose ){
    zVerbose = P("detail"); /* deprecated */
  }
  verboseFlag = (zVerbose!=0) && !is_false(zVerbose);

  /* Extract the event content.
  */
  pTNote = manifest_get(rid, CFTYPE_EVENT, 0);
  if( pTNote==0 ){
    fossil_fatal("Object #%d is not a tech-note", rid);
  }
  zMimetype = wiki_filter_mimetypes(PD("mimetype",pTNote->zMimetype));
  blob_init(&fullbody, pTNote->zWiki, -1);
  blob_init(&title, 0, 0);
  blob_init(&tail, 0, 0);
  if( fossil_strcmp(zMimetype, "text/x-fossil-wiki")==0 ){
    if( !wiki_find_title(&fullbody, &title, &tail) ){
      blob_appendf(&title, "Tech-note %S", zId);
      tail = fullbody;
    }
  }else if( fossil_strcmp(zMimetype, "text/x-markdown")==0 ){
    markdown_to_html(&fullbody, &title, &tail);
    if( blob_size(&title)==0 ){
      blob_appendf(&title, "Tech-note %S", zId);
    }
  }else{
    blob_appendf(&title, "Tech-note %S", zId);
    tail = fullbody;
  }
  style_header("%s", blob_str(&title));
  if( g.perm.WrWiki && g.perm.Write && nextRid==0 ){
    style_submenu_element("Edit", 0, "%R/technoteedit?name=%!S", zId);
  }
  zETime = db_text(0, "SELECT datetime(%.17g)", pTNote->rEventDate);
  style_submenu_element("Context", 0, "%R/timeline?c=%.20s", zId);
  if( g.perm.Hyperlink ){
    if( verboseFlag ){
      style_submenu_element("Plain", 0,
                            "%R/technote?name=%!S&aid=%s&mimetype=text/plain",
                            zId, zUuid);
      if( nextRid ){
        char *zNext;
        zNext = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", nextRid);
        style_submenu_element("Next", 0,"%R/technote?name=%!S&aid=%s&v",
                              zId, zNext);
        free(zNext);
      }
      if( prevRid ){
        char *zPrev;
        zPrev = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", prevRid);
        style_submenu_element("Prev", 0, "%R/technote?name=%!S&aid=%s&v",
                              zId, zPrev);
        free(zPrev);
      }
    }else{
      style_submenu_element("Detail", 0, "%R/technote?name=%!S&aid=%s&v",
                            zId, zUuid);
    }
  }

  if( verboseFlag && g.perm.Hyperlink ){
    int i;
    const char *zClr = 0;
    Blob comment;

    zATime = db_text(0, "SELECT datetime(%.17g)", pTNote->rDate);
    cgi_printf("<p>Tech-note [%z%S</a>] at\n"
           "[%z%s</a>]\n"
           "entered by user <b>%h</b> on\n"
           "[%z%s</a>]:</p>\n"
           "<blockquote>\n",(href("%R/artifact/%!S",zUuid)),(zUuid),(href("%R/timeline?c=%T",zETime)),(zETime),(pTNote->zUser),(href("%R/timeline?c=%T",zATime)),(zATime));
    for(i=0; i<pTNote->nTag; i++){
      if( fossil_strcmp(pTNote->aTag[i].zName,"+bgcolor")==0 ){
        zClr = pTNote->aTag[i].zValue;
      }
    }
    if( zClr && zClr[0]==0 ) zClr = 0;
    if( zClr ){
      cgi_printf("<div style=\"background-color: %h;\">\n",(zClr));
    }else{
      cgi_printf("<div>\n");
    }
    blob_init(&comment, pTNote->zComment, -1);
    wiki_convert(&comment, 0, WIKI_INLINE);
    blob_reset(&comment);
    cgi_printf("</div>\n"
           "</blockquote><hr />\n");
  }

  if( fossil_strcmp(zMimetype, "text/x-fossil-wiki")==0 ){
    wiki_convert(&fullbody, 0, 0);
  }else if( fossil_strcmp(zMimetype, "text/x-markdown")==0 ){
    cgi_append_content(blob_buffer(&tail), blob_size(&tail));
  }else{
    cgi_printf("<pre>\n"
           "%h\n"
           "</pre>\n",(blob_str(&fullbody)));
  }
  style_footer();
  manifest_destroy(pTNote);
}

/*
** WEBPAGE: technoteedit
** WEBPAGE: eventedit
**
** Revise or create a technical note (formerly called an 'event').
**
** Parameters:
**
**    name=ID           Hex hash ID of the tech-note.  If omitted, a new
**                      tech-note is created.
*/
void eventedit_page(void){
  char *zTag;
  int rid = 0;
  Blob event;
  const char *zId;
  int n;
  const char *z;
  char *zBody = (char*)P("w");
  char *zETime = (char*)P("t");
  const char *zComment = P("c");
  const char *zTags = P("g");
  const char *zClr;
  const char *zMimetype = P("mimetype");
  int isNew = 0;

  if( zBody ){
    zBody = mprintf("%s", zBody);
  }
  login_check_credentials();
  zId = P("name");
  if( zId==0 ){
    zId = db_text(0, "SELECT lower(hex(randomblob(20)))");
    isNew = 1;
  }else{
    int nId = strlen(zId);
    if( !validate16(zId, nId) ){
      fossil_redirect_home();
      return;
    }
  }
  zTag = mprintf("event-%s", zId);
  rid = db_int(0,
    "SELECT rid FROM tagxref"
    " WHERE tagid=(SELECT tagid FROM tag WHERE tagname GLOB '%q*')"
    " ORDER BY mtime DESC", zTag
  );
  if( rid && strlen(zId)<40 ){
    zId = db_text(0,
      "SELECT substr(tagname,7) FROM tag WHERE tagname GLOB '%q*'",
      zTag
    );
  }
  free(zTag);

  /* Need both check-in and wiki-write or wiki-create privileges in order
  ** to edit/create an event.
  */
  if( !g.perm.Write || (rid && !g.perm.WrWiki) || (!rid && !g.perm.NewWiki) ){
    login_needed(g.anon.Write && (rid ? g.anon.WrWiki : g.anon.NewWiki));
    return;
  }

  /* Figure out the color */
  if( rid ){
    zClr = db_text("", "SELECT bgcolor FROM event WHERE objid=%d", rid);
  }else{
    zClr = "";
    isNew = 1;
  }
  zClr = PD("clr",zClr);
  if( fossil_strcmp(zClr,"##")==0 ) zClr = PD("cclr","");


  /* If editing an existing event, extract the key fields to use as
  ** a starting point for the edit.
  */
  if( rid
   && (zBody==0 || zETime==0 || zComment==0 || zTags==0 || zMimetype==0)
  ){
    Manifest *pTNote;
    pTNote = manifest_get(rid, CFTYPE_EVENT, 0);
    if( pTNote && pTNote->type==CFTYPE_EVENT ){
      if( zBody==0 ) zBody = pTNote->zWiki;
      if( zETime==0 ){
        zETime = db_text(0, "SELECT datetime(%.17g)", pTNote->rEventDate);
      }
      if( zComment==0 ) zComment = pTNote->zComment;
      if( zMimetype==0 ) zMimetype = pTNote->zMimetype;
    }
    if( zTags==0 ){
      zTags = db_text(0,
        "SELECT group_concat(substr(tagname,5),', ')"
        "  FROM tagxref, tag"
        " WHERE tagxref.rid=%d"
        "   AND tagxref.tagid=tag.tagid"
        "   AND tag.tagname GLOB 'sym-*'",
        rid
      );
    }
  }
  zETime = db_text(0, "SELECT coalesce(datetime(%Q),datetime('now'))", zETime);
  if( P("submit")!=0 && (zBody!=0 && zComment!=0) ){
    char *zDate;
    Blob cksum;
    int nrid, n;
    blob_init(&event, 0, 0);
    db_begin_transaction();
    login_verify_csrf_secret();
    while( fossil_isspace(zComment[0]) ) zComment++;
    n = strlen(zComment);
    while( n>0 && fossil_isspace(zComment[n-1]) ){ n--; }
    if( n>0 ){
      blob_appendf(&event, "C %#F\n", n, zComment);
    }
    zDate = date_in_standard_format("now");
    blob_appendf(&event, "D %s\n", zDate);
    free(zDate);
    zETime[10] = 'T';
    blob_appendf(&event, "E %s %s\n", zETime, zId);
    zETime[10] = ' ';
    if( rid ){
      char *zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
      blob_appendf(&event, "P %s\n", zUuid);
      free(zUuid);
    }
    if( zMimetype && zMimetype[0] ){
      blob_appendf(&event, "N %s\n", zMimetype);
    }
    if( zClr && zClr[0] ){
      blob_appendf(&event, "T +bgcolor * %F\n", zClr);
    }
    if( zTags && zTags[0] ){
      Blob tags, one;
      int i, j;
      Stmt q;
      char *zBlob;

      /* Load the tags string into a blob */
      blob_zero(&tags);
      blob_append(&tags, zTags, -1);

      /* Collapse all sequences of whitespace and "," characters into
      ** a single space character */
      zBlob = blob_str(&tags);
      for(i=j=0; zBlob[i]; i++, j++){
        if( fossil_isspace(zBlob[i]) || zBlob[i]==',' ){
          while( fossil_isspace(zBlob[i+1]) ){ i++; }
          zBlob[j] = ' ';
        }else{
          zBlob[j] = zBlob[i];
        }
      }
      blob_resize(&tags, j);

      /* Parse out each tag and load it into a temporary table for sorting */
      db_multi_exec("CREATE TEMP TABLE newtags(x);");
      while( blob_token(&tags, &one) ){
        db_multi_exec("INSERT INTO newtags VALUES(%B)", &one);
      }
      blob_reset(&tags);

      /* Extract the tags in sorted order and make an entry in the
      ** artifact for each. */
      db_prepare(&q, "SELECT x FROM newtags ORDER BY x");
      while( db_step(&q)==SQLITE_ROW ){
        blob_appendf(&event, "T +sym-%F *\n", db_column_text(&q, 0));
      }
      db_finalize(&q);
    }
    if( !login_is_nobody() ){
      blob_appendf(&event, "U %F\n", login_name());
    }
    blob_appendf(&event, "W %d\n%s\n", strlen(zBody), zBody);
    md5sum_blob(&event, &cksum);
    blob_appendf(&event, "Z %b\n", &cksum);
    blob_reset(&cksum);
    nrid = content_put(&event);
    db_multi_exec("INSERT OR IGNORE INTO unsent VALUES(%d)", nrid);
    if( manifest_crosslink(nrid, &event, MC_NONE)==0 ){
      db_end_transaction(1);
      style_header("Error");
      cgi_printf("Internal error:  Fossil tried to make an invalid artifact for\n"
             "the edited technode.\n");
      style_footer();
      return;
    }
    assert( blob_is_reset(&event) );
    content_deltify(rid, nrid, 0);
    db_end_transaction(0);
    cgi_redirectf("technote?name=%T", zId);
  }
  if( P("cancel")!=0 ){
    cgi_redirectf("technote?name=%T", zId);
    return;
  }
  if( zBody==0 ){
    zBody = mprintf("Insert new content here...");
  }
  if( isNew ){
    style_header("New Tech-note %S", zId);
  }else{
    style_header("Edit Tech-note %S", zId);
  }
  if( P("preview")!=0 ){
    Blob com;
    cgi_printf("<p><b>Timeline comment preview:</b></p>\n"
           "<blockquote>\n"
           "<table border=\"0\">\n");
    if( zClr && zClr[0] ){
      cgi_printf("<tr><td style=\"background-color: %h;\">\n",(zClr));
    }else{
      cgi_printf("<tr><td>\n");
    }
    blob_zero(&com);
    blob_append(&com, zComment, -1);
    wiki_convert(&com, 0, WIKI_INLINE|WIKI_NOBADLINKS);
    cgi_printf("</td></tr></table>\n"
           "</blockquote>\n"
           "<p><b>Page content preview:</b><p>\n"
           "<blockquote>\n");
    blob_init(&event, 0, 0);
    blob_append(&event, zBody, -1);
    wiki_render_by_mimetype(&event, zMimetype);
    cgi_printf("</blockquote><hr />\n");
    blob_reset(&event);
  }
  for(n=2, z=zBody; z[0]; z++){
    if( z[0]=='\n' ) n++;
  }
  if( n<20 ) n = 20;
  if( n>40 ) n = 40;
  cgi_printf("<form method=\"post\" action=\"%R/technoteedit\"><div>\n");
  login_insert_csrf_secret();
  cgi_printf("<input type=\"hidden\" name=\"name\" value=\"%h\" />\n"
         "<table border=\"0\" cellspacing=\"10\">\n",(zId));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Timestamp (UTC):</th>\n"
         "<td valign=\"top\">\n"
         "  <input type=\"text\" name=\"t\" size=\"25\" value=\"%h\" />\n"
         "</td></tr>\n",(zETime));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Timeline Comment:</th>\n"
         "<td valign=\"top\">\n"
         "<textarea name=\"c\" class=\"technoteedit\" cols=\"80\"\n"
         " rows=\"3\" wrap=\"virtual\">%h</textarea>\n"
         "</td></tr>\n",(zComment));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Timeline Background Color:</th>\n"
         "<td valign=\"top\">\n");
  render_color_chooser(0, zClr, 0, "clr", "cclr");
  cgi_printf("</td></tr>\n");

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Tags:</th>\n"
         "<td valign=\"top\">\n"
         "  <input type=\"text\" name=\"g\" size=\"40\" value=\"%h\" />\n"
         "</td></tr>\n",(zTags));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Markup Style:</th>\n"
         "<td valign=\"top\">\n");
  mimetype_option_menu(zMimetype);
  cgi_printf("</td></tr>\n");

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Page&nbsp;Content:</th>\n"
         "<td valign=\"top\">\n"
         "<textarea name=\"w\" class=\"technoteedit\" cols=\"80\"\n"
         " rows=\"%d\" wrap=\"virtual\">%h</textarea>\n"
         "</td></tr>\n",(n),(zBody));

  cgi_printf("<tr><td colspan=\"2\">\n"
         "<input type=\"submit\" name=\"preview\" value=\"Preview Your Changes\" />\n"
         "<input type=\"submit\" name=\"submit\" value=\"Apply These Changes\" />\n"
         "<input type=\"submit\" name=\"cancel\" value=\"Cancel\" />\n"
         "</td></tr></table>\n"
         "</div></form>\n");
  style_footer();
}
