/***************************************************************************/
/* Copyright (c) 2004                                                      */
/* Daniel Sleator, David Temperley, and John Lafferty                      */
/* Copyright (c) 2008 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software,      */
/* and also available at http://www.link.cs.cmu.edu/link/license.html      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

 /****************************************************************************
 *
 *   This is a simple example of the link parser API.  It similates most of
 *   the functionality of the original link grammar parser, allowing sentences
 *   to be typed in either interactively or in "batch" mode (if -batch is
 *   specified on the command line, and stdin is redirected to a file).
 *   The program:
 *     Opens up a dictionary
 *     Iterates:
 *        1. Reads from stdin to get an input string to parse
 *        2. Tokenizes the string to form a Sentence
 *        3. Tries to parse it with cost 0
 *        4. Tries to parse with increasing cost
 *     When a parse is found:
 *        1. Extracts each Linkage
 *        2. Passes it to process_some_linkages()
 *        3. Deletes linkage
 *     After parsing each Sentence is deleted by making a call to
 *     sentence_delete.
 *
 ****************************************************************************/

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Used for terminal resizing */
#ifndef _WIN32
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef HAVE_EDITLINE
#include <editline/readline.h>
#endif 

#ifdef _MSC_VER
#define LINK_GRAMMAR_DLL_EXPORT 0
#endif

#include <link-grammar/link-includes.h>
#include <link-grammar/structures.h>
#include <link-grammar/error.h>

#include "command-line.h"
#include "expand.h"

#define MAX_INPUT 1024
#define DISPLAY_MAX 1024
#define COMMENT_CHAR '%'  /* input lines beginning with this are ignored */

static int batch_errors = 0;
static int input_pending=FALSE;
static Parse_Options  opts;
static Parse_Options  panic_parse_opts;
static int verbosity = 0;
/*############## Added newly ####################*/
char            link_info_filename[500];
/*############## Added ended ####################*/
typedef enum
{
	UNGRAMMATICAL='*',
	PARSE_WITH_DISJUNCT_COST_GT_0=':',  /* Not used anywhere, currently ... */
	NO_LABEL=' '
} Label;

static char *
fget_input_string(FILE *in, FILE *out, Parse_Options opts)
{
#ifdef HAVE_EDITLINE
	static char * pline = NULL;
//newly Added 	const char * prompt = "linkparser> ";

	if (in != stdin)
	{
		static char input_string[MAX_INPUT];
		input_pending = FALSE;
		if (fgets(input_string, MAX_INPUT, in)) return input_string;
		return NULL;
	}

	if (input_pending && pline != NULL)
	{
		input_pending = FALSE;
		return pline;
	}
	if (parse_options_get_batch_mode(opts) ||
	    (verbosity == 0) ||
	    input_pending)
	{
		prompt = "";
	}
	input_pending = FALSE;
	if (pline) free(pline);
	pline = readline(prompt);

	/* Save non-blank lines */
	if (pline && *pline)
	{
		if (*pline) add_history(pline);
	}
	return pline;

#else
	static char input_string[MAX_INPUT];

	if ((!parse_options_get_batch_mode(opts)) && 
	    (verbosity > 0) && 
	    (!input_pending))
	{
//newly Added   fprintf(out, "linkparser> ");
		fflush(out);
	}
	input_pending = FALSE;

	/* For UTF-8 input, I think its still technically correct to
	 * use fgets() and not fgetws() at this point. */
	if (fgets(input_string, MAX_INPUT, in)) return input_string;
	else return NULL;
#endif
}

static int fget_input_char(FILE *in, FILE *out, Parse_Options opts)
{
#ifdef HAVE_EDITLINE
	char * pline = fget_input_string(in, out, opts);
	if (NULL == pline) return EOF;
	if (*pline)
	{
		input_pending = TRUE;
		return *pline;
	}
	return '\n';

#else
	int c;

	if (!parse_options_get_batch_mode(opts) && (verbosity > 0))
//newly Added		fprintf(out, "linkparser> ");
	fflush(out);

	/* For UTF-8 input, I think its still technically correct to
	 * use fgetc() and not fgetwc() at this point. */
	c = fgetc(in);
	if (c != '\n')
	{
		ungetc(c, in);
		input_pending = TRUE;
	}
	return c;
#endif
}

