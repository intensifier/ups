/* config.c - load/save ups configuration information */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)config.c	1.5 04 Jun 1995 (UKC) */
char ups_config_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "obj_bpt.h"
#include "obj_signal.h"
#include "config.h"
#include "va.h"
#include "ui.h"
#include "state.h"
#include "util.h"

static void show_statefile_error PROTO((const char *mesg));

static errf_ofunc_t Old_ofunc;
static const char *Errs_filename;
static int Errs_lnum;

static void
show_statefile_error(mesg)
const char *mesg;
{
	if (Errs_lnum > 0) {
		char *copy;
		
		copy = strf("%s,%d: %s", Errs_filename, Errs_lnum, mesg);
		(*Old_ofunc)(copy);
		free(copy);
	}
	else {
		(*Old_ofunc)(mesg);
	}
}

int
save_state(state_dir, state_path, want_save_sigs)
const char *state_dir, *state_path;
bool want_save_sigs;
{
	struct stat stbuf;
	FILE *fp;
	int res;
	const char *dot;
	
	if (stat(state_dir, &stbuf) != 0) {
		if (errno != ENOENT) {
			failmesg("Can't stat", "state directory", state_dir);
			return -1;
		}

		if (mkdir(state_dir, 0777) != 0) {
			failmesg("Can't create", "state directory", state_dir);
			return -1;
		}
	}
	else if (!S_ISDIR(stbuf.st_mode)) {
		errf("%s should be a directory, but is a %s", state_dir,
		     filetype_to_string((int)stbuf.st_mode));
		return -1;
	}

	if ((fp = fopen_with_twiddle(state_path, "w")) == NULL) {
		failmesg("Can't create", "ups state file", state_path);
		return -1;
	}

	dot = strrchr(state_path, '.');
	fputs("# Do not edit this file - ", fp);
	fputs("it is overwritten automatically by ups.\n", fp);
	fprintf(fp, "# Edit %.*s.config, .upsrc or $HOME/.upsrc instead.\n\n",
		(int)(dot - state_path), state_path);

	res = save_all_breakpoints_to_file(state_path, fp);

	if (res == 0)
		res = write_var_state_to_file(state_path, fp);

	if (res == 0 && want_save_sigs == TRUE)
		res = save_signal_state_to_file(fp);

	if (ferror(fp) || fflush(fp) == EOF) {
		failmesg("Error writing to", "ups state file", state_path);
		res = -1;
	}

	fclose(fp);
	
	return res;
}

void
make_config_paths(state_dir, basepath, p_state_path, p_user_path)
const char *state_dir, *basepath, **p_state_path, **p_user_path;
{
	const char *target_name, *suf;
	int len;

	target_name = base_name(basepath);
	if ((suf = strrchr(target_name, '.')) != NULL)
		len = suf - target_name;
	else
		len = strlen(target_name);

	*p_state_path = strf("%s/%.*s.state", state_dir, len, target_name);
	*p_user_path = strf("%s/%.*s.config", state_dir, len, target_name);
}

void
load_config(state_path, user_path, p_want_auto_start)
const char *state_path, *user_path;
bool *p_want_auto_start;
{
	const char *home;
	bool want_auto_start;
	
	want_auto_start = FALSE;

	if ((home = getenv("HOME")) != NULL) {
		char *path;

		path = strf("%s/.upsrc", home);
		read_config_file(path, FALSE, FALSE, FALSE, FALSE,
				 &want_auto_start, FALSE);
		free(path);
	}
		
	read_config_file(".upsrc", FALSE,  FALSE, FALSE, FALSE,
			 &want_auto_start, FALSE);
	read_config_file(user_path, FALSE,  FALSE, FALSE, FALSE,
			 &want_auto_start, FALSE);
	read_config_file(state_path, TRUE,  FALSE, FALSE, TRUE,
			 &want_auto_start, FALSE);

	set_breakpoint_statefile_path(state_path);

	*p_want_auto_start = want_auto_start;
}

