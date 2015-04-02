#line 1 "../fossil-1.32/src/statrep.c"
/*
** Copyright (c) 2013 Stephen Beal
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
** This file contains code to implement the /reports web page.
**
*/
#include "config.h"
#include <string.h>
#include <time.h>
#include "statrep.h"


/*
** Used by stats_report_xxxxx() to remember which type of events
** to show. Populated by stats_report_init_view() and holds the
** return value of that function.
*/
static int statsReportType = 0;

/*
** Set by stats_report_init_view() to one of the y=XXXX values
** accepted by /timeline?y=XXXX.
*/
static const char *statsReportTimelineYFlag = NULL;


/*
** Generate a submenu element with a single parameter change.
*/
static void statrep_submenu(
  HQuery *pUrl,            /* Base URL */
  const char *zMenuName,   /* Submenu name */
  const char *zParam,      /* Parameter value to add or change */
  const char *zValue,      /* Value of the new parameter */
  const char *zRemove      /* Parameter to omit */
){
  style_submenu_element(zMenuName, zMenuName, "%s",
                        url_render(pUrl, zParam, zValue, zRemove, 0));
}

/*
** Creates a TEMP VIEW named v_reports which is a wrapper around the
** EVENT table filtered on event.type. It looks for the request
** parameter 'type' (reminder: we "should" use 'y' for consistency
** with /timeline, but /reports uses 'y' for the year) and expects it
** to contain one of the conventional values from event.type or the
** value "all", which is treated as equivalent to "*".  By default (if
** no 'y' is specified), "*" is assumed (that is also the default for
** invalid/unknown filter values). That 'y' filter is the one used for
** the event list. Note that a filter of "*" or "all" is equivalent to
** querying against the full event table. The view, however, adds an
** abstraction level to simplify the implementation code for the
** various /reports pages.
**
** Returns one of: 'c', 'w', 'g', 't', 'e', representing the type of
** filter it applies, or '*' if no filter is applied (i.e. if "all" is
** used).
*/
static int stats_report_init_view(){
  const char *zType = PD("type","*");  /* analog to /timeline?y=... */
  const char *zRealType = NULL;        /* normalized form of zType */
  int rc = 0;                          /* result code */
  assert( !statsReportType && "Must not be called more than once." );
  switch( (zType && *zType) ? *zType : 0 ){
    case 'c':
    case 'C':
      zRealType = "ci";
      rc = *zRealType;
      break;
    case 'e':
    case 'E':
      zRealType = "e";
      rc = *zRealType;
      break;
    case 'g':
    case 'G':
      zRealType = "g";
      rc = *zRealType;
      break;
    case 't':
    case 'T':
      zRealType = "t";
      rc = *zRealType;
      break;
    case 'w':
    case 'W':
      zRealType = "w";
      rc = *zRealType;
      break;
    default:
      rc = '*';
      break;
  }
  assert(0 != rc);
  if(zRealType){
    statsReportTimelineYFlag = zRealType;
    db_multi_exec("CREATE TEMP VIEW v_reports AS "
                  "SELECT * FROM event WHERE type GLOB %Q",
                  zRealType);
  }else{
    statsReportTimelineYFlag = "a";
    db_multi_exec("CREATE TEMP VIEW v_reports AS "
                  "SELECT * FROM event");
  }
  return statsReportType = rc;
}

/*
** Returns a string suitable (for a given value of suitable) for
** use in a label with the header of the /reports pages, dependent
** on the 'type' flag. See stats_report_init_view().
** The returned bytes are static.
*/
static const char *stats_report_label_for_type(){
  assert( statsReportType && "Must call stats_report_init_view() first." );
  switch( statsReportType ){
    case 'c':
      return "check-ins";
    case 'e':
      return "technotes";
    case 'w':
      return "wiki changes";
    case 't':
      return "ticket changes";
    case 'g':
      return "tag changes";
    default:
      return "all types";
  }
}