/**************************************************************************
*
*  This procedure displays a linkage graphically.  Since the diagrams
*  are passed as character strings, they need to be deleted with a
*  call to free.
*
**************************************************************************/
//################################### Added newly #################################################
static void process_linkage(Linkage linkage, Parse_Options opts,char * tmp_path,char * file_name)
//static void process_linkage(Linkage linkage, Parse_Options opts)
//#################################################################################################
{
	char * string;
	int j, mode, first_sublinkage;
	int nlink;

	if (!linkage) return;  /* Can happen in timeout mode */

	if (parse_options_get_use_fat_links(opts) &&
	    parse_options_get_display_union(opts))
	{
		linkage_compute_union(linkage);
		first_sublinkage = linkage_get_num_sublinkages(linkage)-1;
	}
	else
	{
		first_sublinkage = 0;
	}

	nlink = linkage_get_num_sublinkages(linkage);
	for (j=first_sublinkage; j<nlink; ++j)
	{
		linkage_set_current_sublinkage(linkage, j);
		if (parse_options_get_display_on(opts))
		{
			string = linkage_print_diagram(linkage);
	//newly Added		fprintf(stdout, "%s", string);
			linkage_free_diagram(string);
		}
		if (parse_options_get_display_links(opts))
		{
			string = linkage_print_links_and_domains(linkage);
        //newly Added 	fprintf(stdout, "%s", string);
			linkage_free_links_and_domains(string);
		}
		if (parse_options_get_display_senses(opts))
		{
			string = linkage_print_senses(linkage);
	//newly Added 	fprintf(stdout, "%s", string);
			linkage_free_senses(string);
		}
		if (parse_options_get_display_disjuncts(opts))
		{
			string = linkage_print_disjuncts(linkage);
	//newly Added 	fprintf(stdout, "%s", string);
			linkage_free_disjuncts(string);
		}
		if (parse_options_get_display_postscript(opts))
		{
		    //################## Added newly #########################################
			 string = linkage_print_postscript(linkage, FALSE,tmp_path,file_name);
		//	string = linkage_print_postscript(linkage, FALSE);
		// 	fprintf(stdout, "%s\n", string);
		    //########################################################################
			linkage_free_postscript(string);
		}
	}
	if ((mode = parse_options_get_display_constituents(opts)))
	{
		string = linkage_print_constituent_tree(linkage, mode);
		if (string != NULL)
		{
	//newly Added 	fprintf(stdout, "%s\n", string);
			linkage_free_constituent_tree_str(string);
		}
		else
		{
			fprintf(stderr, "Can't generate constituents.\n");
			fprintf(stderr, "Constituent processing has been turned off.\n");
		}
	}
}
/* //newly Added
static void print_parse_statistics(Sentence sent, Parse_Options opts)
{
	if (sentence_num_linkages_found(sent) > 0)
	{  
		if (sentence_num_linkages_found(sent) >
			parse_options_get_linkage_limit(opts))
		{
			fprintf(stdout, "Found %d linkage%s (%d of %d random " \
					"linkages had no P.P. violations)",
					sentence_num_linkages_found(sent),
					sentence_num_linkages_found(sent) == 1 ? "" : "s",
					sentence_num_valid_linkages(sent),
					sentence_num_linkages_post_processed(sent)); 
		}
		else
		{
			fprintf(stdout, "Found %d linkage%s (%d had no P.P. violations)",
					sentence_num_linkages_post_processed(sent),
					sentence_num_linkages_found(sent) == 1 ? "" : "s",
					sentence_num_valid_linkages(sent));
		}
		if (sentence_null_count(sent) > 0)
		{
			fprintf(stdout, " at null count %d", sentence_null_count(sent));
		}
		fprintf(stdout, "\n");
	}
}

*/
//############################### Added newly ##################################################################
 static int process_some_linkages(Sentence sent, Parse_Options opts , int num ,char * tmp_path,char * file_name)
