#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>

#include <DexInterpreterFunctions.h>

#define TRUE	1
#define FALSE	0

enum { NORMAL_EXIT = 0, NO_USER_FILE, ERROR_EXIT };

char global_picture_file[2560][256];
int	 global_pictures = 0;
char picture_path[1024] = "pictures\\";

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

	HBITMAP bmp;

	fp = fopen( filename, "r" );
	if ( !fp ) {
		printf( "Error opening %s for read.", filename );
		return( 1 );
	}

	if ( verbose ) fprintf( stderr, "\n  File: %s", filename );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimittedLine( token, line );
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
						printf( "     %s Line %3d Picture file not found: %s\n", filename, line_n, path );
						errors++;
					}
				}

				// If we haven't seen it already in any file, add it to the list of pictures.
				for ( j = 0; j < global_pictures; j++ ) {
					if ( !strcmp( global_picture_file[j], token[i] ) ) break;
				}
				if ( j == global_pictures ) {
					strcpy( global_picture_file[j], token[i] );
					global_pictures++;
				}

			}	
		}

	}

	return( errors );

}

int process_protocol_file ( char *filename, int verbose ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i;

	int taskID[256];
	int tasks = 0;


	int errors = 0;

	char task_file[1024];

	fp = fopen( filename, "r" );
	if ( !fp ) {
		printf( "Error opening %s for read.", filename );
		return( 1 );
	}

	if ( verbose ) fprintf( stderr, "\n  File: %s", filename );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;
		tokens = ParseCommaDelimittedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\n  Line:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}


		if ( tokens == 4 ) {
			// First parameter must be CMD_TASK.
			if ( strcmp( token[0], "CMD_TASK" ) ) {
				printf( "   %s Line %03d Command not CMD_TASK: %s\n", filename, line_n, token[0] );
				errors++;
			}
			// Second parameter is a task ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &taskID[tasks] ) ) {
				printf( "  %s Line %03d Error reading task ID: %s\n", filename, line_n, token[1] );
				errors++;
			}
			else {
				// Task IDs must be unique.
				for ( i = 0; i < tasks; i++ ) {
					if ( taskID[tasks] == taskID[i] ) {
						printf( "  %s Line %03d Duplicate task ID: %s\n", filename, line_n, token[1] );
						errors++;
					}
				}
			}
			strcpy( task_file, token[2] );
			if ( _access( task_file, 0x00 ) ) {
				printf( "  %s Line %03d Cannot access task file: %s\n", filename, line_n, task_file );
				errors++;
			}	
			else errors += process_task_file( task_file, verbose );
			tasks++;
		}
		else if ( tokens != 0 ) {
			printf( "  %s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			errors++;
		}

	}

	return( errors );

}


int process_session_file ( char *filename, int verbose ) {

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i;

	int protocolID[256];
	int protocols = 0;


	int errors = 0;
	char protocol_file[1024];

	fp = fopen( filename, "r" );
	if ( !fp ) {
		printf( "Error opening %s for read.", filename );
		return( 1 );
	}

	if ( verbose ) fprintf( stderr, "\n  File: %s\n", filename );
	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;

		tokens = ParseCommaDelimittedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\nLine:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}

		if ( tokens == 4 ) {
			// First parameter must be CMD_PROTOCOL.
			if ( strcmp( token[0], "CMD_PROTOCOL" ) ) {
				printf( "  %s Line %03d Command not CMD_PROTOCOL: %s\n", filename, line_n, token[0] );
				errors++;
			}
			// Second parameter is a protocol ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &protocolID[protocols] ) ) {
				printf( " %s Line %03d Error reading protocol ID: %s\n", filename, line_n, token[1] );
				errors++;
			}
			else {
				// Protocol IDs must be unique.
				for ( i = 0; i < protocols; i++ ) {
					if ( protocolID[protocols] == protocolID[i] ) {
						printf( " %s Line %03d Duplicate protocol ID: %s\n", filename, line_n, token[1] );
						errors++;
					}
				}
			}
			strcpy( protocol_file, token[2] );
			if ( _access( protocol_file, 0x00 ) ) {
				printf( " %s Line %03d Cannot access protocol file: %s\n", filename, line_n, protocol_file );
				errors++;
			}	
			else errors += process_protocol_file( protocol_file, verbose );
			protocols++;
		}
		else if ( tokens != 0 ) {
			printf( " %s Line %03d Wrong number of parameters: %s\n", filename, line_n, line );
			errors++;
		}				
	}

	return( errors );

}