/*
** A helper for the /reports family of pages which prints out a menu
** of links for the various type=XXX flags. zCurrentViewName must be
** the name/value of the 'view' parameter which is in effect at the
** time this is called. e.g. if called from the 'byuser' view then
** zCurrentViewName must be "byuser". Any URL parameters which need to
** be added to the generated URLs should be passed in zParam. The
** caller is expected to have already encoded any zParam in the %T or
** %t encoding.  */
static void stats_report_event_types_menu(const char *zCurrentViewName,
                                          const char *zParam){
  char *zTop;
  if(zParam && !*zParam){
    zParam = NULL;
  }
  zTop = mprintf("%s/reports?view=%s%s%s", g.zTop, zCurrentViewName,
                 zParam ? "&" : "", zParam);
  cgi_printf("<div>");
  cgi_printf("<span>Types:</span> ");
  if('*' == statsReportType){
    cgi_printf(" <strong>all</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s'>all</a>", zTop);
  }
  if('c' == statsReportType){
    cgi_printf(" <strong>check-ins</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=ci'>check-ins</a>", zTop);
  }
  if('e' == statsReportType){
    cgi_printf(" <strong>technotes</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=e'>technotes</a>", zTop);
  }
  if( 't' == statsReportType ){
    cgi_printf(" <strong>tickets</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=t'>tickets</a>", zTop);
  }
  if( 'g' == statsReportType ){
    cgi_printf(" <strong>tags</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=g'>tags</a>", zTop);
  }
  if( 'w' == statsReportType ){
    cgi_printf(" <strong>wiki</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=w'>wiki</a>", zTop);
  }
  fossil_free(zTop);
  cgi_printf("</div>");
}


/*
** Helper for stats_report_by_month_year(), which generates a list of
** week numbers. zTimeframe should be either a timeframe in the form YYYY
** or YYYY-MM.
*/
static void stats_report_output_week_links(const char *zTimeframe){
  Stmt stWeek = empty_Stmt;
  char yearPart[5] = {0,0,0,0,0};
  memcpy(yearPart, zTimeframe, 4);
  db_prepare(&stWeek,
             "SELECT DISTINCT strftime('%%W',mtime) AS wk, "
             "count(*) AS n, "
             "substr(date(mtime),1,%d) AS ym "
             "FROM v_reports "
             "WHERE ym=%Q AND mtime < current_timestamp "
             "GROUP BY wk ORDER BY wk",
             strlen(zTimeframe),
             zTimeframe);
  while( SQLITE_ROW == db_step(&stWeek) ){
    const char *zWeek = db_column_text(&stWeek,0);
    const int nCount = db_column_int(&stWeek,1);
    cgi_printf("<a href='%R/timeline?"
               "yw=%t-%t&n=%d&y=%s'>%s</a>",
               yearPart, zWeek,
               nCount, statsReportTimelineYFlag, zWeek);
  }
  db_finalize(&stWeek);
}

