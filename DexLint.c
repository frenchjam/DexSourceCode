/*********************************************************************************************************************************/
/*                                                                                                                               */
/*                                                          DexLint.c                                                            */
/*                                                                                                                               */
/*********************************************************************************************************************************/

/*

  A tool to maintain DEX (GRIP) scripts. 
  Starting from a users.dex file, it walks the file hierarchy and verifies that all dependent files are present.
  It also generates a list that can be used to create an archive of those, and only those files that are reachable via the GRIP GUI.

  */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>

#include <DexInterpreterFunctions.h>

#define TRUE	1
#define FALSE	0

enum { NORMAL_EXIT = 0, NO_USER_FILE, NO_LOG_FILE, ERROR_EXIT };

char picture_path[1024] = "..\\DexPictures";
FILE *log;

/*********************************************************************************************************************************/

#define MAX_FILELIST_SIZE	2560

// A list of picture filenames, where duplicate files appear just once.
char global_picture_filenames[MAX_FILELIST_SIZE][256];
int	 global_pictures = 0;

void add_to_global_picture_list ( const char *filename ) {

	int j;

	// If we haven't seen it already in any file, add it to the list of pictures.
	for ( j = 0; j < global_pictures; j++ ) {
		if ( !strcmp( global_picture_filenames[j], filename ) ) break;
	}
	if ( j == global_pictures ) {
		strcpy( global_picture_filenames[j], filename );
		global_pictures++;
	}

}

// A list of script filenames, where duplicate files appear just once.
char global_script_filenames[MAX_FILELIST_SIZE][256];
int	 global_scripts = 0;

void add_to_global_script_list ( const char *filename ) {

	int j;

	// If we haven't seen it already in any file, add it to the list of pictures.
	for ( j = 0; j < global_scripts; j++ ) {
		if ( !strcmp( global_script_filenames[j], filename ) ) break;
	}
	if ( j == global_scripts ) {
		strcpy( global_script_filenames[j], filename );
		global_scripts++;
	}

}

/*********************************************************************************************************************************/

int process_task_file ( char *filename, int verbose ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i, j;

	char local_picture_file[2560][256];
	char path[1024];

	int local_pictures = 0;

	int errors = 0;

	fp = fopen( filename, "r" );
	if ( !fp ) {
		fprintf( log, "      Error opening task file %s for read.\n", filename );
		return( 1 );
	}

	if ( verbose ) fprintf( stderr, "\n  File: %s", filename );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimitedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\n  Line:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}

		for ( i = 1; i < tokens; i++ ) {
			// Look for picture files. They are all .bmp.
			if ( strstr( token[i], ".bmp" ) ) {
				if ( verbose ) fprintf( stderr, "     %d Line %03d Picture File: %s\n", filename, line_n, token[i] );

				// Look to see if we've seen this one before in this file.
				for ( j = 0; j < local_pictures; j++ ) {
					if ( !strcmp( local_picture_file[j], token[i] ) ) break;
				}
				// If we haven't seen it already in this file, signal if it does not exist.
				// If we've already seen it in this file (j != local_pictures ), don't signal the error again.
				if ( j == local_pictures ) {
					strcpy( local_picture_file[j], token[i] );
					local_pictures++;

					strcpy( path, picture_path );
					strcat( path, token[i] );
					if ( _access( path, 0x00 ) ) {
						fprintf( log, "     %s Line %3d Picture file not found: %s\n", filename, line_n, path );
						errors++;
					}

				}

				// Add the file to the list of picture files, if it is not there already.
				add_to_global_picture_list( token[i] );

			}	
		}
	}

	fclose( fp );

	return( errors );

}

/*********************************************************************************************************************************/

