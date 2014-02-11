
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_openOptions = 0;


}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	/* _errFile is always outFile, so will have been freed already
	if ( _errFile ) {
		free( _errFile );
	}
	*/


	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();
	
	// save stdin / stdout / stderr
	int tempIn = dup(0);
	int tempOut = dup(1);
	int tempErr = dup(2);

	// set input
	int fdIn;
	if (_inputFile) { 	// open file for reading
		fdIn = open(_inputFile, O_RDONLY); 
	} else { 			// use default input
		fdIn = dup(tempIn);
	}

	int i;
	int fdOut;
	pid_t child;
	for ( i = 0; i < _numberOfSimpleCommands; i++ ) {

		// redirect input and close fdIn since we're done with it
		dup2(fdIn, 0);
		close(fdIn);
		
		// setup output
		if (i == _numberOfSimpleCommands - 1) { // last simple command
			if (_outFile) { //redirect output
				mode_t openMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // -rw-r-----
				fdOut = open(_outFile, _openOptions, openMode);
			} else { //use default output
				fdOut = dup(tempOut);
			}

	 	} else { // not last simple command, so create a pipe
			int fdPipe[2];
			pipe(fdPipe);
			fdOut = fdPipe[1];
			fdIn = fdPipe[0];
		}
		
		dup2(fdOut, 1);  // redirect output
		if (_errFile) {  // redirect error at same location as output if necessary
			dup2( fdOut, 2); 
		}
		close(fdOut); //close fdOut since we're done with it
 
		child = fork();
		if (child == 0) { //child process
			signal(SIGINT, SIG_DFL); //reset SIGINT for great good
			
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);

			//if the child process reaches this point, execvp has failed
			perror("execvp");
			_exit(1);

		} else if (child < 0) {
			fprintf(stderr, "Fork failed\n");
			_exit(1);
		}

	} // endfor

	// restore in/out/err defaults
	dup2(tempIn, 0);
	dup2(tempOut, 1);
	dup2(tempErr, 2);
	close(tempIn);
	close(tempOut);
	close(tempErr);

	if (!_background) {
		waitpid(child, NULL, 0);
	}

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("myshell $ ");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;


int yyparse(void);

void sigint_handler(int p) {
	printf("\n");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}

main()
{
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
	sa.sa_flags = SA_RESTART; //restart any interrupted system calls
	sigemptyset(&sa.sa_mask);

	//set the SIGINT handler
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	Command::_currentCommand.prompt();
	yyparse();
}