/*
** Implements the "byyear" and "bymonth" reports for /reports.
** If includeMonth is true then it generates the "bymonth" report,
** else the "byyear" report. If zUserName is not NULL and not empty
** then the report is restricted to events created by the named user
** account.
*/
static void stats_report_by_month_year(char includeMonth,
                                       char includeWeeks,
                                       const char *zUserName){
  Stmt query = empty_Stmt;
  int nRowNumber = 0;                /* current TR number */
  int nEventTotal = 0;               /* Total event count */
  int rowClass = 0;                  /* counter for alternating
                                        row colors */
  Blob sql = empty_blob;             /* SQL */
  const char *zTimeLabel = includeMonth ? "Year/Month" : "Year";
  char zPrevYear[5] = {0};           /* For keeping track of when
                                        we change years while looping */
  int nEventsPerYear = 0;            /* Total event count for the
                                        current year */
  char showYearTotal = 0;            /* Flag telling us when to show
                                        the per-year event totals */
  Blob header = empty_blob;          /* Page header text */
  int nMaxEvents  = 1;               /* for calculating length of graph
                                        bars. */
  int iterations = 0;                /* number of weeks/months we iterate
                                        over */
  stats_report_init_view();
  stats_report_event_types_menu( includeMonth ? "bymonth" : "byyear", NULL );
  blob_appendf(&header, "Timeline Events (%s) by year%s",
               stats_report_label_for_type(),
               (includeMonth ? "/month" : ""));
  blob_append_sql(&sql,
               "SELECT substr(date(mtime),1,%d) AS timeframe, "
               "count(*) AS eventCount "
               "FROM v_reports ",
               includeMonth ? 7 : 4);
  if(zUserName&&*zUserName){
    blob_append_sql(&sql, " WHERE user=%Q ", zUserName);
    blob_appendf(&header," for user %q", zUserName);
  }
  blob_append(&sql,
              " GROUP BY timeframe"
              " ORDER BY timeframe DESC",
              -1);
  db_prepare(&query, "%s", blob_sql_text(&sql));
  blob_reset(&sql);
  cgi_printf("<h1>%b</h1>\n"
         "<table class='statistics-report-table-events' border='0' cellpadding='2'\n"
         " cellspacing='0' id='statsTable'>\n"
         "<thead>\n"
         "<th>%s</th>\n"
         "<th>Events</th>\n"
         "<th width='90%%'><!-- relative commits graph --></th>\n"
         "</thead><tbody>\n",(&header),(zTimeLabel));
  blob_reset(&header);
  /*
     Run the query twice. The first time we calculate the maximum
     number of events for a given row. Maybe someone with better SQL
     Fu can re-implement this with a single query.
  */
  while( SQLITE_ROW == db_step(&query) ){
    const int nCount = db_column_int(&query, 1);
    if(nCount>nMaxEvents){
      nMaxEvents = nCount;
    }
    ++iterations;
  }
  db_reset(&query);
  while( SQLITE_ROW == db_step(&query) ){
    const char *zTimeframe = db_column_text(&query, 0);
    const int nCount = db_column_int(&query, 1);
    int nSize = nCount
      ? (int)(100 * nCount / nMaxEvents)
      : 1;
    showYearTotal = 0;
    if(!nSize) nSize = 1;
    if(includeMonth){
      /* For Month/year view, add a separator for each distinct year. */
      if(!*zPrevYear ||
         (0!=fossil_strncmp(zPrevYear,zTimeframe,4))){
        showYearTotal = *zPrevYear;
        if(showYearTotal){
          rowClass = ++nRowNumber % 2;
          cgi_printf("<tr class='row%d'>\n"
                 "<td></td>\n"
                 "<td colspan='2'>Yearly total: %d</td>\n"
                "</tr>\n",(rowClass),(nEventsPerYear));
        }
        nEventsPerYear = 0;
        memcpy(zPrevYear,zTimeframe,4);
        rowClass = ++nRowNumber % 2;
        cgi_printf("<tr class='row%d'>\n"
               "<th colspan='3' class='statistics-report-row-year'>%s</th>\n"
               "</tr>\n",(rowClass),(zPrevYear));
     }
   }
   rowClass = ++nRowNumber % 2;
   nEventTotal += nCount;
   nEventsPerYear += nCount;
  cgi_printf("<tr class='row%d'>\n"
          "<td>\n",(rowClass));
    if(includeMonth){
      cgi_printf("<a href='%R/timeline?"
                 "ym=%t&n=%d&y=%s",
                 zTimeframe, nCount,
                 statsReportTimelineYFlag );
      /* Reminder: n=nCount is not actually correct for bymonth unless
         that was the only user who caused events.
      */
      if( zUserName && *zUserName ){
        cgi_printf("&u=%t", zUserName);
      }
      cgi_printf("' target='_new'>%s</a>",zTimeframe);
    }else {
      cgi_printf("<a href='?view=byweek&y=%s&type=%c",
                 zTimeframe, (char)statsReportType);
      if(zUserName && *zUserName){
        cgi_printf("&u=%t", zUserName);
      }
      cgi_printf("'>%s</a>", zTimeframe);
    }
    cgi_printf("</td><td>%d</td>\n"
           "<td>\n"
           "<div class='statistics-report-graph-line'\n"
           " style='width:%d%%;'>&nbsp;</div>\n"
           "</td>\n"
          "</tr>\n",(nCount),(nSize));
    if(includeWeeks){
      /* This part works fine for months but it terribly slow (4.5s on my PC),
         so it's only shown for by-year for now. Suggestions/patches for
         a better/faster layout are welcomed. */
      cgi_printf("<tr class='row%d'>\n"
             "<td colspan='2' class='statistics-report-week-number-label'>Week #:</td>\n"
             "<td class='statistics-report-week-of-year-list'>\n",(rowClass));
      stats_report_output_week_links(zTimeframe);
      cgi_printf("</td></tr>\n");
    }

    /*
      Potential improvement: calculate the min/max event counts and
      use percent-based graph bars.
    */
  }
  db_finalize(&query);
  if(includeMonth && !showYearTotal && *zPrevYear){
    /* Add final year total separator. */
    rowClass = ++nRowNumber % 2;
    cgi_printf("<tr class='row%d'>\n"
           "<td></td>\n"
           "<td colspan='2'>Yearly total: %d</td>\n"
          "</tr>\n",(rowClass),(nEventsPerYear));
  }
  cgi_printf("</tbody></table>\n");
  if(nEventTotal){
    const char *zAvgLabel = includeMonth ? "month" : "year";
    int nAvg = iterations ? (nEventTotal/iterations) : 0;
    cgi_printf("<br><div>Total events: %d\n"
           "<br>Average per active %s: %d\n"
           "</div>\n",(nEventTotal),(zAvgLabel),(nAvg));
  }
  if( !includeMonth ){
    output_table_sorting_javascript("statsTable","tnx",-1);
  }
}