int
read_config_file(path, from_statefile, must_exist, breakpoints_only,
		 ignore_breakpoints, p_want_auto_start, no_statefile_errors)
const char *path;
bool from_statefile, must_exist, breakpoints_only, ignore_breakpoints;
bool *p_want_auto_start;
bool no_statefile_errors;
{
	FILE *fp;
	char *line;
	int res;
	
	if ((fp = fopen_with_twiddle(path, "r")) == NULL) {
		if (must_exist) {
			failmesg("Can't open", "state file", path);
			return -1;
		}
		
		if (errno != ENOENT)
			failmesg("Warning: can't open", "state file", path);

		return 0;
	}

	if (!no_statefile_errors)
	{
	  Old_ofunc = errf_set_ofunc(show_statefile_error);

	/*  As there may be lots of errors, send them to stderr.
	 */
	/* RGA disable this, so user can see what symbols are being loaded
	   - can be logged so no messages are lost 
	*/
/*	old_message_wn = get_message_wn();*/
/*	set_message_wn(-1);*/
	}

	Errs_filename = path;
	Errs_lnum = 0;

	res = 0;
	
	while ((line = fpgetline(fp)) != NULL) {
		char **fields, **fields2, **fields3, *cmd, **args = NULL;
		int nargs;

		++Errs_lnum;

		line = config_trim_line(line);
		
		fields = ssplit(line, " \t");
		
		if (fields[0] == NULL) {
			free((char *)fields);
			continue;
		}

		cmd = fields[0];
		fields2 = fields3 = NULL;

		/* RGA for C++, function names can have imbeded spaces 
		 so function names are enclosed in double quotes now.  
		 So extract the  function name into arg[0] first 
		 by spliting with '"', then split the rest normally  
		 
		 3.37: use unquoted mangled name so overloaded methods are unique,
		 followed by demangled name in quotes for readability.
		 Change scheme so the demangled quoted names are ignored.
		 */

		if (strchr(line, '"') && (strcmp(cmd, "breakpoint") == 0 ||
					  strcmp(cmd, "function") == 0 ||
					  strcmp(cmd, "format-hint") == 0 ||
					  strcmp(cmd, "format") == 0))
		{
		  char **args1, **args2;
		  int i, nargs1, nargs2;

		  if (strcmp(cmd, "breakpoint") == 0)
		    cmd = "breakpoint";
		  else
		    if (strcmp(cmd, "function") == 0)
		      cmd = "function";
		  else
		    if (strcmp(cmd, "format-hint") == 0)
		      cmd = "format-hint";
		  else
		    cmd = "format";
		  free((char *)fields);
		  fields = ssplit(line, " \t");
		  fields2 = ssplit(line, "\"\t");
		  fields3 = ssplit(fields2[2], " \t");
		  args1 = &fields[1];
		  for (nargs1 = 0; args1[nargs1] != NULL; ++nargs1);
		  args2 = &fields3[0];
		  for (nargs2 = 0; args2[nargs2] != NULL; ++nargs2);
		  args = (char **)e_malloc((2 + nargs2) * sizeof(char *));
		  args[0] = fields[1];
		  for (i = 1; i <= nargs2; ++i)
		    args[i] = fields3[i-1];
                  args[nargs2+1] = NULL;
		}
		else
		  args = &fields[1];

		for (nargs = 0; args[nargs] != NULL; ++nargs)
			;

		if (strcmp(cmd, "breakpoint") == 0) {
			if (handle_add_breakpoint_command(cmd, args, nargs,
							  from_statefile,
							  ignore_breakpoints,
							  fp, &Errs_lnum)!= 0) {
				res = -1;
			}
		}
		else if (strcmp(cmd, "code") == 0) {
			if (handle_add_code_command(cmd, args, nargs,
						    from_statefile,
						    ignore_breakpoints,
						    fp, &Errs_lnum)!= 0) {
				res = -1;
			}
		}
		else if (breakpoints_only) {
			/* Skip rest (tmp bodge pending command table) */
		}
		else if (strcmp(cmd, "format") == 0) {
			if (handle_format_command(cmd, args, nargs, FALSE) != 0)
				res = -1;
		}
		else if (strcmp(cmd, "format-hint") == 0) {
			if (handle_format_command(cmd, args, nargs, TRUE) != 0)
				res = -1;
		}
		else if (strcmp(cmd, "file") == 0 ||
			 strcmp(cmd, "common") == 0 ||
			 strcmp(cmd, "globals") == 0) {
			if (handle_file_command(cmd, args, nargs,
							  from_statefile,
							  fp, &Errs_lnum) != 0)
				res = -1;
		}
		else if (strcmp(cmd, "function") == 0) {
			if (handle_function_command(cmd, args, nargs,
							  from_statefile,
							  fp, &Errs_lnum) != 0)
				res = -1;
		}
		else if (strcmp(cmd, "auto-start") == 0) {
			if (nargs != 1 || (strcmp(*args, "yes") != 0 &&
					   strcmp(*args, "no") != 0)) {
				errf("Usage: auto-start yes|no");
			}
			else {
				*p_want_auto_start = strcmp(*args, "yes") == 0;
			}
		}
		else if (strcmp(cmd, "echo") == 0) {
			bool want_newline;
			
			want_newline = strcmp(*args, "-n") != 0;
			if (!want_newline)
				++args;

			while (*args != NULL) {
				fputs(*args++, stdout);
				if (*args != NULL)
					fputc(' ', stdout);
			}

			if (want_newline)
				fputc('\n', stdout);

			fflush(stdout);
		}
		else if (strcmp(cmd, "signal") == 0) {
			if (handle_signal_command(cmd, args, nargs, FALSE) != 0)
				res = -1;
		}
		else {
			errf("Unrecognised command `%s'", cmd);
		}

		free((char *)fields);
		if (fields2 != NULL)
		{
		  free((char *)fields2);
		  free((char *)fields3);
		  free((char *)args);
		}
	}

	fclose(fp);

	if (!no_statefile_errors)
	{
	  errf_set_ofunc(Old_ofunc);
/*	set_message_wn(old_message_wn);*/
	}
	return res;
}	

