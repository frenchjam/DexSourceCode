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
#include <time.h> 
#include <string.h>
#include <process.h>

#include "..\DexSimulatorApp\resource.h"

#include <DexInterpreterFunctions.h>
#define TRUE	1
#define FALSE	0

enum { NORMAL_EXIT = 0, NO_USER_FILE, NO_LOG_FILE, ERROR_EXIT };

char picture_directory[1024] = "..\\DexPictures\\";
char proofs_directory[1024] = "..\\GripScreenShots\\";
FILE *log;

// Store here temporarily the information that is to be displayed.
// It gets put into the dialog by WM_INITDIALOG.
// I haven't found any other way to pass information to the WM_INITDIALOG than by global variables.
// There aught to be a better way.
 HBITMAP _illustrated_message_picture_bitmap = NULL;
static char _illustrated_message_text[1024] = "";
static char _illustrated_message_label[1024] = "";
static char _illustrated_message_proof[1024] = "";
static char _illustrated_message_type = IDD_OKCANCEL;

// This callback is used for 'popups'. When they close, the APP keeps on going.
BOOL CALLBACK _lintGrabCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	char command[1024];
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText( hDlg, IDC_MESSAGE, _illustrated_message_text );
		if ( _illustrated_message_picture_bitmap ) {
			SendDlgItemMessage( hDlg, IDC_PICTURE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) _illustrated_message_picture_bitmap );
		}
		SetTimer( hDlg, WM_TIMER, 10, NULL );
   		return TRUE;
		break;
		
	case WM_PAINT:
  		return FALSE;
		break;

	case WM_TIMER:
		switch ( _illustrated_message_type ) {
		
		case IDD_OKCANCEL:
			sprintf( command, "boxcutter.exe -c 292,20,768,668 %s", _illustrated_message_proof );
			break;

		case IDD_ABORTRETRYIGNORE:
			sprintf( command, "boxcutter.exe -c 292,20,768,668 %s", _illustrated_message_proof );
			break;

		case IDD_STATUS:
			sprintf( command, "boxcutter.exe -c 292,20,768,668 %s", _illustrated_message_proof );
			break;

		}
 		system( command );
		_sleep( 750 );
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
			break;

	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
		break;

	case WM_SHOWWINDOW:
		return TRUE;
		break;

    case WM_COMMAND:

		switch ( LOWORD( wParam ) ) {
		case IDCANCEL:
		case IDOK:
		case IDRETRY:
		case IDABORT:
		case IDIGNORE:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		}
	}
    return FALSE;
}

int GrabDialog( const char *filename, const char *message, const char *picture, const char *label, int buttons ) {

	int return_code;

	int code = GetLastError();
	char *ptr;

	char picture_path[1024];

	// Store the information that is to be displayed in global variables, so that it can be put into the dialog by WM_INITDIALOG.
	strcpy( _illustrated_message_proof, filename );

	if ( picture ) {
		strcpy(  picture_path, picture_directory );
		strcat( picture_path, picture );
		_illustrated_message_picture_bitmap = (HBITMAP) LoadImage( NULL, picture_path, IMAGE_BITMAP, (int) (.65 * 540), (int) (.65 * 405), LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_VGACOLOR );
	}
	else _illustrated_message_picture_bitmap = NULL;

	if ( message ) {
		strcpy( _illustrated_message_text, message );
		// Strings coming from scripts have end of lines marked with "\n". Need to convert to a real newline.
		for ( ptr = _illustrated_message_text; *ptr; ptr++ ) {
			if ( *ptr == '\\' && *(ptr+1) == 'n' ) {
				*ptr = '\r';
				*(ptr+1) = '\n';
			}
		}
	}
	else _illustrated_message_text[0] = 0;

	if ( label ) strcpy( _illustrated_message_label, label );
	else _illustrated_message_label[0] = 0;

	if ( ( buttons & 0x0000000f ) == MB_ABORTRETRYIGNORE ) {
		_illustrated_message_type = IDD_ABORTRETRYIGNORE;
		return_code = DialogBox( NULL, (LPCSTR) IDD_ABORTRETRYIGNORE, NULL, _lintGrabCallback );
	}
	else if ( ( buttons & 0x0000000f ) == MB_OKCANCEL ) {
		_illustrated_message_type = IDD_OKCANCEL;
		return_code = DialogBox( NULL, (LPCSTR) IDD_OKCANCEL, NULL, _lintGrabCallback );
	}
	else {
		_illustrated_message_type = IDD_STATUS;
		return_code = DialogBox( NULL, (LPCSTR) IDD_STATUS, NULL, _lintGrabCallback );
	}

	return( return_code );

}