/*
** Implements the "byuser" view for /reports.
*/
static void stats_report_by_user(){
  Stmt query = empty_Stmt;
  int nRowNumber = 0;                /* current TR number */
  int nEventTotal = 0;               /* Total event count */
  int rowClass = 0;                  /* counter for alternating
                                        row colors */
  int nMaxEvents = 1;                /* max number of events for
                                        all rows. */
  stats_report_init_view();
  stats_report_event_types_menu("byuser", NULL);
  db_prepare(&query,
               "SELECT user, "
               "COUNT(*) AS eventCount "
               "FROM v_reports "
               "GROUP BY user ORDER BY eventCount DESC");
  cgi_printf("<h1>Timeline Events\n"
         "(%s) by User</h1>\n"
         "<table class='statistics-report-table-events' border='0'\n"
         "cellpadding='2' cellspacing='0' id='statsTable'>\n"
         "<thead><tr>\n"
         "<th>User</th>\n"
         "<th>Events</th>\n"
         "<th width='90%%'><!-- relative commits graph --></th>\n"
         "</tr></thead><tbody>\n",(stats_report_label_for_type()));
  while( SQLITE_ROW == db_step(&query) ){
    const int nCount = db_column_int(&query, 1);
    if(nCount>nMaxEvents){
      nMaxEvents = nCount;
    }
  }
  db_reset(&query);
  while( SQLITE_ROW == db_step(&query) ){
    const char *zUser = db_column_text(&query, 0);
    const int nCount = db_column_int(&query, 1);
    int nSize = nCount
      ? (int)(100 * nCount / nMaxEvents)
      : 0;
    if(!nCount) continue /* arguable! Possible? */;
    else if(!nSize) nSize = 1;
    rowClass = ++nRowNumber % 2;
    nEventTotal += nCount;
   cgi_printf("<tr class='row%d'>\n"
           "<td>\n"
           "<a href=\"?view=bymonth&user=%h&type=%c\">%h</a>\n"
           "</td><td data-sortkey='%08x'>%d</td>\n"
           "<td>\n"
           "<div class='statistics-report-graph-line'\n"
           " style='width:%d%%;'>&nbsp;</div>\n"
           "</td>\n"
          "</tr>\n",(rowClass),(zUser),((char)statsReportType),(zUser),(-nCount),(nCount),(nSize));
    /*
      Potential improvement: calculate the min/max event counts and
      use percent-based graph bars.
    */
  }
  cgi_printf("</tbody></table>\n");
  db_finalize(&query);
  output_table_sorting_javascript("statsTable","tkx",2);
}