int process_protocol_file ( char *filename, int verbose ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i;

	// Keep track of task IDs. They have to be unique in a protocol.
	int taskID[256];
	int tasks = 0;

	int errors = 0;

	char task_file[1024];

	fp = fopen( filename, "r" );
	if ( !fp ) {
		// Signal the error.
		fprintf( log, "    Error opening protocol file %s for read.\n", filename );
		// Tell the caller that we had just the one error.
		return( 1 );
	}

	if ( verbose ) fprintf( stderr, "\n  File: %s", filename );

	// Work our way through the protocol file, line-by-line.
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimitedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\n  Line:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}

		// Lines in protocol files have 4 fields.
		if ( tokens == 4 ) {
			// First parameter must be CMD_TASK.
			if ( strcmp( token[0], "CMD_TASK" ) ) {
				fprintf( log, "   %s Line %03d Command not CMD_TASK: %s\n", filename, line_n, token[0] );
				printf( "   %s Line %03d Command not CMD_TASK: %s\n", filename, line_n, token[0] );
				errors++;
			}
			// Second parameter is a task ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &taskID[tasks] ) ) {
				printf( "  %s Line %03d Error reading task ID: %s\n", filename, line_n, token[1] );
				fprintf( log, "  %s Line %03d Error reading task ID: %s\n", filename, line_n, token[1] );
				errors++;
			}
			else {
				// Task IDs must be unique.
				for ( i = 0; i < tasks; i++ ) {
					if ( taskID[tasks] == taskID[i] ) {
						printf( "  %s Line %03d Duplicate task ID: %s\n", filename, line_n, token[1] );
						fprintf( log, "  %s Line %03d Duplicate task ID: %s\n", filename, line_n, token[1] );
						errors++;
					}
				}
			}
			// This parameter is the name of the task file.
			strcpy( task_file, token[2] );
			// Check if it exists and is readable.
			if ( _access( task_file, 0x00 ) ) {
				printf( "  %s Line %03d Cannot access task file: %s\n", filename, line_n, task_file );
				fprintf( log, "  %s Line %03d Cannot access task file: %s\n", filename, line_n, task_file );
				errors++;
			}	
			else {
				// Add the filename to the master list, if it is not there already.
				add_to_global_script_list( task_file );
				// Process it and count any errors that occur while doing so.
				errors += process_task_file( task_file, verbose );
			}
			// There is a fourth field to the line, which is text describing the task in the menu.
			// For the moment I don't do any error checking on that parameter.

			// Move on to the next task entry.
			tasks++;
		}
		else if ( tokens != 0 ) {
			printf( "  %s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			fprintf( log, "  %s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			errors++;
		}

	}

	fclose( fp );
	return( errors );

}


/*********************************************************************************************************************************/

int process_session_file ( char *filename, int verbose ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i;

	// Keep track of protocol IDs to signal illegal duplicates.
	int protocolID[256];
	int protocols = 0;

	int errors = 0;
	char protocol_file[1024];

	fp = fopen( filename, "r" );
	if ( !fp ) {
		// Signal the error.
		printf( "  Error opening session file %s for read.\n", filename );
		fprintf( log, "  Error opening session file %s for read.\n", filename );
		// Tell the caller that we had just the one error.
		return( 1 );
	}

	if ( verbose ) fprintf( stderr, "\n  File: %s\n", filename );

	// Work our way through the session file line-by-line.
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimitedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\nLine:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}

		// Lines in session files are divided into 4 fields.
		if ( tokens == 4 ) {
			// First parameter must be CMD_PROTOCOL.
			if ( strcmp( token[0], "CMD_PROTOCOL" ) ) {
				printf( "  %s Line %03d Command not CMD_PROTOCOL: %s\n", filename, line_n, token[0] );
				fprintf( log, "  %s Line %03d Command not CMD_PROTOCOL: %s\n", filename, line_n, token[0] );
				errors++;
			}
			// Second parameter is a protocol ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &protocolID[protocols] ) ) {
				printf( " %s Line %03d Error reading protocol ID: %s\n", filename, line_n, token[1] );
				fprintf( log, " %s Line %03d Error reading protocol ID: %s\n", filename, line_n, token[1] );
				errors++;
			}
			else {
				// Protocol IDs must be unique.
				for ( i = 0; i < protocols; i++ ) {
					if ( protocolID[protocols] == protocolID[i] ) {
						printf( " %s Line %03d Duplicate protocol ID: %s\n", filename, line_n, token[1] );
						fprintf( log, " %s Line %03d Duplicate protocol ID: %s\n", filename, line_n, token[1] );
						errors++;
					}
				}
			}
			// The third item is the name of the protocol file.
			strcpy( protocol_file, token[2] );
			// Check if it is present and readable, unless told to ignore it.
			if ( strcmp( protocol_file, "ignore" ) ) {
				if ( _access( protocol_file, 0x00 ) ) {
					printf( " %s Line %03d Cannot access protocol file: %s\n", filename, line_n, protocol_file );
					fprintf( log, " %s Line %03d Cannot access protocol file: %s\n", filename, line_n, protocol_file );
					errors++;
				}	
				else {
					// Add the filename to the master list, if it is not there already.
					add_to_global_script_list( protocol_file );
					// Process it and count any errors that occur while doing so.
					errors += process_protocol_file( protocol_file, verbose );
				}
			}
			// There is a fourth field to the line, which is text describing the protocol in the menu.
			// For the moment I don't do any error checking on that parameter.

			// Move on to next protocol entry.
			protocols++;
		}
		else if ( tokens != 0 ) {
			printf( " %s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			fprintf( log, " %s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			errors++;
		}				
	}

	fclose( fp );
	return( errors );

}