// static int process_some_linkages(Sentence sent, Parse_Options opts)
//################################################################################################################
{
	int c;
	int i, num_to_query, num_to_display, num_displayed;
	Linkage linkage;
//newly Added	double corpus_cost;

//newly Added	if (verbosity > 0) print_parse_statistics(sent, opts);
	num_to_query = MIN(sentence_num_linkages_post_processed(sent),
	                   DISPLAY_MAX);
	if (!parse_options_get_display_bad(opts))
	{
		num_to_display = MIN(sentence_num_valid_linkages(sent),
		                     DISPLAY_MAX);
	}
	else
	{
		num_to_display = MIN(sentence_num_linkages_post_processed(sent),
		                     DISPLAY_MAX);
	}

	for (i=0, num_displayed=0; i<num_to_query; i++)
	{
	  /*####### Added newly #######*/
		 if (i==num)
             {
          /*###########################*/

		if ((sentence_num_violations(sent, i) > 0) &&
			(!parse_options_get_display_bad(opts)))
		{
			continue;
		}

		linkage = linkage_create(i, sent, opts);

	/* Currently, sat solver returns NULL when there ain't no more */
		if (!linkage) break;
		/*##### Added newly ##############
		if (verbosity > 0)
		{
			if ((sentence_num_valid_linkages(sent) == 1) &&
				(!parse_options_get_display_bad(opts)))
			{
				fprintf(stdout, "	Unique linkage, ");
			}
			else if ((parse_options_get_display_bad(opts)) &&
			         (sentence_num_violations(sent, i) > 0))
			{
				fprintf(stdout, "	Linkage %d (bad), ", num_displayed+1);
			}
			else
			{
				fprintf(stdout, "	Linkage %d, ", num_displayed+1);
			}

			if (!linkage_is_canonical(linkage))
			{
				fprintf(stdout, "non-canonical, ");
			}
			if (linkage_is_improper(linkage))
			{
				fprintf(stdout, "improper fat linkage, ");
			}
			if (linkage_has_inconsistent_domains(linkage))
			{
				fprintf(stdout, "inconsistent domains, ");
			}

			corpus_cost = linkage_corpus_cost(linkage);
			if (corpus_cost < 0.0f)
			{
				fprintf(stdout, "cost vector = (UNUSED=%d DIS=%d FAT=%d AND=%d LEN=%d)\n",
				       linkage_unused_word_cost(linkage),
				       linkage_disjunct_cost(linkage),
				       linkage_is_fat(linkage),
				       linkage_and_cost(linkage),
				       linkage_link_cost(linkage));
			}
			else
			{
				fprintf(stdout, "cost vector = (CORP=%6.4f UNUSED=%d DIS=%d FAT=%d AND=%d LEN=%d)\n",
				       corpus_cost,
				       linkage_unused_word_cost(linkage),
				       linkage_disjunct_cost(linkage),
				       linkage_is_fat(linkage),
				       linkage_and_cost(linkage),
				       linkage_link_cost(linkage));
			}
		}*/
                process_linkage(linkage, opts,tmp_path,file_name);
 	//	process_linkage(linkage, opts);
	//################  Added Ended  ##################################
		linkage_delete(linkage);

		if (++num_displayed < num_to_display)
		{
			/*####################### Added newly ##############################
			if (verbosity > 0)
			{
				fprintf(stdout, "Press RETURN for the next linkage.\n");
			}
			#####################################################################*/
			c = fget_input_char(stdin, stdout, opts);
			if (c != '\n') return c;
		}
		else
		{
			break;
		}
    /*#### Added newly #####*/
        } // if num
    /*######################*/
	}
	return 'x';
}