/*
** Implements the "byfile" view for /reports.
*/
static void stats_report_by_file(){
  Stmt query;
  int mxEvent = 1;       /* max number of events across all rows */
  int nRowNumber = 0;

  db_multi_exec(
    "CREATE TEMP TABLE statrep(filename, cnt);"
    "INSERT INTO statrep(filename, cnt)"
    "  SELECT filename.name, count(distinct mlink.mid)"
    "    FROM filename, mlink"
    "   WHERE filename.fnid=mlink.fnid"
    "   GROUP BY 1"
  );
  db_prepare(&query,
    "SELECT filename, cnt FROM statrep ORDER BY cnt DESC, filename /*sort*/"
  );
  mxEvent = db_int(1, "SELECT max(cnt) FROM statrep");
  cgi_printf("<h1>Check-ins Per File</h1>\n"
         "<table class='statistics-report-table-events' border='0'\n"
         "cellpadding='2' cellspacing='0' id='statsTable'>\n"
         "<thead><tr>\n"
         "<th>File</th>\n"
         "<th>Check-ins</th>\n"
         "<th width='90%%'><!-- relative commits graph --></th>\n"
         "</tr></thead><tbody>\n");
  while( SQLITE_ROW == db_step(&query) ){
    const char *zFile = db_column_text(&query, 0);
    const int n = db_column_int(&query, 1);
    int sz;
    if( n<=0 ) continue;
    sz = (int)(100*n/mxEvent);
    if( sz==0 ) sz = 1;
   cgi_printf("<tr class='row%d'>\n"
           "<td>%z%h</a></td>\n"
           "<td>%d</td>\n"
           "<td>\n"
           "<div class='statistics-report-graph-line'\n"
           " style='width:%d%%;'>&nbsp;</div>\n"
           "</td>\n"
          "</tr>\n",(++nRowNumber%2),(href("%R/finfo?name=%T",zFile)),(zFile),(n),(sz));
  }
  cgi_printf("</tbody></table>\n");
  db_finalize(&query);
  output_table_sorting_javascript("statsTable","tNx",2);
}

/*
** Implements the "byweekday" view for /reports.
*/
static void stats_report_day_of_week(){
  Stmt query = empty_Stmt;
  int nRowNumber = 0;                /* current TR number */
  int nEventTotal = 0;               /* Total event count */
  int rowClass = 0;                  /* counter for alternating
                                        row colors */
  int nMaxEvents = 1;                /* max number of events for
                                        all rows. */
  static const char *const daysOfWeek[] = {
  "Monday", "Tuesday", "Wednesday", "Thursday",
  "Friday", "Saturday", "Sunday"
  };

  stats_report_init_view();
  stats_report_event_types_menu("byweekday", NULL);
  db_prepare(&query,
               "SELECT cast(mtime %% 7 AS INTEGER) dow, "
               "COUNT(*) AS eventCount "
               "FROM v_reports "
               "GROUP BY dow ORDER BY dow");
  cgi_printf("<h1>Timeline Events\n"
         "(%s) by Day of the Week</h1>\n"
         "<table class='statistics-report-table-events' border='0'\n"
         "cellpadding='2' cellspacing='0' id='statsTable'>\n"
         "<thead><tr>\n"
         "<th>DoW</th>\n"
         "<th>Day</th>\n"
         "<th>Events</th>\n"
         "<th width='90%%'><!-- relative commits graph --></th>\n"
         "</tr></thead><tbody>\n",(stats_report_label_for_type()));
  while( SQLITE_ROW == db_step(&query) ){
    const int nCount = db_column_int(&query, 1);
    if(nCount>nMaxEvents){
      nMaxEvents = nCount;
    }
  }
  db_reset(&query);
  while( SQLITE_ROW == db_step(&query) ){
    const int dayNum =db_column_int(&query, 0);
    const int nCount = db_column_int(&query, 1);
    int nSize = nCount
      ? (int)(100 * nCount / nMaxEvents)
      : 0;
    if(!nCount) continue /* arguable! Possible? */;
    else if(!nSize) nSize = 1;
    rowClass = ++nRowNumber % 2;
    nEventTotal += nCount;
   cgi_printf("<tr class='row%d'>\n"
           "<td>%d</td>\n"
           "<td>%s</td>\n"
           "<td>%d</td>\n"
           "<td>\n"
           "<div class='statistics-report-graph-line'\n"
           " style='width:%d%%;'>&nbsp;</div>\n"
           "</td>\n"
          "</tr>\n",(rowClass),(dayNum),(daysOfWeek[dayNum]),(nCount),(nSize));
  }
  cgi_printf("</tbody></table>\n");
  db_finalize(&query);
  output_table_sorting_javascript("statsTable","ntnx",1);
}