int main ( int argc, char *argv[] ) {


	int errors = 0;

	char user_file[1024] = "users.dex";
	char log_file[1024] = "DexLintLog.txt";

	char script_batch[1024] = "";
	char picture_batch[1024] = "";

	int popups = TRUE;
	int verbose = FALSE;

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i;

	char session_file[1024];

	int subjectID[256];
	int passcode[256];
	int subjects = 0;

	int arg;

	// Parse the command line arguments.

	for ( arg = 1; arg < argc; arg++ ) {

		// You can inhibit the popup notification of success for failure for use in a makefile.
		if ( !strcmp( argv[arg], "-noquery" ) ) popups = FALSE;

		// Defines where to look for the picture files.
		if ( !strncmp( argv[arg], "-pictures=", strlen( "-pictures=" ) ) ) {
			strcpy( picture_path, argv[arg] + strlen( "-pictures=" ) );
			// Add a trailing backslash if itis not present already.
			if ( picture_path[ strlen( picture_path ) - 1 ] != '\\' ) strcat( picture_path, "\\" );
		}

		// Specify an alternate user.dex file, which is the root of the file structure.
		if ( !strncmp( argv[arg], "-root=", strlen( "-root=" ) ) ) {
			strcpy( user_file, argv[arg] + strlen( "-root=" ) );
		}
		// Specify an alternate log file output.
		if ( !strncmp( argv[arg], "-log=", strlen( "-log=" ) ) ) {
			strcpy( log_file, argv[arg] + strlen( "-log=" ) );
		}
		// Specify an alternate batch file name for creating the tar of script files.
		if ( !strncmp( argv[arg], "-sbatch=", strlen( "-sbatch=" ) ) ) {
			strcpy( script_batch, argv[arg] + strlen( "-sbatch=" ) );
		}
		// Specify an alternate batch file name for creating the tar of script files.
		if ( !strncmp( argv[arg], "-pbatch=", strlen( "-pbatch=" ) ) ) {
			strcpy( picture_batch, argv[arg] + strlen( "-pbatch=" ) );
		}


	}

	log = fopen( log_file, "w" );
	if ( !log ) {

		char msg[1024];
		sprintf( msg, "Error opening %s for write.", log_file );
		printf( "%s\n", msg );
		if ( popups ) MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONERROR );
		exit( NO_LOG_FILE );
	}
	fprintf( log, "%s\nRoot file: %s\n", argv[0], user_file );
	printf( "%s\nRoot file: %s\n", argv[0], user_file );

	// Open the root file, if we can.
	fp = fopen( user_file, "r" );
	if ( !fp ) {

		char msg[1024];
		sprintf( msg, "Error opening %s for read.", user_file );
		printf( "%s\n", msg );
		fprintf( log, "%s\n", msg );
		if ( popups ) MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONERROR );
		exit( NO_USER_FILE );
	}

	// Step through line by line and follow the links to the session files.
	while ( fgets( line, sizeof( line ), fp ) ) {

		// Count the lines.
		line_n++;

		// Break the line into pieces as defined by the DEX/GRIP ICD.
		tokens = ParseCommaDelimitedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\nLine:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}

		// Parse each line and do some syntax error checking.
		if ( tokens == 5 ) {

			// First parameter must be CMD_USER.
			if ( strcmp( token[0], "CMD_USER" ) ) {
				printf( "Line %03d Command not CMD_USER: %s\n", line_n, token[0] );
				fprintf( log, "Line %03d Command not CMD_USER: %s\n", line_n, token[0] );
				errors++;
			}
			// Second parameter is a subject ID. Should be an integer value.
			// Verify also that subject IDs are unique.
			if ( 1 != sscanf( token[1], "%d", &subjectID[subjects] ) ) {
				// Report error for invalid subject ID field.
				printf( "Line %03d Error reading subject ID: %s\n", line_n, token[1] );
				fprintf( log, "Line %03d Error reading subject ID: %s\n", line_n, token[1] );
				errors++;
			}
			else {
				// Check if we have already seen this subject ID.
				for ( i = 0; i < subjects; i++ ) {
					if ( subjectID[subjects] == subjectID[i] ) {
						printf( "Line %03d Duplicate subject ID: %s\n", line_n, token[1] );
						fprintf( log, "Line %03d Duplicate subject ID: %s\n", line_n, token[1] );
						errors++;
					}
				}
			}
			// Next parameter is the passcode for that subject. 
			// Also verify that it is a valid integer and unique to this subject.
			if ( 1 != sscanf( token[2], "%d", &passcode[subjects] ) ) {
				// Report error reading an integer value for the pin number.
				printf( "Line %03d Error reading passcode: %s\n", line_n, token[2] );
				fprintf( log, "Line %03d Error reading passcode: %s\n", line_n, token[2] );
				errors++;
			}
			else {
				// It's better if subject passcodes are unique as well to avoid accidentally using the wrong user ID.
				for ( i = 0; i < subjects; i++ ) {
					if ( passcode[subjects] == passcode[i] ) {
						printf( "Line %03d Duplicate passcode: %s\n", line_n, token[2] );
						fprintf( log, "Line %03d Duplicate passcode: %s\n", line_n, token[2] );
						errors++;
					}
				}
			}
			// The third parameter is the name of the session file.
			// Here we check that it is present and if so, we process it as well.
			strcpy( session_file, token[3] );
			if ( _access( session_file, 0x00 ) ) {
				// The file must not only be present, it also has to be readable.
				// I had a funny problem with this at one point. Maybe MAC OS had created links.
				printf( "Line %03d Cannot access session file: %s\n", line_n, session_file );
				fprintf( log, "Line %03d Cannot access session file: %s\n", line_n, session_file );
				errors++;
			}	
			else {
				// Add the filename to the master list, if it is not there already.
				add_to_global_script_list( session_file );
				// If the file is accessible, process it and add it's error to the ones we found here.
				errors += process_session_file( session_file, verbose );
			}
			// There is a fifth field to the line, which is text describing the subject.
			// For the moment I don't do any error checking on that parameter.

			// Move to next entry in the subject ID and pin number lists.
			subjects++;
		}
		else if ( tokens != 0 ) {
			printf( "%s Line %03d Wrong number of parameters: %s\n", user_file, line_n, line );
			fprintf( log, "%s Line %03d Wrong number of parameters: %s\n", user_file, line_n, line );
			errors++;
		}				
	}

	fclose( fp );


	if ( errors == 0 ) {

		int j;

		// Create a batch file that will do the md5 sums for the script files and put them in an archive.
		if ( strlen( script_batch ) ) {
			fp = fopen( script_batch, "w" );
			fprintf( fp, "@echo on\nSETLOCAL\n" );
			fprintf( fp, "set FILES=" );
			fprintf( fp, "%s", user_file );
			for ( j = 0; j < global_scripts; j++ ) fprintf( fp, " %s", global_script_filenames[j] );
			fprintf( fp, "\n" );
			fprintf( fp, "..\\bin\\MD5Tree.exe %%FILES%% > Scripts.md5\n" );
			fprintf( fp, "\"C:\\Program Files\\GnuWin32\\bin\\tar.exe\" --create --verbose Scripts.md5 %%FILES%% --file=%%1\n" );
			fclose( fp );
		}
		
		// Create a batch file that will do the md5 sums for the picture files and put them in an archive.
		if ( strlen( picture_batch ) ) {
			fp = fopen( picture_batch, "w" );
			fprintf( fp, "@echo on\nSETLOCAL\n" );
			fprintf( fp, "set FILES=" );
			for ( j = 0; j < global_pictures; j++ ) fprintf( fp, "%s ", global_picture_filenames[j] );
			fprintf( fp, "\n" );
			fprintf( fp, "..\\bin\\MD5Tree.exe %%FILES%% > Pictures.md5\n" );
			fprintf( fp, "\"C:\\Program Files\\GnuWin32\\bin\\tar.exe\" --create --verbose Pictures.md5 --directory=%s %%FILES%% --file=%%1\n", picture_path );
			fclose( fp );
		}

		if ( verbose ) {
			fprintf( stderr, "\nNumber of unique script files:  %d\n", global_scripts );
			fprintf( stderr, "\nNumber of unique picture files: %d\n", global_pictures );
			fprintf( stderr, "\nDexLint terminated successfully with 0 errors.\n" );
		}
			
		if ( popups ) {
			char msg[2048];
			sprintf( msg, "DexLint terminated successfully with 0 errors.\n\nNumber of unique script files:  %d\nNumber of unique picture files: %d", global_scripts, global_pictures );
			MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONINFORMATION );
		}
		return( NORMAL_EXIT );
	}
	else {
		char msg[1024];
		sprintf( msg, "DexLint terminated with %d errors.\nSee %s for details.", errors, log_file );
		printf( "%s\n", msg );
		fprintf( log, "%s\n", msg );
		MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONERROR );
		return( ERROR_EXIT );
	}


}