static int there_was_an_error(Label label, Sentence sent, Parse_Options opts)
{
	if (sentence_num_valid_linkages(sent) > 0) {
		if (label == UNGRAMMATICAL) {
			batch_errors++;
			return UNGRAMMATICAL;
		}
		if ((sentence_disjunct_cost(sent, 0) == 0) &&
			(label == PARSE_WITH_DISJUNCT_COST_GT_0)) {
			batch_errors++;
			return PARSE_WITH_DISJUNCT_COST_GT_0;
		}
	} else {
		if (label != UNGRAMMATICAL) {
			batch_errors++;
			return UNGRAMMATICAL;
		}
	}
	return FALSE;
}
//########################## Added newly #####################################################
//static void batch_process_some_linkages(Label label, 
//                                      Sentence sent,
//                                    Parse_Options opts)
 static void batch_process_some_linkages(Label label,
                                        Sentence sent,
                                        Parse_Options opts,char * tmp_path,char * file_name)
//####################Added ended #############################################################
{
	Linkage linkage;

	if (there_was_an_error(label, sent, opts)) {
		/* Note: sentence_num_linkages_found returns total linkages
		 * not valid linkages. So the printed linkage might be bad... 
		 */
		if (sentence_num_linkages_found(sent) > 0) {
			linkage = linkage_create(0, sent, opts);
		//###################### Added newly ######################
			process_linkage(linkage, opts,tmp_path,file_name);
//			process_linkage(linkage, opts);
		//#########################################################
			linkage_delete(linkage);
		}
		fprintf(stdout, "+++++ error %d\n", batch_errors);
	}
}

static int special_command(char *input_string, Dictionary dict)
{
	if (input_string[0] == '\n') return TRUE;
	if (input_string[0] == COMMENT_CHAR) return TRUE;
	if (input_string[0] == '!') {
		if (strncmp(input_string, "!panic_", 7) == 0)
		{
			issue_special_command(input_string+7, panic_parse_opts, dict);
			return TRUE;
		}

		issue_special_command(input_string+1, opts, dict);
		return TRUE;
	}
	return FALSE;
}

static Label strip_off_label(char * input_string)
{
	Label c;

	c = (Label) input_string[0];
	switch(c) {
	case UNGRAMMATICAL:
	case PARSE_WITH_DISJUNCT_COST_GT_0:
		input_string[0] = ' ';
		return c;
	case NO_LABEL:
	default:
		return NO_LABEL;
	}
}

static void setup_panic_parse_options(Parse_Options opts)
{
	parse_options_set_disjunct_costf(opts, 3.0f);
	parse_options_set_min_null_count(opts, 1);
	parse_options_set_max_null_count(opts, MAX_SENTENCE);
	parse_options_set_max_parse_time(opts, 60);
	parse_options_set_use_fat_links(opts, 0);
	parse_options_set_islands_ok(opts, 1);
	parse_options_set_short_length(opts, 6);
	parse_options_set_all_short_connectors(opts, 1);
	parse_options_set_linkage_limit(opts, 100);
	parse_options_set_spell_guess(opts, FALSE);
}

static void print_usage(char *str) {
	fprintf(stderr,
			"Usage: %s [language|dictionary location]\n"
			"		  [-<special \"!\" command>]\n"
			"		  [--version]\n", str);

	fprintf(stderr, "\nSpecial commands are:\n");
	opts = parse_options_create();
	issue_special_command("var", opts, NULL);
	exit(-1);
}

/**
 * On Unix, this checks for the current window size, 
 * and sets the output screen width accordingly. 
 * Not sure how MS Windows does this.
 */