/*
** Helper for stats_report_by_month_year(), which generates a list of
** week numbers. zTimeframe should be either a timeframe in the form YYYY
** or YYYY-MM.
*/
static void stats_report_year_weeks(const char *zUserName){
  const char *zYear = P("y");
  int nYear = zYear ? strlen(zYear) : 0;
  int i = 0;
  Stmt qYears = empty_Stmt;
  char *zDefaultYear = NULL;
  Blob sql = empty_blob;
  int nMaxEvents = 1;                /* max number of events for
                                        all rows. */
  int iterations = 0;                /* # of active time periods. */
  stats_report_init_view();
  if(4==nYear){
    Blob urlParams = empty_blob;
    blob_appendf(&urlParams, "y=%T", zYear);
    stats_report_event_types_menu("byweek", blob_str(&urlParams));
    blob_reset(&urlParams);
  }else{
    stats_report_event_types_menu("byweek", NULL);
  }
  blob_append(&sql,
              "SELECT DISTINCT substr(date(mtime),1,4) AS y "
              "FROM v_reports WHERE 1 ", -1);
  if(zUserName&&*zUserName){
    blob_append_sql(&sql,"AND user=%Q ", zUserName);
  }
  blob_append(&sql,"GROUP BY y ORDER BY y", -1);
  db_prepare(&qYears, "%s", blob_sql_text(&sql));
  blob_reset(&sql);
  cgi_printf("Select year: ");
  while( SQLITE_ROW == db_step(&qYears) ){
    const char *zT = db_column_text(&qYears, 0);
    if( i++ ){
      cgi_printf(" ");
    }
    cgi_printf("<a href='?view=byweek&y=%s&type=%c", zT,
               (char)statsReportType);
    if(zUserName && *zUserName){
      cgi_printf("&user=%t",zUserName);
    }
    cgi_printf("'>%s</a>",zT);
  }
  db_finalize(&qYears);
  cgi_printf("<br/>");
  if(!zYear || !*zYear){
    zDefaultYear = db_text("????", "SELECT strftime('%%Y')");
    zYear = zDefaultYear;
    nYear = 4;
  }
  if(4 == nYear){
    Stmt stWeek = empty_Stmt;
    int rowCount = 0;
    int total = 0;
    Blob header = empty_blob;
    blob_appendf(&header, "Timeline events (%s) for the calendar weeks "
                 "of %h", stats_report_label_for_type(),
                 zYear);
    blob_append_sql(&sql,
                 "SELECT DISTINCT strftime('%%W',mtime) AS wk, "
                 "count(*) AS n "
                 "FROM v_reports "
                 "WHERE %Q=substr(date(mtime),1,4) "
                 "AND mtime < current_timestamp ",
                 zYear);
    if(zUserName&&*zUserName){
      blob_append_sql(&sql, " AND user=%Q ", zUserName);
      blob_appendf(&header," for user %h", zUserName);
    }
    blob_append_sql(&sql, "GROUP BY wk ORDER BY wk DESC");
    cgi_printf("<h1>%h</h1>", blob_str(&header));
    blob_reset(&header);
    cgi_printf("<table class='statistics-report-table-events' "
               "border='0' cellpadding='2' width='100%%' "
               "cellspacing='0' id='statsTable'>");
    cgi_printf("<thead><tr>"
               "<th>Week</th>"
               "<th>Events</th>"
               "<th width='90%%'><!-- relative commits graph --></th>"
               "</tr></thead>"
               "<tbody>");
    db_prepare(&stWeek, "%s", blob_sql_text(&sql));
    blob_reset(&sql);
    while( SQLITE_ROW == db_step(&stWeek) ){
      const int nCount = db_column_int(&stWeek, 1);
      if(nCount>nMaxEvents){
        nMaxEvents = nCount;
      }
      ++iterations;
    }
    db_reset(&stWeek);
    while( SQLITE_ROW == db_step(&stWeek) ){
      const char *zWeek = db_column_text(&stWeek,0);
      const int nCount = db_column_int(&stWeek,1);
      int nSize = nCount
        ? (int)(100 * nCount / nMaxEvents)
        : 0;
      if(!nSize) nSize = 1;
      total += nCount;
      cgi_printf("<tr class='row%d'>", ++rowCount % 2 );
      cgi_printf("<td><a href='%R/timeline?yw=%t-%s&n=%d&y=%s",
                 zYear, zWeek, nCount,
                 statsReportTimelineYFlag);
      if(zUserName && *zUserName){
        cgi_printf("&u=%t",zUserName);
      }
      cgi_printf("'>%s</a></td>",zWeek);

      cgi_printf("<td>%d</td>",nCount);
      cgi_printf("<td>");
      if(nCount){
        cgi_printf("<div class='statistics-report-graph-line'"
                   "style='width:%d%%;'>&nbsp;</div>",
                   nSize);
      }
      cgi_printf("</td></tr>");
    }
    db_finalize(&stWeek);
    free(zDefaultYear);
    cgi_printf("</tbody></table>");
    if(total){
      int nAvg = iterations ? (total/iterations) : 0;
      cgi_printf("<br><div>Total events: %d<br>"
                 "Average per active week: %d</div>",
                 total, nAvg);
    }
    output_table_sorting_javascript("statsTable","tnx",-1);
  }
}

