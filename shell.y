
/*
 * CS-252 Spring 2013
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN NEWLINE GREAT LESS PIPE AMPERSAND GREATGREAT GREATGREATAMPERSAND GREATAMPERSAND

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <fcntl.h> // open() arguments
#include <string.h> // strcmp
#include "command.h"
void yyerror(const char * s);
int yylex();

%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
        ;

simple_command:	
	pipe_list iomodifier_list background_opt NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE { 
		Command::_currentCommand.clear();
		Command::_currentCommand.prompt();
	}
	| error NEWLINE { yyerrok; }
	;

pipe_list:
	pipe_list PIPE {
	} command_and_args {
	}
	| command_and_args { 
	}

	;


command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
		printf("   Yacc: insert argument \"%s\"\n", $1);

	    Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

command_word:
	WORD {
		printf("   Yacc: insert command \"%s\"\n", $1);
	       
		// handle "exit"
		if (strcmp($1, "exit") == 0) { 
			exit(0);
		}

	    Command::_currentSimpleCommand = new SimpleCommand();
	    Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
	iomodifier_list iomodifier
	| /* empty */
	;

iomodifier:
	GREATGREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);

		// append stdout to file
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT;
		Command::_currentCommand._outFile = $2;
	}
	| GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);

		// rewrite the file if it already exists
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT | O_TRUNC;
		Command::_currentCommand._outFile = $2;
	}
	| GREATGREATAMPERSAND WORD {
		//redirect stdout and stderr to file and append
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;

	}
	| GREATAMPERSAND WORD {
		//redirect stdout and stderr to file and truncate
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT | O_TRUNC;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;

	}
	| LESS WORD {
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	} 
	;

background_opt:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	}
	| /* empty */
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