static void check_winsize(Parse_Options popts)
{
/* Neither windows nor MSYS have the ioctl support needed for this. */
#ifdef _WIN32
	parse_options_set_screen_width(popts, 79);
#else 
	struct winsize ws;
	int fd = open("/dev/tty", O_RDWR);

	if (0 != ioctl(fd, TIOCGWINSZ, &ws))
	{
		perror("ioctl(/dev/tty, TIOCGWINSZ)");
		close(fd);
		return;
	}
	close(fd);

	/* printf("rows %i\n", ws.ws_row); */
	/* printf("cols %i\n", ws.ws_col); */

	/* Set the screen width only if the returned value seems
	 * rational: its positive and not insanely tiny.
	 */
	if ((10 < ws.ws_col) && (16123 > ws.ws_col))
	{
		parse_options_set_screen_width(popts, ws.ws_col - 1);
	}
#endif /* _WIN32 */
}

int main(int argc, char * argv[])
{
	FILE            *input_fh = stdin;
	Dictionary      dict;
	const char     *language="en";  /* default to english, and not locale */
	int             num_linkages, i;
	Label           label = NO_LABEL;
	const char      *codeset;
        /*##################Added newly ##########################################################################*/
        int             num=0;
        FILE            *fp_word_info,*fp_num,*fp_lname,*fp_rel,*fp_lcount,*fp_word;
       /*###################Added ended ###########################################################################*/

#if LATER
	/* Try to catch the SIGNWINCH ... except this is not working. */
	struct sigaction winch_act;
	winch_act.sa_handler = winch_handler;
	winch_act.sa_sigaction = NULL;
	sigemptyset (&winch_act.sa_mask);
	winch_act.sa_flags = 0;
	sigaction (SIGWINCH, &winch_act, NULL);
#endif

	i = 1;
	/*######################### Added newly #######################*/
	if (argc > 4){
                 num =atoi(argv[4]);
                 num=num-1;
	//if ((argc > 1) && (argv[1][0] != '-')) {
		/* the dictionary is the first argument if it doesn't begin with "-" */
	//	language = argv[1];
	/*######################## Added newly ########################*/
		i++;
	}
	/*###############Added newly #################*/
        language = argv[1];
        /*###############Added ended #################*/

	/* Get the locale from the environment... 
	 * perhaps we should someday get it from the dictionary ??
	 */
	setlocale(LC_ALL, "");

	/* Check to make sure the current locale is UTF8; if its not,
	 * then force-set this to the english utf8 locale 
	 */
	codeset = nl_langinfo(CODESET);
	if (!strstr(codeset, "UTF") && !strstr(codeset, "utf"))
	{
		fprintf(stderr, 
		    "%s: Warning: locale %s was not UTF-8; force-setting to en_US.UTF-8\n",
		     argv[0], codeset);
		setlocale(LC_CTYPE, "en_US.UTF-8");
	}
   	/*##################Added newly ######################*/
        for (; i<argc-3; i++) {
	//for (; i<argc; i++) {
	/*##################Added ended ######################*/
		if (argv[i][0] == '-')
		{
			if (strcmp("--version", argv[i])==0)
			{
				printf("Version: %s\n", linkgrammar_get_version());
				exit(0);
			}
			/* TBD remove these in version 5.0 */
			else if ((strcmp("-ppoff", argv[i])==0) ||
			         (strcmp("-coff", argv[i])==0) ||
			         (strcmp("-aoff", argv[i])==0))
			{
				fprintf(stderr, "%s: Warning: %s flag ignored\n", argv[0], argv[i]);
			}
		}
		else
		{
			print_usage(argv[0]);
		}
	}

	opts = parse_options_create();
	if (opts == NULL)
	{
		fprintf(stderr, "%s: Fatal error: unable to create parse options\n", argv[0]);
		exit(-1);
	}

	panic_parse_opts = parse_options_create();
	if (panic_parse_opts == NULL)
	{
		fprintf(stderr, "%s: Fatal error: unable to create panic parse options\n", argv[0]);
		exit(-1);
	}
	setup_panic_parse_options(panic_parse_opts);
	parse_options_set_max_sentence_length(opts, 170);
	parse_options_set_panic_mode(opts, TRUE);
	parse_options_set_max_parse_time(opts, 30);
	parse_options_set_linkage_limit(opts, 1000);
	parse_options_set_short_length(opts, 10);
	parse_options_set_disjunct_costf(opts, 2.0f);
	parse_options_set_min_null_count(opts, 0);
	parse_options_set_max_null_count(opts, 0);

	if (language && *language)
		dict = dictionary_create_lang(language);
	else
		dict = dictionary_create_default_lang();

	if (dict == NULL)
	{
		fprintf(stderr, "%s: Fatal error: Unable to open  dictionary.\n", argv[0]);
		exit(-1);
	}

	/* Process the command line like commands */
	for (i=1; i<argc; i++)
	{
		/* TBD remove these in version 5.0 */
		if ((strcmp("-pp", argv[i]) == 0) ||
			(strcmp("-c", argv[i]) == 0) ||
			(strcmp("-a", argv[i]) == 0) ||
			(strcmp("-ppoff", argv[i]) == 0) ||
			(strcmp("-coff", argv[i]) == 0) ||
			(strcmp("-aoff", argv[i]) == 0))
		{
			i++;
		}
		else if (argv[i][0] == '-')
		{
			int rc;
			if (argv[i][1] == '!')
				rc = issue_special_command(argv[i]+2, opts, dict);
			else
				rc = issue_special_command(argv[i]+1, opts, dict);

			if (rc)
				print_usage(argv[0]);
		}
	}

	verbosity = parse_options_get_verbosity(opts);
	check_winsize(opts);
	//#############  Added newly ############################
/*      prt_error("Info: Dictionary version %s.\n",
        linkgrammar_get_dict_version(dict));
	prt_error("Info: Library version %s. Enter \"!help\" for help.\n",
		linkgrammar_get_version());*/
 //##########################################################
	/* Main input loop */
	while (1)
	{
		char *input_string;
		Sentence sent = NULL;

		input_string = fget_input_string(input_fh, stdout, opts);
		check_winsize(opts);

		if (NULL == input_string)
		{
			if (input_fh == stdin) break;
			fclose (input_fh);
			input_fh = stdin;
			continue;
		}

		if ((strcmp(input_string, "!quit") == 0) ||
		    (strcmp(input_string, "!exit") == 0)) break;

		/* We have to handle the !file command inline; its too hairy
		 * otherwise ... */
		if (strncmp(input_string, "!file", 5) == 0)
		{
			char * filename = &input_string[6];
			input_fh = fopen(filename, "r");
			if (NULL == input_fh)
			{
				int perr = errno;
				fprintf(stderr, "Error: %s (%d) %s\n",
				        filename, perr, strerror(perr));
				input_fh = stdin;
				continue;
			}
			continue;
		}

		if (special_command(input_string, dict)) continue;
		if (parse_options_get_echo_on(opts))
		{
			printf("%s ", input_string);
		}

		if (parse_options_get_batch_mode(opts))
		{
			label = strip_off_label(input_string);
		}

		sent = sentence_create(input_string, dict);

		if (sent == NULL) continue;

		if (sentence_length(sent) > parse_options_get_max_sentence_length(opts))
		{
			if (verbosity > 0)
			{
				fprintf(stdout,
				       "Sentence length (%d words) exceeds maximum allowable (%d words)\n",
					sentence_length(sent), parse_options_get_max_sentence_length(opts));
			}
			sentence_delete(sent);
			sent = NULL;
			continue;
		}

		/* First parse with cost 0 or 1 and no null links */
		// parse_options_set_disjunct_costf(opts, 2.0f);
		parse_options_set_min_null_count(opts, 0);
		parse_options_set_max_null_count(opts, 0);
		parse_options_reset_resources(opts);

		num_linkages = sentence_parse(sent, opts);

		//#####################  Added newly  ########################
		if(num+1>num_linkages && num_linkages != 0)
                {
                 sprintf(link_info_filename,"%s/%s_tmp/linkid_word.txt",argv[2],argv[3]);
                 fp_word= fopen(link_info_filename, "a");
                 if(fp_word==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}
                 fprintf(fp_word,";~~~~~~~~~~\n");
                 fclose(fp_word);

                 sprintf(link_info_filename,"%s/%s_tmp/link_numeric_word.txt",argv[2],argv[3]);
                 fp_word_info= fopen(link_info_filename, "a");
                 if(fp_word_info==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}
                 fprintf(fp_word_info,";~~~~~~~~~~\n");
                 fclose(fp_word_info);

                 sprintf(link_info_filename,"%s/%s_tmp/link_name_expand.txt",argv[2],argv[3]);
                 fp_lname =fopen(link_info_filename,"a");
                 if(fp_lname==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}
                 fprintf(fp_lname,";~~~~~~~~~~\n");
                 fclose(fp_lname);

                 sprintf(link_info_filename,"%s/%s_tmp/link_relation_info.txt",argv[2],argv[3]);
                 fp_rel = fopen(link_info_filename,"a");
                 if(fp_rel==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}
                 fprintf(fp_rel,";~~~~~~~~~~\n");
                 fclose(fp_rel);
               }
 	         sprintf(link_info_filename,"%s/%s_tmp/linkage_count.txt",argv[2],argv[3]);
                 fp_lcount = fopen(link_info_filename,"a");
                 if(fp_lcount==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}
                 fprintf(fp_lcount, "(Found maximum of %d linkages )\n",num_linkages);
                 fprintf(fp_lcount,";~~~~~~~~~~\n");
                 fclose(fp_lcount);
                //############################################################################################

		if (num_linkages < 0)
		{
			sentence_delete(sent);
			sent = NULL;
			continue;
		}

#if 0
		/* Try again, this time ommitting the requirement for
		 * definite articles, etc. This should allow for the parsing
		 * of newspaper headlines and other clipped speech.
		 *
		 * XXX Unfortunately, this also allows for the parsing of
		 * all sorts of ungrammatical sentences which should not 
		 * parse, and leads to bad parses of many other unparsable
		 * but otherwise grammatical sentences.  Thus, this trick
		 * pretty much fails; we leave it here to document the 
		 * experiment.
		 */
		if (num_linkages == 0)
		{
			parse_options_set_disjunct_costf(opts, 3.5f);
			num_linkages = sentence_parse(sent, opts);
			if (num_linkages < 0) continue;
		}
#endif

		/* Try using a larger list of disjuncts */
		if ((num_linkages == 0) && parse_options_get_use_cluster_disjuncts(opts))
		{
			int expanded;
			if (verbosity > 0) // newly Added fprintf(stdout, "No standard linkages, expanding disjunct set.\n");
			parse_options_set_disjunct_costf(opts, 2.9f);
			expanded = lg_expand_disjunct_list(sent);
			if (expanded)
			{
				num_linkages = sentence_parse(sent, opts);
			}
			if (0 < num_linkages) printf("Got One !!!!!!!!!!!!!!!!!\n");
		}

		/* If asked to show bad linkages, then show them. */
		if ((num_linkages == 0) && (!parse_options_get_batch_mode(opts)))
		{
			if (parse_options_get_display_bad(opts))
			{
				num_linkages = sentence_num_linkages_found(sent);
			}
		}

		/* Now parse with null links */
		if ((num_linkages == 0) && (!parse_options_get_batch_mode(opts)))
		{
			if (verbosity > 0) // newly Added fprintf(stdout, "No complete linkages found.\n");
		//########################  Added newly ###################################################
               /*  This part of the code written by Maha Laxmi and Shirisha Manju
                *  if no complete linkage is found then redirect the output to standard output in clips format*/
		 sprintf(link_info_filename,"%s/%s_tmp/number.txt",argv[2],argv[3]);
        	 fp_num =fopen(link_info_filename,"a");
	         if(fp_num==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}

         	 sprintf(link_info_filename,"%s/%s_tmp/linkid_word.txt",argv[2],argv[3]);
                 fp_word= fopen(link_info_filename, "a");
                 if(fp_word==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}

        	 sprintf(link_info_filename,"%s/%s_tmp/link_numeric_word.txt",argv[2],argv[3]);
	         fp_word_info= fopen(link_info_filename, "a");
        	 if(fp_word_info==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}

        	 sprintf(link_info_filename,"%s/%s_tmp/link_name_expand.txt",argv[2],argv[3]);
	         fp_lname =fopen(link_info_filename,"a");
        	 if(fp_lname==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}

	         sprintf(link_info_filename,"%s/%s_tmp/link_relation_info.txt",argv[2],argv[3]);
        	 fp_rel = fopen(link_info_filename,"a");
	         if(fp_rel==NULL) {printf("Could not open %s for writing\n",link_info_filename);exit(1);}
	
	 	 fprintf(fp_word, "\n(No complete linkages found)\n");
	         fprintf(fp_word_info, "\n(No complete linkages found)\n");
        	 fprintf(fp_lname, "\n(No complete linkages found)\n");
	         fprintf(fp_rel,"\n(No complete linkages found)\n");

         	 fprintf(fp_lname,";~~~~~~~~~~\n");       fprintf(fp_rel,";~~~~~~~~~~\n");
	         fprintf(fp_word_info,";~~~~~~~~~~\n");   fprintf(fp_word,";~~~~~~~~~~\n");

		 fclose(fp_word_info);
	         fclose(fp_lname);       fclose(fp_rel);       fclose(fp_word);

        	 continue;
	  /*#######################################################################################################*/

			if (parse_options_get_allow_null(opts))
			{
				/* XXX should use expanded disjunct list here too */
				parse_options_set_min_null_count(opts, 1);
				parse_options_set_max_null_count(opts, sentence_length(sent));
				num_linkages = sentence_parse(sent, opts);
			}
		}

		if (parse_options_timer_expired(opts))
		{
			if (verbosity > 0) fprintf(stdout, "Timer is expired!\n");
		}
		if (parse_options_memory_exhausted(opts))
		{
			if (verbosity > 0) fprintf(stdout, "Memory is exhausted!\n");
		}

		if ((num_linkages == 0) &&
			parse_options_resources_exhausted(opts) &&
			parse_options_get_panic_mode(opts))
		{
			/* print_total_time(opts); */
			batch_errors++;
			if (verbosity > 0) fprintf(stdout, "Entering \"panic\" mode...\n");
			parse_options_reset_resources(panic_parse_opts);
			parse_options_set_verbosity(panic_parse_opts, verbosity);
			num_linkages = sentence_parse(sent, panic_parse_opts);
			if (parse_options_timer_expired(panic_parse_opts)) {
				if (verbosity > 0) fprintf(stdout, "Timer is expired!\n");
			}
		}

		/* print_total_time(opts); */

		if (parse_options_get_batch_mode(opts))
		{
			 //################### Added newly #######################################
                            batch_process_some_linkages(label, sent, opts,argv[2],argv[3]);
                          // batch_process_some_linkages(label, sent, opts);
                        //#########################################################################
		}
		else
		{
			//################### Added newly #######################################
                        //   send num as an argument to process_some_linkages
                        int c = process_some_linkages(sent, opts,num,argv[2],argv[3]);
//                      int c = process_some_linkages(sent, opts);
                        //#########################################################################

			if (c == EOF)
			{
				sentence_delete(sent);
				sent = NULL;
				break;
			}
		}
		fflush(stdout);

		sentence_delete(sent);
		sent = NULL;
	}

	if (parse_options_get_batch_mode(opts))
	{
		/* print_time(opts, "Total"); */
		/*################### Added newly #######################################
		fprintf(stderr,
				"%d error%s.\n", batch_errors, (batch_errors==1) ? "" : "s");
		#########################################################################*/
	}

	/* Free stuff, so that mem-leak detectors don't commplain. */
	parse_options_delete(panic_parse_opts);
	parse_options_delete(opts);
	dictionary_delete(dict);

//newly Added	printf ("Bye.\n");
	return 0;
}