/*
** WEBPAGE: reports
**
** Shows activity reports for the repository.
**
** Query Parameters:
**
**   view=REPORT_NAME  Valid values: bymonth, byyear, byuser
**   user=NAME         Restricts statistics to the given user
**   type=TYPE         Restricts the report to a specific event type:
**                     ci (check-in), w (wiki), t (ticket), g (tag)
**                     Defaulting to all event types.
**
** The view-specific query parameters include:
**
** view=byweek:
**
**   y=YYYY            The year to report (default is the server's
**                     current year).
*/
void stats_report_page(){
  HQuery url;                        /* URL for various branch links */
  const char *zView = P("view");    /* Which view/report to show. */
  const char *zUserName = P("user");

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  if(!zUserName) zUserName = P("u");
  url_initialize(&url, "reports");
  if(zUserName && *zUserName){
    url_add_parameter(&url,"user", zUserName);
    statrep_submenu(&url, "(Remove User Flag)", "view", zView, "user");
  }
  statrep_submenu(&url, "By Year", "view", "byyear", 0);
  statrep_submenu(&url, "By Month", "view", "bymonth", 0);
  statrep_submenu(&url, "By Week", "view", "byweek", 0);
  statrep_submenu(&url, "By Weekday", "view", "byweekday", 0);
  statrep_submenu(&url, "By User", "view", "byuser", "user");
  statrep_submenu(&url, "By File", "view", "byfile", "file");
  style_submenu_element("Stats", "Stats", "%R/stat");
  url_reset(&url);
  style_header("Activity Reports");
  if(0==fossil_strcmp(zView,"byyear")){
    stats_report_by_month_year(0, 0, zUserName);
  }else if(0==fossil_strcmp(zView,"bymonth")){
    stats_report_by_month_year(1, 0, zUserName);
  }else if(0==fossil_strcmp(zView,"byweek")){
    stats_report_year_weeks(zUserName);
  }else if(0==fossil_strcmp(zView,"byuser")){
    stats_report_by_user();
  }else if(0==fossil_strcmp(zView,"byweekday")){
    stats_report_day_of_week();
  }else if(0==fossil_strcmp(zView,"byfile")){
    stats_report_by_file();
  }else{
    cgi_printf("<h1>Activity Reports:</h1>\n"
           "<ul>\n"
           "<li>%zEvents by year</a></li>\n"
           "<li>%zEvents by month</a></li>\n"
           "<li>%zEvents by calendar week</a></li>\n"
           "<li>%zEvents by day of the week</a></li>\n"
           "<li>%zEvents by user</a></li>\n"
           "<li>%zEvents by file</a></li>\n"
           "</ul>\n",(href("?view=byyear")),(href("?view=bymonth")),(href("?view=byweek")),(href("?view=byweekday")),(href("?view=byuser")),(href("?view=byfile")));
  }

  style_footer();
}