void
load_state_file()
{
  char *state_path;
  bool want_auto_start;
  static char *lastpath = NULL;
		
  if (lastpath == NULL)
    lastpath = strsave("");
		
  if (prompt_for_string("filename", "Load state from file: ",
			lastpath, &state_path) != 0)
    return;
		
  if (strcmp(state_path, lastpath) != 0) {
    free(lastpath);
    lastpath = strsave(state_path);
  }
		
  want_auto_start = FALSE;
		
  read_config_file(state_path, TRUE,  TRUE, FALSE, TRUE,
		   &want_auto_start, FALSE);
  set_breakpoint_statefile_path(state_path); 
}

void
save_state_file()
{
  const char *mode, *prompt;
  FILE *fp;
  char *state_path;
  int res = 0;
  target_t *xp;

  prompt = "Save state to file: ";
	
  if (prompt_for_output_file(prompt, &state_path, &mode) != 0)
    return;

  if ((fp = fopen_with_twiddle(state_path, mode)) == NULL)
  {
    errf("Can't create `%s' ups state file", state_path);
    return;
  }
  xp = get_current_target();
  if (target_process_exists(xp))
    save_var_display_state(TRUE, FALSE);

  res = save_all_breakpoints_to_file(state_path, fp); 
		
  if (res == 0)
    res = write_var_state_to_file(state_path, fp);
		
  if (res == 0			/* && want_save_sigs == TRUE*/)
    res = save_signal_state_to_file(fp);
		
  if (ferror(fp) || fflush(fp) == EOF) 
    errf("Error writing to `%s' ups state file", state_path);

  fclose(fp);
}