/*********************************************************************************************************************************/

// Keep track of unique picture files and script files.

#define MAX_FILELIST_SIZE	2560

// A list of picture filenames, where each unique file appear just once.
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

// A list of script filenames, where each unique file appears just once.
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

// Keep track of unique picture/message pairs.


#define MAX_PAIRS 2048

typedef struct {
	char message[256];
	char picture[256];
} MessagePair;

typedef struct {
	int n;
	MessagePair entry[MAX_PAIRS];
} MessagePairList;

// There are 3 kinds of message/picture combinations. 
// Keep track of them separately.
MessagePairList	status = {0}, query = {0}, alert = {0};

int add_to_message_pair ( MessagePairList *list, const char *message, const char *picture ) {

	int i;

	if ( !message || !picture ) return( FALSE );
	// Check if we have seen a message/picture pair before in this list.
	// Note that the same message may appear more than once in the list, with different pictures.
	// Similarly, the same picture may appear more than once, with different messages.
	for ( i = 0; i < list->n; i++ ) {
		if ( !strcmp( list->entry[i].message, message ) &&  !strcmp( list->entry[i].picture, picture ) ) break;
	}

	// If we got to the end of the list before finding a match, add it to the end.
	if ( i >= list->n && list->n < MAX_PAIRS ) {
		// Add to end of list.
		strcpy( list->entry[i].message, message );
		strcpy( list->entry[i].picture, picture );
		list->n++;
		return( TRUE );
	}
	// If we found a match before reaching the end of the list, do nothing.
	else return( FALSE );

}