int main ( int argc, char *argv[] ) {


	int errors = 0;

	char user_file[1024] = "users.dex";
	char session_file[1024];

	int popups = TRUE;
	int verbose = FALSE;

	FILE *fp;

	int tokens;
	char *token[MAX_TOKENS];
	char line[2048];
	int line_n = 0;
	int	i;

	int subjectID[256];
	int passcode[256];
	int subjects = 0;

	int arg;

	for ( arg = 1; arg < argc; arg++ ) {
		if ( !strcmp( argv[arg], "-noquery" ) ) popups = FALSE;
		if ( !strncmp( argv[arg], "-pictures=", strlen( "-pictures=" ) ) ) {
			strcpy( picture_path, argv[arg] + strlen( "-pictures=" ) );
			if ( picture_path[ strlen( picture_path ) - 1 ] != '\\' ) strcat( picture_path, "\\" );
		}
	}

	printf( "User Root File: %s\n", user_file );
	fp = fopen( user_file, "r" );
	if ( !fp ) {

		char msg[1024];
		sprintf( msg, "Error opening %s for read.", user_file );
		printf( "%s\n", msg );
		if ( popups ) MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONERROR );
		exit( NO_USER_FILE );
	}

	while ( fgets( line, sizeof( line ), fp ) ) {

		line_n++;

		tokens = ParseCommaDelimittedLine( token, line );
		if ( verbose ) {
			fprintf( stderr, "\nLine:   %s", line );
			fprintf( stderr, "Tokens: %d\n", tokens );
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %s\n", i, token[i] );
		}

		if ( tokens == 5 ) {
			// First parameter must be CMD_USER.
			if ( strcmp( token[0], "CMD_USER" ) ) {
				printf( "Line %03d Command not CMD_USER: %s\n", line_n, token[0] );
				errors++;
			}
			// Second parameter is a subject ID. Should be an integer value.
			if ( 1 != sscanf( token[1], "%d", &subjectID[subjects] ) ) {
				printf( "Line %03d Error reading subject ID: %s\n", line_n, token[1] );
				errors++;
			}
			else {
				// Subject IDs must be unique.
				for ( i = 0; i < subjects; i++ ) {
					if ( subjectID[subjects] == subjectID[i] ) {
						printf( "Line %03d Duplicate subject ID: %s\n", line_n, token[1] );
						errors++;
					}
				}
			}
			// Next parameter is the passcode for that subject. 
			if ( 1 != sscanf( token[2], "%d", &passcode[subjects] ) ) {
				printf( "Line %03d Error reading passcode: %s\n", line_n, token[2] );
				errors++;
			}
			else {
				// It's better if subject passcodes are unique as well to avoid accidentally using the wrong user ID.
				for ( i = 0; i < subjects; i++ ) {
					if ( passcode[subjects] == passcode[i] ) {
						printf( "Line %03d Duplicate passcode: %s\n", line_n, token[2] );
						errors++;
					}
				}
			}
			strcpy( session_file, token[3] );
			if ( _access( session_file, 0x00 ) ) {
				printf( "Line %03d Cannot access session file: %s\n", line_n, session_file );
				errors++;
			}	
			else errors += process_session_file( session_file, verbose );
			subjects++;
		}
		else if ( tokens != 0 ) {
			printf( "Line %03d Wrong number of parameters: %s\n", line_n, line );
			errors++;
		}				
	}

	fclose( fp );


	if ( errors == 0 ) {

		int j;
		char from[1024];
		char to[1024];

		// Generate a batch file to copy just the pictures that we need.
		fp = fopen( "DexLint.bat", "w" );
		fprintf( fp, "del /F /Q pictures\\*\n" );
		for ( j = 0; j < global_pictures; j++ ) {

			strcpy( from, picture_path );
			strcat( from, global_picture_file[j] );

			strcpy( to, "pictures\\" );
			strcat( to, global_picture_file[j] );

			fprintf( fp, "copy /Y /V %s %s\n", from, to );
		}
		fclose( fp );
			
		if ( popups ) MessageBox( NULL, "DexLint terminated successfully.", argv[0], MB_OK | MB_ICONINFORMATION );
		return( NORMAL_EXIT );
	}
	else {
		char msg[1024];
		sprintf( msg, "DexLint terminated with %d errors.", errors );
		printf( "%s\n", msg );
		if ( popups ) MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONERROR );
		return( ERROR_EXIT );
	}


}