void sort_message_pair_list ( MessagePairList *list ) {

	char hold_picture[1024];
	char hold_message[1024];
	int i, j;

	for ( i = 0; i < list->n; i++ ) {
		for ( j = i + 1; j < list->n; j++ ) {
			if ( ( strcmp( list->entry[j].message, list->entry[i].message ) < 0 ) ||
				 ( strcmp( list->entry[j].message, list->entry[i].message ) == 0 && strcmp( list->entry[j].picture, list->entry[i].picture ) < 0 ) )  {
				strcpy( hold_picture, list->entry[j].picture );
				strcpy( hold_message, list->entry[j].message );
				strcpy( list->entry[j].picture, list->entry[i].picture );
				strcpy( list->entry[j].message, list->entry[i].message );
				strcpy( list->entry[i].picture, hold_picture );
				strcpy( list->entry[i].message, hold_message );
			}
		}
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

	char hold_status_message[1024] = "";

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
			for ( i = 0; i < tokens; i++ ) fprintf( stderr, "%2d %03d %s\n", i, sizeof( token[i] ), token[i] );
		}

		// Look for commands that generate message/picture combinations and add new ones to the lists.
		// They are differentiated into 'query', 'status' and 'alert'.
		// These lists are used to generate the proofs of the screens that the subject/operator might see.
		if ( tokens ) {

			// Queries are generated by a single command.
			if ( !strcmp( token[0], "CMD_WAIT_SUBJ_READY" ) ) add_to_message_pair( &query, token[1], token[2] );

			// Status is generated by a combination of two commands.
			// CMD_LOG_MESSAGE can change the text on the screen (unless it is just a message to be logged).
			// CMD_SET_PICTURE changes the picture on the screen. 
			// Here we have to do a little work to figure out when the status changes so that we generate
			//  proofs with each unique picture and message combination.
			else if ( !strcmp( token[0], "CMD_LOG_MESSAGE" ) ) {
				int value, items;
				if ( !strcmp( token[1], "logmsg" ) ) value = 0;
				else if ( !strcmp( token[1], "usermsg" ) ) value = 1;
				else {
					items = sscanf( token[1], "%d", &value );
					value = items && value;
				}
				if ( value != 0 ) {
					if ( token[2] ) strcpy( hold_status_message, token[2] );
					else strcpy( hold_status_message, "" );
				}
			}
			else if ( !strcmp( token[0], "CMD_SET_PICTURE" ) ) add_to_message_pair( &status, hold_status_message, token[1] );

			// Now we check for other commands that can generate picture/message combinations.
			// In general, these commands create 'alerts' to signal conditions that require 
			//  operator attention and intervention.
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_ATTARGET" ) ) add_to_message_pair( &alert, token[13], token[14] );			
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_GRIP" ) ) add_to_message_pair( &alert, token[4], token[5] );
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_GRIPFORCE" ) ) add_to_message_pair( &alert, token[11], token[12] );
			else if ( !strcmp( token[0], "CMD_WAIT_MANIP_SLIP" ) ) add_to_message_pair( &alert, token[11], token[12] );
			else if ( !strcmp( token[0], "CMD_ALIGN_CODA" ) ) add_to_message_pair( &alert, token[1], token[2] );
			else if ( !strcmp( token[0], "CMD_CHK_CODA_ALIGNMENT" ) ) add_to_message_pair( &alert, token[4], token[5] );
			else if ( !strcmp( token[0], "CMD_CHK_CODA_FIELDOFVIEW" ) ) add_to_message_pair( &alert, token[9], token[10] );
			else if ( !strcmp( token[0], "CMD_CHK_CODA_PLACEMENT" ) ) add_to_message_pair( &alert, token[11], token[12] );
			else if ( !strcmp( token[0], "CMD_CHK_MOVEMENTS_AMPL" ) ) add_to_message_pair( &alert, token[6], token[7] );
			else if ( !strcmp( token[0], "CMD_CHK_MOVEMENTS_CYCLES" ) ) add_to_message_pair( &alert, token[7], token[8] );
			else if ( !strcmp( token[0], "CMD_CHK_START_POS" ) ) add_to_message_pair( &alert, token[7], token[8] );
			else if ( !strcmp( token[0], "CMD_CHK_MOVEMENTS_DIR" ) ) add_to_message_pair( &alert, token[6], token[7] );
			else if ( !strcmp( token[0], "CMD_CHK_COLLISIONFORCE" ) ) add_to_message_pair( &alert, token[4], token[5] );
			else if ( !strcmp( token[0], "CMD_CHK_MANIP_VISIBILITY" ) ) add_to_message_pair( &alert, token[3], token[4] );

			// The following two act like alerts in the sense that message is displayed depending on a test or condition.
			// But the response buttons are OK, CANCEL, STATUS, not IGNORE, RETRY, CANCEL, STATUS. 
			// And unlike Retry for an Alert, OK here will retry the step, not the entire task.
			// In that respect, the commands are more like queries than alerts.
			else if ( !strcmp( token[0], "CMD_CHK_MASS_SELECTION" ) ) add_to_message_pair( &query, "Put mass in cradle X and pick up mass from cradle Y.", "TakeMass.bmp" );
			else if ( !strcmp( token[0], "CMD_CHK_HW_CONFIG" ) ) add_to_message_pair( &query, token[1], token[2] );

			else if ( strstr( line, ".bmp" ) ) {
				fprintf( log, "     %s Line %3d Picture file in unrecognized command: %s\n", filename, line_n, token[0] );
				errors++;
			}

		}

		// Here we collect all of the picture files into a single list.
		// This list can then be used to package together only the picture files that are actually needed.
		// Here we don't care what command specified the picture file. If there is a picture file somewhere
		// in a script line, we assume that it is needed and add it to the list.
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

					strcpy( path, picture_directory );
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

	// _check_messages.dex is a special protocol that is generated by DexLint from the list of messages and images that it finds.
	// It would be circular to test it as well. So we skip it.
	if ( !strcmp( "_check_messages.dex", filename ) ) return( 0 );

	// Attempt to open the specified file.
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

// Message strings have to be quoted by the DEX interpreter.
// This puts quotes into the string itself.
// Also, they have a limited length and for the moment commas have to be replaced.

// The return value is a pointer to a static string that gets overwritten every time
//  this routine is called. Since in general the result is used just once in a printf
//  statement, this should not be a problem. But if you want to save the result, 
//  you need to copy the resulting string.
#define DEX_MAX_MESSAGE_LENGTH 128
char *quoteMessage( const char *message ) {

	int i, j = 0;
	static char result[ 2 * DEX_MAX_MESSAGE_LENGTH ];

	if ( !message ) {
		return( "" );
	}


	// Limit the message to the size handled by DEX and replace some special characters.
	for ( i = 0; i < DEX_MAX_MESSAGE_LENGTH && j < sizeof( result ) - 1 && message[i] != 0; i++ ) {
		// Transform linebreaks into \n.
		if ( message[i] == '\n' ) {
			if ( i == DEX_MAX_MESSAGE_LENGTH - 1) break;
			result[j++] = '\\';
			result[j++] = 'n';
		}
		// The DEX parser requires that commas be escaped.
		else if ( message[i] == ',' ) {
			result[j++] = '\\';
			result[j++] = ',';
		}
		// Otherwise, just copy.
		else result[j++] = message[i];

	}
	result[j++] = 0;

	if ( strlen( result ) > 128 ) {
		char original[10240];
		strcpy( original, result );
		result[127] = 0;
		fprintf( stderr, "WARNING: Message too long. Truncating.\n%s\n%s\n", original, result );
	}

	return( result );

}

void init_check_messages( void ) {

	FILE *fp1;
	fp1 = fopen( "_check_special_task.dex", "w" ); fprintf( fp1, "# Special file for debugging.\n" ); fclose( fp1 );
	fp1 = fopen( "_check_a_e_task.dex", "w" ); fprintf( fp1, "# Special file for debugging.\n" ); fclose( fp1 );
	fp1 = fopen( "_check_f_m_task.dex", "w" ); fprintf( fp1, "# Special file for debugging.\n" ); fclose( fp1 );
	fp1 = fopen( "_check_n_s_task.dex", "w" ); fprintf( fp1, "# Special file for debugging.\n" ); fclose( fp1 );
	fp1 = fopen( "_check_t_z_task.dex", "w" ); fprintf( fp1, "# Special file for debugging.\n" ); fclose( fp1 );

}

void add_to_check_messages ( const char *group, const char *picture, const char *message ) {

	char *filename;
	FILE *fp1;
	char msg[1024];

	if ( message[0] < 'A' ) filename = "_check_special_task.dex";
	else if ( message[0] < 'F' ) filename = "_check_a_e_task.dex";
	else if ( message[0] < 'N' ) filename = "_check_f_m_task.dex";
	else if ( message[0] < 'T' ) filename = "_check_n_s_task.dex";
	else filename = "_check_t_z_task.dex";

	strcpy( msg, quoteMessage( message ) );
	fp1 = fopen( filename, "a" );
	fprintf( fp1, "CMD_WAIT_SUBJ_READY,%s,%s,300\n", msg, picture );
	fclose( fp1 );  

}

int main ( int argc, char *argv[] ) {


	int errors = 0;

	char user_file[1024] = "users.dex";
	char log_file[1024] = "DexLintLog.txt";

	char script_batch[1024] = "";
	char picture_batch[1024] = "";
	char message_list[1024] = "DexMessageList.txt";

	int popups = TRUE;
	int verbose = FALSE;
	int generate_proofs = FALSE;

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

	char date_str [9];
	char time_str [9];

	int arg;

	// Parse the command line arguments.

	for ( arg = 1; arg < argc; arg++ ) {

		// You can inhibit the popup notification of success for failure for use in a makefile.
		if ( !strcmp( argv[arg], "-noquery" ) ) popups = FALSE;

		// Defines where to look for the picture files.
		if ( !strncmp( argv[arg], "-pictures=", strlen( "-pictures=" ) ) ) {
			strcpy( picture_directory, argv[arg] + strlen( "-pictures=" ) );
			// Add a trailing backslash if it is not present already.
			if ( picture_directory[ strlen( picture_directory ) - 1 ] != '\\' ) strcat( picture_directory, "\\" );
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

		// Specify a file to store the list of messages.
		if ( !strncmp( argv[arg], "-messages=", strlen( "-messages=" ) ) ) {
			strcpy( message_list, argv[arg] + strlen( "-messages=" ) );
		}

		// Specify if we should generate the proofs.
		if ( !strcmp( argv[arg], "-proofs" ) ) generate_proofs = TRUE;

		// Change the destination directory for the proofs.
		if ( !strncmp( argv[arg], "-proofs=", strlen( "-proofs=" ) ) ) {
			strcpy( proofs_directory, argv[arg] + strlen( "-proofs=" ) );
			if ( proofs_directory[ strlen( proofs_directory ) - 1 ] != '\\' ) strcat( proofs_directory, "\\" );
			generate_proofs = TRUE;
		}

	}

	// Add static pictures required by ASW.exe. Per Bert.
	add_to_global_picture_list( "pause.bmp" );
	add_to_global_picture_list( "default.bmp" );
	add_to_global_picture_list( "wait.bmp" );
	add_to_global_picture_list( "poweroff.bmp" );

	log = fopen( log_file, "w" );
	if ( !log ) {

		char msg[1024];
		sprintf( msg, "Error opening %s for write.", log_file );
		printf( "%s\n", msg );
		if ( popups ) MessageBox( NULL, msg, argv[0], MB_OK | MB_ICONERROR );
		exit( NO_LOG_FILE );
	}

	_strdate( date_str);
	_strtime( time_str );

	fprintf( log, "%s\nDate: %s %s\nRoot file: %s\n", argv[0], date_str, time_str, user_file );
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

		char proof[1024];
		char path[1024];

		int j;

		// Generate a formatted list of messages and pictures.
		init_check_messages();
		fp = fopen( message_list, "w" );
		fprintf( fp, "Proof\tPicture\tMessage\n" );
		sort_message_pair_list( &query );
		for ( j = 0; j < query.n; j++ ) {
			if ( generate_proofs ) {
				sprintf( proof, "DexQuery.%03d.bmp", j+1 );
				strcpy( path, proofs_directory );
				strcat( path, proof );
				GrabDialog( path, query.entry[j].message, query.entry[j].picture, "", MB_OKCANCEL );
			}
			else strcpy( proof, "Query" );
			fprintf( fp, "%s\t%s\t%s\n",  proof, query.entry[j].picture, query.entry[j].message );
			add_to_check_messages( proof, query.entry[j].picture, query.entry[j].message );
		}
		fprintf( fp, "\n" );
		sort_message_pair_list( &status );
		for ( j = 0; j < status.n; j++ ) {
			if ( generate_proofs ) {
				sprintf( proof, "DexState.%03d.bmp", j+1 );
				strcpy( path, proofs_directory );
				strcat( path, proof );
				GrabDialog( path, status.entry[j].message, status.entry[j].picture, "", MB_OK );
			}
			else strcpy( proof, "State" );
			fprintf( fp, "%s\t%s\t%s\n",  proof, status.entry[j].picture, status.entry[j].message );
			add_to_check_messages( proof, status.entry[j].picture, status.entry[j].message );
		}
		fprintf( fp, "\n" );
		sort_message_pair_list( &alert );
		for ( j = 0; j < alert.n; j++ ) {
			if ( generate_proofs ) {
				sprintf( proof, "DexAlert.%03d.bmp", j+1 );
				strcpy( path, proofs_directory );
				strcat( path, proof );
				GrabDialog( path, alert.entry[j].message, alert.entry[j].picture, "", MB_ABORTRETRYIGNORE );
			}
			else strcpy( proof, "Alert" );
			fprintf( fp, "%s\t%s\t%s\n",  proof, alert.entry[j].picture, alert.entry[j].message );
			add_to_check_messages( proof, alert.entry[j].picture, alert.entry[j].message );
		}
		fprintf( fp, "\n" );
		fclose( fp );

		// Create a batch file that will do the md5 sums for the script files and put them in an archive.
		if ( strlen( script_batch ) ) {
			fp = fopen( script_batch, "w" );
			fprintf( fp, "@echo on\nSETLOCAL\n" );
			fprintf( fp, "echo %%date%% %%time%% > ScriptsTimeStamp\n" );
			fprintf( fp, "set FILES=" );
			fprintf( fp, "%s", user_file );
			for ( j = 0; j < global_scripts; j++ ) fprintf( fp, " %s", global_script_filenames[j] );

			fprintf( fp, " _check_special_task.dex" );
			fprintf( fp, " _check_a_e_task.dex" );
			fprintf( fp, " _check_f_m_task.dex" );
			fprintf( fp, " _check_n_s_task.dex" );
			fprintf( fp, " _check_t_z_task.dex" );
			fprintf( fp, " ScriptsTimeStamp\n" );

			fprintf( fp, "..\\bin\\MD5Tree.exe %%FILES%% > Scripts.md5\n" );
			fprintf( fp, "\"C:\\Program Files\\GnuWin32\\bin\\tar.exe\" --create --verbose Scripts.md5 %%FILES%% --file=%%1\n" );
			fclose( fp );
		}
		
		// Create a batch file that will do the md5 sums for the picture files and put them in an archive.
		if ( strlen( picture_batch ) ) {
			fp = fopen( picture_batch, "w" );
			fprintf( fp, "@echo on\nSETLOCAL\n" );
			fprintf( fp, "pushd %s\n", picture_directory );
			fprintf( fp, "echo %%date%% %%time%% > PicturesTimeStamp\n" );
			fprintf( fp, "set FILES=" );
			for ( j = 0; j < global_pictures; j++ ) fprintf( fp, "%s ", global_picture_filenames[j] );
			fprintf( fp, "\n" );
			fprintf( fp, "..\\bin\\MD5Tree.exe %%FILES%% > Pictures.md5\n" );
			fprintf( fp, "popd\n" );
			fprintf( fp, "\"C:\\Program Files\\GnuWin32\\bin\\tar.exe\" --create --verbose --directory=%s Pictures.md5 PicturesTimeStamp %%FILES%% --file=%%1\n", picture_directory );
			fclose( fp );
		}

		if ( verbose ) {
			fprintf( stderr, "\nNumber of unique script files:  %d\n", global_scripts );
			fprintf( stderr, "\nNumber of unique picture files: %d\n", global_pictures );
			fprintf( stderr, "\nDexLint terminated successfully with 0 errors.\n" );
		}
			
		if ( popups ) {
			char msg[2048];
			sprintf( msg, "DexLint terminated successfully with 0 errors.\n\nNumber of unique script files:  %d\nNumber of unique picture files: %d\nNumber of unique queries: %d\nNumber of unique states: %d\nNumber of unique alerts: %d", 
				global_scripts, global_pictures, query.n, status.n, alert.n );
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